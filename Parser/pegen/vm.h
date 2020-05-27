typedef enum _opcodes {
    OP_NAME,
    OP_NUMBER,
    OP_STRING,
    OP_CUT,
    OP_SUCCESS,
    OP_FAILURE,
    // The rest have an argument
    OP_TOKEN,
    OP_RULE,
    OP_RETURN,
} Opcode;

char *opcode_names[] = {
    "OP_NAME",
    "OP_NUMBER",
    "OP_STRING",
    "OP_CUT",
    "OP_SUCCESS",
    "OP_FAILURE",
    // The rest have an argument
    "OP_TOKEN",
    "OP_RULE",
    "OP_RETURN",
};

typedef struct _rule {
    char *name;
    int alts[10];
    int opcodes[100];
} Rule;

typedef struct _frame {
    Rule *rule;
    int ialt;
    int iop;
    int cut;
    int mark;
} Frame;

typedef struct _stack {
    Parser *p;
    int top;
    Frame frames[100];
} Stack;

/* Toy grammar:

start: expr ENDMARKER
expr: term '+' expr | term
term: NAME | NUMBER

*/

static const int n_keyword_lists = 0;
static KeywordToken *reserved_keywords[] = {
};

static Rule all_rules[] = {

    {"start",
     {0, 6, -1},
     {
      OP_RULE, 1, OP_TOKEN, NEWLINE, OP_SUCCESS, 0,
      OP_FAILURE,
     },
    },

    {"expr",
     {0, 8, -1},
     {
      OP_RULE, 2, OP_TOKEN, PLUS, OP_RULE, 1, OP_RETURN, 0,
      OP_RULE, 2, OP_RETURN, 0,
     },
    },

    {"term",
     {0, 3, -1},
     {
      OP_NAME, OP_RETURN, 0,
      OP_NUMBER, OP_RETURN, 0,
     },
    },

};
