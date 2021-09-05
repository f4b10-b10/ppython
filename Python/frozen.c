
/* Frozen modules initializer
 *
 * Frozen modules are written to header files by Programs/_freeze_module.
 * These files are typically put in Python/frozen_modules/.  Each holds
 * an array of bytes named "_Py_M__<module>", which is used below.
 *
 * These files must be regenerated any time the corresponding .pyc
 * file would change (including with changes to the compiler, bytecode
 * format, marshal format).  This can be done with "make regen-frozen".
 * That make target just runs Tools/scripts/freeze_modules.py.
 *
 * The freeze_modules.py script also determines which modules get
 * frozen.  Update the list at the top of the script to add, remove,
 * or modify the target modules.  Then run the script
 * (or run "make regen-frozen").
 *
 * The script does the following:
 *
 * 1. run Programs/_freeze_module on the target modules
 * 2. update the includes and _PyImport_FrozenModules[] in this file
 * 3. update the FROZEN_FILES variable in Makefile.pre.in
 * 4. update the per-module targets in Makefile.pre.in
 * 5. update the lists of modules in PCbuild/_freeze_module.vcxproj and
 *    PCbuild/_freeze_module.vcxproj.filters
 *
 * (Note that most of the data in this file is auto-generated by the script.)
 *
 * Those steps can also be done manually, though this is not recommended.
 * Expect such manual changes to be removed the next time
 * freeze_modules.py runs.
 * */

/* In order to test the support for frozen modules, by default we
   define some simple frozen modules: __hello__, __phello__ (a package),
   and __phello__.spam.  Loading any will print some famous words... */

#include "Python.h"

/* Includes for frozen modules: */
#include "frozen_modules/importlib__bootstrap.h"
#include "frozen_modules/importlib__bootstrap_external.h"
#include "frozen_modules/zipimport.h"
#include "frozen_modules/abc.h"
#include "frozen_modules/codecs.h"
#include "frozen_modules/io.h"
#include "frozen_modules/_collections_abc.h"
#include "frozen_modules/_sitebuiltins.h"
#include "frozen_modules/genericpath.h"
#include "frozen_modules/posixpath.h"
#include "frozen_modules/os.h"
#include "frozen_modules/site.h"
#include "frozen_modules/stat.h"
#include "frozen_modules/hello.h"
/* End includes */

#ifdef MS_WINDOWS
/* Deepfreeze isn't supported on Windows yet. */
#define GET_CODE(name) NULL
#else
#define GET_CODE(name) _Py_get_##name##_toplevel
#endif

/* Start extern declarations */
extern PyObject *_Py_get_importlib__bootstrap_toplevel(void);
extern PyObject *_Py_get_importlib__bootstrap_external_toplevel(void);
extern PyObject *_Py_get_zipimport_toplevel(void);
extern PyObject *_Py_get_abc_toplevel(void);
extern PyObject *_Py_get_codecs_toplevel(void);
extern PyObject *_Py_get_io_toplevel(void);
extern PyObject *_Py_get__collections_abc_toplevel(void);
extern PyObject *_Py_get__sitebuiltins_toplevel(void);
extern PyObject *_Py_get_genericpath_toplevel(void);
extern PyObject *_Py_get_posixpath_toplevel(void);
extern PyObject *_Py_get_os_toplevel(void);
extern PyObject *_Py_get_site_toplevel(void);
extern PyObject *_Py_get_stat_toplevel(void);
extern PyObject *_Py_get_hello_toplevel(void);
extern PyObject *_Py_get_hello_toplevel(void);
extern PyObject *_Py_get_hello_toplevel(void);
/* End extern declarations */

/* Note that a negative size indicates a package. */

static const struct _frozen _PyImport_FrozenModules[] = {
    /* importlib */
    {"_frozen_importlib", _Py_M__importlib__bootstrap, (int)sizeof(_Py_M__importlib__bootstrap), GET_CODE(importlib__bootstrap)},
    {"_frozen_importlib_external", _Py_M__importlib__bootstrap_external, (int)sizeof(_Py_M__importlib__bootstrap_external), GET_CODE(importlib__bootstrap_external)},
    {"zipimport", _Py_M__zipimport, (int)sizeof(_Py_M__zipimport), GET_CODE(zipimport)},

    /* stdlib */
    {"abc", _Py_M__abc, (int)sizeof(_Py_M__abc), GET_CODE(abc)},
    {"codecs", _Py_M__codecs, (int)sizeof(_Py_M__codecs), GET_CODE(codecs)},
    {"io", _Py_M__io, (int)sizeof(_Py_M__io), GET_CODE(io)},
    {"_collections_abc", _Py_M___collections_abc, (int)sizeof(_Py_M___collections_abc), GET_CODE(_collections_abc)},
    {"_sitebuiltins", _Py_M___sitebuiltins, (int)sizeof(_Py_M___sitebuiltins), GET_CODE(_sitebuiltins)},
    {"genericpath", _Py_M__genericpath, (int)sizeof(_Py_M__genericpath), GET_CODE(genericpath)},
    {"posixpath", _Py_M__posixpath, (int)sizeof(_Py_M__posixpath), GET_CODE(posixpath)},
    {"os", _Py_M__os, (int)sizeof(_Py_M__os), GET_CODE(os)},
    {"site", _Py_M__site, (int)sizeof(_Py_M__site), GET_CODE(site)},
    {"stat", _Py_M__stat, (int)sizeof(_Py_M__stat), GET_CODE(stat)},

    /* Test module */
    {"__hello__", _Py_M__hello, (int)sizeof(_Py_M__hello), GET_CODE(hello)},
    {"__phello__", _Py_M__hello, -(int)sizeof(_Py_M__hello), GET_CODE(hello)},
    {"__phello__.spam", _Py_M__hello, (int)sizeof(_Py_M__hello), GET_CODE(hello)},
    {0, 0, 0} /* sentinel */
};

/* Embedding apps may change this pointer to point to their favorite
   collection of frozen modules: */

const struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
