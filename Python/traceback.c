
/* Traceback implementation */

#include "Python.h"

#include "pycore_ast.h"           // asdl_seq_GET()
#include "pycore_call.h"          // _PyObject_CallMethodFormat()
#include "pycore_compile.h"       // _PyAST_Optimize()
#include "pycore_fileutils.h"     // _Py_BEGIN_SUPPRESS_IPH
#include "pycore_frame.h"         // _PyFrame_GetCode()
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_parser.h"        // _PyParser_ASTFromString
#include "pycore_pyarena.h"       // _PyArena_Free()
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_sysmodule.h"     // _PySys_GetAttr()
#include "pycore_traceback.h"     // EXCEPTION_TB_HEADER

#include "../Parser/pegen.h"      // _PyPegen_byte_offset_to_character_offset()
#include "frameobject.h"          // PyFrame_New()

#include "osdefs.h"               // SEP
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#define OFF(x) offsetof(PyTracebackObject, x)

#define PUTS(fd, str) (void)_Py_write_noraise(fd, str, (int)strlen(str))
#define MAX_STRING_LENGTH 500
#define MAX_FRAME_DEPTH 100
#define MAX_NTHREADS 100

/* Function from Parser/tokenizer.c */
extern char* _PyTokenizer_FindEncodingFilename(int, PyObject *);

/*[clinic input]
class TracebackType "PyTracebackObject *" "&PyTraceback_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=928fa06c10151120]*/

#include "clinic/traceback.c.h"

static PyObject *
tb_create_raw(PyTracebackObject *next, PyFrameObject *frame, int lasti,
              int lineno)
{
    PyTracebackObject *tb;
    if ((next != NULL && !PyTraceBack_Check(next)) ||
                    frame == NULL || !PyFrame_Check(frame)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    tb = PyObject_GC_New(PyTracebackObject, &PyTraceBack_Type);
    if (tb != NULL) {
        tb->tb_next = (PyTracebackObject*)Py_XNewRef(next);
        tb->tb_frame = (PyFrameObject*)Py_XNewRef(frame);
        tb->tb_lasti = lasti;
        tb->tb_lineno = lineno;
        PyObject_GC_Track(tb);
    }
    return (PyObject *)tb;
}

/*[clinic input]
@classmethod
TracebackType.__new__ as tb_new

  tb_next: object
  tb_frame: object(type='PyFrameObject *', subclass_of='&PyFrame_Type')
  tb_lasti: int
  tb_lineno: int

Create a new traceback object.
[clinic start generated code]*/

static PyObject *
tb_new_impl(PyTypeObject *type, PyObject *tb_next, PyFrameObject *tb_frame,
            int tb_lasti, int tb_lineno)
/*[clinic end generated code: output=fa077debd72d861a input=01cbe8ec8783fca7]*/
{
    if (tb_next == Py_None) {
        tb_next = NULL;
    } else if (!PyTraceBack_Check(tb_next)) {
        return PyErr_Format(PyExc_TypeError,
                            "expected traceback object or None, got '%s'",
                            Py_TYPE(tb_next)->tp_name);
    }

    return tb_create_raw((PyTracebackObject *)tb_next, tb_frame, tb_lasti,
                         tb_lineno);
}

static PyObject *
tb_dir(PyTracebackObject *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("[ssss]", "tb_frame", "tb_next",
                                   "tb_lasti", "tb_lineno");
}

static PyObject *
tb_next_get(PyTracebackObject *self, void *Py_UNUSED(_))
{
    PyObject* ret = (PyObject*)self->tb_next;
    if (!ret) {
        ret = Py_None;
    }
    return Py_NewRef(ret);
}

static int
tb_next_set(PyTracebackObject *self, PyObject *new_next, void *Py_UNUSED(_))
{
    if (!new_next) {
        PyErr_Format(PyExc_TypeError, "can't delete tb_next attribute");
        return -1;
    }

    /* We accept None or a traceback object, and map None -> NULL (inverse of
       tb_next_get) */
    if (new_next == Py_None) {
        new_next = NULL;
    } else if (!PyTraceBack_Check(new_next)) {
        PyErr_Format(PyExc_TypeError,
                     "expected traceback object, got '%s'",
                     Py_TYPE(new_next)->tp_name);
        return -1;
    }

    /* Check for loops */
    PyTracebackObject *cursor = (PyTracebackObject *)new_next;
    while (cursor) {
        if (cursor == self) {
            PyErr_Format(PyExc_ValueError, "traceback loop detected");
            return -1;
        }
        cursor = cursor->tb_next;
    }

    Py_XSETREF(self->tb_next, (PyTracebackObject *)Py_XNewRef(new_next));

    return 0;
}


static PyMethodDef tb_methods[] = {
   {"__dir__", _PyCFunction_CAST(tb_dir), METH_NOARGS},
   {NULL, NULL, 0, NULL},
};

static PyMemberDef tb_memberlist[] = {
    {"tb_frame",        _Py_T_OBJECT,       OFF(tb_frame),  Py_READONLY|Py_AUDIT_READ},
    {"tb_lasti",        Py_T_INT,          OFF(tb_lasti),  Py_READONLY},
    {"tb_lineno",       Py_T_INT,          OFF(tb_lineno), Py_READONLY},
    {NULL}      /* Sentinel */
};

static PyGetSetDef tb_getsetters[] = {
    {"tb_next", (getter)tb_next_get, (setter)tb_next_set, NULL, NULL},
    {NULL}      /* Sentinel */
};

static void
tb_dealloc(PyTracebackObject *tb)
{
    PyObject_GC_UnTrack(tb);
    Py_TRASHCAN_BEGIN(tb, tb_dealloc)
    Py_XDECREF(tb->tb_next);
    Py_XDECREF(tb->tb_frame);
    PyObject_GC_Del(tb);
    Py_TRASHCAN_END
}

static int
tb_traverse(PyTracebackObject *tb, visitproc visit, void *arg)
{
    Py_VISIT(tb->tb_next);
    Py_VISIT(tb->tb_frame);
    return 0;
}

static int
tb_clear(PyTracebackObject *tb)
{
    Py_CLEAR(tb->tb_next);
    Py_CLEAR(tb->tb_frame);
    return 0;
}

PyTypeObject PyTraceBack_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "traceback",
    sizeof(PyTracebackObject),
    0,
    (destructor)tb_dealloc, /*tp_dealloc*/
    0,                  /*tp_vectorcall_offset*/
    0,    /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    0,                  /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /* tp_hash */
    0,                  /* tp_call */
    0,                  /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                  /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    tb_new__doc__,                              /* tp_doc */
    (traverseproc)tb_traverse,                  /* tp_traverse */
    (inquiry)tb_clear,                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    tb_methods,         /* tp_methods */
    tb_memberlist,      /* tp_members */
    tb_getsetters,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    tb_new,                                     /* tp_new */
};


PyObject*
_PyTraceBack_FromFrame(PyObject *tb_next, PyFrameObject *frame)
{
    assert(tb_next == NULL || PyTraceBack_Check(tb_next));
    assert(frame != NULL);
    int addr = _PyInterpreterFrame_LASTI(frame->f_frame) * sizeof(_Py_CODEUNIT);
    return tb_create_raw((PyTracebackObject *)tb_next, frame, addr,
                         PyFrame_GetLineNumber(frame));
}


