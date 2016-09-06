/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 *
 * To the extent possible under law, the author have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty. http://creativecommons.org/publicdomain/zero/1.0/
 */

/* WARNING: autogenerated file!
 *
 * The blake2s_impl.c is autogenerated from blake2s_impl.c.
 */

#include "Python.h"
#include "pystrhex.h"
#ifdef WITH_THREAD
#include "pythread.h"
#endif

#include "../hashlib.h"
#include "blake2ns.h"

#define HAVE_BLAKE2S 1
#define BLAKE2_LOCAL_INLINE(type) Py_LOCAL_INLINE(type)

#include "impl/blake2.h"
#include "impl/blake2-impl.h" /* for secure_zero_memory() and store48() */

#ifdef BLAKE2_USE_SSE
#include "impl/blake2s.c"
#else
#include "impl/blake2s-ref.c"
#endif


extern PyTypeObject PyBlake2_BLAKE2sType;

typedef struct {
    PyObject_HEAD
    blake2s_param    param;
    blake2s_state    state;
#ifdef WITH_THREAD
    PyThread_type_lock lock;
#endif
} BLAKE2sObject;

#include "clinic/blake2s_impl.c.h"

/*[clinic input]
module _blake2s
class _blake2s.blake2s "BLAKE2sObject *" "&PyBlake2_BLAKE2sType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=edbfcf7557a685a7]*/


static BLAKE2sObject *
new_BLAKE2sObject(PyTypeObject *type)
{
    BLAKE2sObject *self;
    self = (BLAKE2sObject *)type->tp_alloc(type, 0);
#ifdef WITH_THREAD
    if (self != NULL) {
        self->lock = NULL;
    }
#endif
    return self;
}

/*[clinic input]
@classmethod
_blake2s.blake2s.__new__ as py_blake2s_new
    string as data: object = NULL
    *
    digest_size: int(c_default="BLAKE2S_OUTBYTES") = _blake2s.blake2s.MAX_DIGEST_SIZE
    key: Py_buffer = None
    salt: Py_buffer = None
    person: Py_buffer = None
    fanout: int = 1
    depth: int = 1
    leaf_size as leaf_size_obj: object = NULL
    node_offset as node_offset_obj: object = NULL
    node_depth: int = 0
    inner_size: int = 0
    last_node: bool = False

Return a new BLAKE2s hash object.
[clinic start generated code]*/

static PyObject *
py_blake2s_new_impl(PyTypeObject *type, PyObject *data, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, PyObject *leaf_size_obj,
                    PyObject *node_offset_obj, int node_depth,
                    int inner_size, int last_node)
/*[clinic end generated code: output=fe060b258a8cbfc6 input=458cfdcb3d0d47ff]*/
{
    BLAKE2sObject *self = NULL;
    Py_buffer buf;

    unsigned long leaf_size = 0;
    unsigned PY_LONG_LONG node_offset = 0;

    self = new_BLAKE2sObject(type);
    if (self == NULL) {
        goto error;
    }

    /* Zero parameter block. */
    memset(&self->param, 0, sizeof(self->param));

    /* Set digest size. */
    if (digest_size <= 0 || digest_size > BLAKE2S_OUTBYTES) {
        PyErr_Format(PyExc_ValueError,
                "digest_size must be between 1 and %d bytes",
                BLAKE2S_OUTBYTES);
        goto error;
    }
    self->param.digest_length = digest_size;

    /* Set salt parameter. */
    if ((salt->obj != NULL) && salt->len) {
        if (salt->len > BLAKE2S_SALTBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum salt length is %d bytes",
                BLAKE2S_SALTBYTES);
            goto error;
        }
        memcpy(self->param.salt, salt->buf, salt->len);
    }

    /* Set personalization parameter. */
    if ((person->obj != NULL) && person->len) {
        if (person->len > BLAKE2S_PERSONALBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum person length is %d bytes",
                BLAKE2S_PERSONALBYTES);
            goto error;
        }
        memcpy(self->param.personal, person->buf, person->len);
    }

    /* Set tree parameters. */
    if (fanout < 0 || fanout > 255) {
        PyErr_SetString(PyExc_ValueError,
                "fanout must be between 0 and 255");
        goto error;
    }
    self->param.fanout = (uint8_t)fanout;

    if (depth <= 0 || depth > 255) {
        PyErr_SetString(PyExc_ValueError,
                "depth must be between 1 and 255");
        goto error;
    }
    self->param.depth = (uint8_t)depth;

    if (leaf_size_obj != NULL) {
        leaf_size = PyLong_AsUnsignedLong(leaf_size_obj);
        if (leaf_size == (unsigned long) -1 && PyErr_Occurred()) {
            goto error;
        }
        if (leaf_size > 0xFFFFFFFFU) {
            PyErr_SetString(PyExc_OverflowError, "leaf_size is too large");
            goto error;
        }
    }
    self->param.leaf_length = (unsigned int)leaf_size;

    if (node_offset_obj != NULL) {
        node_offset = PyLong_AsUnsignedLongLong(node_offset_obj);
        if (node_offset == (unsigned PY_LONG_LONG) -1 && PyErr_Occurred()) {
            goto error;
        }
    }
