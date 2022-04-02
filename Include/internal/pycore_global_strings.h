#ifndef Py_INTERNAL_GLOBAL_STRINGS_H
#define Py_INTERNAL_GLOBAL_STRINGS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// The data structure & init here are inspired by Tools/scripts/deepfreeze.py.

// All field names generated by ASCII_STR() have a common prefix,
// to help avoid collisions with keywords, etc.

#define STRUCT_FOR_ASCII_STR(LITERAL) \
    struct { \
        PyASCIIObject _ascii; \
        uint8_t _data[sizeof(LITERAL)]; \
    }
#define STRUCT_FOR_STR(NAME, LITERAL) \
    STRUCT_FOR_ASCII_STR(LITERAL) _ ## NAME;
#define STRUCT_FOR_ID(NAME) \
    STRUCT_FOR_ASCII_STR(#NAME) _ ## NAME;

// XXX Order by frequency of use?

/* The following is auto-generated by Tools/scripts/generate_global_objects.py. */
struct _Py_global_strings {
    struct {
        STRUCT_FOR_STR(anon_dictcomp, "<dictcomp>")
        STRUCT_FOR_STR(anon_genexpr, "<genexpr>")
        STRUCT_FOR_STR(anon_lambda, "<lambda>")
        STRUCT_FOR_STR(anon_listcomp, "<listcomp>")
        STRUCT_FOR_STR(anon_module, "<module>")
        STRUCT_FOR_STR(anon_setcomp, "<setcomp>")
        STRUCT_FOR_STR(anon_string, "<string>")
        STRUCT_FOR_STR(anon_unknown, "<unknown>")
        STRUCT_FOR_STR(close_br, "}")
        STRUCT_FOR_STR(comma_sep, ", ")
        STRUCT_FOR_STR(dbl_close_br, "}}")
        STRUCT_FOR_STR(dbl_open_br, "{{")
        STRUCT_FOR_STR(dbl_percent, "%%")
        STRUCT_FOR_STR(dot, ".")
        STRUCT_FOR_STR(dot_locals, ".<locals>")
        STRUCT_FOR_STR(empty, "")
        STRUCT_FOR_STR(list_err, "list index out of range")
        STRUCT_FOR_STR(newline, "\n")
        STRUCT_FOR_STR(open_br, "{")
        STRUCT_FOR_STR(percent, "%")
    } literals;

    struct {
        STRUCT_FOR_ID(False)
        STRUCT_FOR_ID(Py_Repr)
        STRUCT_FOR_ID(TextIOWrapper)
        STRUCT_FOR_ID(True)
        STRUCT_FOR_ID(WarningMessage)
        STRUCT_FOR_ID(_)
        STRUCT_FOR_ID(__IOBase_closed)
        STRUCT_FOR_ID(__abc_tpflags__)
        STRUCT_FOR_ID(__abs__)
        STRUCT_FOR_ID(__abstractmethods__)
        STRUCT_FOR_ID(__add__)
        STRUCT_FOR_ID(__aenter__)
        STRUCT_FOR_ID(__aexit__)
        STRUCT_FOR_ID(__aiter__)
        STRUCT_FOR_ID(__all__)
        STRUCT_FOR_ID(__and__)
        STRUCT_FOR_ID(__anext__)
        STRUCT_FOR_ID(__annotations__)
        STRUCT_FOR_ID(__args__)
        STRUCT_FOR_ID(__await__)
        STRUCT_FOR_ID(__bases__)
        STRUCT_FOR_ID(__bool__)
        STRUCT_FOR_ID(__build_class__)
        STRUCT_FOR_ID(__builtins__)
        STRUCT_FOR_ID(__bytes__)
        STRUCT_FOR_ID(__call__)
        STRUCT_FOR_ID(__cantrace__)
        STRUCT_FOR_ID(__class__)
        STRUCT_FOR_ID(__class_getitem__)
        STRUCT_FOR_ID(__classcell__)
        STRUCT_FOR_ID(__complex__)
        STRUCT_FOR_ID(__contains__)
        STRUCT_FOR_ID(__copy__)
        STRUCT_FOR_ID(__del__)
        STRUCT_FOR_ID(__delattr__)
        STRUCT_FOR_ID(__delete__)
        STRUCT_FOR_ID(__delitem__)
        STRUCT_FOR_ID(__dict__)
        STRUCT_FOR_ID(__dir__)
        STRUCT_FOR_ID(__divmod__)
        STRUCT_FOR_ID(__doc__)
        STRUCT_FOR_ID(__enter__)
        STRUCT_FOR_ID(__eq__)
        STRUCT_FOR_ID(__exit__)
        STRUCT_FOR_ID(__file__)
        STRUCT_FOR_ID(__float__)
        STRUCT_FOR_ID(__floordiv__)
        STRUCT_FOR_ID(__format__)
        STRUCT_FOR_ID(__fspath__)
        STRUCT_FOR_ID(__ge__)
        STRUCT_FOR_ID(__get__)
        STRUCT_FOR_ID(__getattr__)
        STRUCT_FOR_ID(__getattribute__)
        STRUCT_FOR_ID(__getinitargs__)
        STRUCT_FOR_ID(__getitem__)
        STRUCT_FOR_ID(__getnewargs__)
        STRUCT_FOR_ID(__getnewargs_ex__)
        STRUCT_FOR_ID(__getstate__)
        STRUCT_FOR_ID(__gt__)
        STRUCT_FOR_ID(__hash__)
        STRUCT_FOR_ID(__iadd__)
        STRUCT_FOR_ID(__iand__)
        STRUCT_FOR_ID(__ifloordiv__)
        STRUCT_FOR_ID(__ilshift__)
        STRUCT_FOR_ID(__imatmul__)
        STRUCT_FOR_ID(__imod__)
        STRUCT_FOR_ID(__import__)
        STRUCT_FOR_ID(__imul__)
        STRUCT_FOR_ID(__index__)
        STRUCT_FOR_ID(__init__)
        STRUCT_FOR_ID(__init_subclass__)
        STRUCT_FOR_ID(__instancecheck__)
        STRUCT_FOR_ID(__int__)
        STRUCT_FOR_ID(__invert__)
        STRUCT_FOR_ID(__ior__)
        STRUCT_FOR_ID(__ipow__)
        STRUCT_FOR_ID(__irshift__)
        STRUCT_FOR_ID(__isabstractmethod__)
        STRUCT_FOR_ID(__isub__)
        STRUCT_FOR_ID(__iter__)
        STRUCT_FOR_ID(__itruediv__)
        STRUCT_FOR_ID(__ixor__)
        STRUCT_FOR_ID(__le__)
        STRUCT_FOR_ID(__len__)
        STRUCT_FOR_ID(__length_hint__)
        STRUCT_FOR_ID(__loader__)
        STRUCT_FOR_ID(__lshift__)
        STRUCT_FOR_ID(__lt__)
        STRUCT_FOR_ID(__ltrace__)
        STRUCT_FOR_ID(__main__)
        STRUCT_FOR_ID(__matmul__)
        STRUCT_FOR_ID(__missing__)
        STRUCT_FOR_ID(__mod__)
        STRUCT_FOR_ID(__module__)
        STRUCT_FOR_ID(__mro_entries__)
        STRUCT_FOR_ID(__mul__)
        STRUCT_FOR_ID(__name__)
        STRUCT_FOR_ID(__ne__)
        STRUCT_FOR_ID(__neg__)
        STRUCT_FOR_ID(__new__)
        STRUCT_FOR_ID(__newobj__)
        STRUCT_FOR_ID(__newobj_ex__)
        STRUCT_FOR_ID(__next__)
        STRUCT_FOR_ID(__note__)
        STRUCT_FOR_ID(__or__)
        STRUCT_FOR_ID(__orig_class__)
        STRUCT_FOR_ID(__origin__)
        STRUCT_FOR_ID(__package__)
        STRUCT_FOR_ID(__parameters__)
        STRUCT_FOR_ID(__path__)
        STRUCT_FOR_ID(__pos__)
        STRUCT_FOR_ID(__pow__)
        STRUCT_FOR_ID(__prepare__)
        STRUCT_FOR_ID(__qualname__)
        STRUCT_FOR_ID(__radd__)
        STRUCT_FOR_ID(__rand__)
        STRUCT_FOR_ID(__rdivmod__)
        STRUCT_FOR_ID(__reduce__)
        STRUCT_FOR_ID(__reduce_ex__)
        STRUCT_FOR_ID(__repr__)
        STRUCT_FOR_ID(__reversed__)
        STRUCT_FOR_ID(__rfloordiv__)
        STRUCT_FOR_ID(__rlshift__)
        STRUCT_FOR_ID(__rmatmul__)
        STRUCT_FOR_ID(__rmod__)
        STRUCT_FOR_ID(__rmul__)
        STRUCT_FOR_ID(__ror__)
        STRUCT_FOR_ID(__round__)
        STRUCT_FOR_ID(__rpow__)
        STRUCT_FOR_ID(__rrshift__)
        STRUCT_FOR_ID(__rshift__)
        STRUCT_FOR_ID(__rsub__)
        STRUCT_FOR_ID(__rtruediv__)
        STRUCT_FOR_ID(__rxor__)
        STRUCT_FOR_ID(__set__)
        STRUCT_FOR_ID(__set_name__)
        STRUCT_FOR_ID(__setattr__)
        STRUCT_FOR_ID(__setitem__)
        STRUCT_FOR_ID(__setstate__)
        STRUCT_FOR_ID(__sizeof__)
        STRUCT_FOR_ID(__slotnames__)
        STRUCT_FOR_ID(__slots__)
        STRUCT_FOR_ID(__spec__)
        STRUCT_FOR_ID(__str__)
        STRUCT_FOR_ID(__sub__)
        STRUCT_FOR_ID(__subclasscheck__)
        STRUCT_FOR_ID(__subclasshook__)
        STRUCT_FOR_ID(__truediv__)
        STRUCT_FOR_ID(__trunc__)
        STRUCT_FOR_ID(__typing_subst__)
        STRUCT_FOR_ID(__warningregistry__)
        STRUCT_FOR_ID(__weakref__)
        STRUCT_FOR_ID(__xor__)
        STRUCT_FOR_ID(_abc_impl)
        STRUCT_FOR_ID(_annotation)
        STRUCT_FOR_ID(_blksize)
        STRUCT_FOR_ID(_bootstrap)
        STRUCT_FOR_ID(_dealloc_warn)
        STRUCT_FOR_ID(_finalizing)
        STRUCT_FOR_ID(_find_and_load)
        STRUCT_FOR_ID(_fix_up_module)
        STRUCT_FOR_ID(_get_sourcefile)
        STRUCT_FOR_ID(_handle_fromlist)
        STRUCT_FOR_ID(_initializing)
        STRUCT_FOR_ID(_is_text_encoding)
        STRUCT_FOR_ID(_lock_unlock_module)
        STRUCT_FOR_ID(_showwarnmsg)
        STRUCT_FOR_ID(_shutdown)
        STRUCT_FOR_ID(_slotnames)
        STRUCT_FOR_ID(_strptime_time)
        STRUCT_FOR_ID(_uninitialized_submodules)
        STRUCT_FOR_ID(_warn_unawaited_coroutine)
        STRUCT_FOR_ID(_xoptions)
        STRUCT_FOR_ID(add)
        STRUCT_FOR_ID(append)
        STRUCT_FOR_ID(big)
        STRUCT_FOR_ID(buffer)
        STRUCT_FOR_ID(builtins)
        STRUCT_FOR_ID(c_call)
        STRUCT_FOR_ID(c_exception)
        STRUCT_FOR_ID(c_return)
        STRUCT_FOR_ID(call)
        STRUCT_FOR_ID(clear)
        STRUCT_FOR_ID(close)
        STRUCT_FOR_ID(closed)
        STRUCT_FOR_ID(code)
        STRUCT_FOR_ID(copy)
        STRUCT_FOR_ID(copyreg)
        STRUCT_FOR_ID(decode)
        STRUCT_FOR_ID(default)
        STRUCT_FOR_ID(defaultaction)
        STRUCT_FOR_ID(dictcomp)
        STRUCT_FOR_ID(difference_update)
        STRUCT_FOR_ID(dispatch_table)
        STRUCT_FOR_ID(displayhook)
        STRUCT_FOR_ID(enable)
        STRUCT_FOR_ID(encode)
        STRUCT_FOR_ID(encoding)
        STRUCT_FOR_ID(end_lineno)
        STRUCT_FOR_ID(end_offset)
        STRUCT_FOR_ID(errors)
        STRUCT_FOR_ID(excepthook)
        STRUCT_FOR_ID(exception)
        STRUCT_FOR_ID(extend)
        STRUCT_FOR_ID(filename)
        STRUCT_FOR_ID(fileno)
        STRUCT_FOR_ID(fillvalue)
        STRUCT_FOR_ID(filters)
        STRUCT_FOR_ID(find_class)
        STRUCT_FOR_ID(flush)
        STRUCT_FOR_ID(genexpr)
        STRUCT_FOR_ID(get)
        STRUCT_FOR_ID(get_source)
        STRUCT_FOR_ID(getattr)
        STRUCT_FOR_ID(getstate)
        STRUCT_FOR_ID(ignore)
        STRUCT_FOR_ID(importlib)
        STRUCT_FOR_ID(inf)
        STRUCT_FOR_ID(intersection)
        STRUCT_FOR_ID(isatty)
        STRUCT_FOR_ID(isinstance)
        STRUCT_FOR_ID(items)
        STRUCT_FOR_ID(iter)
        STRUCT_FOR_ID(join)
        STRUCT_FOR_ID(keys)
        STRUCT_FOR_ID(lambda)
        STRUCT_FOR_ID(last_traceback)
        STRUCT_FOR_ID(last_type)
        STRUCT_FOR_ID(last_value)
        STRUCT_FOR_ID(latin1)
        STRUCT_FOR_ID(len)
        STRUCT_FOR_ID(line)
        STRUCT_FOR_ID(lineno)
        STRUCT_FOR_ID(listcomp)
        STRUCT_FOR_ID(little)
        STRUCT_FOR_ID(locale)
        STRUCT_FOR_ID(match)
        STRUCT_FOR_ID(metaclass)
        STRUCT_FOR_ID(mode)
        STRUCT_FOR_ID(modules)
        STRUCT_FOR_ID(mro)
        STRUCT_FOR_ID(msg)
        STRUCT_FOR_ID(n_fields)
        STRUCT_FOR_ID(n_sequence_fields)
        STRUCT_FOR_ID(n_unnamed_fields)
        STRUCT_FOR_ID(name)
        STRUCT_FOR_ID(newlines)
        STRUCT_FOR_ID(obj)
        STRUCT_FOR_ID(offset)
        STRUCT_FOR_ID(onceregistry)
        STRUCT_FOR_ID(opcode)
        STRUCT_FOR_ID(open)
        STRUCT_FOR_ID(parent)
        STRUCT_FOR_ID(partial)
        STRUCT_FOR_ID(path)
        STRUCT_FOR_ID(peek)
        STRUCT_FOR_ID(persistent_id)
        STRUCT_FOR_ID(persistent_load)
        STRUCT_FOR_ID(print_file_and_line)
        STRUCT_FOR_ID(ps1)
        STRUCT_FOR_ID(ps2)
        STRUCT_FOR_ID(raw)
        STRUCT_FOR_ID(read)
        STRUCT_FOR_ID(read1)
        STRUCT_FOR_ID(readable)
        STRUCT_FOR_ID(readall)
        STRUCT_FOR_ID(readinto)
        STRUCT_FOR_ID(readinto1)
        STRUCT_FOR_ID(readline)
        STRUCT_FOR_ID(reducer_override)
        STRUCT_FOR_ID(reload)
        STRUCT_FOR_ID(replace)
        STRUCT_FOR_ID(reset)
        STRUCT_FOR_ID(return)
        STRUCT_FOR_ID(reversed)
        STRUCT_FOR_ID(seek)
        STRUCT_FOR_ID(seekable)
        STRUCT_FOR_ID(send)
        STRUCT_FOR_ID(setcomp)
        STRUCT_FOR_ID(setstate)
        STRUCT_FOR_ID(sort)
        STRUCT_FOR_ID(stderr)
        STRUCT_FOR_ID(stdin)
        STRUCT_FOR_ID(stdout)
        STRUCT_FOR_ID(strict)
        STRUCT_FOR_ID(symmetric_difference_update)
        STRUCT_FOR_ID(tell)
        STRUCT_FOR_ID(text)
        STRUCT_FOR_ID(threading)
        STRUCT_FOR_ID(throw)
        STRUCT_FOR_ID(top)
        STRUCT_FOR_ID(truncate)
        STRUCT_FOR_ID(unraisablehook)
        STRUCT_FOR_ID(values)
        STRUCT_FOR_ID(version)
        STRUCT_FOR_ID(warnings)
        STRUCT_FOR_ID(warnoptions)
        STRUCT_FOR_ID(writable)
        STRUCT_FOR_ID(write)
        STRUCT_FOR_ID(zipimporter)
    } identifiers;
    struct {
        PyASCIIObject _ascii;
        uint8_t _data[2];
    } ascii[128];
    struct {
        PyCompactUnicodeObject _latin1;
        uint8_t _data[2];
    } latin1[128];
};
/* End auto-generated code */

#undef ID
#undef STR


#define _Py_ID(NAME) \
     (_Py_SINGLETON(strings.identifiers._ ## NAME._ascii.ob_base))
#define _Py_STR(NAME) \
     (_Py_SINGLETON(strings.literals._ ## NAME._ascii.ob_base))

/* _Py_DECLARE_STR() should precede all uses of _Py_STR() in a function.

   This is true even if the same string has already been declared
   elsewhere, even in the same file.  Mismatched duplicates are detected
   by Tools/scripts/generate-global-objects.py.

   Pairing _Py_DECLARE_STR() with every use of _Py_STR() makes sure the
   string keeps working even if the declaration is removed somewhere
   else.  It also makes it clear what the actual string is at every
   place it is being used. */
#define _Py_DECLARE_STR(name, str)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_STRINGS_H */
