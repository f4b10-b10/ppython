"""Utilities to support packages."""

import os
import sys
import importlib
import imp
import os.path
from warnings import warn
from types import ModuleType

__all__ = [
    'get_importer', 'iter_importers', 'get_loader', 'find_loader',
    'walk_packages', 'iter_modules', 'get_data',
    'ImpImporter', 'ImpLoader', 'read_code', 'extend_path',
]

def read_code(stream):
    # This helper is needed in order for the PEP 302 emulation to
    # correctly handle compiled files
    import marshal

    magic = stream.read(4)
    if magic != imp.get_magic():
        return None

    stream.read(8) # Skip timestamp and size
    return marshal.load(stream)


def simplegeneric(func):
    """Make a trivial single-dispatch generic function"""
    registry = {}
    def wrapper(*args, **kw):
        ob = args[0]
        try:
            cls = ob.__class__
        except AttributeError:
            cls = type(ob)
        try:
            mro = cls.__mro__
        except AttributeError:
            try:
                class cls(cls, object):
                    pass
                mro = cls.__mro__[1:]
            except TypeError:
                mro = object,   # must be an ExtensionClass or some such  :(
        for t in mro:
            if t in registry:
                return registry[t](*args, **kw)
        else:
            return func(*args, **kw)
    try:
        wrapper.__name__ = func.__name__
    except (TypeError, AttributeError):
        pass    # Python 2.3 doesn't allow functions to be renamed

    def register(typ, func=None):
        if func is None:
            return lambda f: register(typ, f)
        registry[typ] = func
        return func

    wrapper.__dict__ = func.__dict__
    wrapper.__doc__ = func.__doc__
    wrapper.register = register
    return wrapper


def walk_packages(path=None, prefix='', onerror=None):
    """Yields (module_loader, name, ispkg) for all modules recursively
    on path, or, if path is None, all accessible modules.

    'path' should be either None or a list of paths to look for
    modules in.

    'prefix' is a string to output on the front of every module name
    on output.

    Note that this function must import all *packages* (NOT all
    modules!) on the given path, in order to access the __path__
    attribute to find submodules.

    'onerror' is a function which gets called with one argument (the
    name of the package which was being imported) if any exception
    occurs while trying to import a package.  If no onerror function is
    supplied, ImportErrors are caught and ignored, while all other
    exceptions are propagated, terminating the search.

    Examples:

    # list all modules python can access
    walk_packages()

    # list all submodules of ctypes
    walk_packages(ctypes.__path__, ctypes.__name__+'.')
    """

    def seen(p, m={}):
        if p in m:
            return True
        m[p] = True

    for importer, name, ispkg in iter_modules(path, prefix):
        yield importer, name, ispkg

        if ispkg:
            try:
                __import__(name)
            except ImportError:
                if onerror is not None:
                    onerror(name)
            except Exception:
                if onerror is not None:
                    onerror(name)
                else:
                    raise
            else:
                path = getattr(sys.modules[name], '__path__', None) or []

                # don't traverse path items we've seen before
                path = [p for p in path if not seen(p)]

                yield from walk_packages(path, name+'.', onerror)


def iter_modules(path=None, prefix=''):
    """Yields (module_loader, name, ispkg) for all submodules on path,
    or, if path is None, all top-level modules on sys.path.

    'path' should be either None or a list of paths to look for
    modules in.

    'prefix' is a string to output on the front of every module name
    on output.
    """

    if path is None:
        importers = iter_importers()
    else:
        importers = map(get_importer, path)

    yielded = {}
    for i in importers:
        for name, ispkg in iter_importer_modules(i, prefix):
            if name not in yielded:
                yielded[name] = 1
                yield i, name, ispkg


#@simplegeneric
def iter_importer_modules(importer, prefix=''):
    if not hasattr(importer, 'iter_modules'):
        return []
    return importer.iter_modules(prefix)

iter_importer_modules = simplegeneric(iter_importer_modules)

# Implement a file walker for the normal importlib path hook
def _iter_file_finder_modules(importer, prefix=''):
    if importer.path is None or not os.path.isdir(importer.path):
        return

    yielded = {}
    import inspect
    try:
        filenames = os.listdir(importer.path)
    except OSError:
        # ignore unreadable directories like import does
        filenames = []
    filenames.sort()  # handle packages before same-named modules

    for fn in filenames:
        modname = inspect.getmodulename(fn)
        if modname=='__init__' or modname in yielded:
            continue

        path = os.path.join(importer.path, fn)
        ispkg = False

        if not modname and os.path.isdir(path) and '.' not in fn:
            modname = fn
            try:
                dircontents = os.listdir(path)
            except OSError:
                # ignore unreadable directories like import does
                dircontents = []
            for fn in dircontents:
                subname = inspect.getmodulename(fn)
                if subname=='__init__':
                    ispkg = True
                    break
            else:
                continue    # not a package

        if modname and '.' not in modname:
            yielded[modname] = 1
            yield prefix + modname, ispkg

