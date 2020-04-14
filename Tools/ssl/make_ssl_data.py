#! /usr/bin/env python3

"""
This script should be called *manually* when we want to upgrade SSLError
`library` and `reason` mnemnonics to a more recent OpenSSL version.

It takes two arguments:
- the path to the OpenSSL source tree (e.g. git checkout)
- the path to the C file to be generated
  (probably Modules/_ssl_data.h)
"""

import datetime
import glob
import os
import re
import sys
import _ssl


def parse_error_codes(h_file, prefix, libcode):
    pat = re.compile(r"#\s*define\W+(%s([\w]+))\W+(\d+)\b" % re.escape(prefix))
    codes = []
    with open(h_file, "r", encoding="latin1") as f:
        for line in f:
            match = pat.search(line)
            if match:
                code, name, num = match.groups()
                num = int(num)
                # e.g. ("SSL_R_BAD_DATA", ("ERR_LIB_SSL", "BAD_DATA", 390))
                codes.append((code, (libcode, name, num)))
    assert codes, f"no codes found in {h_file}"
    return codes

if __name__ == "__main__":
    openssl_inc = sys.argv[1]
    outfile = sys.argv[2]
    use_stdout = outfile == '-'
    f = sys.stdout if use_stdout else open(outfile, "w")
    # mnemonic -> (library code, error prefix, header file)
    error_libraries = {}
    for error_header in glob.glob(os.path.join(openssl_inc, 'include/openssl/*err.h')):
        base = os.path.basename(error_header)
        if base in ('buffererr.h', 'objectserr.h', 'storeerr.h'):
            # Deprecated in 3.0.
            continue
        mnemonic = base[:-5].upper()
        if mnemonic == "":
            # err.h
            lib_codes = {
                code: num
                for (code, (_, _, num)) in parse_error_codes(error_header, 'ERR_LIB_', None)
            }
        else:
            error_libraries[mnemonic] = (f'ERR_LIB_{mnemonic}', f'{mnemonic}_R_', error_header)

    # Read codes from libraries
    new_codes = []
    for libcode, prefix, h_file in sorted(error_libraries.values()):
        new_codes += parse_error_codes(h_file, prefix, libcode)
    new_code_nums = set((libcode, num)
                        for (code, (libcode, name, num)) in new_codes)

    # Merge with existing codes (in case some old codes disappeared).
    codes = {}
    for errname, (libnum, errnum) in _ssl.err_names_to_codes.items():
        lib = error_libraries[_ssl.lib_codes_to_names[libnum]]
        libcode = lib[0]              # e.g. ERR_LIB_PEM
        errcode = lib[1] + errname    # e.g. SSL_R_BAD_SSL_SESSION_ID_LENGTH
        # Only keep it if the numeric codes weren't reused
        if (libcode, errnum) not in new_code_nums:
            codes[errcode] = libcode, errname, errnum
    codes.update(dict(new_codes))

    def w(l):
        f.write(l + "\n")
    w("/* File generated by Tools/ssl/make_ssl_data.py */")
    w("/* Generated on %s */" % datetime.datetime.now().isoformat())
    w("")

    w("static struct py_ssl_library_code library_codes[] = {")
    for mnemo, (libcode, _, _) in sorted(error_libraries.items()):
        w(f'#ifdef {libcode}')
        w('    {"%s", %s},' % (mnemo, libcode))
        w('#endif')
    w('    { NULL }')
    w('};')
    w("")

    w("static struct py_ssl_error_code error_codes[] = {")
    for errcode, (libcode, name, num) in sorted(codes.items()):
        w('  #ifdef %s' % (errcode))
        w('    {"%s", %s, %s},' % (name, libcode, errcode))
        w('  #else')
        w('    {"%s", %s, %d},' % (name, lib_codes[libcode], num))
        w('  #endif')
    w('    { NULL }')
    w('};')
    if not use_stdout:
        f.close()