int
PyTraceBack_Here(PyFrameObject *frame)
{
    PyObject *exc = PyErr_GetRaisedException();
    assert(PyExceptionInstance_Check(exc));
    PyObject *tb = PyException_GetTraceback(exc);
    PyObject *newtb = _PyTraceBack_FromFrame(tb, frame);
    Py_XDECREF(tb);
    if (newtb == NULL) {
        _PyErr_ChainExceptions1(exc);
        return -1;
    }
    PyException_SetTraceback(exc, newtb);
    Py_XDECREF(newtb);
    PyErr_SetRaisedException(exc);
    return 0;
}

/* Insert a frame into the traceback for (funcname, filename, lineno). */
void _PyTraceback_Add(const char *funcname, const char *filename, int lineno)
{
    PyObject *globals;
    PyCodeObject *code;
    PyFrameObject *frame;
    PyThreadState *tstate = _PyThreadState_GET();

    /* Save and clear the current exception. Python functions must not be
       called with an exception set. Calling Python functions happens when
       the codec of the filesystem encoding is implemented in pure Python. */
    PyObject *exc = _PyErr_GetRaisedException(tstate);

    globals = PyDict_New();
    if (!globals)
        goto error;
    code = PyCode_NewEmpty(filename, funcname, lineno);
    if (!code) {
        Py_DECREF(globals);
        goto error;
    }
    frame = PyFrame_New(tstate, code, globals, NULL);
    Py_DECREF(globals);
    Py_DECREF(code);
    if (!frame)
        goto error;
    frame->f_lineno = lineno;

    _PyErr_SetRaisedException(tstate, exc);
    PyTraceBack_Here(frame);
    Py_DECREF(frame);
    return;

error:
    _PyErr_ChainExceptions1(exc);
}

static PyObject *
_Py_FindSourceFile(PyObject *filename, char* namebuf, size_t namelen, PyObject *io)
{
    Py_ssize_t i;
    PyObject *binary;
    PyObject *v;
    Py_ssize_t npath;
    size_t taillen;
    PyObject *syspath;
    PyObject *path;
    const char* tail;
    PyObject *filebytes;
    const char* filepath;
    Py_ssize_t len;
    PyObject* result;
    PyObject *open = NULL;

    filebytes = PyUnicode_EncodeFSDefault(filename);
    if (filebytes == NULL) {
        PyErr_Clear();
        return NULL;
    }
    filepath = PyBytes_AS_STRING(filebytes);

    /* Search tail of filename in sys.path before giving up */
    tail = strrchr(filepath, SEP);
    if (tail == NULL)
        tail = filepath;
    else
        tail++;
    taillen = strlen(tail);

    PyThreadState *tstate = _PyThreadState_GET();
    syspath = _PySys_GetAttr(tstate, &_Py_ID(path));
    if (syspath == NULL || !PyList_Check(syspath))
        goto error;
    npath = PyList_Size(syspath);

    open = PyObject_GetAttr(io, &_Py_ID(open));
    for (i = 0; i < npath; i++) {
        v = PyList_GetItem(syspath, i);
        if (v == NULL) {
            PyErr_Clear();
            break;
        }
        if (!PyUnicode_Check(v))
            continue;
        path = PyUnicode_EncodeFSDefault(v);
        if (path == NULL) {
            PyErr_Clear();
            continue;
        }
        len = PyBytes_GET_SIZE(path);
        if (len + 1 + (Py_ssize_t)taillen >= (Py_ssize_t)namelen - 1) {
            Py_DECREF(path);
            continue; /* Too long */
        }
        strcpy(namebuf, PyBytes_AS_STRING(path));
        Py_DECREF(path);
        if (strlen(namebuf) != (size_t)len)
            continue; /* v contains '\0' */
        if (len > 0 && namebuf[len-1] != SEP)
            namebuf[len++] = SEP;
        strcpy(namebuf+len, tail);

        binary = _PyObject_CallMethodFormat(tstate, open, "ss", namebuf, "rb");
        if (binary != NULL) {
            result = binary;
            goto finally;
        }
        PyErr_Clear();
    }
    goto error;

error:
    result = NULL;
finally:
    Py_XDECREF(open);
    Py_DECREF(filebytes);
    return result;
}

/* Writes indent spaces. Returns 0 on success and non-zero on failure.
 */
int
_Py_WriteIndent(int indent, PyObject *f)
{
    char buf[11] = "          ";
    assert(strlen(buf) == 10);
    while (indent > 0) {
        if (indent < 10) {
            buf[indent] = '\0';
        }
        if (PyFile_WriteString(buf, f) < 0) {
            return -1;
        }
        indent -= 10;
    }
    return 0;
}

/* Writes indent spaces, followed by the margin if it is not `\0`.
   Returns 0 on success and non-zero on failure.
 */
int
_Py_WriteIndentedMargin(int indent, const char *margin, PyObject *f)
{
    if (_Py_WriteIndent(indent, f) < 0) {
        return -1;
    }
    if (margin) {
        if (PyFile_WriteString(margin, f) < 0) {
            return -1;
        }
    }
    return 0;
}

static PyObject*
join_string_list(const char *join, PyObject* seq)
{
    PyObject *separator = PyUnicode_FromString(join);
    if (!separator) {
        return NULL;
    }
    PyObject *result = PyUnicode_Join(separator, seq);
    Py_DECREF(separator);
    return result;
}

static int
get_source_lines(PyObject *filename, int lineno, int end_lineno, PyObject **lines)
{
    int fd;
    int i;
    char *found_encoding;
    const char *encoding;
    PyObject *io;
    PyObject *binary;
    PyObject *fob = NULL;
    PyObject *lineobj = NULL;
    PyObject *res;
    char buf[MAXPATHLEN+1];

    /* open the file */
    if (filename == NULL)
        return 0;

    if (lines == NULL)
        return 0;

    /* Do not attempt to open things like <string> or <stdin> */
    assert(PyUnicode_Check(filename));
    if (PyUnicode_READ_CHAR(filename, 0) == '<') {
        Py_ssize_t len = PyUnicode_GET_LENGTH(filename);
        if (len > 0 && PyUnicode_READ_CHAR(filename, len - 1) == '>') {
            return 0;
        }
    }

    io = PyImport_ImportModule("io");
    if (io == NULL) {
        return -1;
    }

    binary = _PyObject_CallMethod(io, &_Py_ID(open), "Os", filename, "rb");
    if (binary == NULL) {
        PyErr_Clear();

        binary = _Py_FindSourceFile(filename, buf, sizeof(buf), io);
        if (binary == NULL) {
            Py_DECREF(io);
            return -1;
        }
    }

    /* use the right encoding to decode the file as unicode */
    fd = PyObject_AsFileDescriptor(binary);
    if (fd < 0) {
        Py_DECREF(io);
        Py_DECREF(binary);
        return 0;
    }
    found_encoding = _PyTokenizer_FindEncodingFilename(fd, filename);
    if (found_encoding == NULL)
        PyErr_Clear();
    encoding = (found_encoding != NULL) ? found_encoding : "utf-8";
    /* Reset position */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        Py_DECREF(io);
        Py_DECREF(binary);
        PyMem_Free(found_encoding);
        return 0;
    }
    fob = _PyObject_CallMethod(io, &_Py_ID(TextIOWrapper),
                               "Os", binary, encoding);
    Py_DECREF(io);
    PyMem_Free(found_encoding);

    if (fob == NULL) {
        PyErr_Clear();

        res = PyObject_CallMethodNoArgs(binary, &_Py_ID(close));
        Py_DECREF(binary);
        if (res)
            Py_DECREF(res);
        else
            PyErr_Clear();
        return 0;
    }
    Py_DECREF(binary);

    /* get lines between lineno and end_lineno, inclusive */
    PyObject *lines_accum = PyList_New(end_lineno - lineno + 1);
    if (!lines_accum) {
        goto cleanup_fob;
    }
    for (i = 1; i <= end_lineno; i++) {
        lineobj = PyFile_GetLine(fob, -1);
        if (i >= lineno) {
            if (!lineobj || !PyUnicode_Check(lineobj)) {
                Py_XSETREF(lineobj, PyUnicode_FromString(""));
                if (!lineobj) {
                    goto cleanup_fob;
                }
            }
            PyList_SET_ITEM(lines_accum, i - lineno, lineobj);
        }
    }
    *lines = join_string_list("\n", lines_accum);
