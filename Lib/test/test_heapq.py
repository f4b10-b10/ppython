"""Unittests for heapq."""

import random
import unittest
import doctest

from test import support
from unittest import TestCase, skipUnless
from operator import itemgetter
from itertools import chain, product

py_heapq = support.import_fresh_module('heapq', blocked=['_heapq'])
c_heapq = support.import_fresh_module('heapq', fresh=['_heapq'])

# _heapq.nlargest/nsmallest are saved in heapq._nlargest/_smallest when
# _heapq is imported, so check them there
func_names = ['heapify', 'heappop', 'heappush', 'heappushpop', 'heapreplace',
              '_heappop_max', '_heapreplace_max', '_heapify_max', 'merge']

class TestModules(TestCase):
    def test_py_functions(self):
        for fname in func_names:
            self.assertEqual(getattr(py_heapq, fname).__module__, 'heapq')

    @skipUnless(c_heapq, 'requires _heapq')
    def test_c_functions(self):
        for fname in func_names:
            self.assertEqual(getattr(c_heapq, fname).__module__, '_heapq')

def load_tests(loader, tests, ignore):
    # The 'merge' function has examples in its docstring which we should test
    # with 'doctest'.
    #
    # However, doctest can't easily find all docstrings in the module (loading
    # it through import_fresh_module seems to confuse it), so we specifically
    # create a finder which returns the doctests from the merge method.

    class HeapqMergeDocTestFinder:
        def find(self, *args, **kwargs):
            dtf = doctest.DocTestFinder()
            return dtf.find(py_heapq.merge)

    tests.addTests(doctest.DocTestSuite(py_heapq,
                                        test_finder=HeapqMergeDocTestFinder()))
    return tests

