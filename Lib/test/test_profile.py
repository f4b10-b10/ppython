"""Test suite for the profile module."""

import sys
import pstats
import unittest
import os
from difflib import unified_diff
from io import StringIO
from test.support import TESTFN, run_unittest, unlink
from contextlib import contextmanager

import profile
from test import support
from test.profilee import testfunc, timer
from test.support import script_helper

TEMPDIR = os.path.abspath(support.TESTFN) + '-profiledir'
outputdir = TEMPDIR + '-output-test'
outputname = os.path.join(outputdir, 'output_stats')


class ProfileTest(unittest.TestCase):

    profilerclass = profile.Profile
    profilermodule = profile
    methodnames = ['print_stats', 'print_callers', 'print_callees']
    expected_max_output = ':0(max)'

    def tearDown(self):
        unlink(TESTFN)

    def get_expected_output(self):
        return _ProfileOutput

    @classmethod
    def do_profiling(cls):
        results = []
        prof = cls.profilerclass(timer, 0.001)
        start_timer = timer()
        prof.runctx("testfunc()", globals(), locals())
        results.append(timer() - start_timer)
        for methodname in cls.methodnames:
            s = StringIO()
            stats = pstats.Stats(prof, stream=s)
            stats.strip_dirs().sort_stats("stdname")
            getattr(stats, methodname)()
            output = s.getvalue().splitlines()
            mod_name = testfunc.__module__.rsplit('.', 1)[1]
            # Only compare against stats originating from the test file.
            # Prevents outside code (e.g., the io module) from causing
            # unexpected output.
            output = [line.rstrip() for line in output if mod_name in line]
            results.append('\n'.join(output))
        return results

    def test_cprofile(self):
        results = self.do_profiling()
        expected = self.get_expected_output()
        self.assertEqual(results[0], 1000)
        for i, method in enumerate(self.methodnames):
            if results[i+1] != expected[method]:
                print("Stats.%s output for %s doesn't fit expectation!" %
                      (method, self.profilerclass.__name__))
                print('\n'.join(unified_diff(
                                  results[i+1].split('\n'),
                                  expected[method].split('\n'))))

    def test_calling_conventions(self):
        # Issue #5330: profile and cProfile wouldn't report C functions called
        # with keyword arguments. We test all calling conventions.
        stmts = [
            "max([0])",
            "max([0], key=int)",
            "max([0], **dict(key=int))",
            "max(*([0],))",
            "max(*([0],), key=int)",
            "max(*([0],), **dict(key=int))",
        ]
        for stmt in stmts:
            s = StringIO()
            prof = self.profilerclass(timer, 0.001)
            prof.runctx(stmt, globals(), locals())
            stats = pstats.Stats(prof, stream=s)
            stats.print_stats()
            res = s.getvalue()
            self.assertIn(self.expected_max_output, res,
                "Profiling {0!r} didn't report max:\n{1}".format(stmt, res))

    def test_run(self):
        with silent():
            self.profilermodule.run("int('1')")
        self.profilermodule.run("int('1')", filename=TESTFN)
        self.assertTrue(os.path.exists(TESTFN))

    def test_runctx(self):
        with silent():
            self.profilermodule.runctx("testfunc()", globals(), locals())
        self.profilermodule.runctx("testfunc()", globals(), locals(),
                                  filename=TESTFN)
        self.assertTrue(os.path.exists(TESTFN))


class ProfileCommandLineTest(unittest.TestCase):

    profilename = 'profile'

    def profilecmd(self, *args, **kwargs):
        rc, out, err = script_helper.assert_python_ok('-m', self.profilename,
                                                      *args, **kwargs)
        return out.replace(os.linesep.encode(), b'\n')

    def profilecmd_failure(self, *args):
        return script_helper.assert_python_failure('-m', self.profilename,
                                                   *args)

    def test_outputfile(self):
        for opt in '-o', '--outfile':
            try:
                scriptfile = support.findfile('profilee.py')
                with support.temp_cwd(outputdir):
                    out = self.profilecmd(opt, outputname, scriptfile)
                self.assertEqual(out, b'')
            finally:
                support.rmtree(outputdir)

    def test_sort_by_call_count(self):
        for opt in '-s', '--sort':
            scriptfile = support.findfile('profilee.py')
            out = self.profilecmd(opt, 'ncalls', scriptfile)
            self.assertIn(b'Ordered by: call count', out)

    def test_sort_by_total_time(self):
        for opt in '-s', '--sort':
            scriptfile = support.findfile('profilee.py')
            for arg in 'time', 'tottime':
                out = self.profilecmd(opt, arg, scriptfile)
                self.assertIn(b'Ordered by: internal time', out)

    def test_sort_by_cumulative_time(self):
        for opt in '-s', '--sort':
            scriptfile = support.findfile('profilee.py')
            for arg in 'cumtime', 'cumulative':
                out = self.profilecmd(opt, arg, scriptfile)
                self.assertIn(b'Ordered by: cumulative time', out)

    def test_sort_by_filename(self):
        for opt in '-s', '--sort':
            scriptfile = support.findfile('profilee.py')
            for arg in 'file', 'filename', 'module':
                out = self.profilecmd(opt, 'filename', scriptfile)
                self.assertIn(b'Ordered by: file name', out)

    def test_sort_by_primitive_call_count_time(self):
        for opt in '-s', '--sort':
            scriptfile = support.findfile('profilee.py')
            out = self.profilecmd(opt, 'pcalls', scriptfile)
            self.assertIn(b'Ordered by: primitive call count', out)

    def test_sort_by_line_number(self):
        for opt in '-s', '--sort':
            scriptfile = support.findfile('profilee.py')
            out = self.profilecmd(opt, 'line', scriptfile)
            self.assertIn(b'Ordered by: line number', out)

    def test_sort_by_standard_name(self):
        for opt in '-s', '--sort':
            scriptfile = support.findfile('profilee.py')
            out = self.profilecmd(opt, 'stdname', scriptfile)
            self.assertIn(b'Ordered by: standard name', out)


