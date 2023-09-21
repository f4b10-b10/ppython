/* interpreters module */
/* low-level access to interpreter primitives */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "interpreteridobject.h"
#include "pycore_pybuffer.h"      // _PyBuffer_ReleaseInInterpreterAndRawFree()
#include "pycore_interp.h"        // _PyInterpreterState_LookUpID()

#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>        // SwitchToThread()
#elif defined(HAVE_SCHED_H)
#include <sched.h>          // sched_yield()
#endif


/*
This module has the following process-global state:

_globals (static struct globals):
    module_count (int)
    channels (struct _channels):
        numopen (int64_t)
        next_id; (int64_t)
        mutex (PyThread_type_lock)
        head (linked list of struct _channelref *):
            id (int64_t)
            objcount (Py_ssize_t)
            next (struct _channelref *):
                ...
            chan (struct _channel *):
                open (int)
                mutex (PyThread_type_lock)
                closing (struct _channel_closing *):
                    ref (struct _channelref *):
                        ...
                ends (struct _channelends *):
                    numsendopen (int64_t)
                    numrecvopen (int64_t)
                    send (struct _channelend *):
                        interp (int64_t)
                        open (int)
                        next (struct _channelend *)
                    recv (struct _channelend *):
                        ...
                queue (struct _channelqueue *):
                    count (int64_t)
                    first (struct _channelitem *):
                        next (struct _channelitem *):
                            ...
                        data (_PyCrossInterpreterData *):
                            data (void *)
                            obj (PyObject *)
                            interp (int64_t)
                            new_object (xid_newobjectfunc)
                            free (xid_freefunc)
                    last (struct _channelitem *):
                        ...

The above state includes the following allocations by the module:

* 1 top-level mutex (to protect the rest of the state)
* for each channel:
   * 1 struct _channelref
   * 1 struct _channel
   * 0-1 struct _channel_closing
   * 1 struct _channelends
   * 2 struct _channelend
   * 1 struct _channelqueue
* for each item in each channel:
   * 1 struct _channelitem
   * 1 _PyCrossInterpreterData

The only objects in that global state are the references held by each
channel's queue, which are safely managed via the _PyCrossInterpreterData_*()
API..  The module does not create any objects that are shared globally.
*/

#define MODULE_NAME "_xxinterpchannels"


#define GLOBAL_MALLOC(TYPE) \
    PyMem_RawMalloc(sizeof(TYPE))
#define GLOBAL_FREE(VAR) \
    PyMem_RawFree(VAR)


struct xid_class_registry {
    size_t count;
#define MAX_XID_CLASSES 5
    struct {
        PyTypeObject *cls;
    } added[MAX_XID_CLASSES];
};

static int
register_xid_class(PyTypeObject *cls, crossinterpdatafunc shared,
                   struct xid_class_registry *classes)
{
    int res = _PyCrossInterpreterData_RegisterClass(cls, shared);
    if (res == 0) {
        assert(classes->count < MAX_XID_CLASSES);
        // The class has refs elsewhere, so we need to incref here.
        classes->added[classes->count].cls = cls;
        classes->count += 1;
    }
    return res;
}

static void
clear_xid_class_registry(struct xid_class_registry *classes)
{
    while (classes->count > 0) {
        classes->count -= 1;
        PyTypeObject *cls = classes->added[classes->count].cls;
        _PyCrossInterpreterData_UnregisterClass(cls);
    }
}

#define XID_IGNORE_EXC 1
#define XID_FREE 2

static int
_release_xid_data(_PyCrossInterpreterData *data, int flags)
{
    int ignoreexc = flags & XID_IGNORE_EXC;
    PyObject *exc;
    if (ignoreexc) {
        exc = PyErr_GetRaisedException();
    }
    int res;
    if (flags & XID_FREE) {
        res = _PyCrossInterpreterData_ReleaseAndRawFree(data);
    }
    else {
        res = _PyCrossInterpreterData_Release(data);
    }
    if (res < 0) {
        /* The owning interpreter is already destroyed. */
        if (ignoreexc) {
            // XXX Emit a warning?
            PyErr_Clear();
        }
    }
    if (flags & XID_FREE) {
        /* Either way, we free the data. */
    }
    if (ignoreexc) {
        PyErr_SetRaisedException(exc);
    }
    return res;
}


static PyInterpreterState *
_get_current_interp(void)
{
    // PyInterpreterState_Get() aborts if lookup fails, so don't need
    // to check the result for NULL.
    return PyInterpreterState_Get();
}

static PyObject *
_get_current_module(void)
{
    PyObject *name = PyUnicode_FromString(MODULE_NAME);
    if (name == NULL) {
        return NULL;
    }
    PyObject *mod = PyImport_GetModule(name);
    Py_DECREF(name);
    if (mod == NULL) {
        return NULL;
    }
    assert(mod != Py_None);
    return mod;
}

static PyObject *
get_module_from_owned_type(PyTypeObject *cls)
{
    assert(cls != NULL);
    return _get_current_module();
    // XXX Use the more efficient API now that we use heap types:
    //return PyType_GetModule(cls);
}

static struct PyModuleDef moduledef;

static PyObject *
get_module_from_type(PyTypeObject *cls)
{
    assert(cls != NULL);
    return _get_current_module();
    // XXX Use the more efficient API now that we use heap types:
    //return PyType_GetModuleByDef(cls, &moduledef);
}

static PyObject *
add_new_exception(PyObject *mod, const char *name, PyObject *base)
{
    assert(!PyObject_HasAttrStringWithError(mod, name));
    PyObject *exctype = PyErr_NewException(name, base, NULL);
    if (exctype == NULL) {
        return NULL;
    }
    int res = PyModule_AddType(mod, (PyTypeObject *)exctype);
    if (res < 0) {
        Py_DECREF(exctype);
        return NULL;
    }
    return exctype;
}

#define ADD_NEW_EXCEPTION(MOD, NAME, BASE) \
    add_new_exception(MOD, MODULE_NAME "." Py_STRINGIFY(NAME), BASE)

static PyTypeObject *
add_new_type(PyObject *mod, PyType_Spec *spec, crossinterpdatafunc shared,
             struct xid_class_registry *classes)
{
    PyTypeObject *cls = (PyTypeObject *)PyType_FromMetaclass(
                NULL, mod, spec, NULL);
    if (cls == NULL) {
        return NULL;
    }
    if (PyModule_AddType(mod, cls) < 0) {
        Py_DECREF(cls);
        return NULL;
    }
    if (shared != NULL) {
        if (register_xid_class(cls, shared, classes)) {
            Py_DECREF(cls);
            return NULL;
        }
    }
    return cls;
}

static int
wait_for_lock(PyThread_type_lock mutex, PY_TIMEOUT_T timeout)
{
    PyLockStatus res = PyThread_acquire_lock_timed_with_retries(mutex, timeout);
    if (res == PY_LOCK_INTR) {
        /* KeyboardInterrupt, etc. */
        assert(PyErr_Occurred());
        return -1;
    }
    else if (res == PY_LOCK_FAILURE) {
        assert(!PyErr_Occurred());
        assert(timeout > 0);
        PyErr_SetString(PyExc_TimeoutError, "timed out");
        return -1;
    }
    assert(res == PY_LOCK_ACQUIRED);
    PyThread_release_lock(mutex);
    return 0;
}


/* Cross-interpreter Buffer Views *******************************************/

// XXX Release when the original interpreter is destroyed.

typedef struct {
    PyObject_HEAD
    Py_buffer *view;
    int64_t interp;
} XIBufferViewObject;

static PyObject *
xibufferview_from_xid(PyTypeObject *cls, _PyCrossInterpreterData *data)
{
    assert(data->data != NULL);
    assert(data->obj == NULL);
    assert(data->interp >= 0);
    XIBufferViewObject *self = PyObject_Malloc(sizeof(XIBufferViewObject));
    if (self == NULL) {
        return NULL;
    }
    PyObject_Init((PyObject *)self, cls);
    self->view = (Py_buffer *)data->data;
    self->interp = data->interp;
    return (PyObject *)self;
}

static void
xibufferview_dealloc(XIBufferViewObject *self)
{
    PyInterpreterState *interp = _PyInterpreterState_LookUpID(self->interp);
    /* If the interpreter is no longer alive then we have problems,
       since other objects may be using the buffer still. */
    assert(interp != NULL);

    if (_PyBuffer_ReleaseInInterpreterAndRawFree(interp, self->view) < 0) {
        // XXX Emit a warning?
        PyErr_Clear();
    }

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    /* "Instances of heap-allocated types hold a reference to their type."
     * See: https://docs.python.org/3.11/howto/isolating-extensions.html#garbage-collection-protocol
     * See: https://docs.python.org/3.11/c-api/typeobj.html#c.PyTypeObject.tp_traverse
    */
    // XXX Why don't we implement Py_TPFLAGS_HAVE_GC, e.g. Py_tp_traverse,
    // like we do for _abc._abc_data?
    Py_DECREF(tp);
}

static int
xibufferview_getbuf(XIBufferViewObject *self, Py_buffer *view, int flags)
{
    /* Only PyMemoryView_FromObject() should ever call this,
       via _memoryview_from_xid() below. */
    *view = *self->view;
    view->obj = (PyObject *)self;
    // XXX Should we leave it alone?
    view->internal = NULL;
    return 0;
}

static PyType_Slot XIBufferViewType_slots[] = {
    {Py_tp_dealloc, (destructor)xibufferview_dealloc},
    {Py_bf_getbuffer, (getbufferproc)xibufferview_getbuf},
    // We don't bother with Py_bf_releasebuffer since we don't need it.
    {0, NULL},
};