#ifdef HAVE_BLAKE2S
    if (node_offset > 0xFFFFFFFFFFFFULL) {
        /* maximum 2**48 - 1 */
         PyErr_SetString(PyExc_OverflowError, "node_offset is too large");
         goto error;
     }
    store48(&(self->param.node_offset), node_offset);
#else
    self->param.node_offset = node_offset;
#endif

    if (node_depth < 0 || node_depth > 255) {
        PyErr_SetString(PyExc_ValueError,
                "node_depth must be between 0 and 255");
        goto error;
    }
    self->param.node_depth = node_depth;

    if (inner_size < 0 || inner_size > BLAKE2S_OUTBYTES) {
        PyErr_Format(PyExc_ValueError,
                "inner_size must be between 0 and is %d",
                BLAKE2S_OUTBYTES);
        goto error;
    }
    self->param.inner_length = inner_size;

    /* Set key length. */
    if ((key->obj != NULL) && key->len) {
        if (key->len > BLAKE2S_KEYBYTES) {
            PyErr_Format(PyExc_ValueError,
                "maximum key length is %d bytes",
                BLAKE2S_KEYBYTES);
            goto error;
        }
        self->param.key_length = key->len;
    }

    /* Initialize hash state. */
    if (blake2s_init_param(&self->state, &self->param) < 0) {
        PyErr_SetString(PyExc_RuntimeError,
                "error initializing hash state");
        goto error;
    }

    /* Set last node flag (must come after initialization). */
    self->state.last_node = last_node;

    /* Process key block if any. */
    if (self->param.key_length) {
        uint8_t block[BLAKE2S_BLOCKBYTES];
        memset(block, 0, sizeof(block));
        memcpy(block, key->buf, key->len);
        blake2s_update(&self->state, block, sizeof(block));
        secure_zero_memory(block, sizeof(block));
    }

    /* Process initial data if any. */
    if (data != NULL) {
        GET_BUFFER_VIEW_OR_ERROR(data, &buf, goto error);

        if (buf.len >= HASHLIB_GIL_MINSIZE) {
            Py_BEGIN_ALLOW_THREADS
            blake2s_update(&self->state, buf.buf, buf.len);
            Py_END_ALLOW_THREADS
        } else {
            blake2s_update(&self->state, buf.buf, buf.len);
        }
        PyBuffer_Release(&buf);
    }

    return (PyObject *)self;

  error:
    if (self != NULL) {
        Py_DECREF(self);
    }
    return NULL;
}

/*[clinic input]
_blake2s.blake2s.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_blake2s_blake2s_copy_impl(BLAKE2sObject *self)
/*[clinic end generated code: output=6c5bada404b7aed7 input=c8858e887ae4a07a]*/
{
    BLAKE2sObject *cpy;

    if ((cpy = new_BLAKE2sObject(Py_TYPE(self))) == NULL)
        return NULL;

    ENTER_HASHLIB(self);
    cpy->param = self->param;
    cpy->state = self->state;
    LEAVE_HASHLIB(self);
    return (PyObject *)cpy;
}

/*[clinic input]
_blake2s.blake2s.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyObject *
_blake2s_blake2s_update(BLAKE2sObject *self, PyObject *obj)
/*[clinic end generated code: output=fe8438a1d3cede87 input=47a408b9a3cc05c5]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(obj, &buf);

#ifdef WITH_THREAD
    if (self->lock == NULL && buf.len >= HASHLIB_GIL_MINSIZE)
        self->lock = PyThread_allocate_lock();

    if (self->lock != NULL) {
       Py_BEGIN_ALLOW_THREADS
       PyThread_acquire_lock(self->lock, 1);
       blake2s_update(&self->state, buf.buf, buf.len);
       PyThread_release_lock(self->lock);
       Py_END_ALLOW_THREADS
    } else {
        blake2s_update(&self->state, buf.buf, buf.len);
    }
#else
    blake2s_update(&self->state, buf.buf, buf.len);
#endif /* !WITH_THREAD */
    PyBuffer_Release(&buf);

    Py_INCREF(Py_None);
    return Py_None;
}

/*[clinic input]
_blake2s.blake2s.digest

Return the digest value as a string of binary data.
[clinic start generated code]*/