def regenerate_expected_output(filename, cls):
    filename = filename.rstrip('co')
    print('Regenerating %s...' % filename)
    results = cls.do_profiling()

    newfile = []
    with open(filename, 'r') as f:
        for line in f:
            newfile.append(line)
            if line.startswith('#--cut'):
                break

    with open(filename, 'w') as f:
        f.writelines(newfile)
        f.write("_ProfileOutput = {}\n")
        for i, method in enumerate(cls.methodnames):
            f.write('_ProfileOutput[%r] = """\\\n%s"""\n' % (
                    method, results[i+1]))
        f.write('\nif __name__ == "__main__":\n    main()\n')

@contextmanager
def silent():
    stdout = sys.stdout
    try:
        sys.stdout = StringIO()
        yield
    finally:
        sys.stdout = stdout

def test_main():
    run_unittest(ProfileTest)
    run_unittest(ProfileCommandLineTest)

def main():
    if '-r' not in sys.argv:
        test_main()
    else:
        regenerate_expected_output(__file__, ProfileTest)


# Don't remove this comment. Everything below it is auto-generated.
#--cut--------------------------------------------------------------------------
_ProfileOutput = {}
_ProfileOutput['print_stats'] = """\
       28   27.972    0.999   27.972    0.999 profilee.py:110(__getattr__)
        1  269.996  269.996  999.769  999.769 profilee.py:25(testfunc)
     23/3  149.937    6.519  169.917   56.639 profilee.py:35(factorial)
       20   19.980    0.999   19.980    0.999 profilee.py:48(mul)
        2   39.986   19.993  599.830  299.915 profilee.py:55(helper)
        4  115.984   28.996  119.964   29.991 profilee.py:73(helper1)
        2   -0.006   -0.003  139.946   69.973 profilee.py:84(helper2_indirect)
        8  311.976   38.997  399.912   49.989 profilee.py:88(helper2)
        8   63.976    7.997   79.960    9.995 profilee.py:98(subhelper)"""
_ProfileOutput['print_callers'] = """\
:0(append)                        <- profilee.py:73(helper1)(4)  119.964
:0(exc_info)                      <- profilee.py:73(helper1)(4)  119.964
:0(hasattr)                       <- profilee.py:73(helper1)(4)  119.964
                                     profilee.py:88(helper2)(8)  399.912
profilee.py:110(__getattr__)      <- :0(hasattr)(12)   11.964
                                     profilee.py:98(subhelper)(16)   79.960
profilee.py:25(testfunc)          <- <string>:1(<module>)(1)  999.767
profilee.py:35(factorial)         <- profilee.py:25(testfunc)(1)  999.769
                                     profilee.py:35(factorial)(20)  169.917
                                     profilee.py:84(helper2_indirect)(2)  139.946
profilee.py:48(mul)               <- profilee.py:35(factorial)(20)  169.917
profilee.py:55(helper)            <- profilee.py:25(testfunc)(2)  999.769
profilee.py:73(helper1)           <- profilee.py:55(helper)(4)  599.830
profilee.py:84(helper2_indirect)  <- profilee.py:55(helper)(2)  599.830
profilee.py:88(helper2)           <- profilee.py:55(helper)(6)  599.830
                                     profilee.py:84(helper2_indirect)(2)  139.946
profilee.py:98(subhelper)         <- profilee.py:88(helper2)(8)  399.912"""
_ProfileOutput['print_callees'] = """\
:0(hasattr)                       -> profilee.py:110(__getattr__)(12)   27.972
<string>:1(<module>)              -> profilee.py:25(testfunc)(1)  999.769
profilee.py:110(__getattr__)      ->
profilee.py:25(testfunc)          -> profilee.py:35(factorial)(1)  169.917
                                     profilee.py:55(helper)(2)  599.830
profilee.py:35(factorial)         -> profilee.py:35(factorial)(20)  169.917
                                     profilee.py:48(mul)(20)   19.980
profilee.py:48(mul)               ->
profilee.py:55(helper)            -> profilee.py:73(helper1)(4)  119.964
                                     profilee.py:84(helper2_indirect)(2)  139.946
                                     profilee.py:88(helper2)(6)  399.912
profilee.py:73(helper1)           -> :0(append)(4)   -0.004
profilee.py:84(helper2_indirect)  -> profilee.py:35(factorial)(2)  169.917
                                     profilee.py:88(helper2)(2)  399.912
profilee.py:88(helper2)           -> :0(hasattr)(8)   11.964
                                     profilee.py:98(subhelper)(8)   79.960
profilee.py:98(subhelper)         -> profilee.py:110(__getattr__)(16)   27.972"""

if __name__ == "__main__":
    main()