static PyType_Spec XIBufferViewType_spec = {
    .name = MODULE_NAME ".CrossInterpreterBufferView",
    .basicsize = sizeof(XIBufferViewObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = XIBufferViewType_slots,
};


/* extra XID types **********************************************************/

static PyTypeObject * _get_current_xibufferview_type(void);

static PyObject *
_memoryview_from_xid(_PyCrossInterpreterData *data)
{
    PyTypeObject *cls = _get_current_xibufferview_type();
    if (cls == NULL) {
        return NULL;
    }
    PyObject *obj = xibufferview_from_xid(cls, data);
    if (obj == NULL) {
        return NULL;
    }
    return PyMemoryView_FromObject(obj);
}

static int
_memoryview_shared(PyThreadState *tstate, PyObject *obj,
                   _PyCrossInterpreterData *data)
{
    Py_buffer *view = PyMem_RawMalloc(sizeof(Py_buffer));
    if (view == NULL) {
        return -1;
    }
    if (PyObject_GetBuffer(obj, view, PyBUF_FULL_RO) < 0) {
        PyMem_RawFree(view);
        return -1;
    }
    _PyCrossInterpreterData_Init(data, tstate->interp, view, NULL,
                                 _memoryview_from_xid);
    return 0;
}

static int
register_builtin_xid_types(struct xid_class_registry *classes)
{
    PyTypeObject *cls;
    crossinterpdatafunc func;

    // builtin memoryview
    cls = &PyMemoryView_Type;
    func = _memoryview_shared;
    if (register_xid_class(cls, func, classes)) {
        return -1;
    }

    return 0;
}


/* module state *************************************************************/

typedef struct {
    struct xid_class_registry xid_classes;

    /* Added at runtime by interpreters module. */
    PyTypeObject *send_channel_type;
    PyTypeObject *recv_channel_type;

    /* heap types */
    PyTypeObject *ChannelIDType;
    PyTypeObject *XIBufferViewType;

    /* exceptions */
    PyObject *ChannelError;
    PyObject *ChannelNotFoundError;
    PyObject *ChannelClosedError;
    PyObject *ChannelEmptyError;
    PyObject *ChannelNotEmptyError;
} module_state;

static inline module_state *
get_module_state(PyObject *mod)
{
    assert(mod != NULL);
    module_state *state = PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static module_state *
_get_current_module_state(void)
{
    PyObject *mod = _get_current_module();
    if (mod == NULL) {
        // XXX import it?
        PyErr_SetString(PyExc_RuntimeError,
                        MODULE_NAME " module not imported yet");
        return NULL;
    }
    module_state *state = get_module_state(mod);
    Py_DECREF(mod);
    return state;
}

static int
traverse_module_state(module_state *state, visitproc visit, void *arg)
{
    /* heap types */
    Py_VISIT(state->ChannelIDType);
    Py_VISIT(state->XIBufferViewType);

    /* exceptions */
    Py_VISIT(state->ChannelError);
    Py_VISIT(state->ChannelNotFoundError);
    Py_VISIT(state->ChannelClosedError);
    Py_VISIT(state->ChannelEmptyError);
    Py_VISIT(state->ChannelNotEmptyError);

    return 0;
}

static int
clear_module_state(module_state *state)
{
    Py_CLEAR(state->send_channel_type);
    Py_CLEAR(state->recv_channel_type);

    /* heap types */
    if (state->ChannelIDType != NULL) {
        (void)_PyCrossInterpreterData_UnregisterClass(state->ChannelIDType);
    }
    Py_CLEAR(state->ChannelIDType);
    Py_CLEAR(state->XIBufferViewType);

    /* exceptions */
    Py_CLEAR(state->ChannelError);
    Py_CLEAR(state->ChannelNotFoundError);
    Py_CLEAR(state->ChannelClosedError);
    Py_CLEAR(state->ChannelEmptyError);
    Py_CLEAR(state->ChannelNotEmptyError);

    return 0;
}


static PyTypeObject *
_get_current_xibufferview_type(void)
{
    module_state *state = _get_current_module_state();
    if (state == NULL) {
        return NULL;
    }
    return state->XIBufferViewType;
}


/* channel-specific code ****************************************************/

#define CHANNEL_SEND 1
#define CHANNEL_BOTH 0
#define CHANNEL_RECV -1


/* channel errors */

#define ERR_CHANNEL_NOT_FOUND -2
#define ERR_CHANNEL_CLOSED -3
#define ERR_CHANNEL_INTERP_CLOSED -4
#define ERR_CHANNEL_EMPTY -5
#define ERR_CHANNEL_NOT_EMPTY -6
#define ERR_CHANNEL_MUTEX_INIT -7
#define ERR_CHANNELS_MUTEX_INIT -8
#define ERR_NO_NEXT_CHANNEL_ID -9
#define ERR_CHANNEL_CLOSED_WAITING -10

static int
exceptions_init(PyObject *mod)
{
    module_state *state = get_module_state(mod);
    if (state == NULL) {
        return -1;
    }

#define ADD(NAME, BASE) \
    do { \
        assert(state->NAME == NULL); \
        state->NAME = ADD_NEW_EXCEPTION(mod, NAME, BASE); \
        if (state->NAME == NULL) { \
            return -1; \
        } \
    } while (0)

    // A channel-related operation failed.
    ADD(ChannelError, PyExc_RuntimeError);
    // An operation tried to use a channel that doesn't exist.
    ADD(ChannelNotFoundError, state->ChannelError);
    // An operation tried to use a closed channel.
    ADD(ChannelClosedError, state->ChannelError);
    // An operation tried to pop from an empty channel.
    ADD(ChannelEmptyError, state->ChannelError);
    // An operation tried to close a non-empty channel.
    ADD(ChannelNotEmptyError, state->ChannelError);
#undef ADD

    return 0;
}

static int
handle_channel_error(int err, PyObject *mod, int64_t cid)
{
    if (err == 0) {
        assert(!PyErr_Occurred());
        return 0;
    }
    assert(err < 0);
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    if (err == ERR_CHANNEL_NOT_FOUND) {
        PyErr_Format(state->ChannelNotFoundError,
                     "channel %" PRId64 " not found", cid);
    }
    else if (err == ERR_CHANNEL_CLOSED) {
        PyErr_Format(state->ChannelClosedError,
                     "channel %" PRId64 " is closed", cid);
    }
    else if (err == ERR_CHANNEL_CLOSED_WAITING) {
        PyErr_Format(state->ChannelClosedError,
                     "channel %" PRId64 " has closed", cid);
    }
    else if (err == ERR_CHANNEL_INTERP_CLOSED) {
        PyErr_Format(state->ChannelClosedError,
                     "channel %" PRId64 " is already closed", cid);
    }
    else if (err == ERR_CHANNEL_EMPTY) {
        PyErr_Format(state->ChannelEmptyError,
                     "channel %" PRId64 " is empty", cid);
    }
    else if (err == ERR_CHANNEL_NOT_EMPTY) {
        PyErr_Format(state->ChannelNotEmptyError,
                     "channel %" PRId64 " may not be closed "
                     "if not empty (try force=True)",
                     cid);
    }
    else if (err == ERR_CHANNEL_MUTEX_INIT) {
        PyErr_SetString(state->ChannelError,
                        "can't initialize mutex for new channel");
    }
    else if (err == ERR_CHANNELS_MUTEX_INIT) {
        PyErr_SetString(state->ChannelError,
                        "can't initialize mutex for channel management");
    }
    else if (err == ERR_NO_NEXT_CHANNEL_ID) {
        PyErr_SetString(state->ChannelError,
                        "failed to get a channel ID");
    }
    else {
        assert(PyErr_Occurred());
    }
    return 1;
}


/* the channel queue */

typedef uintptr_t _channelitem_id_t;

typedef struct wait_info {
    PyThread_type_lock mutex;
    enum {
        WAITING_NO_STATUS = 0,
        WAITING_ACQUIRED = 1,
        WAITING_RELEASING = 2,
        WAITING_RELEASED = 3,
    } status;
    int received;
    _channelitem_id_t itemid;
} _waiting_t;

static int
_waiting_init(_waiting_t *waiting)
{
    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    *waiting = (_waiting_t){
        .mutex = mutex,
        .status = WAITING_NO_STATUS,
    };
    return 0;
}

static void
_waiting_clear(_waiting_t *waiting)
{
    assert(waiting->status != WAITING_ACQUIRED
           && waiting->status != WAITING_RELEASING);
    if (waiting->mutex != NULL) {
        PyThread_free_lock(waiting->mutex);
        waiting->mutex = NULL;
    }
}

static _channelitem_id_t
_waiting_get_itemid(_waiting_t *waiting)
{
    return waiting->itemid;
}

static void
_waiting_acquire(_waiting_t *waiting)
{
    assert(waiting->status == WAITING_NO_STATUS);
    PyThread_acquire_lock(waiting->mutex, NOWAIT_LOCK);
    waiting->status = WAITING_ACQUIRED;
}

static void
_waiting_release(_waiting_t *waiting, int received)
{
    assert(waiting->mutex != NULL);
    assert(waiting->status == WAITING_ACQUIRED);
    assert(!waiting->received);

    waiting->status = WAITING_RELEASING;
    PyThread_release_lock(waiting->mutex);
    if (waiting->received != received) {
        assert(received == 1);
        waiting->received = received;
    }
    waiting->status = WAITING_RELEASED;
}

static void
_waiting_finish_releasing(_waiting_t *waiting)
{
    while (waiting->status == WAITING_RELEASING) {
#ifdef MS_WINDOWS
        SwitchToThread();
#elif defined(HAVE_SCHED_H)
        sched_yield();
#endif
    }
}

struct _channelitem;

typedef struct _channelitem {
    _PyCrossInterpreterData *data;
    _waiting_t *waiting;
    struct _channelitem *next;
} _channelitem;

static inline _channelitem_id_t
_channelitem_ID(_channelitem *item)
{
    return (_channelitem_id_t)item;
}

static void
_channelitem_init(_channelitem *item,
                  _PyCrossInterpreterData *data, _waiting_t *waiting)
{
    *item = (_channelitem){
        .data = data,
        .waiting = waiting,
    };
    if (waiting != NULL) {
        waiting->itemid = _channelitem_ID(item);
    }
}

static void
_channelitem_clear(_channelitem *item)
{
    item->next = NULL;

    if (item->data != NULL) {
        // It was allocated in channel_send().
        (void)_release_xid_data(item->data, XID_IGNORE_EXC & XID_FREE);
        item->data = NULL;
    }

    if (item->waiting != NULL) {
        if (item->waiting->status == WAITING_ACQUIRED) {
            _waiting_release(item->waiting, 0);
        }
        item->waiting = NULL;
    }
}

static _channelitem *
_channelitem_new(_PyCrossInterpreterData *data, _waiting_t *waiting)
{
    _channelitem *item = GLOBAL_MALLOC(_channelitem);
    if (item == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    _channelitem_init(item, data, waiting);
    return item;
}

static void
_channelitem_free(_channelitem *item)
{
    _channelitem_clear(item);
    GLOBAL_FREE(item);
}

static void
_channelitem_free_all(_channelitem *item)
{
    while (item != NULL) {
        _channelitem *last = item;
        item = item->next;
        _channelitem_free(last);
    }
}

static void
_channelitem_popped(_channelitem *item,
                    _PyCrossInterpreterData **p_data, _waiting_t **p_waiting)
{
    assert(item->waiting == NULL || item->waiting->status == WAITING_ACQUIRED);
    *p_data = item->data;
    *p_waiting = item->waiting;
    // We clear them here, so they won't be released in _channelitem_clear().
    item->data = NULL;
    item->waiting = NULL;
    _channelitem_free(item);
}

typedef struct _channelqueue {
    int64_t count;
    _channelitem *first;
    _channelitem *last;
} _channelqueue;

static _channelqueue *
_channelqueue_new(void)
{
    _channelqueue *queue = GLOBAL_MALLOC(_channelqueue);
    if (queue == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    queue->count = 0;
    queue->first = NULL;
    queue->last = NULL;
    return queue;
}

static void
_channelqueue_clear(_channelqueue *queue)
{
    _channelitem_free_all(queue->first);
    queue->count = 0;
    queue->first = NULL;
    queue->last = NULL;
}

static void
_channelqueue_free(_channelqueue *queue)
{
    _channelqueue_clear(queue);
    GLOBAL_FREE(queue);
}

static int
_channelqueue_put(_channelqueue *queue,
                  _PyCrossInterpreterData *data, _waiting_t *waiting)
{
    _channelitem *item = _channelitem_new(data, waiting);
    if (item == NULL) {
        return -1;
    }

    queue->count += 1;
    if (queue->first == NULL) {
        queue->first = item;
    }
    else {
        queue->last->next = item;
    }
    queue->last = item;

    if (waiting != NULL) {
        _waiting_acquire(waiting);
    }

    return 0;
}

static int
_channelqueue_get(_channelqueue *queue,
                  _PyCrossInterpreterData **p_data, _waiting_t **p_waiting)
{
    _channelitem *item = queue->first;
    if (item == NULL) {
        return ERR_CHANNEL_EMPTY;
    }
    queue->first = item->next;
    if (queue->last == item) {
        queue->last = NULL;
    }
    queue->count -= 1;

    _channelitem_popped(item, p_data, p_waiting);
    return 0;
}

static int
_channelqueue_find(_channelqueue *queue, _channelitem_id_t itemid,
                   _channelitem **p_item, _channelitem **p_prev)
{
    _channelitem *prev = NULL;
    _channelitem *item = NULL;
    if (queue->first != NULL) {
        if (_channelitem_ID(queue->first) == itemid) {
            item = queue->first;
        }
        else {
            prev = queue->first;
            while (prev->next != NULL) {
                if (_channelitem_ID(prev->next) == itemid) {
                    item = prev->next;
                    break;
                }
                prev = prev->next;
            }
            if (item == NULL) {
                prev = NULL;
            }
        }
    }
    if (p_item != NULL) {
        *p_item = item;
    }
    if (p_prev != NULL) {
        *p_prev = prev;
    }
    return (item != NULL);
}

static void
_channelqueue_remove(_channelqueue *queue, _channelitem_id_t itemid,
                     _PyCrossInterpreterData **p_data, _waiting_t **p_waiting)
{
    _channelitem *prev = NULL;
    _channelitem *item = NULL;
    int found = _channelqueue_find(queue, itemid, &item, &prev);
    if (!found) {
        return;
    }

    assert(item->waiting != NULL);
    assert(!item->waiting->received);
    if (prev == NULL) {
        assert(queue->first == item);
        queue->first = item->next;
    }
    else {
        assert(queue->first != item);
        assert(prev->next == item);
        prev->next = item->next;
    }
    item->next = NULL;

    if (queue->last == item) {
        queue->last = prev;
    }
    queue->count -= 1;

    _channelitem_popped(item, p_data, p_waiting);
}

static void
_channelqueue_drop_interpreter(_channelqueue *queue, int64_t interp)
{
    _channelitem *prev = NULL;
    _channelitem *next = queue->first;
    while (next != NULL) {
        _channelitem *item = next;
        next = item->next;
        if (item->data->interp == interp) {
            if (prev == NULL) {
                queue->first = item->next;
            }
            else {
                prev->next = item->next;
            }
            _channelitem_free(item);
            queue->count -= 1;
        }
        else {
            prev = item;
        }
    }
}


/* channel-interpreter associations */

struct _channelend;

typedef struct _channelend {
    struct _channelend *next;
    int64_t interp;
    int open;
} _channelend;

static _channelend *
_channelend_new(int64_t interp)
{
    _channelend *end = GLOBAL_MALLOC(_channelend);
    if (end == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    end->next = NULL;
    end->interp = interp;
    end->open = 1;
    return end;
}

static void
_channelend_free(_channelend *end)
{
    GLOBAL_FREE(end);
}

static void
_channelend_free_all(_channelend *end)
{
    while (end != NULL) {
        _channelend *last = end;
        end = end->next;
        _channelend_free(last);
    }
}

static _channelend *
_channelend_find(_channelend *first, int64_t interp, _channelend **pprev)
{
    _channelend *prev = NULL;
    _channelend *end = first;
    while (end != NULL) {
        if (end->interp == interp) {
            break;
        }
        prev = end;
        end = end->next;
    }
    if (pprev != NULL) {
        *pprev = prev;
    }
    return end;
}

typedef struct _channelassociations {
    // Note that the list entries are never removed for interpreter
    // for which the channel is closed.  This should not be a problem in
    // practice.  Also, a channel isn't automatically closed when an
    // interpreter is destroyed.
    int64_t numsendopen;
    int64_t numrecvopen;
    _channelend *send;
    _channelend *recv;
} _channelends;

static _channelends *
_channelends_new(void)
{
    _channelends *ends = GLOBAL_MALLOC(_channelends);
    if (ends== NULL) {
        return NULL;
    }
    ends->numsendopen = 0;
    ends->numrecvopen = 0;
    ends->send = NULL;
    ends->recv = NULL;
    return ends;
}

static void
_channelends_clear(_channelends *ends)
{
    _channelend_free_all(ends->send);
    ends->send = NULL;
    ends->numsendopen = 0;

    _channelend_free_all(ends->recv);
    ends->recv = NULL;
    ends->numrecvopen = 0;
}

static void
_channelends_free(_channelends *ends)
{
    _channelends_clear(ends);
    GLOBAL_FREE(ends);
}

static _channelend *
_channelends_add(_channelends *ends, _channelend *prev, int64_t interp,
                 int send)
{
    _channelend *end = _channelend_new(interp);
    if (end == NULL) {
        return NULL;
    }

    if (prev == NULL) {
        if (send) {
            ends->send = end;
        }
        else {
            ends->recv = end;
        }
    }
    else {
        prev->next = end;
    }
    if (send) {
        ends->numsendopen += 1;
    }
    else {
        ends->numrecvopen += 1;
    }
    return end;
}

static int
_channelends_associate(_channelends *ends, int64_t interp, int send)
{
    _channelend *prev;
    _channelend *end = _channelend_find(send ? ends->send : ends->recv,
                                        interp, &prev);
    if (end != NULL) {
        if (!end->open) {
            return ERR_CHANNEL_CLOSED;
        }
        // already associated
        return 0;
    }
    if (_channelends_add(ends, prev, interp, send) == NULL) {
        return -1;
    }
    return 0;
}

static int
_channelends_is_open(_channelends *ends)
{
    if (ends->numsendopen != 0 || ends->numrecvopen != 0) {
        return 1;
    }
    if (ends->send == NULL && ends->recv == NULL) {
        return 1;
    }
    return 0;
}

static void
_channelends_close_end(_channelends *ends, _channelend *end, int send)
{
    end->open = 0;
    if (send) {
        ends->numsendopen -= 1;
    }
    else {
        ends->numrecvopen -= 1;
    }
}

static int
_channelends_close_interpreter(_channelends *ends, int64_t interp, int which)
{
    _channelend *prev;
    _channelend *end;
    if (which >= 0) {  // send/both
        end = _channelend_find(ends->send, interp, &prev);
        if (end == NULL) {
            // never associated so add it
            end = _channelends_add(ends, prev, interp, 1);
            if (end == NULL) {
                return -1;
            }
        }
        _channelends_close_end(ends, end, 1);
    }
    if (which <= 0) {  // recv/both
        end = _channelend_find(ends->recv, interp, &prev);
        if (end == NULL) {
            // never associated so add it
            end = _channelends_add(ends, prev, interp, 0);
            if (end == NULL) {
                return -1;
            }
        }
        _channelends_close_end(ends, end, 0);
    }
    return 0;
}

static void
_channelends_drop_interpreter(_channelends *ends, int64_t interp)
{
    _channelend *end;
    end = _channelend_find(ends->send, interp, NULL);
    if (end != NULL) {
        _channelends_close_end(ends, end, 1);
    }
    end = _channelend_find(ends->recv, interp, NULL);
    if (end != NULL) {
        _channelends_close_end(ends, end, 0);
    }
}

static void
_channelends_close_all(_channelends *ends, int which, int force)
{
    // XXX Handle the ends.
    // XXX Handle force is True.

    // Ensure all the "send"-associated interpreters are closed.
    _channelend *end;
    for (end = ends->send; end != NULL; end = end->next) {
        _channelends_close_end(ends, end, 1);
    }

    // Ensure all the "recv"-associated interpreters are closed.
    for (end = ends->recv; end != NULL; end = end->next) {
        _channelends_close_end(ends, end, 0);
    }
}


/* each channel's state */

struct _channel;
struct _channel_closing;
static void _channel_clear_closing(struct _channel *);
static void _channel_finish_closing(struct _channel *);

typedef struct _channel {
    PyThread_type_lock mutex;
    _channelqueue *queue;
    _channelends *ends;
    int open;
    struct _channel_closing *closing;
} _PyChannelState;

static _PyChannelState *
_channel_new(PyThread_type_lock mutex)
{
    _PyChannelState *chan = GLOBAL_MALLOC(_PyChannelState);
    if (chan == NULL) {
        return NULL;
    }
    chan->mutex = mutex;
    chan->queue = _channelqueue_new();
    if (chan->queue == NULL) {
        GLOBAL_FREE(chan);
        return NULL;
    }
    chan->ends = _channelends_new();
    if (chan->ends == NULL) {
        _channelqueue_free(chan->queue);
        GLOBAL_FREE(chan);
        return NULL;
    }
    chan->open = 1;
    chan->closing = NULL;
    return chan;
}

static void
_channel_free(_PyChannelState *chan)
{
    _channel_clear_closing(chan);
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    _channelqueue_free(chan->queue);
    _channelends_free(chan->ends);
    PyThread_release_lock(chan->mutex);

    PyThread_free_lock(chan->mutex);
    GLOBAL_FREE(chan);
}

static int
_channel_add(_PyChannelState *chan, int64_t interp,
             _PyCrossInterpreterData *data, _waiting_t *waiting)
{
    int res = -1;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    if (!chan->open) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }
    if (_channelends_associate(chan->ends, interp, 1) != 0) {
        res = ERR_CHANNEL_INTERP_CLOSED;
        goto done;
    }

    if (_channelqueue_put(chan->queue, data, waiting) != 0) {
        goto done;
    }
    // Any errors past this point must cause a _waiting_release() call.

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static int
_channel_next(_PyChannelState *chan, int64_t interp,
              _PyCrossInterpreterData **p_data, _waiting_t **p_waiting)
{
    int err = 0;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    if (!chan->open) {
        err = ERR_CHANNEL_CLOSED;
        goto done;
    }
    if (_channelends_associate(chan->ends, interp, 0) != 0) {
        err = ERR_CHANNEL_INTERP_CLOSED;
        goto done;
    }

    int empty = _channelqueue_get(chan->queue, p_data, p_waiting);
    assert(empty == 0 || empty == ERR_CHANNEL_EMPTY);
    assert(!PyErr_Occurred());
    if (empty && chan->closing != NULL) {
        chan->open = 0;
    }

done:
    PyThread_release_lock(chan->mutex);
    if (chan->queue->count == 0) {
        _channel_finish_closing(chan);
    }
    return err;
}

static void
_channel_remove(_PyChannelState *chan, _channelitem_id_t itemid)
{
    _PyCrossInterpreterData *data = NULL;
    _waiting_t *waiting = NULL;

    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    _channelqueue_remove(chan->queue, itemid, &data, &waiting);
    PyThread_release_lock(chan->mutex);

    (void)_release_xid_data(data, XID_IGNORE_EXC | XID_FREE);
    if (waiting != NULL) {
        _waiting_release(waiting, 0);
    }

    if (chan->queue->count == 0) {
        _channel_finish_closing(chan);
    }
}

static int
_channel_close_interpreter(_PyChannelState *chan, int64_t interp, int end)
{
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    int res = -1;
    if (!chan->open) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }

    if (_channelends_close_interpreter(chan->ends, interp, end) != 0) {
        goto done;
    }
    chan->open = _channelends_is_open(chan->ends);
    // XXX Activate the "closing" mechanism?

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static void
_channel_drop_interpreter(_PyChannelState *chan, int64_t interp)
{
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    _channelqueue_drop_interpreter(chan->queue, interp);
    _channelends_drop_interpreter(chan->ends, interp);
    chan->open = _channelends_is_open(chan->ends);

    PyThread_release_lock(chan->mutex);
}

static int
_channel_close_all(_PyChannelState *chan, int end, int force)
{
    int res = -1;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);

    if (!chan->open) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }

    if (!force && chan->queue->count > 0) {
        res = ERR_CHANNEL_NOT_EMPTY;
        goto done;
    }

    chan->open = 0;

    // We *could* also just leave these in place, since we've marked
    // the channel as closed already.
    _channelends_close_all(chan->ends, end, force);

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}


/* the set of channels */

struct _channelref;

typedef struct _channelref {
    int64_t id;
    _PyChannelState *chan;
    struct _channelref *next;
    // The number of ChannelID objects referring to this channel.
    Py_ssize_t objcount;
} _channelref;

static _channelref *
_channelref_new(int64_t id, _PyChannelState *chan)
{
    _channelref *ref = GLOBAL_MALLOC(_channelref);
    if (ref == NULL) {
        return NULL;
    }
    ref->id = id;
    ref->chan = chan;
    ref->next = NULL;
    ref->objcount = 0;
    return ref;
}

//static void
//_channelref_clear(_channelref *ref)
//{
//    ref->id = -1;
//    ref->chan = NULL;
//    ref->next = NULL;
//    ref->objcount = 0;
//}

static void
_channelref_free(_channelref *ref)
{
    if (ref->chan != NULL) {
        _channel_clear_closing(ref->chan);
    }
    //_channelref_clear(ref);
    GLOBAL_FREE(ref);
}

static _channelref *
_channelref_find(_channelref *first, int64_t id, _channelref **pprev)
{
    _channelref *prev = NULL;
    _channelref *ref = first;
    while (ref != NULL) {
        if (ref->id == id) {
            break;
        }
        prev = ref;
        ref = ref->next;
    }
    if (pprev != NULL) {
        *pprev = prev;
    }
    return ref;
}

typedef struct _channels {
    PyThread_type_lock mutex;
    _channelref *head;
    int64_t numopen;
    int64_t next_id;
} _channels;

static void
_channels_init(_channels *channels, PyThread_type_lock mutex)
{
    channels->mutex = mutex;
    channels->head = NULL;
    channels->numopen = 0;
    channels->next_id = 0;
}

static void
_channels_fini(_channels *channels)
{
    assert(channels->numopen == 0);
    assert(channels->head == NULL);
    if (channels->mutex != NULL) {
        PyThread_free_lock(channels->mutex);
        channels->mutex = NULL;
    }
}

static int64_t
_channels_next_id(_channels *channels)  // needs lock
{
    int64_t id = channels->next_id;
    if (id < 0) {
        /* overflow */
        return -1;
    }
    channels->next_id += 1;
    return id;
}

static int
_channels_lookup(_channels *channels, int64_t id, PyThread_type_lock *pmutex,
                 _PyChannelState **res)
{
    int err = -1;
    _PyChannelState *chan = NULL;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    if (pmutex != NULL) {
        *pmutex = NULL;
    }

    _channelref *ref = _channelref_find(channels->head, id, NULL);
    if (ref == NULL) {
        err = ERR_CHANNEL_NOT_FOUND;
        goto done;
    }
    if (ref->chan == NULL || !ref->chan->open) {
        err = ERR_CHANNEL_CLOSED;
        goto done;
    }

    if (pmutex != NULL) {
        // The mutex will be closed by the caller.
        *pmutex = channels->mutex;
    }

    chan = ref->chan;
    err = 0;

done:
    if (pmutex == NULL || *pmutex == NULL) {
        PyThread_release_lock(channels->mutex);
    }
    *res = chan;
    return err;
}

static int64_t
_channels_add(_channels *channels, _PyChannelState *chan)
{
    int64_t cid = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    // Create a new ref.
    int64_t id = _channels_next_id(channels);
    if (id < 0) {
        cid = ERR_NO_NEXT_CHANNEL_ID;
        goto done;
    }
    _channelref *ref = _channelref_new(id, chan);
    if (ref == NULL) {
        goto done;
    }

    // Add it to the list.
    // We assume that the channel is a new one (not already in the list).
    ref->next = channels->head;
    channels->head = ref;
    channels->numopen += 1;

    cid = id;
done:
    PyThread_release_lock(channels->mutex);
    return cid;
}

/* forward */
static int _channel_set_closing(struct _channelref *, PyThread_type_lock);

static int
_channels_close(_channels *channels, int64_t cid, _PyChannelState **pchan,
                int end, int force)
{
    int res = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    if (pchan != NULL) {
        *pchan = NULL;
    }

    _channelref *ref = _channelref_find(channels->head, cid, NULL);
    if (ref == NULL) {
        res = ERR_CHANNEL_NOT_FOUND;
        goto done;
    }

    if (ref->chan == NULL) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }
    else if (!force && end == CHANNEL_SEND && ref->chan->closing != NULL) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }
    else {
        int err = _channel_close_all(ref->chan, end, force);
        if (err != 0) {
            if (end == CHANNEL_SEND && err == ERR_CHANNEL_NOT_EMPTY) {
                if (ref->chan->closing != NULL) {
                    res = ERR_CHANNEL_CLOSED;
                    goto done;
                }
                // Mark the channel as closing and return.  The channel
                // will be cleaned up in _channel_next().
                PyErr_Clear();
                int err = _channel_set_closing(ref, channels->mutex);
                if (err != 0) {
                    res = err;
                    goto done;
                }
                if (pchan != NULL) {
                    *pchan = ref->chan;
                }
                res = 0;
            }
            else {
                res = err;
            }
            goto done;
        }
        if (pchan != NULL) {
            *pchan = ref->chan;
        }
        else  {
            _channel_free(ref->chan);
        }
        ref->chan = NULL;
    }

    res = 0;
done:
    PyThread_release_lock(channels->mutex);
    return res;
}

static void
_channels_remove_ref(_channels *channels, _channelref *ref, _channelref *prev,
                     _PyChannelState **pchan)
{
    if (ref == channels->head) {
        channels->head = ref->next;
    }
    else {
        prev->next = ref->next;
    }
    channels->numopen -= 1;

    if (pchan != NULL) {
        *pchan = ref->chan;
    }
    _channelref_free(ref);
}

static int
_channels_remove(_channels *channels, int64_t id, _PyChannelState **pchan)
{
    int res = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    if (pchan != NULL) {
        *pchan = NULL;
    }

    _channelref *prev = NULL;
    _channelref *ref = _channelref_find(channels->head, id, &prev);
    if (ref == NULL) {
        res = ERR_CHANNEL_NOT_FOUND;
        goto done;
    }

    _channels_remove_ref(channels, ref, prev, pchan);

    res = 0;
done:
    PyThread_release_lock(channels->mutex);
    return res;
}

static int
_channels_add_id_object(_channels *channels, int64_t id)
{
    int res = -1;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    _channelref *ref = _channelref_find(channels->head, id, NULL);
    if (ref == NULL) {
        res = ERR_CHANNEL_NOT_FOUND;
        goto done;
    }
    ref->objcount += 1;

    res = 0;
done:
    PyThread_release_lock(channels->mutex);
    return res;
}

static void
_channels_drop_id_object(_channels *channels, int64_t id)
{
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    _channelref *prev = NULL;
    _channelref *ref = _channelref_find(channels->head, id, &prev);
    if (ref == NULL) {
        // Already destroyed.
        goto done;
    }
    ref->objcount -= 1;

    // Destroy if no longer used.
    if (ref->objcount == 0) {
        _PyChannelState *chan = NULL;
        _channels_remove_ref(channels, ref, prev, &chan);
        if (chan != NULL) {
            _channel_free(chan);
        }
    }

done:
    PyThread_release_lock(channels->mutex);
}

static int64_t *
_channels_list_all(_channels *channels, int64_t *count)
{
    int64_t *cids = NULL;
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);
    int64_t *ids = PyMem_NEW(int64_t, (Py_ssize_t)(channels->numopen));
    if (ids == NULL) {
        goto done;
    }
    _channelref *ref = channels->head;
    for (int64_t i=0; ref != NULL; ref = ref->next, i++) {
        ids[i] = ref->id;
    }
    *count = channels->numopen;

    cids = ids;
done:
    PyThread_release_lock(channels->mutex);
    return cids;
}