static PyObject *
_blake2s_blake2s_digest_impl(BLAKE2sObject *self)
/*[clinic end generated code: output=80e81a48c6f79cf9 input=feb9a220135bdeba]*/
{
    uint8_t digest[BLAKE2S_OUTBYTES];
    blake2s_state state_cpy;

    ENTER_HASHLIB(self);
    state_cpy = self->state;
    blake2s_final(&state_cpy, digest, self->param.digest_length);
    LEAVE_HASHLIB(self);
    return PyBytes_FromStringAndSize((const char *)digest,
            self->param.digest_length);
}

/*[clinic input]
_blake2s.blake2s.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_blake2s_blake2s_hexdigest_impl(BLAKE2sObject *self)
/*[clinic end generated code: output=db6c5028c0a3c2e5 input=4e4877b8bd7aea91]*/
{
    uint8_t digest[BLAKE2S_OUTBYTES];
    blake2s_state state_cpy;

    ENTER_HASHLIB(self);
    state_cpy = self->state;
    blake2s_final(&state_cpy, digest, self->param.digest_length);
    LEAVE_HASHLIB(self);
    return _Py_strhex((const char *)digest, self->param.digest_length);
}


static PyMethodDef py_blake2s_methods[] = {
    _BLAKE2S_BLAKE2S_COPY_METHODDEF
    _BLAKE2S_BLAKE2S_DIGEST_METHODDEF
    _BLAKE2S_BLAKE2S_HEXDIGEST_METHODDEF
    _BLAKE2S_BLAKE2S_UPDATE_METHODDEF
    {NULL, NULL}
};



static PyObject *
py_blake2s_get_name(BLAKE2sObject *self, void *closure)
{
    return PyUnicode_FromString("blake2s");
}



static PyObject *
py_blake2s_get_block_size(BLAKE2sObject *self, void *closure)
{
    return PyLong_FromLong(BLAKE2S_BLOCKBYTES);
}



static PyObject *
py_blake2s_get_digest_size(BLAKE2sObject *self, void *closure)
{
    return PyLong_FromLong(self->param.digest_length);
}


static PyGetSetDef py_blake2s_getsetters[] = {
    {"name", (getter)py_blake2s_get_name,
        NULL, NULL, NULL},
    {"block_size", (getter)py_blake2s_get_block_size,
        NULL, NULL, NULL},
    {"digest_size", (getter)py_blake2s_get_digest_size,
        NULL, NULL, NULL},
    {NULL}
};


static void
py_blake2s_dealloc(PyObject *self)
{
    BLAKE2sObject *obj = (BLAKE2sObject *)self;

    /* Try not to leave state in memory. */
    secure_zero_memory(&obj->param, sizeof(obj->param));
    secure_zero_memory(&obj->state, sizeof(obj->state));
#ifdef WITH_THREAD
    if (obj->lock) {
        PyThread_free_lock(obj->lock);
        obj->lock = NULL;
    }
#endif
    PyObject_Del(self);
}


PyTypeObject PyBlake2_BLAKE2sType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_blake2.blake2s",        /* tp_name            */
    sizeof(BLAKE2sObject),    /* tp_size            */
    0,                        /* tp_itemsize        */
    py_blake2s_dealloc,       /* tp_dealloc         */
    0,                        /* tp_print           */
    0,                        /* tp_getattr         */
    0,                        /* tp_setattr         */
    0,                        /* tp_compare         */
    0,                        /* tp_repr            */
    0,                        /* tp_as_number       */
    0,                        /* tp_as_sequence     */
    0,                        /* tp_as_mapping      */
    0,                        /* tp_hash            */
    0,                        /* tp_call            */
    0,                        /* tp_str             */
    0,                        /* tp_getattro        */
    0,                        /* tp_setattro        */
    0,                        /* tp_as_buffer       */
    Py_TPFLAGS_DEFAULT,       /* tp_flags           */
    py_blake2s_new__doc__,    /* tp_doc             */
    0,                        /* tp_traverse        */
    0,                        /* tp_clear           */
    0,                        /* tp_richcompare     */
    0,                        /* tp_weaklistoffset  */
    0,                        /* tp_iter            */
    0,                        /* tp_iternext        */
    py_blake2s_methods,       /* tp_methods         */
    0,                        /* tp_members         */
    py_blake2s_getsetters,    /* tp_getset          */
    0,                        /* tp_base            */
    0,                        /* tp_dict            */
    0,                        /* tp_descr_get       */
    0,                        /* tp_descr_set       */
    0,                        /* tp_dictoffset      */
    0,                        /* tp_init            */
    0,                        /* tp_alloc           */
    py_blake2s_new,           /* tp_new             */
};