cleanup_fob:
    Py_XDECREF(lines_accum);
    PyErr_Clear();
    res = PyObject_CallMethodNoArgs(fob, &_Py_ID(close));
    if (res) {
        Py_DECREF(res);
    }
    else {
        PyErr_Clear();
    }
    Py_DECREF(fob);

    return 0;
}

static int
_write_line_with_margin_and_indent(PyObject *f, PyObject *line, int indent,
                                   int margin_indent, const char *margin)
{
    if (line == NULL) {
        return -1;
    }

    if (_Py_WriteIndentedMargin(margin_indent, margin, f) < 0) {
        return -1;
    }

    /* Write some spaces before the line */
    if (_Py_WriteIndent(indent, f) < 0) {
        return -1;
    }

    /* finally display the line */
    if (PyFile_WriteObject(line, f, Py_PRINT_RAW) < 0) {
        return -1;
    }

    if (PyFile_WriteString("\n", f) < 0) {
        return -1;
    }

    return 0;
}

#define IS_WHITESPACE(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\f'))

static int
display_source_line_with_margin(PyObject *f, PyObject *filename,
                                int lineno, int indent,
                                int margin_indent, const char *margin,
                                int *truncation, PyObject **line)
{
    PyObject *lineobj = NULL;
    int i;
    int result;
    int kind;
    const void *data;

    result = get_source_lines(filename, lineno, lineno, &lineobj);
    if (result || lineobj == NULL) {
        Py_XDECREF(lineobj);
        return result;
    }

    if (line) {
        *line = Py_NewRef(lineobj);
    }

    /* remove the indentation of the line */
    kind = PyUnicode_KIND(lineobj);
    data = PyUnicode_DATA(lineobj);
    for (i=0; i < PyUnicode_GET_LENGTH(lineobj); i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (!IS_WHITESPACE(ch))
            break;
    }
    if (i) {
        PyObject *truncated;
        truncated = PyUnicode_Substring(lineobj, i, PyUnicode_GET_LENGTH(lineobj));
        if (truncated) {
            Py_SETREF(lineobj, truncated);
        } else {
            PyErr_Clear();
        }
    }

    if (truncation != NULL) {
        *truncation = i - indent;
    }

    if (_write_line_with_margin_and_indent(f, lineobj, indent, margin_indent, margin)) {
        goto error;
    }

    Py_XDECREF(lineobj);
    return 0;
error:
    Py_XDECREF(lineobj);
    return -1;
}

int
_Py_DisplaySourceLine(PyObject *f, PyObject *filename, int lineno, int indent,
                      int *truncation, PyObject **line)
{
    return display_source_line_with_margin(f, filename, lineno, indent, 0,
                                           NULL, truncation, line);
}

/* AST based Traceback Specialization
 *
 * When displaying a new traceback line, for certain syntactical constructs
 * (e.g a subscript, an arithmetic operation) we try to create a representation
 * that separates the primary source of error from the rest.
 *
 * Example specialization of BinOp nodes:
 *  Traceback (most recent call last):
 *    File "/home/isidentical/cpython/cpython/t.py", line 10, in <module>
 *      add_values(1, 2, 'x', 3, 4)
 *    File "/home/isidentical/cpython/cpython/t.py", line 2, in add_values
 *      return a + b + c + d + e
 *             ~~~~~~^~~
 *  TypeError: 'NoneType' object is not subscriptable
 */

// The below functions are helper functions for anchor extraction

// Get segment_lines[lineno] in C string form
static const char
*_get_segment_str(PyObject *segment_lines, Py_ssize_t lineno, Py_ssize_t *size)
{
    return PyUnicode_AsUTF8AndSize(PyList_GET_ITEM(segment_lines, lineno), size);
}

// Gets the next valid offset in segment_lines[lineno], if the current offset is not valid
static int
_next_valid_offset(PyObject *segment_lines, Py_ssize_t *lineno, Py_ssize_t *offset)
{
    Py_ssize_t str_len = 0;
    const char *segment_str = NULL;
    while (*lineno < PyList_GET_SIZE(segment_lines)) {
        segment_str = _get_segment_str(segment_lines, *lineno, &str_len);
        if (!segment_str) {
            return -1;
        }
        if (*offset < str_len) {
            break;
        }
        *offset = 0;
        ++*lineno;
    }
    assert(*lineno < PyList_GET_SIZE(segment_lines));
    assert(segment_str);
    assert(*offset < str_len);
    return 0;
}

// Get the next valid offset
static int
_increment_offset(PyObject *segment_lines, Py_ssize_t *lineno, Py_ssize_t *offset)
{
    ++*offset;
    return _next_valid_offset(segment_lines, lineno, offset);
}

// Get the next valid offset at least on the next line
static int
_nextline(PyObject *segment_lines, Py_ssize_t *lineno, Py_ssize_t *offset)
{
    *offset = 0;
    ++*lineno;
    return _next_valid_offset(segment_lines, lineno, offset);
}

// Get the next valid non-"\#" character that satisfies the stop predicate
static int
_increment_until(PyObject *segment_lines, Py_ssize_t *lineno,
                 Py_ssize_t *offset, int (*stop)(char))
{
    while (1) {
        Py_ssize_t str_len;
        const char *segment_str = _get_segment_str(segment_lines, *lineno, &str_len);
        if (!segment_str || *offset >= str_len) {
            return -1;
        }
        char ch = segment_str[*offset];
        // jump to next line if we encounter line break or comment
        if (ch == '\\' || ch == '#') {
            if (_nextline(segment_lines, lineno, offset)) {
                return -1;
            }
        } else if (!stop(ch)) {
            if (_increment_offset(segment_lines, lineno, offset)) {
                return -1;
            }
        } else {
            break;
        }
    }
    return 0;
}

// is the character a binary op character? (not whitespace or closing paren)
static int
_is_op_char(char ch)
{
    if (!IS_WHITESPACE(ch) && ch != ')') {
        return 1;
    }
    return 0;
}

static int
_is_open_bracket_char(char ch)
{
    return ch == '[';
}

static int
_is_open_paren_char(char ch)
{
    return ch == '(';
}

