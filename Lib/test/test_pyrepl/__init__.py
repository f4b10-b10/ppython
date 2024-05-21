import os
from test.support import requires, load_package_tests
from test.support.import_helper import import_module

# Optionally test pyrepl.  This currently requires that the
# 'curses' resource be given on the regrtest command line using the -u
# option.  Additionally, we need to attempt to import curses and readline.
requires("curses")
curses = import_module("curses")
readline = import_module("readline")


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