iter_importer_modules.register(
    importlib.machinery.FileFinder, _iter_file_finder_modules)

class ImpImporter:
    """PEP 302 Importer that wraps Python's "classic" import algorithm

    ImpImporter(dirname) produces a PEP 302 importer that searches that
    directory.  ImpImporter(None) produces a PEP 302 importer that searches
    the current sys.path, plus any modules that are frozen or built-in.

    Note that ImpImporter does not currently support being used by placement
    on sys.meta_path.
    """

    def __init__(self, path=None):
        warn("This emulation is deprecated, use 'importlib' instead",
             DeprecationWarning)
        self.path = path

    def find_module(self, fullname, path=None):
        # Note: we ignore 'path' argument since it is only used via meta_path
        subname = fullname.split(".")[-1]
        if subname != fullname and self.path is None:
            return None
        if self.path is None:
            path = None
        else:
            path = [os.path.realpath(self.path)]
        try:
            file, filename, etc = imp.find_module(subname, path)
        except ImportError:
            return None
        return ImpLoader(fullname, file, filename, etc)

    def iter_modules(self, prefix=''):
        if self.path is None or not os.path.isdir(self.path):
            return

        yielded = {}
        import inspect
        try:
            filenames = os.listdir(self.path)
        except OSError:
            # ignore unreadable directories like import does
            filenames = []
        filenames.sort()  # handle packages before same-named modules

        for fn in filenames:
            modname = inspect.getmodulename(fn)
            if modname=='__init__' or modname in yielded:
                continue

            path = os.path.join(self.path, fn)
            ispkg = False

            if not modname and os.path.isdir(path) and '.' not in fn:
                modname = fn
                try:
                    dircontents = os.listdir(path)
                except OSError:
                    # ignore unreadable directories like import does
                    dircontents = []
                for fn in dircontents:
                    subname = inspect.getmodulename(fn)
                    if subname=='__init__':
                        ispkg = True
                        break
                else:
                    continue    # not a package

            if modname and '.' not in modname:
                yielded[modname] = 1
                yield prefix + modname, ispkg


class ImpLoader:
    """PEP 302 Loader that wraps Python's "classic" import algorithm
    """
    code = source = None

    def __init__(self, fullname, file, filename, etc):
        warn("This emulation is deprecated, use 'importlib' instead",
             DeprecationWarning)
        self.file = file
        self.filename = filename
        self.fullname = fullname
        self.etc = etc

    def load_module(self, fullname):
        self._reopen()
        try:
            mod = imp.load_module(fullname, self.file, self.filename, self.etc)
        finally:
            if self.file:
                self.file.close()
        # Note: we don't set __loader__ because we want the module to look
        # normal; i.e. this is just a wrapper for standard import machinery
        return mod

    def get_data(self, pathname):
        with open(pathname, "rb") as file:
            return file.read()

    def _reopen(self):
        if self.file and self.file.closed:
            mod_type = self.etc[2]
            if mod_type==imp.PY_SOURCE:
                self.file = open(self.filename, 'r')
            elif mod_type in (imp.PY_COMPILED, imp.C_EXTENSION):
                self.file = open(self.filename, 'rb')

    def _fix_name(self, fullname):
        if fullname is None:
            fullname = self.fullname
        elif fullname != self.fullname:
            raise ImportError("Loader for module %s cannot handle "
                              "module %s" % (self.fullname, fullname))
        return fullname

    def is_package(self, fullname):
        fullname = self._fix_name(fullname)
        return self.etc[2]==imp.PKG_DIRECTORY

    def get_code(self, fullname=None):
        fullname = self._fix_name(fullname)
        if self.code is None:
            mod_type = self.etc[2]
            if mod_type==imp.PY_SOURCE:
                source = self.get_source(fullname)
                self.code = compile(source, self.filename, 'exec')
            elif mod_type==imp.PY_COMPILED:
                self._reopen()
                try:
                    self.code = read_code(self.file)
                finally:
                    self.file.close()
            elif mod_type==imp.PKG_DIRECTORY:
                self.code = self._get_delegate().get_code()
        return self.code

    def get_source(self, fullname=None):
        fullname = self._fix_name(fullname)
        if self.source is None:
            mod_type = self.etc[2]
            if mod_type==imp.PY_SOURCE:
                self._reopen()
                try:
                    self.source = self.file.read()
                finally:
                    self.file.close()
            elif mod_type==imp.PY_COMPILED:
                if os.path.exists(self.filename[:-1]):
                    f = open(self.filename[:-1], 'r')
                    self.source = f.read()
                    f.close()
            elif mod_type==imp.PKG_DIRECTORY:
                self.source = self._get_delegate().get_source()
        return self.source


    def _get_delegate(self):
        return ImpImporter(self.filename).find_module('__init__')

    def get_filename(self, fullname=None):
        fullname = self._fix_name(fullname)
        mod_type = self.etc[2]
        if mod_type==imp.PKG_DIRECTORY:
            return self._get_delegate().get_filename()
        elif mod_type in (imp.PY_SOURCE, imp.PY_COMPILED, imp.C_EXTENSION):
            return self.filename
        return None