static void
_channels_drop_interpreter(_channels *channels, int64_t interp)
{
    PyThread_acquire_lock(channels->mutex, WAIT_LOCK);

    _channelref *ref = channels->head;
    for (; ref != NULL; ref = ref->next) {
        if (ref->chan != NULL) {
            _channel_drop_interpreter(ref->chan, interp);
        }
    }

    PyThread_release_lock(channels->mutex);
}


/* support for closing non-empty channels */

struct _channel_closing {
    struct _channelref *ref;
};

static int
_channel_set_closing(struct _channelref *ref, PyThread_type_lock mutex) {
    struct _channel *chan = ref->chan;
    if (chan == NULL) {
        // already closed
        return 0;
    }
    int res = -1;
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    if (chan->closing != NULL) {
        res = ERR_CHANNEL_CLOSED;
        goto done;
    }
    chan->closing = GLOBAL_MALLOC(struct _channel_closing);
    if (chan->closing == NULL) {
        goto done;
    }
    chan->closing->ref = ref;

    res = 0;
done:
    PyThread_release_lock(chan->mutex);
    return res;
}

static void
_channel_clear_closing(struct _channel *chan) {
    PyThread_acquire_lock(chan->mutex, WAIT_LOCK);
    if (chan->closing != NULL) {
        GLOBAL_FREE(chan->closing);
        chan->closing = NULL;
    }
    PyThread_release_lock(chan->mutex);
}

