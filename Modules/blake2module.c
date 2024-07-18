/*
 * Written in 2013 by Dmitry Chestnykh <dmitry@codingrobots.com>
 * Modified for CPython by Christian Heimes <christian@python.org>
 * Updated to usa HACL* by Jonathan Protzenko <jonathan@protzenko.fr>
 *
 * To the extent possible under law, the author have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty. http://creativecommons.org/publicdomain/zero/1.0/
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "pyconfig.h"
#include "Python.h"
#include "hashlib.h"
#include "pycore_strhex.h"       // _Py_strhex()

// QUICK CPU AUTODETECTION
//
// See https://github.com/python/cpython/pull/119316 -- we only enable
// vectorized versions for Intel CPUs, even though HACL*'s "vec128" modules also
// run on ARM NEON. (We could enable them on POWER -- but I don't have access to
// a test machine to see if that speeds anything up.)
//
// Note that configure.ac and the rest of the build are written in such a way
// that if the configure script finds suitable flags to compile HACL's SIMD128
// (resp. SIMD256) files, then Hacl_Hash_Blake2b_Simd128.c (resp. ...) will be
// pulled into the build automatically, and then only the CPU autodetection will
// need to be updated here.

#if defined(__x86_64__) && defined(__GNUC__)
#include <cpuid.h>
#elif defined(_M_X64)
#include <intrin.h>
#endif

#include <stdbool.h>

// ECX
#define ECX_SSE3 (1 << 0)
#define ECX_SSSE3 (1 << 9)
#define ECX_SSE4_1 (1 << 19)
#define ECX_SSE4_2 (1 << 20)
#define ECX_AVX (1 << 28)

// EBX
#define EBX_AVX2 (1 << 5)

// EDX
#define EDX_SSE (1 << 25)
#define EDX_SSE2 (1 << 26)
#define EDX_CMOV (1 << 15)

// zero-initialized by default
static bool sse, sse2, sse3, sse41, sse42, cmov, avx, avx2;

static void detect_cpu_features(void) {
  static bool done = false;
  if (!done) {
    int eax1 = 0, ebx1 = 0, ecx1 = 0, edx1 = 0;
    int eax7 = 0, ebx7 = 0, ecx7 = 0, edx7 = 0;
#if defined(__x86_64__) && defined(__GNUC__)
    __cpuid_count(1, 0, eax1, ebx1, ecx1, edx1);
    __cpuid_count(7, 0, eax7, ebx7, ecx7, edx7);
#elif defined(_M_X64)
    int info1[4] = { 0 };
    int info7[4] = { 0 };
    __cpuidex(info1, 1, 0);
    __cpuidex(info7, 7, 0);
    eax1 = info1[0];
    ebx1 = info1[1];
    ecx1 = info1[2];
    edx1 = info1[3];
    eax7 = info7[0];
    ebx7 = info7[1];
    ecx7 = info7[2];
    edx7 = info7[3];
#else
    (void) eax1; (void) ebx1; (void) ecx1; (void) edx1;
    (void) eax7; (void) ebx7; (void) ecx7; (void) edx7;
#endif

    avx = (ecx1 & ECX_AVX) != 0;

    avx2 = (ebx7 & EBX_AVX2) != 0;

    sse = (edx1 & EDX_SSE) != 0;
    sse2 = (edx1 & EDX_SSE2) != 0;
    cmov = (edx1 & EDX_CMOV) != 0;

    sse3 = (ecx1 & ECX_SSE3) != 0;
    /* ssse3 = (ecx1 & ECX_SSSE3) != 0; */
    sse41 = (ecx1 & ECX_SSE4_1) != 0;
    sse42 = (ecx1 & ECX_SSE4_2) != 0;

    done = true;
  }
}

static inline bool has_simd128(void) {
  // For now this is Intel-only, could conceivably be #ifdef'd to something
  // else.
  return sse && sse2 && sse3 && sse41 && sse42 && cmov;
}

static inline bool has_simd256(void) {
  return avx && avx2;
}

