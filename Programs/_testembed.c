#include <Python.h>
#include <stdio.h>

/*********************************************************
 * Embedded interpreter tests that need a custom exe
 *
 * Executed via 'EmbeddingTests' in Lib/test/test_capi.py
 *********************************************************/

static void _testembed_Py_Initialize(void)
{
    /* HACK: the "./" at front avoids a search along the PATH in
       Modules/getpath.c */
    Py_SetProgramName(L"./_testembed");
    Py_Initialize();
}


/*****************************************************
 * Test repeated initialisation and subinterpreters
 *****************************************************/

static void print_subinterp(void)
{
    /* Just output some debug stuff */
    PyThreadState *ts = PyThreadState_Get();
    printf("interp %p, thread state %p: ", ts->interp, ts);
    fflush(stdout);
    PyRun_SimpleString(
        "import sys;"
        "print('id(modules) =', id(sys.modules));"
        "sys.stdout.flush()"
    );
}

static int test_repeated_init_and_subinterpreters(void)
{
    PyThreadState *mainstate, *substate;
#ifdef WITH_THREAD
    PyGILState_STATE gilstate;
#endif
    int i, j;

    for (i=0; i<15; i++) {
        printf("--- Pass %d ---\n", i);
        _testembed_Py_Initialize();
        mainstate = PyThreadState_Get();

#ifdef WITH_THREAD
        PyEval_InitThreads();
        PyEval_ReleaseThread(mainstate);

        gilstate = PyGILState_Ensure();
#endif
        print_subinterp();
        PyThreadState_Swap(NULL);

        for (j=0; j<3; j++) {
            substate = Py_NewInterpreter();
            print_subinterp();
            Py_EndInterpreter(substate);
        }

        PyThreadState_Swap(mainstate);
        print_subinterp();
#ifdef WITH_THREAD
        PyGILState_Release(gilstate);
#endif

        PyEval_RestoreThread(mainstate);
        Py_Finalize();
    }
    return 0;
}

/*****************************************************
 * Test forcing a particular IO encoding
 *****************************************************/

static void check_stdio_details(const char *encoding, const char * errors)
{
    /* Output info for the test case to check */
    if (encoding) {
        printf("Expected encoding: %s\n", encoding);
    } else {
        printf("Expected encoding: default\n");
    }
    if (errors) {
        printf("Expected errors: %s\n", errors);
    } else {
        printf("Expected errors: default\n");
    }
    fflush(stdout);
    /* Force the given IO encoding */
    Py_SetStandardStreamEncoding(encoding, errors);
    _testembed_Py_Initialize();
    PyRun_SimpleString(
        "import sys;"
        "print('stdin: {0.encoding}:{0.errors}'.format(sys.stdin));"
        "print('stdout: {0.encoding}:{0.errors}'.format(sys.stdout));"
        "print('stderr: {0.encoding}:{0.errors}'.format(sys.stderr));"
        "sys.stdout.flush()"
    );
    Py_Finalize();
}

static int test_forced_io_encoding(void)
{
    /* Ensure consistent "defaults" */
    printf("Setting PYTHONIOENCODING=UTF-8:surrogateescape\n");
    setenv("PYTHONIOENCODING", "UTF-8:surrogateescape", 1);
    /* Check various combinations */
    printf("--- Use defaults ---\n");
    check_stdio_details(NULL, NULL);
    printf("--- Set errors only ---\n");
    check_stdio_details(NULL, "ignore");
    printf("--- Set encoding only ---\n");
    check_stdio_details("latin-1", NULL);
    printf("--- Set encoding and errors ---\n");
    check_stdio_details("latin-1", "replace");

    /* Check calling after initialization fails */
    Py_Initialize();

    if (Py_SetStandardStreamEncoding(NULL, NULL) == 0) {
        printf("Unexpected success calling Py_SetStandardStreamEncoding");
    }
    Py_Finalize();
    return 0;
}

/* *********************************************************
 * List of test cases and the function that implements it.
 * 
 * Names are compared case-sensitively with the first
 * argument. If no match is found, or no first argument was
 * provided, the names of all test cases are printed and
 * the exit code will be -1.
 *
 * The int returned from test functions is used as the exit
 * code, and test_capi treats all non-zero exit codes as a
 * failed test. 
 *********************************************************/
struct TestCase
{
    const char *name;
    int (*func)(void);
};

static struct TestCase TestCases[] = {
    { "forced_io_encoding", test_forced_io_encoding },
    { "repeated_init_and_subinterpreters", test_repeated_init_and_subinterpreters },
    { NULL, NULL }
};

int main(int argc, char *argv[])
{
    if (argc > 1) {
        for (struct TestCase *tc = TestCases; tc && tc->name; tc++) {
            if (strcmp(argv[1], tc->name) == 0)
                return (*tc->func)();
        }
    }

    /* No match found, or no test name provided, so display usage */
    printf("Python " PY_VERSION " _testembed executable for embedded interpreter tests\n"
           "Normally executed via 'EmbeddingTests' in Lib/test/test_capi.py\n\n"
           "Usage: %s TESTNAME\n\nAll available tests:\n", argv[0]);
    for (struct TestCase *tc = TestCases; tc && tc->name; tc++) {
        printf("  %s\n", tc->name);
    }

    /* Non-zero exit code will cause test_capi.py tests to fail.
       This is intentional. */
    return -1;
}