static void
_channel_finish_closing(struct _channel *chan) {
    struct _channel_closing *closing = chan->closing;
    if (closing == NULL) {
        return;
    }
    _channelref *ref = closing->ref;
    _channel_clear_closing(chan);
    // Do the things that would have been done in _channels_close().
    ref->chan = NULL;
    _channel_free(chan);
}


/* "high"-level channel-related functions */

// Create a new channel.
static int64_t
channel_create(_channels *channels)
{
    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return ERR_CHANNEL_MUTEX_INIT;
    }
    _PyChannelState *chan = _channel_new(mutex);
    if (chan == NULL) {
        PyThread_free_lock(mutex);
        return -1;
    }
    int64_t id = _channels_add(channels, chan);
    if (id < 0) {
        _channel_free(chan);
    }
    return id;
}

// Completely destroy the channel.
static int
channel_destroy(_channels *channels, int64_t id)
{
    _PyChannelState *chan = NULL;
    int err = _channels_remove(channels, id, &chan);
    if (err != 0) {
        return err;
    }
    if (chan != NULL) {
        _channel_free(chan);
    }
    return 0;
}

// Push an object onto the channel.
// The current interpreter gets associated with the send end of the channel.
// Optionally request to be notified when it is received.
static int
channel_send(_channels *channels, int64_t id, PyObject *obj,
             _waiting_t *waiting)
{
    PyInterpreterState *interp = _get_current_interp();
    if (interp == NULL) {
        return -1;
    }
    int64_t interpid = PyInterpreterState_GetID(interp);

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = NULL;
    int err = _channels_lookup(channels, id, &mutex, &chan);
    if (err != 0) {
        return err;
    }
    assert(chan != NULL);
    // Past this point we are responsible for releasing the mutex.

    if (chan->closing != NULL) {
        PyThread_release_lock(mutex);
        return ERR_CHANNEL_CLOSED;
    }

    // Convert the object to cross-interpreter data.
    _PyCrossInterpreterData *data = GLOBAL_MALLOC(_PyCrossInterpreterData);
    if (data == NULL) {
        PyThread_release_lock(mutex);
        return -1;
    }
    if (_PyObject_GetCrossInterpreterData(obj, data) != 0) {
        PyThread_release_lock(mutex);
        GLOBAL_FREE(data);
        return -1;
    }

    // Add the data to the channel.
    int res = _channel_add(chan, interpid, data, waiting);
    PyThread_release_lock(mutex);
    if (res != 0) {
        // We may chain an exception here:
        (void)_release_xid_data(data, 0);
        GLOBAL_FREE(data);
        return res;
    }

    return 0;
}