static int
extract_anchors_from_expr(PyObject *segment_lines, expr_ty expr,
                          Py_ssize_t *left_anchor_lineno, Py_ssize_t *right_anchor_lineno,
                          Py_ssize_t *left_anchor_col, Py_ssize_t *right_anchor_col,
                          char** primary_error_char, char** secondary_error_char)
{
    switch (expr->kind) {
        case BinOp_kind: {
            // anchor begin: first binary op char after left subexpression
            // anchor end: 1 or 2 characters after anchor begin
            expr_ty left = expr->v.BinOp.left;
            expr_ty right = expr->v.BinOp.right;
            *left_anchor_lineno = left->end_lineno - 2;
            *left_anchor_col = left->end_col_offset;
            if (_next_valid_offset(
                segment_lines, left_anchor_lineno, left_anchor_col
            )) {
                return 0;
            }
            // keep going until the current char is not whitespace or ')'
            if (_increment_until(
                segment_lines, left_anchor_lineno, left_anchor_col, _is_op_char
            )) {
                return 0;
            }
            *right_anchor_lineno = *left_anchor_lineno;
            *right_anchor_col = *left_anchor_col + 1;

            Py_ssize_t str_len = 0;
            const char *segment_str = _get_segment_str(
                segment_lines, *left_anchor_lineno, &str_len
            );
            if (!segment_str) {
                return 0;
            }

            // Check whether if this is a two-character operator (e.g. //)
            if (
                *right_anchor_col < str_len &&
                (
                    // operator char should not be in the right subexpression
                    right->lineno - 2 > *right_anchor_lineno ||
                    *right_anchor_col < right->col_offset
                )
            ) {
                char ch = segment_str[*right_anchor_col];
                if (_is_op_char(ch) && ch != '\\' && ch != '#') {
                    ++*right_anchor_col;
                }
            }
            // Set the error characters
            *primary_error_char = "~";
            *secondary_error_char = "^";
            return 1;
        }
        case Subscript_kind: {
            // anchor begin: first "[" after the value subexpression
            // anchor end: end of the entire subscript expression
            *left_anchor_lineno = expr->v.Subscript.value->end_lineno - 2;
            *left_anchor_col = expr->v.Subscript.value->end_col_offset;
            if (_next_valid_offset(
                segment_lines, left_anchor_lineno, left_anchor_col
            )) {
                return 0;
            }
            if (_increment_until(
                segment_lines, left_anchor_lineno, left_anchor_col, _is_open_bracket_char
            )) {
                return 0;
            }
            *right_anchor_lineno = expr->end_lineno - 2;
            *right_anchor_col = expr->end_col_offset;

            // Set the error characters
            *primary_error_char = "~";
            *secondary_error_char = "^";
            return 1;
        }
        case Call_kind:
            // anchor positions determined similarly to Subscript
            *left_anchor_lineno = expr->v.Call.func->end_lineno - 2;
            *left_anchor_col = expr->v.Call.func->end_col_offset;
            if (_next_valid_offset(
                segment_lines, left_anchor_lineno, left_anchor_col
            )) {
                return 0;
            }
            if (_increment_until(
                segment_lines, left_anchor_lineno, left_anchor_col, _is_open_paren_char
            )) {
                return 0;
            }
            *right_anchor_lineno = expr->end_lineno - 2;
            *right_anchor_col = expr->end_col_offset;

            // Set the error characters
            *primary_error_char = "~";
            *secondary_error_char = "^";
            return 1;
        default:
            return 0;
    }
}

static int
extract_anchors_from_stmt(PyObject *segment_lines, stmt_ty statement,
                          Py_ssize_t *left_anchor_lineno, Py_ssize_t *right_anchor_lineno,
                          Py_ssize_t *left_anchor_col, Py_ssize_t *right_anchor_col,
                          char** primary_error_char, char** secondary_error_char)
{
    switch (statement->kind) {
        case Expr_kind: {
            return extract_anchors_from_expr(segment_lines, statement->v.Expr.value,
                                             left_anchor_lineno, right_anchor_lineno,
                                             left_anchor_col, right_anchor_col,
                                             primary_error_char, secondary_error_char);
        }
        default:
            return 0;
    }
}

// Returns:
// 1 if anchors were found
// 0 if anchors could not be computed
// -1 on error
static int
extract_anchors_from_line(PyObject *filename, PyObject *lines,
                          Py_ssize_t start_offset, Py_ssize_t end_offset,
                          Py_ssize_t *left_anchor_lineno, Py_ssize_t *right_anchor_lineno,
                          Py_ssize_t *left_anchor_col, Py_ssize_t *right_anchor_col,
                          char** primary_error_char, char** secondary_error_char)
{
    int res = -1;
    PyArena *arena = NULL;
    PyObject *segment = NULL;
    PyObject *segment_lines = NULL;
    PyObject *tmp;

    segment = join_string_list("\n", lines);
    if (!segment) {
        goto done;
    }

    // truncate segment
    Py_ssize_t num_lines = PyList_Size(lines);
    PyObject *last_string = PyList_GET_ITEM(lines, num_lines - 1);
    Py_ssize_t offset_from_right = PyUnicode_GET_LENGTH(last_string) - end_offset;
    Py_ssize_t join_end_offset = PyUnicode_GET_LENGTH(segment) - offset_from_right;
    tmp = PyUnicode_Substring(
        segment, start_offset, join_end_offset
    );
    if (!tmp) {
        goto done;
    }
    Py_SETREF(segment, tmp);

    // same as `lines`, but first/last strings are truncated
    segment_lines = PyUnicode_Splitlines(segment, 0);
    if (!segment_lines) {
        goto done;
    }

    // segment = "(\n" + segment + "\n)"
    PyObject *paren_str = PyUnicode_FromString("(\n");
    if (!paren_str) {
        goto done;
    }
    tmp = PyUnicode_Concat(paren_str, segment);
    Py_DECREF(paren_str);
    if (!tmp) {
        goto done;
    }
    Py_SETREF(segment, tmp);

    paren_str = PyUnicode_FromString("\n)");
    if (!paren_str) {
        goto done;
    }
    tmp = PyUnicode_Concat(segment, paren_str);
    Py_DECREF(paren_str);
    if (!tmp) {
        goto done;
    }
    Py_SETREF(segment, tmp);

    const char *segment_str = PyUnicode_AsUTF8(segment);
    if (!segment_str) {
        goto done;
    }

    arena = _PyArena_New();
    if (!arena) {
        goto done;
    }

    PyCompilerFlags flags = _PyCompilerFlags_INIT;

    mod_ty module = _PyParser_ASTFromString(segment_str, filename, Py_file_input,
                                            &flags, arena);
    if (!module) {
        if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_SyntaxError)) {
            // AST parsing failed due to SyntaxError - ignore it
            PyErr_Clear();
            res = 0;
        }
        goto done;
    }
    if (!_PyAST_Optimize(module, arena, _Py_GetConfig()->optimization_level, 0)) {
        goto done;
    }

    assert(module->kind == Module_kind);
    if (asdl_seq_LEN(module->v.Module.body) == 1) {
        stmt_ty statement = asdl_seq_GET(module->v.Module.body, 0);
        res = extract_anchors_from_stmt(segment_lines, statement,
                                        left_anchor_lineno, right_anchor_lineno,
                                        left_anchor_col, right_anchor_col,
                                        primary_error_char, secondary_error_char);
    } else {
        res = 0;
    }