try:
    import zipimport
    from zipimport import zipimporter

    def iter_zipimport_modules(importer, prefix=''):
        dirlist = sorted(zipimport._zip_directory_cache[importer.archive])
        _prefix = importer.prefix
        plen = len(_prefix)
        yielded = {}
        import inspect
        for fn in dirlist:
            if not fn.startswith(_prefix):
                continue

            fn = fn[plen:].split(os.sep)

            if len(fn)==2 and fn[1].startswith('__init__.py'):
                if fn[0] not in yielded:
                    yielded[fn[0]] = 1
                    yield fn[0], True

            if len(fn)!=1:
                continue

            modname = inspect.getmodulename(fn[0])
            if modname=='__init__':
                continue

            if modname and '.' not in modname and modname not in yielded:
                yielded[modname] = 1
                yield prefix + modname, False

    iter_importer_modules.register(zipimporter, iter_zipimport_modules)

except ImportError:
    pass


def get_importer(path_item):
    """Retrieve a PEP 302 importer for the given path item

    The returned importer is cached in sys.path_importer_cache
    if it was newly created by a path hook.

    The cache (or part of it) can be cleared manually if a
    rescan of sys.path_hooks is necessary.
    """
    try:
        importer = sys.path_importer_cache[path_item]
    except KeyError:
        for path_hook in sys.path_hooks:
            try:
                importer = path_hook(path_item)
                sys.path_importer_cache.setdefault(path_item, importer)
                break
            except ImportError:
                pass
        else:
            importer = None
    return importer


def iter_importers(fullname=""):
    """Yield PEP 302 importers for the given module name

    If fullname contains a '.', the importers will be for the package
    containing fullname, otherwise they will be all registered top level
    importers (i.e. those on both sys.meta_path and sys.path_hooks).

    If the named module is in a package, that package is imported as a side
    effect of invoking this function.

    If no module name is specified, all top level importers are produced.
    """
    if fullname.startswith('.'):
        msg = "Relative module name {!r} not supported".format(fullname)
        raise ImportError(msg)
    if '.' in fullname:
        # Get the containing package's __path__
        pkg_name = fullname.rpartition(".")[0]
        pkg = importlib.import_module(pkg)
        path = getattr(sys.modules[pkg], '__path__', None)
        if path is None:
            return
    else:
        yield from sys.meta_path
        path = sys.path
    for item in path:
        yield get_importer(item)

def get_loader(module_or_name):
    """Get a PEP 302 "loader" object for module_or_name

    Returns None if the module cannot be found or imported.
    If the named module is not already imported, its containing package
    (if any) is imported, in order to establish the package __path__.
    """
    if module_or_name in sys.modules:
        module_or_name = sys.modules[module_or_name]
    if isinstance(module_or_name, ModuleType):
        module = module_or_name
        loader = getattr(module, '__loader__', None)
        if loader is not None:
            return loader
        fullname = module.__name__
    else:
        fullname = module_or_name
    return find_loader(fullname)


def find_loader(fullname):
    """Find a PEP 302 "loader" object for fullname

    This is s convenience wrapper around :func:`importlib.find_loader` that
    sets the *path* argument correctly when searching for submodules, and
    also ensures parent packages (if any) are imported before searching for
    submodules.
    """
    if fullname.startswith('.'):
        msg = "Relative module name {!r} not supported".format(fullname)
        raise ImportError(msg)
    path = None
    pkg_name = fullname.rpartition(".")[0]
    if pkg_name:
        pkg = importlib.import_module(pkg_name)
        path = getattr(pkg, "__path__", None)
        if path is None:
            return None
    try:
        return importlib.find_loader(fullname, path)
    except (ImportError, AttributeError, TypeError, ValueError) as ex:
        # This hack fixes an impedance mismatch between pkgutil and
        # importlib, where the latter raises other errors for cases where
        # pkgutil previously threw ImportError
        msg = "Error while finding loader for {!r} ({}: {})"
        raise ImportError(msg.format(fullname, type(ex), ex)) from ex