// Basically, un-send an object.
static void
channel_clear_sent(_channels *channels, int64_t cid, _waiting_t *waiting)
{
    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = NULL;
    int err = _channels_lookup(channels, cid, &mutex, &chan);
    if (err != 0) {
        // The channel was already closed, etc.
        assert(waiting->status == WAITING_RELEASED);
        return;  // Ignore the error.
    }
    assert(chan != NULL);
    // Past this point we are responsible for releasing the mutex.

    _channelitem_id_t itemid = _waiting_get_itemid(waiting);
    _channel_remove(chan, itemid);

    PyThread_release_lock(mutex);
}

// Like channel_send(), but strictly wait for the object to be received.
static int
channel_send_wait(_channels *channels, int64_t cid, PyObject *obj,
                   PY_TIMEOUT_T timeout)
{
    // We use a stack variable here, so we must ensure that &waiting
    // is not held by any channel item at the point this function exits.
    _waiting_t waiting;
    if (_waiting_init(&waiting) < 0) {
        assert(PyErr_Occurred());
        return -1;
    }

    /* Queue up the object. */
    int res = channel_send(channels, cid, obj, &waiting);
    if (res < 0) {
        assert(waiting.status == WAITING_NO_STATUS);
        goto finally;
    }

    /* Wait until the object is received. */
    if (wait_for_lock(waiting.mutex, timeout) < 0) {
        assert(PyErr_Occurred());
        _waiting_finish_releasing(&waiting);
        /* The send() call is failing now, so make sure the item
           won't be received. */
        channel_clear_sent(channels, cid, &waiting);
        assert(waiting.status == WAITING_RELEASED);
        if (!waiting.received) {
            res = -1;
            goto finally;
        }
        // XXX Emit a warning if not a TimeoutError?
        PyErr_Clear();
    }
    else {
        _waiting_finish_releasing(&waiting);
        assert(waiting.status == WAITING_RELEASED);
        if (!waiting.received) {
            res = ERR_CHANNEL_CLOSED_WAITING;
            goto finally;
        }
    }

    /* success! */
    res = 0;

finally:
    _waiting_clear(&waiting);
    return res;
}