done:
    if (res > 0) {
        // Normalize the AST offsets to byte offsets and adjust them with the
        // start of the actual line (instead of the source code segment).
        assert(segment_lines != NULL);
        assert(*left_anchor_lineno >= 0);
        assert(*left_anchor_col >= 0);
        assert(*right_anchor_lineno >= 0);
        assert(*right_anchor_col >= 0);
        *left_anchor_col = _PyPegen_byte_offset_to_character_offset(
            PyList_GET_ITEM(segment_lines, *left_anchor_lineno), *left_anchor_col
        );
        *right_anchor_col = _PyPegen_byte_offset_to_character_offset(
            PyList_GET_ITEM(segment_lines, *right_anchor_lineno), *right_anchor_col
        );
        if (*left_anchor_lineno == 0) {
            *left_anchor_col += start_offset;
        }
        if (*right_anchor_lineno == 0) {
            *right_anchor_col += start_offset;
        }
    }
    Py_XDECREF(segment);
    Py_XDECREF(segment_lines);
    if (arena) {
        _PyArena_Free(arena);
    }
    return res;
}

#define _TRACEBACK_SOURCE_LINE_INDENT 4

static inline int
ignore_source_errors(void) {
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            return -1;
        }
        PyErr_Clear();
    }
    return 0;
}

// helper data structure to keep track of which lines to output
typedef struct SignificantLines {
    // we ony add a maximum of 8 lines
    Py_ssize_t lines[8];
    size_t size;
} SignificantLines;

static void significant_lines_init(SignificantLines *sl) {
    sl->size = 0;
}

static void significant_lines_append(SignificantLines* sl, Py_ssize_t line, Py_ssize_t max_line)
{
    if (line < 0 || line > max_line) {
        return;
    }
    assert(sl->size < 8);
    sl->lines[sl->size++] = line;
}

static int significant_lines_compare(const void *a, const void *b)
{
    return (int)(*(Py_ssize_t *)a - *(Py_ssize_t *)b);
}

// sort lines and remove duplicate lines
static void significant_lines_process(SignificantLines *sl)
{
    qsort(sl->lines, sl->size, sizeof(Py_ssize_t), significant_lines_compare);
    Py_ssize_t lines[8];
    size_t idx = 0;
    for (size_t i = 0; i < sl->size; i++) {
        if (i && sl->lines[i] == sl->lines[i - 1]) {
            continue;
        }
        lines[idx++] = sl->lines[i];
    }
    memcpy(sl->lines, lines, idx * sizeof(Py_ssize_t));
    sl->size = idx;
}

// output lines[lineno] along with carets
static int
print_error_location_carets(PyObject *lines, Py_ssize_t lineno,
                            Py_ssize_t start_offset, Py_ssize_t end_offset,
                            Py_ssize_t left_end_lineno, Py_ssize_t right_start_lineno,
                            Py_ssize_t left_end_offset, Py_ssize_t right_start_offset,
                            const char *primary, const char *secondary,
                            PyObject *f, int indent, int margin_indent, const char *margin)
{
    Py_ssize_t num_lines = PyList_Size(lines);
    PyObject *line = PyList_GET_ITEM(lines, lineno);
    int special_chars = (
        left_end_lineno != -1 && left_end_offset != -1 &&
        right_start_lineno != -1 && right_start_offset != -1
    );
    Py_ssize_t len = (lineno == num_lines - 1) ? end_offset : PyUnicode_GET_LENGTH(line);
    PyObject *carets = PyList_New(len);
    if (!carets) {
        goto error;
    }
    int kind = PyUnicode_KIND(line);
    const void *data = PyUnicode_DATA(line);
    bool has_non_ws = 0;
    for (Py_ssize_t col = 0; col < len; col++) {
        const char *ch = primary;
        if (!has_non_ws) {
            Py_UCS4 ch = PyUnicode_READ(kind, data, col);
            if (!IS_WHITESPACE(ch)) {
                has_non_ws = 1;
            }
        }
        if (!has_non_ws || (lineno == 0 && col < start_offset)) {
            // before first non-ws char of the line, or before start of instruction
            ch = " ";
        } else if (
            special_chars &&
            (lineno > left_end_lineno || (lineno == left_end_lineno && col >= left_end_offset)) &&
            (lineno < right_start_lineno || (lineno == right_start_lineno && col < right_start_offset))
        ) {
            // within anchors
            ch = secondary;
        } // else ch = primary

        PyObject *str = PyUnicode_FromString(ch);
        if (!str) {
            goto error;
        }
        PyList_SET_ITEM(carets, col, str);
    }
    PyObject *caret_line_str = join_string_list("", carets);
    if (!caret_line_str) {
        goto error;
    }
    int res = _write_line_with_margin_and_indent(f, caret_line_str, indent, margin_indent, margin);
    Py_DECREF(caret_line_str);
    if (res) {
        goto error;
    }
    Py_DECREF(carets);
    return 0;
error:
    Py_XDECREF(carets);
    return -1;
}

static int
_is_all_whitespace(PyObject *line)
{
    int kind = PyUnicode_KIND(line);
    const void *data = PyUnicode_DATA(line);
    for (Py_ssize_t i = 0; i < PyUnicode_GET_LENGTH(line); i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (!IS_WHITESPACE(ch))
            return 0;
    }
    return 1;
}

// C implementation of textwrap.dedent.
// Returns a new reference to a list of dedented lines, NULL on failure.
// Sets `truncation` to the number of characters truncated.
// In abnormal cases (errors, whitespace-only input), `truncation` is set to 0.
static PyObject*
dedent(PyObject *lines, Py_ssize_t *truncation) {
    *truncation = 0;
    PyObject *split = PyUnicode_Splitlines(lines, 0);
    if (!split) {
        return NULL;
    }
    // Replace whitespace only lines with empty lines
    Py_ssize_t num_lines = PyList_Size(split);
    assert(num_lines > 0);
    for (Py_ssize_t i = 0; i < num_lines; i++) {
        if (_is_all_whitespace(PyList_GET_ITEM(split, i))) {
            PyObject *empty = PyUnicode_FromString("");
            if (!empty) {
                goto error;
            }
            PyList_SetItem(split, i, empty);
        }
    }

    // Find a reference line - the first non-empty line.
    // It is guaranteed to have a non-whitespace character.
    Py_ssize_t ref_lineno = 0;
    for (; ref_lineno < num_lines; ref_lineno++) {
        if (PyUnicode_GET_LENGTH(PyList_GET_ITEM(split, ref_lineno)) > 0) {
            break;
        }
    }
    if (ref_lineno == num_lines) {
        // empty input
        goto done;
    }

    // Compute the number of characters to dedent by.
    // Increment `col` until either lines[ref_line][col] is non-ws,
    // or there is another line i with lines[i][col] != lines[ref_line][col].
    Py_ssize_t col = 0;
    PyObject *ref_line = PyList_GET_ITEM(split, ref_lineno);
    Py_ssize_t ref_line_len = PyUnicode_GET_LENGTH(ref_line);
    for (; col < ref_line_len; col++) {
        Py_UCS4 ref_ch = PyUnicode_READ_CHAR(ref_line, col);
        if (!IS_WHITESPACE(ref_ch)) {
            goto dedent_compute_end;
        }
        // every line before ref_line is empty
        for (Py_ssize_t i = ref_lineno + 1; i < num_lines; i++) {
            PyObject* line = PyList_GET_ITEM(split, i);
            if (PyUnicode_GET_LENGTH(line) == 0) {
                continue;
            }
            // col >= len(line) implies the line is whitespace,
            // which cannot happen since we replaced whitespace lines
            // with empty strings.
            assert(col < PyUnicode_GET_LENGTH(line));
            Py_UCS4 ch = PyUnicode_READ_CHAR(line, col);
            if (ch != ref_ch) {
                goto dedent_compute_end;
            }
        }
    }
dedent_compute_end:

    *truncation = col;
    // truncate strings
    if (col == 0) {
        goto done;
    }
    for (Py_ssize_t i = 0; i < num_lines; i++) {
        PyObject* line = PyList_GET_ITEM(split, i);
        Py_ssize_t line_len = PyUnicode_GET_LENGTH(line);
        if (line_len == 0) {
            continue;
        }
        assert(col < line_len);
        PyObject* truncated_line = PyUnicode_Substring(line, col, line_len);
        if (!truncated_line) {
            goto error;
        }
        PyList_SetItem(split, i, truncated_line);
    }

done:
    return split;
error:
    Py_XDECREF(split);
    return NULL;
}