def extend_path(path, name):
    """Extend a package's path.

    Intended use is to place the following code in a package's __init__.py:

        from pkgutil import extend_path
        __path__ = extend_path(__path__, __name__)

    This will add to the package's __path__ all subdirectories of
    directories on sys.path named after the package.  This is useful
    if one wants to distribute different parts of a single logical
    package as multiple directories.

    It also looks for *.pkg files beginning where * matches the name
    argument.  This feature is similar to *.pth files (see site.py),
    except that it doesn't special-case lines starting with 'import'.
    A *.pkg file is trusted at face value: apart from checking for
    duplicates, all entries found in a *.pkg file are added to the
    path, regardless of whether they are exist the filesystem.  (This
    is a feature.)

    If the input path is not a list (as is the case for frozen
    packages) it is returned unchanged.  The input path is not
    modified; an extended copy is returned.  Items are only appended
    to the copy at the end.

    It is assumed that sys.path is a sequence.  Items of sys.path that
    are not (unicode or 8-bit) strings referring to existing
    directories are ignored.  Unicode items of sys.path that cause
    errors when used as filenames may cause this function to raise an
    exception (in line with os.path.isdir() behavior).
    """

    if not isinstance(path, list):
        # This could happen e.g. when this is called from inside a
        # frozen package.  Return the path unchanged in that case.
        return path

    sname_pkg = name + ".pkg"

    path = path[:] # Start with a copy of the existing path

    parent_package, _, final_name = name.rpartition('.')
    if parent_package:
        try:
            search_path = sys.modules[parent_package].__path__
        except (KeyError, AttributeError):
            # We can't do anything: find_loader() returns None when
            # passed a dotted name.
            return path
    else:
        search_path = sys.path

    for dir in search_path:
        if not isinstance(dir, str):
            continue

        finder = get_importer(dir)
        if finder is not None:
            # Is this finder PEP 420 compliant?
            if hasattr(finder, 'find_loader'):
                loader, portions = finder.find_loader(final_name)
            else:
                # No, no need to call it
                loader = None
                portions = []

            for portion in portions:
                # XXX This may still add duplicate entries to path on
                # case-insensitive filesystems
                if portion not in path:
                    path.append(portion)

        # XXX Is this the right thing for subpackages like zope.app?
        # It looks for a file named "zope.app.pkg"
        pkgfile = os.path.join(dir, sname_pkg)
        if os.path.isfile(pkgfile):
            try:
                f = open(pkgfile)
            except IOError as msg:
                sys.stderr.write("Can't open %s: %s\n" %
                                 (pkgfile, msg))
            else:
                for line in f:
                    line = line.rstrip('\n')
                    if not line or line.startswith('#'):
                        continue
                    path.append(line) # Don't check for existence!
                f.close()

    return path

def get_data(package, resource):
    """Get a resource from a package.

    This is a wrapper round the PEP 302 loader get_data API. The package
    argument should be the name of a package, in standard module format
    (foo.bar). The resource argument should be in the form of a relative
    filename, using '/' as the path separator. The parent directory name '..'
    is not allowed, and nor is a rooted name (starting with a '/').

    The function returns a binary string, which is the contents of the
    specified resource.

    For packages located in the filesystem, which have already been imported,
    this is the rough equivalent of

        d = os.path.dirname(sys.modules[package].__file__)
        data = open(os.path.join(d, resource), 'rb').read()

    If the package cannot be located or loaded, or it uses a PEP 302 loader
    which does not support get_data(), then None is returned.
    """

    loader = get_loader(package)
    if loader is None or not hasattr(loader, 'get_data'):
        return None
    mod = sys.modules.get(package) or loader.load_module(package)
    if mod is None or not hasattr(mod, '__file__'):
        return None

    # Modify the resource name to be compatible with the loader.get_data
    # signature - an os.path format "filename" starting with the dirname of
    # the package's __file__
    parts = resource.split('/')
    parts.insert(0, os.path.dirname(mod.__file__))
    resource_name = os.path.join(*parts)
    return loader.get_data(resource_name)