// Pop the next object off the channel.  Fail if empty.
// The current interpreter gets associated with the recv end of the channel.
// XXX Support a "wait" mutex?
static int
channel_recv(_channels *channels, int64_t id, PyObject **res)
{
    int err;
    *res = NULL;

    PyInterpreterState *interp = _get_current_interp();
    if (interp == NULL) {
        // XXX Is this always an error?
        if (PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }
    int64_t interpid = PyInterpreterState_GetID(interp);

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = NULL;
    err = _channels_lookup(channels, id, &mutex, &chan);
    if (err != 0) {
        return err;
    }
    assert(chan != NULL);
    // Past this point we are responsible for releasing the mutex.

    // Pop off the next item from the channel.
    _PyCrossInterpreterData *data = NULL;
    _waiting_t *waiting = NULL;
    err = _channel_next(chan, interpid, &data, &waiting);
    PyThread_release_lock(mutex);
    if (err != 0) {
        return err;
    }
    else if (data == NULL) {
        assert(!PyErr_Occurred());
        return 0;
    }

    // Convert the data back to an object.
    PyObject *obj = _PyCrossInterpreterData_NewObject(data);
    if (obj == NULL) {
        assert(PyErr_Occurred());
        // It was allocated in channel_send(), so we free it.
        (void)_release_xid_data(data, XID_IGNORE_EXC | XID_FREE);
        if (waiting != NULL) {
            _waiting_release(waiting, 0);
        }
        return -1;
    }
    // It was allocated in channel_send(), so we free it.
    int release_res = _release_xid_data(data, XID_FREE);
    if (release_res < 0) {
        // The source interpreter has been destroyed already.
        assert(PyErr_Occurred());
        Py_DECREF(obj);
        if (waiting != NULL) {
            _waiting_release(waiting, 0);
        }
        return -1;
    }

    // Notify the sender.
    if (waiting != NULL) {
        _waiting_release(waiting, 1);
    }

    *res = obj;
    return 0;
}

// Disallow send/recv for the current interpreter.
// The channel is marked as closed if no other interpreters
// are currently associated.
static int
channel_release(_channels *channels, int64_t cid, int send, int recv)
{
    PyInterpreterState *interp = _get_current_interp();
    if (interp == NULL) {
        return -1;
    }
    int64_t interpid = PyInterpreterState_GetID(interp);

    // Look up the channel.
    PyThread_type_lock mutex = NULL;
    _PyChannelState *chan = NULL;
    int err = _channels_lookup(channels, cid, &mutex, &chan);
    if (err != 0) {
        return err;
    }
    // Past this point we are responsible for releasing the mutex.

    // Close one or both of the two ends.
    int res = _channel_close_interpreter(chan, interpid, send-recv);
    PyThread_release_lock(mutex);
    return res;
}

// Close the channel (for all interpreters).  Fail if it's already closed.
// Close immediately if it's empty.  Otherwise, disallow sending and
// finally close once empty.  Optionally, immediately clear and close it.
static int
channel_close(_channels *channels, int64_t id, int end, int force)
{
    return _channels_close(channels, id, NULL, end, force);
}

// Return true if the identified interpreter is associated
// with the given end of the channel.
static int
channel_is_associated(_channels *channels, int64_t cid, int64_t interp,
                       int send)
{
    _PyChannelState *chan = NULL;
    int err = _channels_lookup(channels, cid, NULL, &chan);
    if (err != 0) {
        return err;
    }
    else if (send && chan->closing != NULL) {
        return ERR_CHANNEL_CLOSED;
    }

    _channelend *end = _channelend_find(send ? chan->ends->send : chan->ends->recv,
                                        interp, NULL);

    return (end != NULL && end->open);
}


/* ChannelID class */

typedef struct channelid {
    PyObject_HEAD
    int64_t id;
    int end;
    int resolve;
    _channels *channels;
} channelid;

struct channel_id_converter_data {
    PyObject *module;
    int64_t cid;
    int end;
};

static int
channel_id_converter(PyObject *arg, void *ptr)
{
    int64_t cid;
    int end = 0;
    struct channel_id_converter_data *data = ptr;
    module_state *state = get_module_state(data->module);
    assert(state != NULL);
    if (PyObject_TypeCheck(arg, state->ChannelIDType)) {
        cid = ((channelid *)arg)->id;
        end = ((channelid *)arg)->end;
    }
    else if (PyIndex_Check(arg)) {
        cid = PyLong_AsLongLong(arg);
        if (cid == -1 && PyErr_Occurred()) {
            return 0;
        }
        if (cid < 0) {
            PyErr_Format(PyExc_ValueError,
                        "channel ID must be a non-negative int, got %R", arg);
            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "channel ID must be an int, got %.100s",
                     Py_TYPE(arg)->tp_name);
        return 0;
    }
    data->cid = cid;
    data->end = end;
    return 1;
}

static int
newchannelid(PyTypeObject *cls, int64_t cid, int end, _channels *channels,
             int force, int resolve, channelid **res)
{
    *res = NULL;

    channelid *self = PyObject_New(channelid, cls);
    if (self == NULL) {
        return -1;
    }
    self->id = cid;
    self->end = end;
    self->resolve = resolve;
    self->channels = channels;

    int err = _channels_add_id_object(channels, cid);
    if (err != 0) {
        if (force && err == ERR_CHANNEL_NOT_FOUND) {
            assert(!PyErr_Occurred());
        }
        else {
            Py_DECREF((PyObject *)self);
            return err;
        }
    }

    *res = self;
    return 0;
}

static _channels * _global_channels(void);

static PyObject *
_channelid_new(PyObject *mod, PyTypeObject *cls,
               PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "send", "recv", "force", "_resolve", NULL};
    int64_t cid;
    int end;
    struct channel_id_converter_data cid_data = {
        .module = mod,
    };
    int send = -1;
    int recv = -1;
    int force = 0;
    int resolve = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&|$pppp:ChannelID.__new__", kwlist,
                                     channel_id_converter, &cid_data,
                                     &send, &recv, &force, &resolve)) {
        return NULL;
    }
    cid = cid_data.cid;
    end = cid_data.end;

    // Handle "send" and "recv".
    if (send == 0 && recv == 0) {
        PyErr_SetString(PyExc_ValueError,
                        "'send' and 'recv' cannot both be False");
        return NULL;
    }
    else if (send == 1) {
        if (recv == 0 || recv == -1) {
            end = CHANNEL_SEND;
        }
        else {
            assert(recv == 1);
            end = 0;
        }
    }
    else if (recv == 1) {
        assert(send == 0 || send == -1);
        end = CHANNEL_RECV;
    }

    PyObject *id = NULL;
    int err = newchannelid(cls, cid, end, _global_channels(),
                           force, resolve,
                           (channelid **)&id);
    if (handle_channel_error(err, mod, cid)) {
        assert(id == NULL);
        return NULL;
    }
    assert(id != NULL);
    return id;
}

static void
channelid_dealloc(PyObject *self)
{
    int64_t cid = ((channelid *)self)->id;
    _channels *channels = ((channelid *)self)->channels;

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    /* "Instances of heap-allocated types hold a reference to their type."
     * See: https://docs.python.org/3.11/howto/isolating-extensions.html#garbage-collection-protocol
     * See: https://docs.python.org/3.11/c-api/typeobj.html#c.PyTypeObject.tp_traverse
    */
    // XXX Why don't we implement Py_TPFLAGS_HAVE_GC, e.g. Py_tp_traverse,
    // like we do for _abc._abc_data?
    Py_DECREF(tp);

    _channels_drop_id_object(channels, cid);
}

static PyObject *
channelid_repr(PyObject *self)
{
    PyTypeObject *type = Py_TYPE(self);
    const char *name = _PyType_Name(type);

    channelid *cid = (channelid *)self;
    const char *fmt;
    if (cid->end == CHANNEL_SEND) {
        fmt = "%s(%" PRId64 ", send=True)";
    }
    else if (cid->end == CHANNEL_RECV) {
        fmt = "%s(%" PRId64 ", recv=True)";
    }
    else {
        fmt = "%s(%" PRId64 ")";
    }
    return PyUnicode_FromFormat(fmt, name, cid->id);
}

static PyObject *
channelid_str(PyObject *self)
{
    channelid *cid = (channelid *)self;
    return PyUnicode_FromFormat("%" PRId64 "", cid->id);
}

static PyObject *
channelid_int(PyObject *self)
{
    channelid *cid = (channelid *)self;
    return PyLong_FromLongLong(cid->id);
}

static Py_hash_t
channelid_hash(PyObject *self)
{
    channelid *cid = (channelid *)self;
    PyObject *id = PyLong_FromLongLong(cid->id);
    if (id == NULL) {
        return -1;
    }
    Py_hash_t hash = PyObject_Hash(id);
    Py_DECREF(id);
    return hash;
}

static PyObject *
channelid_richcompare(PyObject *self, PyObject *other, int op)
{
    PyObject *res = NULL;
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    PyObject *mod = get_module_from_type(Py_TYPE(self));
    if (mod == NULL) {
        return NULL;
    }
    module_state *state = get_module_state(mod);
    if (state == NULL) {
        goto done;
    }

    if (!PyObject_TypeCheck(self, state->ChannelIDType)) {
        res = Py_NewRef(Py_NotImplemented);
        goto done;
    }

    channelid *cid = (channelid *)self;
    int equal;
    if (PyObject_TypeCheck(other, state->ChannelIDType)) {
        channelid *othercid = (channelid *)other;
        equal = (cid->end == othercid->end) && (cid->id == othercid->id);
    }
    else if (PyLong_Check(other)) {
        /* Fast path */
        int overflow;
        long long othercid = PyLong_AsLongLongAndOverflow(other, &overflow);
        if (othercid == -1 && PyErr_Occurred()) {
            goto done;
        }
        equal = !overflow && (othercid >= 0) && (cid->id == othercid);
    }
    else if (PyNumber_Check(other)) {
        PyObject *pyid = PyLong_FromLongLong(cid->id);
        if (pyid == NULL) {
            goto done;
        }
        res = PyObject_RichCompare(pyid, other, op);
        Py_DECREF(pyid);
        goto done;
    }
    else {
        res = Py_NewRef(Py_NotImplemented);
        goto done;
    }

    if ((op == Py_EQ && equal) || (op == Py_NE && !equal)) {
        res = Py_NewRef(Py_True);
    }
    else {
        res = Py_NewRef(Py_False);
    }

done:
    Py_DECREF(mod);
    return res;
}

static PyTypeObject * _get_current_channelend_type(int end);

static PyObject *
_channelobj_from_cid(PyObject *cid, int end)
{
    PyObject *cls = (PyObject *)_get_current_channelend_type(end);
    if (cls == NULL) {
        return NULL;
    }
    PyObject *chan = PyObject_CallFunctionObjArgs(cls, cid, NULL);
    Py_DECREF(cls);
    if (chan == NULL) {
        return NULL;
    }
    return chan;
}

struct _channelid_xid {
    int64_t id;
    int end;
    int resolve;
};

static PyObject *
_channelid_from_xid(_PyCrossInterpreterData *data)
{
    struct _channelid_xid *xid = (struct _channelid_xid *)data->data;

    // It might not be imported yet, so we can't use _get_current_module().
    PyObject *mod = PyImport_ImportModule(MODULE_NAME);
    if (mod == NULL) {
        return NULL;
    }
    assert(mod != Py_None);
    module_state *state = get_module_state(mod);
    if (state == NULL) {
        return NULL;
    }

    // Note that we do not preserve the "resolve" flag.
    PyObject *cid = NULL;
    int err = newchannelid(state->ChannelIDType, xid->id, xid->end,
                           _global_channels(), 0, 0,
                           (channelid **)&cid);
    if (err != 0) {
        assert(cid == NULL);
        (void)handle_channel_error(err, mod, xid->id);
        goto done;
    }
    assert(cid != NULL);
    if (xid->end == 0) {
        goto done;
    }
    if (!xid->resolve) {
        goto done;
    }

    /* Try returning a high-level channel end but fall back to the ID. */
    PyObject *chan = _channelobj_from_cid(cid, xid->end);
    if (chan == NULL) {
        PyErr_Clear();
        goto done;
    }
    Py_DECREF(cid);
    cid = chan;

done:
    Py_DECREF(mod);
    return cid;
}

static int
_channelid_shared(PyThreadState *tstate, PyObject *obj,
                  _PyCrossInterpreterData *data)
{
    if (_PyCrossInterpreterData_InitWithSize(
            data, tstate->interp, sizeof(struct _channelid_xid), obj,
            _channelid_from_xid
            ) < 0)
    {
        return -1;
    }
    struct _channelid_xid *xid = (struct _channelid_xid *)data->data;
    xid->id = ((channelid *)obj)->id;
    xid->end = ((channelid *)obj)->end;
    xid->resolve = ((channelid *)obj)->resolve;
    return 0;
}

static PyObject *
channelid_end(PyObject *self, void *end)
{
    int force = 1;
    channelid *cid = (channelid *)self;
    if (end != NULL) {
        PyObject *id = NULL;
        int err = newchannelid(Py_TYPE(self), cid->id, *(int *)end,
                               cid->channels, force, cid->resolve,
                               (channelid **)&id);
        if (err != 0) {
            assert(id == NULL);
            PyObject *mod = get_module_from_type(Py_TYPE(self));
            if (mod == NULL) {
                return NULL;
            }
            (void)handle_channel_error(err, mod, cid->id);
            Py_DECREF(mod);
            return NULL;
        }
        assert(id != NULL);
        return id;
    }

    if (cid->end == CHANNEL_SEND) {
        return PyUnicode_InternFromString("send");
    }
    if (cid->end == CHANNEL_RECV) {
        return PyUnicode_InternFromString("recv");
    }
    return PyUnicode_InternFromString("both");
}