static int
tb_displayline(PyTracebackObject* tb, PyObject *f, PyObject *filename, int lineno,
               PyFrameObject *frame, PyObject *name, int margin_indent, const char *margin)
{
    if (filename == NULL || name == NULL) {
        return -1;
    }

    if (_Py_WriteIndentedMargin(margin_indent, margin, f) < 0) {
        return -1;
    }

    PyObject *line = PyUnicode_FromFormat("  File \"%U\", line %d, in %U\n",
                                          filename, lineno, name);
    if (line == NULL) {
        return -1;
    }

    int res = PyFile_WriteObject(line, f, Py_PRINT_RAW);
    Py_DECREF(line);
    if (res < 0) {
        return -1;
    }

    int err = 0;

    int code_offset = tb->tb_lasti;
    PyCodeObject* code = _PyFrame_GetCode(frame->f_frame);

    int start_line;
    int end_line;
    int start_col_byte_offset;
    int end_col_byte_offset;
    if (!PyCode_Addr2Location(code, code_offset, &start_line, &start_col_byte_offset,
                              &end_line, &end_col_byte_offset)) {
        start_line = end_line = lineno;
        start_col_byte_offset = end_col_byte_offset = -1;
    }

    if (start_line < 0) {
        // in case something went wrong
        start_line = lineno;
    }
    // only fetch first line if location information is missing
    if (end_line < 0 || start_col_byte_offset < 0 || end_col_byte_offset < 0) {
        end_line = lineno;
    }

    PyObject* lines_original = NULL;
    PyObject* lines = NULL;
    Py_ssize_t num_lines = 0;
    int rc = get_source_lines(filename, start_line, end_line, &lines_original);
    if (rc || !lines_original) {
        /* ignore errors since we can't report them, can we? */
        err = ignore_source_errors();
        goto error;
    }

    Py_ssize_t truncation = 0;
    lines = dedent(lines_original, &truncation);
    if (!lines) {
        goto error;
    }
    num_lines = PyList_Size(lines);

    // only output first line if no column location is given
    if (start_col_byte_offset < 0 || end_col_byte_offset < 0) {
        if (_write_line_with_margin_and_indent(
            f, PyList_GET_ITEM(lines, 0), _TRACEBACK_SOURCE_LINE_INDENT, margin_indent, margin
        )) {
            goto error;
        }
        goto done;
    }

    // When displaying errors, we will use the following generic structure:
    //
    //  ERROR LINE ERROR LINE ERROR LINE ERROR LINE ERROR LINE ERROR LINE ERROR LINE
    //        ~~~~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^~~~~~~~~~~~~~~~~~~~
    //        |              |-> left_end_offset     |                  |-> end_offset
    //        |-> start_offset                       |-> right_start_offset
    //
    // In general we will only have (start_offset, end_offset) but we can gather more information
    // by analyzing the AST of the text between *start_offset* and *end_offset*. If this succeeds
    // we could get *left_end_offset* and *right_start_offset* and some selection of characters for
    // the different ranges (primary_error_char and secondary_error_char). If we cannot obtain the
    // AST information or we cannot identify special ranges within it, then left_end_offset and
    // right_end_offset will be set to -1.
    //
    // To support displaying errors that span multiple lines, *left_end_lineno* and
    // *right_start_lineno* contain the line numbers of the special ranges.
    //
    // To keep the column indicators pertinent, they are not shown when the primary character
    // spans all of the error lines.

    PyObject *lines_original_split = PyUnicode_Splitlines(lines_original, 0);
    assert(PyList_Size(lines_original_split) == num_lines);
    if (!lines_original_split) {
        goto error;
    }

    // Convert the utf-8 byte offset to the actual character offset so we print the right number of carets.
    Py_ssize_t start_offset = _PyPegen_byte_offset_to_character_offset(
        PyList_GET_ITEM(lines_original_split, 0), start_col_byte_offset
    );
    if (start_offset < 0) {
        err = ignore_source_errors() < 0;
        Py_DECREF(lines_original_split);
        goto error;
    }

    Py_ssize_t end_offset = _PyPegen_byte_offset_to_character_offset(
        PyList_GET_ITEM(lines_original_split, num_lines - 1), end_col_byte_offset
    );
    Py_DECREF(lines_original_split);
    if (end_offset < 0) {
        err = ignore_source_errors() < 0;
        goto error;
    }

    // adjust start/end offset based on dedent
    start_offset = (start_offset < truncation) ? 0 : start_offset - truncation;
    end_offset = (end_offset < truncation) ? 0 : end_offset - truncation;

    Py_ssize_t left_end_lineno = -1;
    Py_ssize_t left_end_offset = -1;
    Py_ssize_t right_start_lineno = -1;
    Py_ssize_t right_start_offset = -1;

    char *primary_error_char = "^";
    char *secondary_error_char = primary_error_char;

    res = extract_anchors_from_line(filename, lines, start_offset, end_offset,
                                    &left_end_lineno, &right_start_lineno,
                                    &left_end_offset, &right_start_offset,
                                    &primary_error_char, &secondary_error_char);
    if (res < 0 && ignore_source_errors() < 0) {
        goto error;
    }

    int show_carets = 1;

    // only display significant lines: first line, last line, lines around anchor start/end
    SignificantLines sl;
    significant_lines_init(&sl);
    significant_lines_append(&sl, 0, num_lines - 1);
    significant_lines_append(&sl, num_lines - 1, num_lines - 1);

    if (res == 0) {
        // Elide indicators if primary char spans the frame line
        PyObject *tmp = PyUnicode_Substring(PyList_GET_ITEM(lines, 0), 0, start_offset);
        int before_start_empty = tmp && _is_all_whitespace(tmp);
        Py_XDECREF(tmp);
        PyObject *last_line = PyList_GET_ITEM(lines, num_lines - 1);
        tmp = PyUnicode_Substring(last_line, end_offset, PyUnicode_GET_LENGTH(last_line));
        int after_end_empty = tmp && _is_all_whitespace(tmp);
        Py_XDECREF(tmp);
        if (before_start_empty && after_end_empty) {
            show_carets = 0;
        }
        // clear anchor fields
        left_end_lineno = left_end_offset = right_start_lineno = right_start_offset = -1;
    } else {
        for (int i = -1; i <= 1; ++i) {
            significant_lines_append(&sl, i + left_end_lineno, num_lines - 1);
            significant_lines_append(&sl, i + right_start_lineno, num_lines - 1);
        }
    }

    // sort and dedupe significant lines
    significant_lines_process(&sl);

    for (size_t i = 0; i < sl.size; i++) {
        if (i > 0) {
            Py_ssize_t linediff = sl.lines[i] - sl.lines[i - 1];
            if (linediff == 2) {
                // only 1 line in between - just print it out
                if (_write_line_with_margin_and_indent(
                    f, PyList_GET_ITEM(lines, sl.lines[i] - 1), _TRACEBACK_SOURCE_LINE_INDENT, margin_indent, margin
                )) {
                    goto error;
                }
                if (show_carets && print_error_location_carets(
                    lines, sl.lines[i] - 1,
                    start_offset, end_offset,
                    left_end_lineno, right_start_lineno,
                    left_end_offset, right_start_offset,
                    primary_error_char, secondary_error_char,
                    f, _TRACEBACK_SOURCE_LINE_INDENT, margin_indent, margin
                )) {
                    goto error;
                }
            } else if (linediff > 2) {
                // more than 1 line in between - abbreviate
                PyObject *abbrv_str = PyUnicode_FromFormat("...<%d lines>...", (int)linediff - 1);
                if (!abbrv_str) {
                    goto error;
                }
                int write_res = _write_line_with_margin_and_indent(
                    f, abbrv_str, _TRACEBACK_SOURCE_LINE_INDENT, margin_indent, margin
                );
                Py_DECREF(abbrv_str);
                if (write_res) {
                    goto error;
                }
            }
        }
        // print the current line
        if (_write_line_with_margin_and_indent(
            f, PyList_GET_ITEM(lines, sl.lines[i]), _TRACEBACK_SOURCE_LINE_INDENT, margin_indent, margin
        )) {
            goto error;
        }
        if (show_carets && print_error_location_carets(
            lines, sl.lines[i],
            start_offset, end_offset,
            left_end_lineno, right_start_lineno,
            left_end_offset, right_start_offset,
            primary_error_char, secondary_error_char,
            f, _TRACEBACK_SOURCE_LINE_INDENT, margin_indent, margin
        )) {
            goto error;
        }
    }

done:
    Py_DECREF(lines_original);
    Py_DECREF(lines);
    return 0;
error:
    Py_XDECREF(lines_original);
    Py_XDECREF(lines);
    return err;
}