#include "_hacl/Hacl_Hash_Blake2b.h"
#include "_hacl/Hacl_Hash_Blake2s.h"

// MODULE TYPE SLOTS

static PyType_Spec blake2b_type_spec;
static PyType_Spec blake2s_type_spec;

PyDoc_STRVAR(blake2mod__doc__,
"_blake2b provides BLAKE2b for hashlib\n"
);

typedef struct {
    PyTypeObject* blake2b_type;
    PyTypeObject* blake2s_type;
} Blake2State;

static inline Blake2State*
blake2_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (Blake2State *)state;
}

static struct PyMethodDef blake2mod_functions[] = {
    {NULL, NULL}
};

static int
_blake2_traverse(PyObject *module, visitproc visit, void *arg)
{
    Blake2State *state = blake2_get_state(module);
    Py_VISIT(state->blake2b_type);
    Py_VISIT(state->blake2s_type);
    return 0;
}

static int
_blake2_clear(PyObject *module)
{
    Blake2State *state = blake2_get_state(module);
    Py_CLEAR(state->blake2b_type);
    Py_CLEAR(state->blake2s_type);
    return 0;
}

static void
_blake2_free(void *module)
{
    _blake2_clear((PyObject *)module);
}

#define ADD_INT(d, name, value) do { \
    PyObject *x = PyLong_FromLong(value); \
    if (!x) \
        return -1; \
    if (PyDict_SetItemString(d, name, x) < 0) { \
        Py_DECREF(x); \
        return -1; \
    } \
    Py_DECREF(x); \
} while(0)

#define ADD_INT_CONST(NAME, VALUE) do { \
    if (PyModule_AddIntConstant(m, NAME, VALUE) < 0) { \
        return -1; \
    } \
} while (0)

static int
blake2_exec(PyObject *m)
{
    // This is called at module initialization-time, and so appears to be as
    // good a place as any to probe the CPU flags.
    detect_cpu_features();

    Blake2State* st = blake2_get_state(m);

    st->blake2b_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2b_type_spec, NULL);

    if (NULL == st->blake2b_type)
        return -1;
    /* BLAKE2b */
    if (PyModule_AddType(m, st->blake2b_type) < 0) {
        return -1;
    }

    PyObject *d = st->blake2b_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", HACL_HASH_BLAKE2B_SALT_BYTES);
    ADD_INT(d, "PERSON_SIZE", HACL_HASH_BLAKE2B_PERSONAL_BYTES);
    ADD_INT(d, "MAX_KEY_SIZE", HACL_HASH_BLAKE2B_KEY_BYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", HACL_HASH_BLAKE2B_OUT_BYTES);

    ADD_INT_CONST("BLAKE2B_SALT_SIZE", HACL_HASH_BLAKE2B_SALT_BYTES);
    ADD_INT_CONST("BLAKE2B_PERSON_SIZE", HACL_HASH_BLAKE2B_PERSONAL_BYTES);
    ADD_INT_CONST("BLAKE2B_MAX_KEY_SIZE", HACL_HASH_BLAKE2B_KEY_BYTES);
    ADD_INT_CONST("BLAKE2B_MAX_DIGEST_SIZE", HACL_HASH_BLAKE2B_OUT_BYTES);

    /* BLAKE2s */
    st->blake2s_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &blake2s_type_spec, NULL);

    if (NULL == st->blake2s_type)
        return -1;

    if (PyModule_AddType(m, st->blake2s_type) < 0) {
        return -1;
    }

    d = st->blake2s_type->tp_dict;
    ADD_INT(d, "SALT_SIZE", HACL_HASH_BLAKE2S_SALT_BYTES);
    ADD_INT(d, "PERSON_SIZE", HACL_HASH_BLAKE2S_PERSONAL_BYTES);
    ADD_INT(d, "MAX_KEY_SIZE", HACL_HASH_BLAKE2S_KEY_BYTES);
    ADD_INT(d, "MAX_DIGEST_SIZE", HACL_HASH_BLAKE2S_OUT_BYTES);

    ADD_INT_CONST("BLAKE2S_SALT_SIZE", HACL_HASH_BLAKE2S_SALT_BYTES);
    ADD_INT_CONST("BLAKE2S_PERSON_SIZE", HACL_HASH_BLAKE2S_PERSONAL_BYTES);
    ADD_INT_CONST("BLAKE2S_MAX_KEY_SIZE", HACL_HASH_BLAKE2S_KEY_BYTES);
    ADD_INT_CONST("BLAKE2S_MAX_DIGEST_SIZE", HACL_HASH_BLAKE2S_OUT_BYTES);

    return 0;
}

