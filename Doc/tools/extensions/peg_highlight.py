from pygments.lexer import RegexLexer, bygroups, include
from pygments.token import (Comment, Generic, Keyword, Name, Operator,
                            Punctuation, Text)

from sphinx.highlighting import lexers

class PEGLexer(RegexLexer):
    """Pygments Lexer for PEG grammar (.gram) files

    This lexer stripts the following elements from the grammar:

        - Meta-tags
        - Variable assignments
        - Actions
        - Lookaheads
        - Rule types
        - Rule options
    """
    name = "PEG"
    aliases = ["peg"]
    filenames = ["*.gram"]
    _name = r"([^\W\d]\w*)"
    _text_ws = r"(\s*)"

    tokens = {
        "ws": [
            (r"\n", Text),
            (r"\s+", Text),
            (r"#.*$", Comment.Singleline),
        ],
        "lookaheads": [
            (r"(&\w+\s?)", bygroups(None)),
            (r"(&'.+'\s?)", bygroups(None)),
            (r'(&".+"\s?)', bygroups(None)),
            (r"(&\(.+\)\s?)", bygroups(None)),
            (r"(!\w+\s?)", bygroups(None)),
            (r"(!'.+'\s?)", bygroups(None)),
            (r'(!".+"\s?)', bygroups(None)),
            (r"(!\(.+\)\s?)", bygroups(None)),
        ],
        "metas": [
            (r"(@\w+ '''(.|\n)+?''')", bygroups(None)),
            (r"^(@.*)$", bygroups(None)),
        ],
        "actions": [
            (r"{(.|\n)+?}", bygroups(None)),
        ],
        "strings": [
            (r"'\w+?'", Keyword),
            (r'"\w+?"', Keyword),
            (r"'\W+?'", Text),
            (r'"\W+?"', Text),
        ],
        "variables": [
            (
                _name + _text_ws + "(=)",
                bygroups(None, None, None),
            ),
        ],
        "root": [
            include("ws"),
            include("lookaheads"),
            include("metas"),
            include("actions"),
            include("strings"),
            include("variables"),
            (
                r"\b(?!(NULL|EXTRA))([A-Z_]+)\b\s*(?!\()",
                Text,
            ),
            (
                r"^\s*" + _name + "\s*" + '(\[.*\])?' + "\s*" + '(\(.+\))?' + "\s*(:)",
                bygroups(Name.Function, None, None, Punctuation)
            ),
            (_name, Name.Function),
            (r"[\||\.|\+|\*|\?]", Operator),
            (r"{|}|\(|\)|\[|\]", Punctuation),
            (r".", Text),
        ],
    }


def setup(app):
    lexers["peg"] = PEGLexer()
    return {'version': '1.0', 'parallel_read_safe': True}