static int _channelid_end_send = CHANNEL_SEND;
static int _channelid_end_recv = CHANNEL_RECV;

static PyGetSetDef channelid_getsets[] = {
    {"end", (getter)channelid_end, NULL,
     PyDoc_STR("'send', 'recv', or 'both'")},
    {"send", (getter)channelid_end, NULL,
     PyDoc_STR("the 'send' end of the channel"), &_channelid_end_send},
    {"recv", (getter)channelid_end, NULL,
     PyDoc_STR("the 'recv' end of the channel"), &_channelid_end_recv},
    {NULL}
};

PyDoc_STRVAR(channelid_doc,
"A channel ID identifies a channel and may be used as an int.");

static PyType_Slot channelid_typeslots[] = {
    {Py_tp_dealloc, (destructor)channelid_dealloc},
    {Py_tp_doc, (void *)channelid_doc},
    {Py_tp_repr, (reprfunc)channelid_repr},
    {Py_tp_str, (reprfunc)channelid_str},
    {Py_tp_hash, channelid_hash},
    {Py_tp_richcompare, channelid_richcompare},
    {Py_tp_getset, channelid_getsets},
    // number slots
    {Py_nb_int, (unaryfunc)channelid_int},
    {Py_nb_index,  (unaryfunc)channelid_int},
    {0, NULL},
};

static PyType_Spec channelid_typespec = {
    .name = MODULE_NAME ".ChannelID",
    .basicsize = sizeof(channelid),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = channelid_typeslots,
};


/* SendChannel and RecvChannel classes */

// XXX Use a new __xid__ protocol instead?

static PyTypeObject *
_get_current_channelend_type(int end)
{
    module_state *state = _get_current_module_state();
    if (state == NULL) {
        return NULL;
    }
    PyTypeObject *cls;
    if (end == CHANNEL_SEND) {
        cls = state->send_channel_type;
    }
    else {
        assert(end == CHANNEL_RECV);
        cls = state->recv_channel_type;
    }
    if (cls == NULL) {
        PyObject *highlevel = PyImport_ImportModule("interpreters");
        if (highlevel == NULL) {
            PyErr_Clear();
            highlevel = PyImport_ImportModule("test.support.interpreters");
            if (highlevel == NULL) {
                return NULL;
            }
        }
        Py_DECREF(highlevel);
        if (end == CHANNEL_SEND) {
            cls = state->send_channel_type;
        }
        else {
            cls = state->recv_channel_type;
        }
        assert(cls != NULL);
    }
    return cls;
}

static PyObject *
_channelend_from_xid(_PyCrossInterpreterData *data)
{
    channelid *cid = (channelid *)_channelid_from_xid(data);
    if (cid == NULL) {
        return NULL;
    }
    PyTypeObject *cls = _get_current_channelend_type(cid->end);
    if (cls == NULL) {
        Py_DECREF(cid);
        return NULL;
    }
    PyObject *obj = PyObject_CallOneArg((PyObject *)cls, (PyObject *)cid);
    Py_DECREF(cid);
    return obj;
}

static int
_channelend_shared(PyThreadState *tstate, PyObject *obj,
                    _PyCrossInterpreterData *data)
{
    PyObject *cidobj = PyObject_GetAttrString(obj, "_id");
    if (cidobj == NULL) {
        return -1;
    }
    int res = _channelid_shared(tstate, cidobj, data);
    Py_DECREF(cidobj);
    if (res < 0) {
        return -1;
    }
    data->new_object = _channelend_from_xid;
    return 0;
}

static int
set_channelend_types(PyObject *mod, PyTypeObject *send, PyTypeObject *recv)
{
    module_state *state = get_module_state(mod);
    if (state == NULL) {
        return -1;
    }
    struct xid_class_registry *xid_classes = &state->xid_classes;

    if (state->send_channel_type != NULL
        || state->recv_channel_type != NULL)
    {
        PyErr_SetString(PyExc_TypeError, "already registered");
        return -1;
    }
    state->send_channel_type = (PyTypeObject *)Py_NewRef(send);
    state->recv_channel_type = (PyTypeObject *)Py_NewRef(recv);

    if (register_xid_class(send, _channelend_shared, xid_classes)) {
        return -1;
    }
    if (register_xid_class(recv, _channelend_shared, xid_classes)) {
        return -1;
    }

    return 0;
}


/* module level code ********************************************************/

/* globals is the process-global state for the module.  It holds all
   the data that we need to share between interpreters, so it cannot
   hold PyObject values. */
static struct globals {
    int module_count;
    _channels channels;
} _globals = {0};

static int
_globals_init(void)
{
    // XXX This isn't thread-safe.
    _globals.module_count++;
    if (_globals.module_count > 1) {
        // Already initialized.
        return 0;
    }

    assert(_globals.channels.mutex == NULL);
    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return ERR_CHANNELS_MUTEX_INIT;
    }
    _channels_init(&_globals.channels, mutex);
    return 0;
}

static void
_globals_fini(void)
{
    // XXX This isn't thread-safe.
    _globals.module_count--;
    if (_globals.module_count > 0) {
        return;
    }

    _channels_fini(&_globals.channels);
}

static _channels *
_global_channels(void) {
    return &_globals.channels;
}


static void
clear_interpreter(void *data)
{
    if (_globals.module_count == 0) {
        return;
    }
    PyInterpreterState *interp = (PyInterpreterState *)data;
    assert(interp == _get_current_interp());
    int64_t id = PyInterpreterState_GetID(interp);
    _channels_drop_interpreter(&_globals.channels, id);
}


static PyObject *
channelsmod_create(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int64_t cid = channel_create(&_globals.channels);
    if (cid < 0) {
        (void)handle_channel_error(-1, self, cid);
        return NULL;
    }
    module_state *state = get_module_state(self);
    if (state == NULL) {
        return NULL;
    }
    PyObject *id = NULL;
    int err = newchannelid(state->ChannelIDType, cid, 0,
                           &_globals.channels, 0, 0,
                           (channelid **)&id);
    if (handle_channel_error(err, self, cid)) {
        assert(id == NULL);
        err = channel_destroy(&_globals.channels, cid);
        if (handle_channel_error(err, self, cid)) {
            // XXX issue a warning?
        }
        return NULL;
    }
    assert(id != NULL);
    assert(((channelid *)id)->channels != NULL);
    return id;
}

PyDoc_STRVAR(channelsmod_create_doc,
"channel_create() -> cid\n\
\n\
Create a new cross-interpreter channel and return a unique generated ID.");

static PyObject *
channelsmod_destroy(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:channel_destroy", kwlist,
                                     channel_id_converter, &cid_data)) {
        return NULL;
    }
    cid = cid_data.cid;

    int err = channel_destroy(&_globals.channels, cid);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channelsmod_destroy_doc,
"channel_destroy(cid)\n\
\n\
Close and finalize the channel.  Afterward attempts to use the channel\n\
will behave as though it never existed.");

static PyObject *
channelsmod_list_all(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int64_t count = 0;
    int64_t *cids = _channels_list_all(&_globals.channels, &count);
    if (cids == NULL) {
        if (count == 0) {
            return PyList_New(0);
        }
        return NULL;
    }
    PyObject *ids = PyList_New((Py_ssize_t)count);
    if (ids == NULL) {
        goto finally;
    }
    module_state *state = get_module_state(self);
    if (state == NULL) {
        Py_DECREF(ids);
        ids = NULL;
        goto finally;
    }
    int64_t *cur = cids;
    for (int64_t i=0; i < count; cur++, i++) {
        PyObject *id = NULL;
        int err = newchannelid(state->ChannelIDType, *cur, 0,
                               &_globals.channels, 0, 0,
                               (channelid **)&id);
        if (handle_channel_error(err, self, *cur)) {
            assert(id == NULL);
            Py_SETREF(ids, NULL);
            break;
        }
        assert(id != NULL);
        PyList_SET_ITEM(ids, (Py_ssize_t)i, id);
    }

finally:
    PyMem_Free(cids);
    return ids;
}

PyDoc_STRVAR(channelsmod_list_all_doc,
"channel_list_all() -> [cid]\n\
\n\
Return the list of all IDs for active channels.");

static PyObject *
channelsmod_list_interpreters(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "send", NULL};
    int64_t cid;            /* Channel ID */
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    int send = 0;           /* Send or receive end? */
    int64_t id;
    PyObject *ids, *id_obj;
    PyInterpreterState *interp;

    if (!PyArg_ParseTupleAndKeywords(
            args, kwds, "O&$p:channel_list_interpreters",
            kwlist, channel_id_converter, &cid_data, &send)) {
        return NULL;
    }
    cid = cid_data.cid;

    ids = PyList_New(0);
    if (ids == NULL) {
        goto except;
    }

    interp = PyInterpreterState_Head();
    while (interp != NULL) {
        id = PyInterpreterState_GetID(interp);
        assert(id >= 0);
        int res = channel_is_associated(&_globals.channels, cid, id, send);
        if (res < 0) {
            (void)handle_channel_error(res, self, cid);
            goto except;
        }
        if (res) {
            id_obj = PyInterpreterState_GetIDObject(interp);
            if (id_obj == NULL) {
                goto except;
            }
            res = PyList_Insert(ids, 0, id_obj);
            Py_DECREF(id_obj);
            if (res < 0) {
                goto except;
            }
        }
        interp = PyInterpreterState_Next(interp);
    }

    goto finally;

except:
    Py_CLEAR(ids);

finally:
    return ids;
}

PyDoc_STRVAR(channelsmod_list_interpreters_doc,
"channel_list_interpreters(cid, *, send) -> [id]\n\
\n\
Return the list of all interpreter IDs associated with an end of the channel.\n\
\n\
The 'send' argument should be a boolean indicating whether to use the send or\n\
receive end.");


