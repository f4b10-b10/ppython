import copy
import os
import re
import time
import unittest


class StructSeqTest(unittest.TestCase):

    def test_tuple(self):
        t = time.gmtime()
        self.assertIsInstance(t, tuple)
        astuple = tuple(t)
        self.assertEqual(len(t), len(astuple))
        self.assertEqual(t, astuple)

        # Check that slicing works the same way; at one point, slicing t[i:j] with
        # 0 < i < j could produce NULLs in the result.
        for i in range(-len(t), len(t)):
            self.assertEqual(t[i:], astuple[i:])
            for j in range(-len(t), len(t)):
                self.assertEqual(t[i:j], astuple[i:j])

        for j in range(-len(t), len(t)):
            self.assertEqual(t[:j], astuple[:j])

        self.assertRaises(IndexError, t.__getitem__, -len(t)-1)
        self.assertRaises(IndexError, t.__getitem__, len(t))
        for i in range(-len(t), len(t)-1):
            self.assertEqual(t[i], astuple[i])

    def test_repr(self):
        t = time.gmtime()
        self.assertTrue(repr(t))
        t = time.gmtime(0)
        self.assertEqual(repr(t),
            "time.struct_time(tm_year=1970, tm_mon=1, tm_mday=1, tm_hour=0, "
            "tm_min=0, tm_sec=0, tm_wday=3, tm_yday=1, tm_isdst=0)")
        # os.stat() gives a complicated struct sequence.
        st = os.stat(__file__)
        rep = repr(st)
        self.assertTrue(rep.startswith("os.stat_result"))
        self.assertIn("st_mode=", rep)
        self.assertIn("st_ino=", rep)
        self.assertIn("st_dev=", rep)

    def test_concat(self):
        t1 = time.gmtime()
        t2 = t1 + tuple(t1)
        for i in range(len(t1)):
            self.assertEqual(t2[i], t2[i+len(t1)])

    def test_repeat(self):
        t1 = time.gmtime()
        t2 = 3 * t1
        for i in range(len(t1)):
            self.assertEqual(t2[i], t2[i+len(t1)])
            self.assertEqual(t2[i], t2[i+2*len(t1)])

    def test_contains(self):
        t1 = time.gmtime()
        for item in t1:
            self.assertIn(item, t1)
        self.assertNotIn(-42, t1)

    def test_hash(self):
        t1 = time.gmtime()
        self.assertEqual(hash(t1), hash(tuple(t1)))

    def test_cmp(self):
        t1 = time.gmtime()
        t2 = type(t1)(t1)
        self.assertEqual(t1, t2)
        self.assertTrue(not (t1 < t2))
        self.assertTrue(t1 <= t2)
        self.assertTrue(not (t1 > t2))
        self.assertTrue(t1 >= t2)
        self.assertTrue(not (t1 != t2))

    def test_fields(self):
        t = time.gmtime()
        self.assertEqual(len(t), t.n_sequence_fields)
        self.assertEqual(t.n_unnamed_fields, 0)
        self.assertEqual(t.n_fields, time._STRUCT_TM_ITEMS)

    def test_constructor(self):
        t = time.struct_time

        self.assertRaises(TypeError, t)
        self.assertRaises(TypeError, t, None)
        self.assertRaises(TypeError, t, "123")
        self.assertRaises(TypeError, t, "123", dict={})
        self.assertRaises(TypeError, t, "123456789", dict=None)

        s = "123456789"
        self.assertEqual("".join(t(s)), s)

    def test_eviltuple(self):
        class Exc(Exception):
            pass

        # Devious code could crash structseqs' constructors
        class C:
            def __getitem__(self, i):
                raise Exc
            def __len__(self):
                return 9

        self.assertRaises(Exc, time.struct_time, C())

    def test_reduce(self):
        t = time.gmtime()
        x = t.__reduce__()

    def test_extended_getslice(self):
        # Test extended slicing by comparing with list slicing.
        t = time.gmtime()
        L = list(t)
        indices = (0, None, 1, 3, 19, 300, -1, -2, -31, -300)
        for start in indices:
            for stop in indices:
                # Skip step 0 (invalid)
                for step in indices[1:]:
                    self.assertEqual(list(t[start:stop:step]),
                                     L[start:stop:step])

    def test_match_args(self):
        expected_args = ('tm_year', 'tm_mon', 'tm_mday', 'tm_hour', 'tm_min',
                         'tm_sec', 'tm_wday', 'tm_yday', 'tm_isdst')
        self.assertEqual(time.struct_time.__match_args__, expected_args)

    def test_match_args_with_unnamed_fields(self):
        expected_args = ('st_mode', 'st_ino', 'st_dev', 'st_nlink', 'st_uid',
                         'st_gid', 'st_size')
        self.assertEqual(os.stat_result.n_unnamed_fields, 3)
        self.assertEqual(os.stat_result.__match_args__, expected_args)

    def test_copy_replace_all_fields_visible(self):
        assert (
            os.times_result.n_unnamed_fields == 0
            and os.times_result.n_sequence_fields == os.times_result.n_fields
        )

        t = os.times()

        # visible fields
        self.assertEqual(copy.replace(t), t)
        self.assertIsInstance(copy.replace(t), os.times_result)
        self.assertEqual(copy.replace(t, user=1.5), (1.5, *t[1:]))
        self.assertEqual(copy.replace(t, system=2.5), (t[0], 2.5, *t[2:]))
        self.assertEqual(copy.replace(t, user=1.5, system=2.5), (1.5, 2.5, *t[2:]))

        # unknown fields
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, error=-1)
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, user=1, error=-1)

    def test_copy_replace_with_invisible_fields(self):
        assert (
            time.struct_time.n_unnamed_fields == 0
            and time.struct_time.n_sequence_fields < time.struct_time.n_fields
        )

        t = time.gmtime(0)

        # visible fields
        t2 = copy.replace(t)
        self.assertEqual(t2, (1970, 1, 1, 0, 0, 0, 3, 1, 0))
        self.assertIsInstance(t2, time.struct_time)
        t3 = copy.replace(t, tm_year=2000)
        self.assertEqual(t3, (2000, 1, 1, 0, 0, 0, 3, 1, 0))
        self.assertEqual(t3.tm_year, 2000)
        t4 = copy.replace(t, tm_mon=2)
        self.assertEqual(t4, (1970, 2, 1, 0, 0, 0, 3, 1, 0))
        self.assertEqual(t4.tm_mon, 2)
        t5 = copy.replace(t, tm_year=2000, tm_mon=2)
        self.assertEqual(t5, (2000, 2, 1, 0, 0, 0, 3, 1, 0))
        self.assertEqual(t5.tm_year, 2000)
        self.assertEqual(t5.tm_mon, 2)

        # named invisible fields
        self.assertTrue(hasattr(t, 'tm_zone'), f"{t} has no attribute 'tm_zone'")
        with self.assertRaisesRegex(AttributeError, 'readonly attribute'):
            t.tm_zone = 'some other zone'
        self.assertEqual(t2.tm_zone, t.tm_zone)
        self.assertEqual(t3.tm_zone, t.tm_zone)
        self.assertEqual(t4.tm_zone, t.tm_zone)
        t6 = copy.replace(t, tm_zone='some other zone')
        self.assertEqual(t, t6)
        self.assertEqual(t6.tm_zone, 'some other zone')
        t7 = copy.replace(t, tm_year=2000, tm_zone='some other zone')
        self.assertEqual(t7, (2000, 1, 1, 0, 0, 0, 3, 1, 0))
        self.assertEqual(t7.tm_year, 2000)
        self.assertEqual(t7.tm_zone, 'some other zone')

        # unknown fields
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, error=2)
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, tm_year=2000, error=2)
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, tm_zone='some other zone', error=2)

    def test_copy_replace_with_unnamed_fields(self):
        assert os.stat_result.n_unnamed_fields > 0

        r = os.stat_result(range(os.stat_result.n_sequence_fields))

        error_message = re.escape('__replace__() is not supported')
        with self.assertRaisesRegex(TypeError, error_message):
            copy.replace(r)
        with self.assertRaisesRegex(TypeError, error_message):
            copy.replace(r, st_mode=1)
        with self.assertRaisesRegex(TypeError, error_message):
            copy.replace(r, error=2)
        with self.assertRaisesRegex(TypeError, error_message):
            copy.replace(r, st_mode=1, error=2)


if __name__ == "__main__":
    unittest.main()
