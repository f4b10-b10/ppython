/*
 * C extensions module to test importing multiple modules from one compiled
 * file (issue16421). This file defines 3 modules (_testimportmodule,
 * foo, bar), only the first one is called the same as the compiled file.
 */
#include<Python.h>

static struct PyModuleDef _testimportmultiple = {
    PyModuleDef_HEAD_INIT,
    "_testimportmultiple",
    "_testimportmultiple doc",
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit__testimportmultiple()
{
    return PyModule_Create(&_testimportmultiple);
}

static struct PyModuleDef _foomodule = {
    PyModuleDef_HEAD_INIT,
    "foo",
    "foo doc",
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_foo()
{
    return PyModule_Create(&_foomodule);
}

static struct PyModuleDef _barmodule = {
    PyModuleDef_HEAD_INIT,
    "bar",
    "bar doc",
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_bar(){
    return PyModule_Create(&_barmodule);
}

