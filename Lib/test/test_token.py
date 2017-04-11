import filecmp
import os
import subprocess
import sys
from test import support
import token
import unittest


TOKEN_FILE = support.findfile('token.py')
TOKEN_INCLUDE_FILE = os.path.join(os.path.split(__file__)[0],
                                  '..', '..', 'Include', 'token.h')
TEST_PY_FILE = 'token_test.py'


class TestTokenGeneration(unittest.TestCase):

    def _copy_file_without_generated_tokens(self, source_file, dest_file):
        with open(source_file, 'rb') as fp:
            lines = fp.readlines()
        nl = lines[0][len(lines[0].strip()):]
        with open(dest_file, 'wb') as fp:
            fp.writelines(lines[:lines.index(b"#--start constants--" + nl) + 1])
            fp.writelines(lines[lines.index(b"#--end constants--" + nl):])
        self.addCleanup(support.unlink, dest_file)

    def _generate_tokens(self, skeleton_file, target_token_py_file):
        proc = subprocess.Popen([sys.executable,
                                 TOKEN_FILE,
                                 skeleton_file,
                                 target_token_py_file], stderr=subprocess.PIPE)
        stderr = proc.communicate()[1]
        return proc.returncode, stderr

    @unittest.skipUnless(os.path.exists(TOKEN_FILE),
                     'test only works from source build directory')
    def test_real_token_file(self):
        self._copy_file_without_generated_tokens(TOKEN_FILE, TEST_PY_FILE)
        self.assertFalse(filecmp.cmp(TOKEN_FILE, TEST_PY_FILE))
        self.assertEqual((0, b''), self._generate_tokens(TOKEN_INCLUDE_FILE,
                                                         TEST_PY_FILE))
        self.assertTrue(filecmp.cmp(TOKEN_FILE, TEST_PY_FILE))

    def test_missing_output_file_causes_error(self):
        rc, stderr = self._generate_tokens(os.devnull, 'not_here.txt')
        self.assertNotEqual(rc, 0)
        self.assertIn(b'I/O error', stderr)
        self.assertIn(b'not_here.txt', stderr)


if __name__ == '__main__':
    unittest.main()