static const int TB_RECURSIVE_CUTOFF = 3; // Also hardcoded in traceback.py.

static int
tb_print_line_repeated(PyObject *f, long cnt)
{
    cnt -= TB_RECURSIVE_CUTOFF;
    PyObject *line = PyUnicode_FromFormat(
        (cnt > 1)
          ? "  [Previous line repeated %ld more times]\n"
          : "  [Previous line repeated %ld more time]\n",
        cnt);
    if (line == NULL) {
        return -1;
    }
    int err = PyFile_WriteObject(line, f, Py_PRINT_RAW);
    Py_DECREF(line);
    return err;
}

static int
tb_printinternal(PyTracebackObject *tb, PyObject *f, long limit,
                 int indent, const char *margin)
{
    PyCodeObject *code = NULL;
    Py_ssize_t depth = 0;
    PyObject *last_file = NULL;
    int last_line = -1;
    PyObject *last_name = NULL;
    long cnt = 0;
    PyTracebackObject *tb1 = tb;
    while (tb1 != NULL) {
        depth++;
        tb1 = tb1->tb_next;
    }
    while (tb != NULL && depth > limit) {
        depth--;
        tb = tb->tb_next;
    }
    while (tb != NULL) {
        code = PyFrame_GetCode(tb->tb_frame);
        if (last_file == NULL ||
            code->co_filename != last_file ||
            last_line == -1 || tb->tb_lineno != last_line ||
            last_name == NULL || code->co_name != last_name) {
            if (cnt > TB_RECURSIVE_CUTOFF) {
                if (tb_print_line_repeated(f, cnt) < 0) {
                    goto error;
                }
            }
            last_file = code->co_filename;
            last_line = tb->tb_lineno;
            last_name = code->co_name;
            cnt = 0;
        }
        cnt++;
        if (cnt <= TB_RECURSIVE_CUTOFF) {
            if (tb_displayline(tb, f, code->co_filename, tb->tb_lineno,
                               tb->tb_frame, code->co_name, indent, margin) < 0) {
                goto error;
            }

            if (PyErr_CheckSignals() < 0) {
                goto error;
            }
        }
        Py_CLEAR(code);
        tb = tb->tb_next;
    }
    if (cnt > TB_RECURSIVE_CUTOFF) {
        if (tb_print_line_repeated(f, cnt) < 0) {
            goto error;
        }
    }
    return 0;
error:
    Py_XDECREF(code);
    return -1;
}

#define PyTraceBack_LIMIT 1000

int
_PyTraceBack_Print_Indented(PyObject *v, int indent, const char *margin,
                            const char *header_margin, const char *header, PyObject *f)
{
    PyObject *limitv;
    long limit = PyTraceBack_LIMIT;

    if (v == NULL) {
        return 0;
    }
    if (!PyTraceBack_Check(v)) {
        PyErr_BadInternalCall();
        return -1;
    }
    limitv = PySys_GetObject("tracebacklimit");
    if (limitv && PyLong_Check(limitv)) {
        int overflow;
        limit = PyLong_AsLongAndOverflow(limitv, &overflow);
        if (overflow > 0) {
            limit = LONG_MAX;
        }
        else if (limit <= 0) {
            return 0;
        }
    }
    if (_Py_WriteIndentedMargin(indent, header_margin, f) < 0) {
        return -1;
    }

    if (PyFile_WriteString(header, f) < 0) {
        return -1;
    }

    if (tb_printinternal((PyTracebackObject *)v, f, limit, indent, margin) < 0) {
        return -1;
    }

    return 0;
}

int
PyTraceBack_Print(PyObject *v, PyObject *f)
{
    int indent = 0;
    const char *margin = NULL;
    const char *header_margin = NULL;
    const char *header = EXCEPTION_TB_HEADER;

    return _PyTraceBack_Print_Indented(v, indent, margin, header_margin, header, f);
}

/* Format an integer in range [0; 0xffffffff] to decimal and write it
   into the file fd.

   This function is signal safe. */

void
_Py_DumpDecimal(int fd, size_t value)
{
    /* maximum number of characters required for output of %lld or %p.
       We need at most ceil(log10(256)*SIZEOF_LONG_LONG) digits,
       plus 1 for the null byte.  53/22 is an upper bound for log10(256). */
    char buffer[1 + (sizeof(size_t)*53-1) / 22 + 1];
    char *ptr, *end;

    end = &buffer[Py_ARRAY_LENGTH(buffer) - 1];
    ptr = end;
    *ptr = '\0';
    do {
        --ptr;
        assert(ptr >= buffer);
        *ptr = '0' + (value % 10);
        value /= 10;
    } while (value);

    (void)_Py_write_noraise(fd, ptr, end - ptr);
}

/* Format an integer as hexadecimal with width digits into fd file descriptor.
   The function is signal safe. */
void
_Py_DumpHexadecimal(int fd, uintptr_t value, Py_ssize_t width)
{
    char buffer[sizeof(uintptr_t) * 2 + 1], *ptr, *end;
    const Py_ssize_t size = Py_ARRAY_LENGTH(buffer) - 1;

    if (width > size)
        width = size;
    /* it's ok if width is negative */

    end = &buffer[size];
    ptr = end;
    *ptr = '\0';
    do {
        --ptr;
        assert(ptr >= buffer);
        *ptr = Py_hexdigits[value & 15];
        value >>= 4;
    } while ((end - ptr) < width || value);

    (void)_Py_write_noraise(fd, ptr, end - ptr);
}