#undef ADD_INT
#undef ADD_INT_CONST

static PyModuleDef_Slot _blake2_slots[] = {
    {Py_mod_exec, blake2_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef blake2_module = {
    PyModuleDef_HEAD_INIT,
    "_blake2",
    .m_doc = blake2mod__doc__,
    .m_size = sizeof(Blake2State),
    .m_methods = blake2mod_functions,
    .m_slots = _blake2_slots,
    .m_traverse = _blake2_traverse,
    .m_clear = _blake2_clear,
    .m_free = _blake2_free,
};

PyMODINIT_FUNC
PyInit__blake2(void)
{
    return PyModuleDef_Init(&blake2_module);
}

// IMPLEMENTATION OF METHODS

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include <stdbool.h>
#include "Python.h"

// The HACL* API does not offer an agile API that can deal with either Blake2S
// or Blake2B -- the reason is that the underlying states are optimized (uint32s
// for S, uint64s for B). Therefore, we use a tagged union in this module to
// correctly dispatch. Note that the previous incarnation of this code
// transformed the Blake2b implementation into the Blake2s one using a script,
// so this is an improvement.
//
// The 128 and 256 versions are only available if i) we were able to compile
// them, and ii) if the CPU we run on also happens to have the right instruction
// set.
typedef enum { Blake2s, Blake2b, Blake2s_128, Blake2b_256 } blake2_impl;

static inline bool is_blake2b(blake2_impl impl) {
  return impl == Blake2b || impl == Blake2b_256;
}

static inline bool is_blake2s(blake2_impl impl) {
  return !is_blake2b(impl);
}

static inline blake2_impl type_to_impl(PyTypeObject *type) {
    if (!strcmp(type->tp_name, blake2b_type_spec.name))
#ifdef HACL_CAN_COMPILE_SIMD256
      if (has_simd256())
        return Blake2b_256;
      else
#endif
        return Blake2b;
    else if (!strcmp(type->tp_name, blake2s_type_spec.name))
#ifdef HACL_CAN_COMPILE_SIMD128
      if (has_simd128())
        return Blake2s_128;
      else
#endif
        return Blake2s;
    else
        Py_UNREACHABLE();
}

typedef struct {
    PyObject_HEAD
    union {
        Hacl_Hash_Blake2s_state_t *blake2s_state;
        Hacl_Hash_Blake2b_state_t *blake2b_state;
#ifdef HACL_CAN_COMPILE_SIMD128
        Hacl_Hash_Blake2s_Simd128_state_t *blake2s_128_state;
#endif
#ifdef HACL_CAN_COMPILE_SIMD256
        Hacl_Hash_Blake2b_SImd256_state_t *blake2b_256_state;
#endif
    };
    blake2_impl impl;
    bool use_mutex;
    PyMutex mutex;
} Blake2Object;

#include "clinic/blake2module.c.h"

/*[clinic input]
module _blake2
class _blake2.blake2b "Blake2Object *" "&PyBlake2_BLAKE2bType"
class _blake2.blake2s "Blake2Object *" "&PyBlake2_BLAKE2sType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b7526666bd18af83]*/


static Blake2Object *
new_Blake2Object(PyTypeObject *type)
{
    Blake2Object *self;
    self = (Blake2Object *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(self);

    return self;
}

/* HACL* takes a uint32_t for the length of its parameter, but Py_ssize_t can be
 * 64 bits so we loop in <4gig chunks when needed. */

#if PY_SSIZE_T_MAX > UINT32_MAX
#define HACL_UPDATE_LOOP(update,state,buf,len) \
  while (len > UINT32_MAX) { \
    update(state, buf, UINT32_MAX); \
    len -= UINT32_MAX; \
    buf += UINT32_MAX; \
  }
#else
#define HACL_UPDATE_LOOP(update,state,buf,len)
#endif

#define HACL_UPDATE(update,state,buf,len) do { \
  /* Note: we explicitly ignore the error code on the basis that it would take >
   * 1 billion years to overflow the maximum admissible length for SHA2-256
   * (namely, 2^61-1 bytes). */ \
  HACL_UPDATE_LOOP(update,state,buf,len) \
  /* Cast to uint32_t is safe: len <= UINT32_MAX at this point. */ \
  update(state, buf, (uint32_t) len); \
} while (0)

static void update(Blake2Object *self, uint8_t *buf, Py_ssize_t len) {
    switch (self->impl) {
      // These need to be ifdef'd out otherwise it's an unresolved symbol at
      // link-time.
#ifdef HACL_CAN_COMPILE_SIMD256
        case Blake2b_256:
            HACL_UPDATE(Hacl_Hash_Blake2b_Simd256_update,self->blake2b_256_state, buf, len);
            return;
#endif
#ifdef HACL_CAN_COMPILE_SIMD128
        case Blake2s_128:
            HACL_UPDATE(Hacl_Hash_Blake2s_Simd128_update,self->blake2s_128_state, buf, len);
            return;
#endif
        case Blake2b:
            HACL_UPDATE(Hacl_Hash_Blake2b_update,self->blake2b_state, buf, len);
            return;
        case Blake2s:
            HACL_UPDATE(Hacl_Hash_Blake2s_update,self->blake2s_state, buf, len);
            return;
        default:
            Py_UNREACHABLE();
    }
}

static PyObject *
py_blake2b_or_s_new(PyTypeObject *type, PyObject *data, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node, int usedforsecurity)

{
    Blake2Object *self = NULL;
    Py_buffer buf;

    self = new_Blake2Object(type);
    if (self == NULL) {
        goto error;
    }

    self->impl = type_to_impl(type);

    // Using Blake2b because we statically know that these are greater than the
    // Blake2s sizes -- this avoids a VLA.
    uint8_t salt_[HACL_HASH_BLAKE2B_SALT_BYTES] = { 0 };
    uint8_t personal_[HACL_HASH_BLAKE2B_PERSONAL_BYTES] = { 0 };

    /* Validate digest size. */
    if (digest_size <= 0 ||
        (unsigned) digest_size > (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_OUT_BYTES : HACL_HASH_BLAKE2S_OUT_BYTES))
    {
        PyErr_Format(PyExc_ValueError,
                "digest_size for %s must be between 1 and %d bytes, here it is %d",
                is_blake2b(self->impl) ? "Blake2b" : "Blake2s",
                is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_OUT_BYTES : HACL_HASH_BLAKE2S_OUT_BYTES,
                digest_size);
        goto error;
    }

    /* Validate salt parameter. */
    if ((salt->obj != NULL) && salt->len) {
        if (salt->len > (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_SALT_BYTES : HACL_HASH_BLAKE2S_SALT_BYTES)) {
            PyErr_Format(PyExc_ValueError,
                "maximum salt length is %d bytes",
                (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_SALT_BYTES : HACL_HASH_BLAKE2S_SALT_BYTES));
            goto error;
        }
        memcpy(salt_, salt->buf, salt->len);
    }

    /* Validate personalization parameter. */
    if ((person->obj != NULL) && person->len) {
        if (person->len > (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_PERSONAL_BYTES : HACL_HASH_BLAKE2S_PERSONAL_BYTES)) {
            PyErr_Format(PyExc_ValueError,
                "maximum person length is %d bytes",
                (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_PERSONAL_BYTES : HACL_HASH_BLAKE2S_PERSONAL_BYTES));
            goto error;
        }
        memcpy(personal_, person->buf, person->len);
    }

    /* Validate tree parameters. */
    if (fanout < 0 || fanout > 255) {
        PyErr_SetString(PyExc_ValueError,
                "fanout must be between 0 and 255");
        goto error;
    }

    if (depth <= 0 || depth > 255) {
        PyErr_SetString(PyExc_ValueError,
                "depth must be between 1 and 255");
        goto error;
    }

    if (leaf_size > 0xFFFFFFFFU) {
        PyErr_SetString(PyExc_OverflowError, "leaf_size is too large");
        goto error;
    }

    if (is_blake2s(self->impl) && node_offset > 0xFFFFFFFFFFFFULL) {
        /* maximum 2**48 - 1 */
         PyErr_SetString(PyExc_OverflowError, "node_offset is too large");
         goto error;
     }

    if (node_depth < 0 || node_depth > 255) {
        PyErr_SetString(PyExc_ValueError,
                "node_depth must be between 0 and 255");
        goto error;
    }

    if (inner_size < 0 ||
        (unsigned) inner_size > (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_OUT_BYTES : HACL_HASH_BLAKE2S_OUT_BYTES)) {
        PyErr_Format(PyExc_ValueError,
                "inner_size must be between 0 and is %d",
                (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_OUT_BYTES : HACL_HASH_BLAKE2S_OUT_BYTES));
        goto error;
    }

    /* Set key length. */
    if ((key->obj != NULL) && key->len) {
        if (key->len > (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_KEY_BYTES : HACL_HASH_BLAKE2S_KEY_BYTES)) {
            PyErr_Format(PyExc_ValueError,
                "maximum key length is %d bytes",
                (is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_KEY_BYTES : HACL_HASH_BLAKE2S_KEY_BYTES));
            goto error;
        }
    }

    // Unlike the state types, the parameters share a single (client-friendly)
    // structure.

    Hacl_Hash_Blake2b_blake2_params params = {
        .digest_length = digest_size,
        .key_length = key->len,
        .fanout = fanout,
        .depth = depth,
        .leaf_length = leaf_size,
        .node_offset = node_offset,
        .node_depth = node_depth,
        .inner_length = inner_size,
        .salt = salt_,
        .personal = personal_
    };

    switch (self->impl) {
#if HACL_CAN_COMPILE_SIMD256
        case Blake2b_256:
            self->blake2b_256_state = Hacl_Hash_Blake2b_Simd256_malloc_with_params_and_key(&params, last_node, key->buf);
            break;
#endif
#if HACL_CAN_COMPILE_SIMD128
        case Blake2s_128:
            self->blake2s_128_state = Hacl_Hash_Blake2s_Simd128_malloc_with_params_and_key(&params, last_node, key->buf);
            break;
#endif
        case Blake2b:
            self->blake2b_state = Hacl_Hash_Blake2b_malloc_with_params_and_key(&params, last_node, key->buf);
            break;
        case Blake2s:
            self->blake2s_state = Hacl_Hash_Blake2s_malloc_with_params_and_key(&params, last_node, key->buf);
            break;
        default:
            Py_UNREACHABLE();
    }

    /* Process initial data if any. */
    if (data != NULL) {
        GET_BUFFER_VIEW_OR_ERROR(data, &buf, goto error);

        if (buf.len >= HASHLIB_GIL_MINSIZE) {
            Py_BEGIN_ALLOW_THREADS
            update(self, buf.buf, buf.len);
            Py_END_ALLOW_THREADS
        } else {
            update(self, buf.buf, buf.len);
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
@classmethod
_blake2.blake2b.__new__ as py_blake2b_new
    data: object(c_default="NULL") = b''
    /
    *
    digest_size: int(c_default="HACL_HASH_BLAKE2B_OUT_BYTES") = _blake2.blake2b.MAX_DIGEST_SIZE
    key: Py_buffer(c_default="NULL", py_default="b''") = None
    salt: Py_buffer(c_default="NULL", py_default="b''") = None
    person: Py_buffer(c_default="NULL", py_default="b''") = None
    fanout: int = 1
    depth: int = 1
    leaf_size: unsigned_long = 0
    node_offset: unsigned_long_long = 0
    node_depth: int = 0
    inner_size: int = 0
    last_node: bool = False
    usedforsecurity: bool = True

Return a new BLAKE2b hash object.
[clinic start generated code]*/

static PyObject *
py_blake2b_new_impl(PyTypeObject *type, PyObject *data, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node, int usedforsecurity)
/*[clinic end generated code: output=32bfd8f043c6896f input=8fee2b7b11428b2d]*/
{
    return py_blake2b_or_s_new(type, data, digest_size, key, salt, person, fanout, depth, leaf_size, node_offset, node_depth, inner_size, last_node, usedforsecurity);
}

/*[clinic input]
@classmethod
_blake2.blake2s.__new__ as py_blake2s_new
    data: object(c_default="NULL") = b''
    /
    *
    digest_size: int(c_default="HACL_HASH_BLAKE2S_OUT_BYTES") = _blake2.blake2s.MAX_DIGEST_SIZE
    key: Py_buffer(c_default="NULL", py_default="b''") = None
    salt: Py_buffer(c_default="NULL", py_default="b''") = None
    person: Py_buffer(c_default="NULL", py_default="b''") = None
    fanout: int = 1
    depth: int = 1
    leaf_size: unsigned_long = 0
    node_offset: unsigned_long_long = 0
    node_depth: int = 0
    inner_size: int = 0
    last_node: bool = False
    usedforsecurity: bool = True

Return a new BLAKE2s hash object.
[clinic start generated code]*/

static PyObject *
py_blake2s_new_impl(PyTypeObject *type, PyObject *data, int digest_size,
                    Py_buffer *key, Py_buffer *salt, Py_buffer *person,
                    int fanout, int depth, unsigned long leaf_size,
                    unsigned long long node_offset, int node_depth,
                    int inner_size, int last_node, int usedforsecurity)
/*[clinic end generated code: output=556181f73905c686 input=8165a11980eac7f3]*/
{
    return py_blake2b_or_s_new(type, data, digest_size, key, salt, person, fanout, depth, leaf_size, node_offset, node_depth, inner_size, last_node, usedforsecurity);
}

/*[clinic input]
_blake2.blake2b.copy

Return a copy of the hash object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_copy_impl(Blake2Object *self)
/*[clinic end generated code: output=622d1c56b91c50d8 input=e383c2d199fd8a2e]*/
{
    Blake2Object *cpy;

    if ((cpy = new_Blake2Object(Py_TYPE(self))) == NULL)
        return NULL;

    ENTER_HASHLIB(self);
    switch (self->impl) {
#if HACL_CAN_COMPILE_SIMD256
        case Blake2b_256:
            cpy->blake2b_256_state = Hacl_Hash_Blake2b_Simd256_copy(self->blake2b_256_state);
            break;
#endif
#if HACL_CAN_COMPILE_SIMD128
        case Blake2s:
            cpy->blake2s_128_state = Hacl_Hash_Blake2s_Simd128_copy(self->blake2s_128_state);
            break;
#endif
        case Blake2b:
            cpy->blake2b_state = Hacl_Hash_Blake2b_copy(self->blake2b_state);
            break;
        case Blake2s:
            cpy->blake2s_state = Hacl_Hash_Blake2s_copy(self->blake2s_state);
            break;
        default:
            Py_UNREACHABLE();
    }
    cpy->impl = self->impl;
    LEAVE_HASHLIB(self);
    return (PyObject *)cpy;
}

/*[clinic input]
_blake2.blake2b.update

    data: object
    /

Update this hash object's state with the provided bytes-like object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_update(Blake2Object *self, PyObject *data)
/*[clinic end generated code: output=e6d1ac88471df308 input=ffc4aa6a6a225d31]*/
{
    Py_buffer buf;

    GET_BUFFER_VIEW_OR_ERROUT(data, &buf);

    if (!self->use_mutex && buf.len >= HASHLIB_GIL_MINSIZE) {
        self->use_mutex = true;
    }
    if (self->use_mutex) {
        Py_BEGIN_ALLOW_THREADS
        PyMutex_Lock(&self->mutex);
        update(self, buf.buf, buf.len);
        PyMutex_Unlock(&self->mutex);
        Py_END_ALLOW_THREADS
    } else {
        update(self, buf.buf, buf.len);
    }

    PyBuffer_Release(&buf);

    Py_RETURN_NONE;
}

/*[clinic input]
_blake2.blake2b.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_digest_impl(Blake2Object *self)
/*[clinic end generated code: output=31ab8ad477f4a2f7 input=7d21659e9c5fff02]*/
{
    uint8_t digest[HACL_HASH_BLAKE2B_OUT_BYTES];

    ENTER_HASHLIB(self);
    uint8_t digest_length = 0;
    switch (self->impl) {
#if HACL_CAN_COMPILE_SIMD256
        case Blake2b_256:
            digest_length = Hacl_Hash_Blake2b_Simd256_digest(self->blake2b_256_state, digest);
            break;
#endif
#if HACL_CAN_COMPILE_SIMD128
        case Blake2s_128:
            digest_length = Hacl_Hash_Blake2s_Simd128_digest(self->blake2s_128_state, digest);
            break;
#endif
        case Blake2b:
            digest_length = Hacl_Hash_Blake2b_digest(self->blake2b_state, digest);
            break;
        case Blake2s:
            digest_length = Hacl_Hash_Blake2s_digest(self->blake2s_state, digest);
            break;
        default:
            Py_UNREACHABLE();
    }
    LEAVE_HASHLIB(self);
    return PyBytes_FromStringAndSize((const char *)digest, digest_length);
}

/*[clinic input]
_blake2.blake2b.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyObject *
_blake2_blake2b_hexdigest_impl(Blake2Object *self)
/*[clinic end generated code: output=5ef54b138db6610a input=76930f6946351f56]*/
{
    uint8_t digest[HACL_HASH_BLAKE2B_OUT_BYTES];

    ENTER_HASHLIB(self);
    uint8_t digest_length = 0;
    switch (self->impl) {
#if HACL_CAN_COMPILE_SIMD256
        case Blake2b_256:
            digest_length = Hacl_Hash_Blake2b_Simd256_digest(self->blake2b_256_state, digest);
            break;
#endif
#if HACL_CAN_COMPILE_SIMD128
        case Blake2s_128:
            digest_length = Hacl_Hash_Blake2s_Simd128_digest(self->blake2s_128_state, digest);
            break;
#endif
        case Blake2b:
            digest_length = Hacl_Hash_Blake2b_digest(self->blake2b_state, digest);
            break;
        case Blake2s:
            digest_length = Hacl_Hash_Blake2s_digest(self->blake2s_state, digest);
            break;
        default:
            Py_UNREACHABLE();
    }
    LEAVE_HASHLIB(self);
    return _Py_strhex((const char *)digest, digest_length);
}


static PyMethodDef py_blake2b_methods[] = {
    _BLAKE2_BLAKE2B_COPY_METHODDEF
    _BLAKE2_BLAKE2B_DIGEST_METHODDEF
    _BLAKE2_BLAKE2B_HEXDIGEST_METHODDEF
    _BLAKE2_BLAKE2B_UPDATE_METHODDEF
    {NULL, NULL}
};


static PyObject *
py_blake2b_get_name(Blake2Object *self, void *closure)
{
    return PyUnicode_FromString(is_blake2b(self->impl) ? "blake2b" : "blake2s");
}



static PyObject *
py_blake2b_get_block_size(Blake2Object *self, void *closure)
{
    return PyLong_FromLong(is_blake2b(self->impl) ? HACL_HASH_BLAKE2B_BLOCK_BYTES : HACL_HASH_BLAKE2S_BLOCK_BYTES);
}



static PyObject *
py_blake2b_get_digest_size(Blake2Object *self, void *closure)
{
    switch (self->impl) {
#if HACL_CAN_COMPILE_SIMD256
        case Blake2b_256:
            return PyLong_FromLong(Hacl_Hash_Blake2b_Simd256_info(self->blake2b_256_state).digest_length);
#endif
#if HACL_CAN_COMPILE_SIMD128
        case Blake2s_128:
            return PyLong_FromLong(Hacl_Hash_Blake2s_Simd128_info(self->blake2s_128_state).digest_length);
#endif
        case Blake2b:
            return PyLong_FromLong(Hacl_Hash_Blake2b_info(self->blake2b_state).digest_length);
        case Blake2s:
            return PyLong_FromLong(Hacl_Hash_Blake2s_info(self->blake2s_state).digest_length);
        default:
            Py_UNREACHABLE();
    }
}


static PyGetSetDef py_blake2b_getsetters[] = {
    {"name", (getter)py_blake2b_get_name,
        NULL, NULL, NULL},
    {"block_size", (getter)py_blake2b_get_block_size,
        NULL, NULL, NULL},
    {"digest_size", (getter)py_blake2b_get_digest_size,
        NULL, NULL, NULL},
    {NULL}
};


static void
py_blake2b_dealloc(Blake2Object *self)
{
    switch (self->impl) {
#if HACL_CAN_COMPILE_SIMD256
        case Blake2b_256:
            if (self->blake2b_256_state != NULL)
                Hacl_Hash_Blake2b_Simd256_free(self->blake2b_256_state);
            break;
#endif
#if HACL_CAN_COMPILE_SIMD128
        case Blake2s_128:
            if (self->blake2s_128_state != NULL)
                Hacl_Hash_Blake2s_Simd128_free(self->blake2s_128_state);
            break;
#endif
        case Blake2b:
            // This happens if we hit "goto error" in the middle of the
            // initialization function. We leverage the fact that tp_alloc
            // guarantees that the contents of the object are NULL-initialized
            // (see documentation for PyType_GenericAlloc) to detect this case.
            if (self->blake2b_state != NULL)
                Hacl_Hash_Blake2b_free(self->blake2b_state);
            break;
        case Blake2s:
            if (self->blake2s_state != NULL)
                Hacl_Hash_Blake2s_free(self->blake2s_state);
            break;
        default:
            Py_UNREACHABLE();
    }

    PyTypeObject *type = Py_TYPE(self);
    PyObject_Free(self);
    Py_DECREF(type);
}

static PyType_Slot blake2b_type_slots[] = {
    {Py_tp_dealloc, py_blake2b_dealloc},
    {Py_tp_doc, (char *)py_blake2b_new__doc__},
    {Py_tp_methods, py_blake2b_methods},
    {Py_tp_getset, py_blake2b_getsetters},
    {Py_tp_new, py_blake2b_new},
    {0,0}
};

static PyType_Slot blake2s_type_slots[] = {
    {Py_tp_dealloc, py_blake2b_dealloc},
    {Py_tp_doc, (char *)py_blake2s_new__doc__},
    {Py_tp_methods, py_blake2b_methods},
    {Py_tp_getset, py_blake2b_getsetters},
    // only the constructor differs, so that it can receive a clinic-generated
    // default digest length suitable for blake2s
    {Py_tp_new, py_blake2s_new},
    {0,0}
};

static PyType_Spec blake2b_type_spec = {
    .name = "_blake2.blake2b",
    .basicsize =  sizeof(Blake2Object),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = blake2b_type_slots
};

static PyType_Spec blake2s_type_spec = {
    .name = "_blake2.blake2s",
    .basicsize =  sizeof(Blake2Object),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .slots = blake2s_type_slots
};