class TestHeap:

    def test_push_pop(self):
        # 1) Push 256 random numbers and pop them off, verifying all's OK.
        heap = []
        data = []
        self.check_invariant(heap)
        for i in range(256):
            item = random.random()
            data.append(item)
            self.module.heappush(heap, item)
            self.check_invariant(heap)
        results = []
        while heap:
            item = self.module.heappop(heap)
            self.check_invariant(heap)
            results.append(item)
        data_sorted = data[:]
        data_sorted.sort()
        self.assertEqual(data_sorted, results)
        # 2) Check that the invariant holds for a sorted array
        self.check_invariant(results)

        self.assertRaises(TypeError, self.module.heappush, [])
        try:
            self.assertRaises(TypeError, self.module.heappush, None, None)
            self.assertRaises(TypeError, self.module.heappop, None)
        except AttributeError:
            pass

    def check_invariant(self, heap):
        # Check the heap invariant.
        for pos, item in enumerate(heap):
            if pos: # pos 0 has no parent
                parentpos = (pos-1) >> 1
                self.assertTrue(heap[parentpos] <= item)

    def test_heapify(self):
        for size in list(range(30)) + [20000]:
            heap = [random.random() for dummy in range(size)]
            self.module.heapify(heap)
            self.check_invariant(heap)

        self.assertRaises(TypeError, self.module.heapify, None)

    def test_naive_nbest(self):
        data = [random.randrange(2000) for i in range(1000)]
        heap = []
        for item in data:
            self.module.heappush(heap, item)
            if len(heap) > 10:
                self.module.heappop(heap)
        heap.sort()
        self.assertEqual(heap, sorted(data)[-10:])

    def heapiter(self, heap):
        # An iterator returning a heap's elements, smallest-first.
        try:
            while 1:
                yield self.module.heappop(heap)
        except IndexError:
            pass

    def test_nbest(self):
        # Less-naive "N-best" algorithm, much faster (if len(data) is big
        # enough <wink>) than sorting all of data.  However, if we had a max
        # heap instead of a min heap, it could go faster still via
        # heapify'ing all of data (linear time), then doing 10 heappops
        # (10 log-time steps).
        data = [random.randrange(2000) for i in range(1000)]
        heap = data[:10]
        self.module.heapify(heap)
        for item in data[10:]:
            if item > heap[0]:  # this gets rarer the longer we run
                self.module.heapreplace(heap, item)
        self.assertEqual(list(self.heapiter(heap)), sorted(data)[-10:])

        self.assertRaises(TypeError, self.module.heapreplace, None)
        self.assertRaises(TypeError, self.module.heapreplace, None, None)
        self.assertRaises(IndexError, self.module.heapreplace, [], None)

    def test_nbest_with_pushpop(self):
        data = [random.randrange(2000) for i in range(1000)]
        heap = data[:10]
        self.module.heapify(heap)
        for item in data[10:]:
            self.module.heappushpop(heap, item)
        self.assertEqual(list(self.heapiter(heap)), sorted(data)[-10:])
        self.assertEqual(self.module.heappushpop([], 'x'), 'x')

    def test_heappushpop(self):
        h = []
        x = self.module.heappushpop(h, 10)
        self.assertEqual((h, x), ([], 10))

        h = [10]
        x = self.module.heappushpop(h, 10.0)
        self.assertEqual((h, x), ([10], 10.0))
        self.assertEqual(type(h[0]), int)
        self.assertEqual(type(x), float)

        h = [10];
        x = self.module.heappushpop(h, 9)
        self.assertEqual((h, x), ([10], 9))

        h = [10];
        x = self.module.heappushpop(h, 11)
        self.assertEqual((h, x), ([11], 10))

    def test_heappop_max(self):
        # _heapop_max has an optimization for one-item lists which isn't
        # covered in other tests, so test that case explicitly here
        h = [3, 2]
        self.assertEqual(self.module._heappop_max(h), 3)
        self.assertEqual(self.module._heappop_max(h), 2)

    def test_heapsort(self):
        # Exercise everything with repeated heapsort checks
        for trial in range(100):
            size = random.randrange(50)
            data = [random.randrange(25) for i in range(size)]
            if trial & 1:     # Half of the time, use heapify
                heap = data[:]
                self.module.heapify(heap)
            else:             # The rest of the time, use heappush
                heap = []
                for item in data:
                    self.module.heappush(heap, item)
            heap_sorted = [self.module.heappop(heap) for i in range(size)]
            self.assertEqual(heap_sorted, sorted(data))

    def test_merge(self):
        lengths = range(25)
        keys = [None, itemgetter(0), itemgetter(1), itemgetter(1, 0)]
        reverses = [False, True]
        for n, key, reverse in product(lengths, keys, reverses):
            inputs = [
                sorted(
                    [(random.choice('ABC'), random.randrange(-500, 500))
                     for _ in range(random.randrange(20))],
                    key=key, reverse=reverse
                )
                for _ in range(n)
            ]
            expected = sorted(chain(*inputs), key=key, reverse=reverse)
            with self.subTest(inputs=inputs, key=key, reverse=reverse):
                m = self.module.merge(*inputs, key=key, reverse=reverse)
                self.assertEqual(list(m), expected)
                self.assertEqual(list(m), [])

    def test_empty_merges(self):
        # Merging two empty lists (with or without a key) should produce
        # another empty list.
        self.assertEqual(list(self.module.merge([], [])), [])
        self.assertEqual(list(self.module.merge([], [], key=lambda: 6)), [])

    def test_merge_does_not_suppress_index_error(self):
        # Issue 19018: Heapq.merge suppresses IndexError from user generator
        def iterable():
            s = list(range(10))
            for i in range(20):
                yield s[i]       # IndexError when i > 10
        with self.assertRaises(IndexError):
            list(self.module.merge(iterable(), iterable()))

    def test_merge_stability(self):
        class Int(int):
            pass
        inputs = [[], [], [], []]
        for i in range(20000):
            stream = random.randrange(4)
            x = random.randrange(500)
            obj = Int(x)
            obj.pair = (x, stream)
            inputs[stream].append(obj)
        for stream in inputs:
            stream.sort()
        result = [i.pair for i in self.module.merge(*inputs)]
        self.assertEqual(result, sorted(result))

    def test_nsmallest(self):
        data = [(random.randrange(2000), i) for i in range(1000)]
        for f in (None, lambda x:  x[0] * 547 % 2000):
            for n in (0, 1, 2, 10, 100, 400, 999, 1000, 1100):
                self.assertEqual(list(self.module.nsmallest(n, data)),
                                 sorted(data)[:n])
                self.assertEqual(list(self.module.nsmallest(n, data, key=f)),
                                 sorted(data, key=f)[:n])

    def test_nlargest(self):
        data = [(random.randrange(2000), i) for i in range(1000)]
        for f in (None, lambda x:  x[0] * 547 % 2000):
            for n in (0, 1, 2, 10, 100, 400, 999, 1000, 1100):
                self.assertEqual(list(self.module.nlargest(n, data)),
                                 sorted(data, reverse=True)[:n])
                self.assertEqual(list(self.module.nlargest(n, data, key=f)),
                                 sorted(data, key=f, reverse=True)[:n])

    def test_comparison_operator(self):
        # Issue 3051: Make sure heapq works with both __lt__
        # For python 3.0, __le__ alone is not enough
        def hsort(data, comp):
            data = [comp(x) for x in data]
            self.module.heapify(data)
            return [self.module.heappop(data).x for i in range(len(data))]
        class LT:
            def __init__(self, x):
                self.x = x
            def __lt__(self, other):
                return self.x > other.x
        class LE:
            def __init__(self, x):
                self.x = x
            def __le__(self, other):
                return self.x >= other.x
        data = [random.random() for i in range(100)]
        target = sorted(data, reverse=True)
        self.assertEqual(hsort(data, LT), target)
        self.assertRaises(TypeError, data, LE)