void
_Py_DumpASCII(int fd, PyObject *text)
{
    PyASCIIObject *ascii = _PyASCIIObject_CAST(text);
    Py_ssize_t i, size;
    int truncated;
    int kind;
    void *data = NULL;
    Py_UCS4 ch;

    if (!PyUnicode_Check(text))
        return;

    size = ascii->length;
    kind = ascii->state.kind;
    if (ascii->state.compact) {
        if (ascii->state.ascii)
            data = ascii + 1;
        else
            data = _PyCompactUnicodeObject_CAST(text) + 1;
    }
    else {
        data = _PyUnicodeObject_CAST(text)->data.any;
        if (data == NULL)
            return;
    }

    if (MAX_STRING_LENGTH < size) {
        size = MAX_STRING_LENGTH;
        truncated = 1;
    }
    else {
        truncated = 0;
    }

    // Is an ASCII string?
    if (ascii->state.ascii) {
        assert(kind == PyUnicode_1BYTE_KIND);
        char *str = data;

        int need_escape = 0;
        for (i=0; i < size; i++) {
            ch = str[i];
            if (!(' ' <= ch && ch <= 126)) {
                need_escape = 1;
                break;
            }
        }
        if (!need_escape) {
            // The string can be written with a single write() syscall
            (void)_Py_write_noraise(fd, str, size);
            goto done;
        }
    }

    for (i=0; i < size; i++) {
        ch = PyUnicode_READ(kind, data, i);
        if (' ' <= ch && ch <= 126) {
            /* printable ASCII character */
            char c = (char)ch;
            (void)_Py_write_noraise(fd, &c, 1);
        }
        else if (ch <= 0xff) {
            PUTS(fd, "\\x");
            _Py_DumpHexadecimal(fd, ch, 2);
        }
        else if (ch <= 0xffff) {
            PUTS(fd, "\\u");
            _Py_DumpHexadecimal(fd, ch, 4);
        }
        else {
            PUTS(fd, "\\U");
            _Py_DumpHexadecimal(fd, ch, 8);
        }
    }

done:
    if (truncated) {
        PUTS(fd, "...");
    }
}

/* Write a frame into the file fd: "File "xxx", line xxx in xxx".

   This function is signal safe. */

static void
dump_frame(int fd, _PyInterpreterFrame *frame)
{
    PyCodeObject *code =_PyFrame_GetCode(frame);
    PUTS(fd, "  File ");
    if (code->co_filename != NULL
        && PyUnicode_Check(code->co_filename))
    {
        PUTS(fd, "\"");
        _Py_DumpASCII(fd, code->co_filename);
        PUTS(fd, "\"");
    } else {
        PUTS(fd, "???");
    }

    int lineno = PyUnstable_InterpreterFrame_GetLine(frame);
    PUTS(fd, ", line ");
    if (lineno >= 0) {
        _Py_DumpDecimal(fd, (size_t)lineno);
    }
    else {
        PUTS(fd, "???");
    }
    PUTS(fd, " in ");

    if (code->co_name != NULL
       && PyUnicode_Check(code->co_name)) {
        _Py_DumpASCII(fd, code->co_name);
    }
    else {
        PUTS(fd, "???");
    }

    PUTS(fd, "\n");
}

static void
dump_traceback(int fd, PyThreadState *tstate, int write_header)
{
    _PyInterpreterFrame *frame;
    unsigned int depth;

    if (write_header) {
        PUTS(fd, "Stack (most recent call first):\n");
    }

    frame = tstate->current_frame;
    if (frame == NULL) {
        PUTS(fd, "  <no Python frame>\n");
        return;
    }

    depth = 0;
    while (1) {
        if (MAX_FRAME_DEPTH <= depth) {
            PUTS(fd, "  ...\n");
            break;
        }
        dump_frame(fd, frame);
        frame = frame->previous;
        if (frame == NULL) {
            break;
        }
        if (frame->owner == FRAME_OWNED_BY_CSTACK) {
            /* Trampoline frame */
            frame = frame->previous;
        }
        if (frame == NULL) {
            break;
        }
        /* Can't have more than one shim frame in a row */
        assert(frame->owner != FRAME_OWNED_BY_CSTACK);
        depth++;
    }
}

/* Dump the traceback of a Python thread into fd. Use write() to write the
   traceback and retry if write() is interrupted by a signal (failed with
   EINTR), but don't call the Python signal handler.

   The caller is responsible to call PyErr_CheckSignals() to call Python signal
   handlers if signals were received. */
void
_Py_DumpTraceback(int fd, PyThreadState *tstate)
{
    dump_traceback(fd, tstate, 1);
}

/* Write the thread identifier into the file 'fd': "Current thread 0xHHHH:\" if
   is_current is true, "Thread 0xHHHH:\n" otherwise.

   This function is signal safe. */

static void
write_thread_id(int fd, PyThreadState *tstate, int is_current)
{
    if (is_current)
        PUTS(fd, "Current thread 0x");
    else
        PUTS(fd, "Thread 0x");
    _Py_DumpHexadecimal(fd,
                        tstate->thread_id,
                        sizeof(unsigned long) * 2);
    PUTS(fd, " (most recent call first):\n");
}

/* Dump the traceback of all Python threads into fd. Use write() to write the
   traceback and retry if write() is interrupted by a signal (failed with
   EINTR), but don't call the Python signal handler.

   The caller is responsible to call PyErr_CheckSignals() to call Python signal
   handlers if signals were received. */
const char*
_Py_DumpTracebackThreads(int fd, PyInterpreterState *interp,
                         PyThreadState *current_tstate)
{
    PyThreadState *tstate;
    unsigned int nthreads;

    if (current_tstate == NULL) {
        /* _Py_DumpTracebackThreads() is called from signal handlers by
           faulthandler.

           SIGSEGV, SIGFPE, SIGABRT, SIGBUS and SIGILL are synchronous signals
           and are thus delivered to the thread that caused the fault. Get the
           Python thread state of the current thread.

           PyThreadState_Get() doesn't give the state of the thread that caused
           the fault if the thread released the GIL, and so
           _PyThreadState_GET() cannot be used. Read the thread specific
           storage (TSS) instead: call PyGILState_GetThisThreadState(). */
        current_tstate = PyGILState_GetThisThreadState();
    }

    if (interp == NULL) {
        if (current_tstate == NULL) {
            interp = _PyGILState_GetInterpreterStateUnsafe();
            if (interp == NULL) {
                /* We need the interpreter state to get Python threads */
                return "unable to get the interpreter state";
            }
        }
        else {
            interp = current_tstate->interp;
        }
    }
    assert(interp != NULL);

    /* Get the current interpreter from the current thread */
    tstate = PyInterpreterState_ThreadHead(interp);
    if (tstate == NULL)
        return "unable to get the thread head state";

    /* Dump the traceback of each thread */
    tstate = PyInterpreterState_ThreadHead(interp);
    nthreads = 0;
    _Py_BEGIN_SUPPRESS_IPH
    do
    {
        if (nthreads != 0)
            PUTS(fd, "\n");
        if (nthreads >= MAX_NTHREADS) {
            PUTS(fd, "...\n");
            break;
        }
        write_thread_id(fd, tstate, tstate == current_tstate);
        if (tstate == current_tstate && tstate->interp->gc.collecting) {
            PUTS(fd, "  Garbage-collecting\n");
        }
        dump_traceback(fd, tstate, 0);
        tstate = PyThreadState_Next(tstate);
        nthreads++;
    } while (tstate != NULL);
    _Py_END_SUPPRESS_IPH

    return NULL;
}
