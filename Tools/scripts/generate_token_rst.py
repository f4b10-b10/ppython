# This script generates the token-list.inc documentation file.

header = """\
.. Auto-generated by Tools/scripts/generate_token_rst.py
.. data::
"""


def main(token_py='Lib/token.py', outfile='Doc/library/token-list.inc'):
    token = {}
    with open(token_py) as fp:
        code = fp.read()
    exec(code, token)
    tok_name = token['tok_name']
    with open(outfile, 'w') as fobj:
        fobj.write(header)
        for value in sorted(tok_name):
            if token['ERRORTOKEN'] < value < token['N_TOKENS']:
                continue
            name = tok_name[value]
            fobj.write("   %s\n" % (name,))

    print("%s regenerated from %s" % (outfile, token_py))


if __name__ == '__main__':
    import sys
    main(*sys.argv[1:])