class TestHeapPython(TestHeap, TestCase):
    module = py_heapq


@skipUnless(c_heapq, 'requires _heapq')
class TestHeapC(TestHeap, TestCase):
    module = c_heapq


#==============================================================================

class LenOnly:
    "Dummy sequence class defining __len__ but not __getitem__."
    def __len__(self):
        return 10

class CmpErr:
    "Dummy element that always raises an error during comparison"
    def __eq__(self, other):
        raise ZeroDivisionError
    __ne__ = __lt__ = __le__ = __gt__ = __ge__ = __eq__

def R(seqn):
    'Regular generator'
    for i in seqn:
        yield i

class G:
    'Sequence using __getitem__'
    def __init__(self, seqn):
        self.seqn = seqn
    def __getitem__(self, i):
        return self.seqn[i]

class I:
    'Sequence using iterator protocol'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def __next__(self):
        if self.i >= len(self.seqn): raise StopIteration
        v = self.seqn[self.i]
        self.i += 1
        return v

class Ig:
    'Sequence using iterator protocol defined with a generator'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        for val in self.seqn:
            yield val

class X:
    'Missing __getitem__ and __iter__'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __next__(self):
        if self.i >= len(self.seqn): raise StopIteration
        v = self.seqn[self.i]
        self.i += 1
        return v

class N:
    'Iterator missing __next__()'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self

class E:
    'Test propagation of exceptions'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def __next__(self):
        3 // 0

class S:
    'Test immediate stop'
    def __init__(self, seqn):
        pass
    def __iter__(self):
        return self
    def __next__(self):
        raise StopIteration

def L(seqn):
    'Test multiple tiers of iterators'
    return chain(map(lambda x:x, R(Ig(G(seqn)))))


class SideEffectLT:
    def __init__(self, value, heap):
        self.value = value
        self.heap = heap

    def __lt__(self, other):
        self.heap[:] = []
        return self.value < other.value