static PyObject *
channelsmod_send(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "obj", "blocking", "timeout", NULL};
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    PyObject *obj;
    int blocking = 1;
    PyObject *timeout_obj = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O|$pO:channel_send", kwlist,
                                     channel_id_converter, &cid_data, &obj,
                                     &blocking, &timeout_obj)) {
        return NULL;
    }

    int64_t cid = cid_data.cid;
    PY_TIMEOUT_T timeout;
    if (PyThread_ParseTimeoutArg(timeout_obj, blocking, &timeout) < 0) {
        return NULL;
    }

    /* Queue up the object. */
    int err = 0;
    if (blocking) {
        err = channel_send_wait(&_globals.channels, cid, obj, timeout);
    }
    else {
        err = channel_send(&_globals.channels, cid, obj, NULL);
    }
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(channelsmod_send_doc,
"channel_send(cid, obj, blocking=True)\n\
\n\
Add the object's data to the channel's queue.\n\
By default this waits for the object to be received.");

static PyObject *
channelsmod_send_buffer(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "obj", "blocking", "timeout", NULL};
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    PyObject *obj;
    int blocking = 1;
    PyObject *timeout_obj = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&O|$pO:channel_send_buffer", kwlist,
                                     channel_id_converter, &cid_data, &obj,
                                     &blocking, &timeout_obj)) {
        return NULL;
    }

    int64_t cid = cid_data.cid;
    PY_TIMEOUT_T timeout;
    if (PyThread_ParseTimeoutArg(timeout_obj, blocking, &timeout) < 0) {
        return NULL;
    }

    PyObject *tempobj = PyMemoryView_FromObject(obj);
    if (tempobj == NULL) {
        return NULL;
    }

    /* Queue up the object. */
    int err = 0;
    if (blocking) {
        err = channel_send_wait(&_globals.channels, cid, tempobj, timeout);
    }
    else {
        err = channel_send(&_globals.channels, cid, tempobj, NULL);
    }
    Py_DECREF(tempobj);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(channelsmod_send_buffer_doc,
"channel_send_buffer(cid, obj, blocking=True)\n\
\n\
Add the object's buffer to the channel's queue.\n\
By default this waits for the object to be received.");

static PyObject *
channelsmod_recv(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "default", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    PyObject *dflt = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&|O:channel_recv", kwlist,
                                     channel_id_converter, &cid_data, &dflt)) {
        return NULL;
    }
    cid = cid_data.cid;

    PyObject *obj = NULL;
    int err = channel_recv(&_globals.channels, cid, &obj);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_XINCREF(dflt);
    if (obj == NULL) {
        // Use the default.
        if (dflt == NULL) {
            (void)handle_channel_error(ERR_CHANNEL_EMPTY, self, cid);
            return NULL;
        }
        obj = Py_NewRef(dflt);
    }
    Py_XDECREF(dflt);
    return obj;
}

PyDoc_STRVAR(channelsmod_recv_doc,
"channel_recv(cid, [default]) -> obj\n\
\n\
Return a new object from the data at the front of the channel's queue.\n\
\n\
If there is nothing to receive then raise ChannelEmptyError, unless\n\
a default value is provided.  In that case return it.");

static PyObject *
channelsmod_close(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cid", "send", "recv", "force", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    int send = 0;
    int recv = 0;
    int force = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&|$ppp:channel_close", kwlist,
                                     channel_id_converter, &cid_data,
                                     &send, &recv, &force)) {
        return NULL;
    }
    cid = cid_data.cid;

    int err = channel_close(&_globals.channels, cid, send-recv, force);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channelsmod_close_doc,
"channel_close(cid, *, send=None, recv=None, force=False)\n\
\n\
Close the channel for all interpreters.\n\
\n\
If the channel is empty then the keyword args are ignored and both\n\
ends are immediately closed.  Otherwise, if 'force' is True then\n\
all queued items are released and both ends are immediately\n\
closed.\n\
\n\
If the channel is not empty *and* 'force' is False then following\n\
happens:\n\
\n\
 * recv is True (regardless of send):\n\
   - raise ChannelNotEmptyError\n\
 * recv is None and send is None:\n\
   - raise ChannelNotEmptyError\n\
 * send is True and recv is not True:\n\
   - fully close the 'send' end\n\
   - close the 'recv' end to interpreters not already receiving\n\
   - fully close it once empty\n\
\n\
Closing an already closed channel results in a ChannelClosedError.\n\
\n\
Once the channel's ID has no more ref counts in any interpreter\n\
the channel will be destroyed.");

static PyObject *
channelsmod_release(PyObject *self, PyObject *args, PyObject *kwds)
{
    // Note that only the current interpreter is affected.
    static char *kwlist[] = {"cid", "send", "recv", "force", NULL};
    int64_t cid;
    struct channel_id_converter_data cid_data = {
        .module = self,
    };
    int send = 0;
    int recv = 0;
    int force = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "O&|$ppp:channel_release", kwlist,
                                     channel_id_converter, &cid_data,
                                     &send, &recv, &force)) {
        return NULL;
    }
    cid = cid_data.cid;
    if (send == 0 && recv == 0) {
        send = 1;
        recv = 1;
    }

    // XXX Handle force is True.
    // XXX Fix implicit release.

    int err = channel_release(&_globals.channels, cid, send, recv);
    if (handle_channel_error(err, self, cid)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(channelsmod_release_doc,
"channel_release(cid, *, send=None, recv=None, force=True)\n\
\n\
Close the channel for the current interpreter.  'send' and 'recv'\n\
(bool) may be used to indicate the ends to close.  By default both\n\
ends are closed.  Closing an already closed end is a noop.");

static PyObject *
channelsmod__channel_id(PyObject *self, PyObject *args, PyObject *kwds)
{
    module_state *state = get_module_state(self);
    if (state == NULL) {
        return NULL;
    }
    PyTypeObject *cls = state->ChannelIDType;

    PyObject *mod = get_module_from_owned_type(cls);
    assert(mod == self);
    Py_DECREF(mod);

    return _channelid_new(self, cls, args, kwds);
}

static PyObject *
channelsmod__register_end_types(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"send", "recv", NULL};
    PyObject *send;
    PyObject *recv;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "OO:_register_end_types", kwlist,
                                     &send, &recv)) {
        return NULL;
    }
    if (!PyType_Check(send)) {
        PyErr_SetString(PyExc_TypeError, "expected a type for 'send'");
        return NULL;
    }
    if (!PyType_Check(recv)) {
        PyErr_SetString(PyExc_TypeError, "expected a type for 'recv'");
        return NULL;
    }
    PyTypeObject *cls_send = (PyTypeObject *)send;
    PyTypeObject *cls_recv = (PyTypeObject *)recv;

    if (set_channelend_types(self, cls_send, cls_recv) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef module_functions[] = {
    {"create",                     channelsmod_create,
     METH_NOARGS,                  channelsmod_create_doc},
    {"destroy",                    _PyCFunction_CAST(channelsmod_destroy),
     METH_VARARGS | METH_KEYWORDS, channelsmod_destroy_doc},
    {"list_all",                   channelsmod_list_all,
     METH_NOARGS,                  channelsmod_list_all_doc},
    {"list_interpreters",          _PyCFunction_CAST(channelsmod_list_interpreters),
     METH_VARARGS | METH_KEYWORDS, channelsmod_list_interpreters_doc},
    {"send",                       _PyCFunction_CAST(channelsmod_send),
     METH_VARARGS | METH_KEYWORDS, channelsmod_send_doc},
    {"send_buffer",                _PyCFunction_CAST(channelsmod_send_buffer),
     METH_VARARGS | METH_KEYWORDS, channelsmod_send_buffer_doc},
    {"recv",                       _PyCFunction_CAST(channelsmod_recv),
     METH_VARARGS | METH_KEYWORDS, channelsmod_recv_doc},
    {"close",                      _PyCFunction_CAST(channelsmod_close),
     METH_VARARGS | METH_KEYWORDS, channelsmod_close_doc},
    {"release",                    _PyCFunction_CAST(channelsmod_release),
     METH_VARARGS | METH_KEYWORDS, channelsmod_release_doc},
    {"_channel_id",                _PyCFunction_CAST(channelsmod__channel_id),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"_register_end_types",        _PyCFunction_CAST(channelsmod__register_end_types),
     METH_VARARGS | METH_KEYWORDS, NULL},

    {NULL,                        NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static int
module_exec(PyObject *mod)
{
    if (_globals_init() != 0) {
        return -1;
    }
    struct xid_class_registry *xid_classes = NULL;

    module_state *state = get_module_state(mod);
    if (state == NULL) {
        goto error;
    }
    xid_classes = &state->xid_classes;

    /* Add exception types */
    if (exceptions_init(mod) != 0) {
        goto error;
    }

    /* Add other types */

    // ChannelID
    state->ChannelIDType = add_new_type(
            mod, &channelid_typespec, _channelid_shared, xid_classes);
    if (state->ChannelIDType == NULL) {
        goto error;
    }

    state->XIBufferViewType = add_new_type(mod, &XIBufferViewType_spec, NULL,
                                           xid_classes);
    if (state->XIBufferViewType == NULL) {
        goto error;
    }

    if (register_builtin_xid_types(xid_classes) < 0) {
        goto error;
    }

    /* Make sure chnnels drop objects owned by this interpreter. */
    PyInterpreterState *interp = _get_current_interp();
    PyUnstable_AtExit(interp, clear_interpreter, (void *)interp);

    return 0;

error:
    if (xid_classes != NULL) {
        clear_xid_class_registry(xid_classes);
    }
    _globals_fini();
    return -1;
}

static struct PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL},
};

static int
module_traverse(PyObject *mod, visitproc visit, void *arg)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);
    traverse_module_state(state, visit, arg);
    return 0;
}

static int
module_clear(PyObject *mod)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);

    // Before clearing anything, we unregister the various XID types. */
    clear_xid_class_registry(&state->xid_classes);

    // Now we clear the module state.
    clear_module_state(state);
    return 0;
}

static void
module_free(void *mod)
{
    module_state *state = get_module_state(mod);
    assert(state != NULL);

    // Before clearing anything, we unregister the various XID types. */
    clear_xid_class_registry(&state->xid_classes);

    // Now we clear the module state.
    clear_module_state(state);

    _globals_fini();
}

static struct PyModuleDef moduledef = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME,
    .m_doc = module_doc,
    .m_size = sizeof(module_state),
    .m_methods = module_functions,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = (freefunc)module_free,
};

PyMODINIT_FUNC
PyInit__xxinterpchannels(void)
{
    return PyModuleDef_Init(&moduledef);
}
