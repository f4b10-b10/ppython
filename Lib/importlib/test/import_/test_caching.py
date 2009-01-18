"""Test that sys.modules is used properly by import."""
from ..support import import_, mock_modules, importlib_only, import_state

import sys
from types import MethodType
import unittest


class UseCache(unittest.TestCase):

    """When it comes to sys.modules, import prefers it over anything else.

    Once a name has been resolved, sys.modules is checked to see if it contains
    the module desired. If so, then it is returned [use cache]. If it is not
    found, then the proper steps are taken to perform the import, but
    sys.modules is still used to return the imported module (e.g., not what a
    loader returns) [from cache on return]. This also applies to imports of
    things contained within a package and thus get assigned as an attribute
    [from cache to attribute] or pulled in thanks to a fromlist import
    [from cache for fromlist].

    """
    def test_using_cache(self):
        # [use cache]
        module_to_use = "some module found!"
        sys.modules['some_module'] = module_to_use
        module = import_('some_module')
        self.assertEqual(id(module_to_use), id(module))

    def create_mock(self, *names, return_=None):
        mock = mock_modules(*names)
        original_load = mock.load_module
        def load_module(self, fullname):
            original_load(fullname)
            return return_
        mock.load_module = MethodType(load_module, mock)
        return mock

    # __import__ inconsistent between loaders and built-in import when it comes
    #   to when to use the module in sys.modules and when not to.
    @importlib_only
    def test_using_cache_after_loader(self):
        # [from cache on return]
        with self.create_mock('module') as mock:
            with import_state(meta_path=[mock]):
                module = import_('module')
                self.assertEquals(id(module), id(sys.modules['module']))

    # See test_using_cache_after_loader() for reasoning.
    @importlib_only
    def test_using_cache_for_assigning_to_attribute(self):
        # [from cache to attribute]
        with self.create_mock('pkg.__init__', 'pkg.module') as importer:
            with import_state(meta_path=[importer]):
                module = import_('pkg.module')
                self.assert_(hasattr(module, 'module'))
                self.assert_(id(module.module), id(sys.modules['pkg.module']))

    # See test_using_cache_after_loader() for reasoning.
    @importlib_only
    def test_using_cache_for_fromlist(self):
        # [from cache for fromlist]
        with self.create_mock('pkg.__init__', 'pkg.module') as importer:
            with import_state(meta_path=[importer]):
                module = import_('pkg', fromlist=['module'])
                self.assert_(hasattr(module, 'module'))
                self.assertEquals(id(module.module), id(sys.modules['pkg.module']))


def test_main():
    from test.support import run_unittest
    run_unittest(UseCache)

if __name__ == '__main__':
    test_main()