class TestErrorHandling:

    def test_non_sequence(self):
        for f in (self.module.heapify, self.module.heappop):
            self.assertRaises((TypeError, AttributeError), f, 10)
        for f in (self.module.heappush, self.module.heapreplace,
                  self.module.nlargest, self.module.nsmallest):
            self.assertRaises((TypeError, AttributeError), f, 10, 10)

    def test_len_only(self):
        for f in (self.module.heapify, self.module.heappop):
            self.assertRaises((TypeError, AttributeError), f, LenOnly())
        for f in (self.module.heappush, self.module.heapreplace):
            self.assertRaises((TypeError, AttributeError), f, LenOnly(), 10)
        for f in (self.module.nlargest, self.module.nsmallest):
            self.assertRaises(TypeError, f, 2, LenOnly())

    def test_cmp_err(self):
        seq = [CmpErr(), CmpErr(), CmpErr()]
        for f in (self.module.heapify, self.module.heappop):
            self.assertRaises(ZeroDivisionError, f, seq)
        for f in (self.module.heappush, self.module.heapreplace):
            self.assertRaises(ZeroDivisionError, f, seq, 10)
        for f in (self.module.nlargest, self.module.nsmallest):
            self.assertRaises(ZeroDivisionError, f, 2, seq)

    def test_arg_parsing(self):
        for f in (self.module.heapify, self.module.heappop,
                  self.module.heappush, self.module.heapreplace,
                  self.module.nlargest, self.module.nsmallest):
            self.assertRaises((TypeError, AttributeError), f, 10)

    def test_iterable_args(self):
        for f in (self.module.nlargest, self.module.nsmallest):
            for s in ("123", "", range(1000), (1, 1.2), range(2000,2200,5)):
                for g in (G, I, Ig, L, R):
                    self.assertEqual(list(f(2, g(s))), list(f(2,s)))
                self.assertEqual(list(f(2, S(s))), [])
                self.assertRaises(TypeError, f, 2, X(s))
                self.assertRaises(TypeError, f, 2, N(s))
                self.assertRaises(ZeroDivisionError, f, 2, E(s))

    # Issue #17278: the heap may change size while it's being walked.

    def test_heappush_mutating_heap(self):
        heap = []
        heap.extend(SideEffectLT(i, heap) for i in range(200))
        # Python version raises IndexError, C version RuntimeError
        with self.assertRaises((IndexError, RuntimeError)):
            self.module.heappush(heap, SideEffectLT(5, heap))

    def test_heappop_mutating_heap(self):
        heap = []
        heap.extend(SideEffectLT(i, heap) for i in range(200))
        # Python version raises IndexError, C version RuntimeError
        with self.assertRaises((IndexError, RuntimeError)):
            self.module.heappop(heap)

    def test_comparison_operator_modifiying_heap(self):
        # See bpo-39421: Strong references need to be taken
        # when comparing objects as they can alter the heap
        class EvilClass(int):
            def __lt__(self, o):
                heap.clear()
                return NotImplemented

        heap = []
        self.module.heappush(heap, EvilClass(0))
        self.assertRaises(IndexError, self.module.heappushpop, heap, 1)

    def test_comparison_operator_modifiying_heap_two_heaps(self):

        class h(int):
            def __lt__(self, o):
                list2.clear()
                return NotImplemented

        class g(int):
            def __lt__(self, o):
                list1.clear()
                return NotImplemented

        list1, list2 = [], []

        self.module.heappush(list1, h(0))
        self.module.heappush(list2, g(0))

        self.assertRaises((IndexError, RuntimeError), self.module.heappush, list1, g(1))
        self.assertRaises((IndexError, RuntimeError), self.module.heappush, list2, h(1))

    def test_merge_err(self):
        nexterr_immediate = E
        def nexterr_delayed():
            yield from range(10)
            raise ZeroDivisionError
        merge = self.module.merge
        for key in [None, lambda x:x]:
            for reverse in [False, True]:
                # test comparison failure during initialization
                args = [[CmpErr()]] + [range(100)]*10
                mo = merge(*args, key=key, reverse=reverse)
                self.assertRaises(ZeroDivisionError, list, mo)

                for n in range(2,10):
                    # test comparison failure during intermediate steps
                    args = [[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, CmpErr()]]*n
                    mo = merge(*args, key=key, reverse=reverse)
                    self.assertRaises(ZeroDivisionError, list, mo)
                for n in range(1, 10):
                    # test __next__ error during initialization
                    args = [nexterr_immediate(range(10)) for _ in range(n)]
                    mo = merge(*args, key=key, reverse=reverse)
                    self.assertRaises(ZeroDivisionError, list, mo)
                for n in range(1, 10):
                    # test __next__ error during intermediate steps
                    args = [nexterr_delayed() for _ in range(n)]
                    mo = merge(*args, key=key, reverse=reverse)
                    self.assertRaises(ZeroDivisionError, list, mo)

        for reverse in [False, True]:
            # test error during key computation
            mo = merge(range(10), range(10), key=lambda x: x // 0, reverse=reverse)
            self.assertRaises(ZeroDivisionError, list, mo)
            mo = merge(range(10), key=object(), reverse=reverse)
            self.assertRaises(TypeError, list, merge)

class TestErrorHandlingPython(TestErrorHandling, TestCase):
    module = py_heapq

@skipUnless(c_heapq, 'requires _heapq')
class TestErrorHandlingC(TestErrorHandling, TestCase):
    module = c_heapq


if __name__ == "__main__":
    unittest.main()
