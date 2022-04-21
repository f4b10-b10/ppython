/* Auto-generated by Tools/scripts/generate_opcode_h.py from Lib/opcode.py */
#ifndef Py_OPCODE_H
#define Py_OPCODE_H
#ifdef __cplusplus
extern "C" {
#endif


/* Instruction opcodes for compiled code */
#define CACHE                                    0
#define POP_TOP                                  1
#define PUSH_NULL                                2
#define NOP                                      9
#define UNARY_POSITIVE                          10
#define UNARY_NEGATIVE                          11
#define UNARY_NOT                               12
#define UNARY_INVERT                            15
#define BINARY_SUBSCR                           25
#define GET_LEN                                 30
#define MATCH_MAPPING                           31
#define MATCH_SEQUENCE                          32
#define MATCH_KEYS                              33
#define PUSH_EXC_INFO                           35
#define CHECK_EXC_MATCH                         36
#define CHECK_EG_MATCH                          37
#define WITH_EXCEPT_START                       49
#define GET_AITER                               50
#define GET_ANEXT                               51
#define BEFORE_ASYNC_WITH                       52
#define BEFORE_WITH                             53
#define END_ASYNC_FOR                           54
#define STORE_SUBSCR                            60
#define DELETE_SUBSCR                           61
#define GET_ITER                                68
#define GET_YIELD_FROM_ITER                     69
#define PRINT_EXPR                              70
#define LOAD_BUILD_CLASS                        71
#define LOAD_ASSERTION_ERROR                    74
#define RETURN_GENERATOR                        75
#define LIST_TO_TUPLE                           82
#define RETURN_VALUE                            83
#define IMPORT_STAR                             84
#define SETUP_ANNOTATIONS                       85
#define YIELD_VALUE                             86
#define ASYNC_GEN_WRAP                          87
#define PREP_RERAISE_STAR                       88
#define POP_EXCEPT                              89
#define HAVE_ARGUMENT                           90
#define STORE_NAME                              90
#define DELETE_NAME                             91
#define UNPACK_SEQUENCE                         92
#define FOR_ITER                                93
#define UNPACK_EX                               94
#define STORE_ATTR                              95
#define DELETE_ATTR                             96
#define STORE_GLOBAL                            97
#define DELETE_GLOBAL                           98
#define SWAP                                    99
#define LOAD_CONST                             100
#define LOAD_NAME                              101
#define BUILD_TUPLE                            102
#define BUILD_LIST                             103
#define BUILD_SET                              104
#define BUILD_MAP                              105
#define LOAD_ATTR                              106
#define COMPARE_OP                             107
#define IMPORT_NAME                            108
#define IMPORT_FROM                            109
#define JUMP_FORWARD                           110
#define JUMP_IF_FALSE_OR_POP                   111
#define JUMP_IF_TRUE_OR_POP                    112
#define POP_JUMP_FORWARD_IF_FALSE              114
#define POP_JUMP_FORWARD_IF_TRUE               115
#define LOAD_GLOBAL                            116
#define IS_OP                                  117
#define CONTAINS_OP                            118
#define RERAISE                                119
#define COPY                                   120
#define BINARY_OP                              122
#define SEND                                   123
#define LOAD_FAST                              124
#define STORE_FAST                             125
#define DELETE_FAST                            126
#define POP_JUMP_FORWARD_IF_NOT_NONE           128
#define POP_JUMP_FORWARD_IF_NONE               129
#define RAISE_VARARGS                          130
#define GET_AWAITABLE                          131
#define MAKE_FUNCTION                          132
#define BUILD_SLICE                            133
#define JUMP_BACKWARD_NO_INTERRUPT             134
#define MAKE_CELL                              135
#define LOAD_CLOSURE                           136
#define LOAD_DEREF                             137
#define STORE_DEREF                            138
#define DELETE_DEREF                           139
#define JUMP_BACKWARD                          140
#define CALL_FUNCTION_EX                       142
#define EXTENDED_ARG                           144
#define LIST_APPEND                            145
#define SET_ADD                                146
#define MAP_ADD                                147
#define LOAD_CLASSDEREF                        148
#define COPY_FREE_VARS                         149
#define RESUME                                 151
#define MATCH_CLASS                            152
#define FORMAT_VALUE                           155
#define BUILD_CONST_KEY_MAP                    156
#define BUILD_STRING                           157
#define LOAD_METHOD                            160
#define LIST_EXTEND                            162
#define SET_UPDATE                             163
#define DICT_MERGE                             164
#define DICT_UPDATE                            165
#define PRECALL                                166
#define CALL                                   171
#define KW_NAMES                               172
#define POP_JUMP_BACKWARD_IF_NOT_NONE          173
#define POP_JUMP_BACKWARD_IF_NONE              174
#define POP_JUMP_BACKWARD_IF_FALSE             175
#define POP_JUMP_BACKWARD_IF_TRUE              176
#define BINARY_OP_ADAPTIVE                       3
#define BINARY_OP_ADD_FLOAT                      4
#define BINARY_OP_ADD_INT                        5
#define BINARY_OP_ADD_UNICODE                    6
#define BINARY_OP_INPLACE_ADD_UNICODE            7
#define BINARY_OP_MULTIPLY_FLOAT                 8
#define BINARY_OP_MULTIPLY_INT                  13
#define BINARY_OP_SUBTRACT_FLOAT                14
#define BINARY_OP_SUBTRACT_INT                  16
#define BINARY_SUBSCR_ADAPTIVE                  17
#define BINARY_SUBSCR_DICT                      18
#define BINARY_SUBSCR_GETITEM                   19
#define BINARY_SUBSCR_LIST_INT                  20
#define BINARY_SUBSCR_TUPLE_INT                 21
#define CALL_ADAPTIVE                           22
#define CALL_PY_EXACT_ARGS                      23
#define CALL_PY_WITH_DEFAULTS                   24
#define COMPARE_OP_ADAPTIVE                     26
#define COMPARE_OP_FLOAT_JUMP                   27
#define COMPARE_OP_INT_JUMP                     28
#define COMPARE_OP_STR_JUMP                     29
#define JUMP_BACKWARD_QUICK                     34
#define LOAD_ATTR_ADAPTIVE                      38
#define LOAD_ATTR_INSTANCE_VALUE                39
#define LOAD_ATTR_MODULE                        40
#define LOAD_ATTR_SLOT                          41
#define LOAD_ATTR_WITH_HINT                     42
#define LOAD_CONST__LOAD_FAST                   43
#define LOAD_FAST__LOAD_CONST                   44
#define LOAD_FAST__LOAD_FAST                    45
#define LOAD_GLOBAL_ADAPTIVE                    46
#define LOAD_GLOBAL_BUILTIN                     47
#define LOAD_GLOBAL_MODULE                      48
#define LOAD_METHOD_ADAPTIVE                    55
#define LOAD_METHOD_CLASS                       56
#define LOAD_METHOD_MODULE                      57
#define LOAD_METHOD_NO_DICT                     58
#define LOAD_METHOD_WITH_DICT                   59
#define LOAD_METHOD_WITH_VALUES                 62
#define PRECALL_ADAPTIVE                        63
#define PRECALL_BOUND_METHOD                    64
#define PRECALL_BUILTIN_CLASS                   65
#define PRECALL_BUILTIN_FAST_WITH_KEYWORDS      66
#define PRECALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS  67
#define PRECALL_NO_KW_BUILTIN_FAST              72
#define PRECALL_NO_KW_BUILTIN_O                 73
#define PRECALL_NO_KW_ISINSTANCE                76
#define PRECALL_NO_KW_LEN                       77
#define PRECALL_NO_KW_LIST_APPEND               78
#define PRECALL_NO_KW_METHOD_DESCRIPTOR_FAST    79
#define PRECALL_NO_KW_METHOD_DESCRIPTOR_NOARGS  80
#define PRECALL_NO_KW_METHOD_DESCRIPTOR_O       81
#define PRECALL_NO_KW_STR_1                    113
#define PRECALL_NO_KW_TUPLE_1                  121
#define PRECALL_NO_KW_TYPE_1                   127
#define PRECALL_PYFUNC                         141
#define RESUME_QUICK                           143
#define STORE_ATTR_ADAPTIVE                    150
#define STORE_ATTR_INSTANCE_VALUE              153
#define STORE_ATTR_SLOT                        154
#define STORE_ATTR_WITH_HINT                   158
#define STORE_FAST__LOAD_FAST                  159
#define STORE_FAST__STORE_FAST                 161
#define STORE_SUBSCR_ADAPTIVE                  167
#define STORE_SUBSCR_DICT                      168
#define STORE_SUBSCR_LIST_INT                  169
#define UNPACK_SEQUENCE_ADAPTIVE               170
#define UNPACK_SEQUENCE_LIST                   177
#define UNPACK_SEQUENCE_TUPLE                  178
#define UNPACK_SEQUENCE_TWO_TUPLE              179
#define DO_TRACING                             255

extern const uint8_t _PyOpcode_Caches[256];

extern const uint8_t _PyOpcode_Deopt[256];

#ifdef NEED_OPCODE_TABLES
static const uint32_t _PyOpcode_RelativeJump[8] = {
    0U,
    0U,
    536870912U,
    135118848U,
    4163U,
    122880U,
    0U,
    0U,
};
static const uint32_t _PyOpcode_Jump[8] = {
    0U,
    0U,
    536870912U,
    135118848U,
    4163U,
    122880U,
    0U,
    0U,
};

const uint8_t _PyOpcode_Caches[256] = {
    [BINARY_SUBSCR] = 4,
    [STORE_SUBSCR] = 1,
    [UNPACK_SEQUENCE] = 1,
    [STORE_ATTR] = 4,
    [LOAD_ATTR] = 4,
    [COMPARE_OP] = 2,
    [LOAD_GLOBAL] = 5,
    [BINARY_OP] = 1,
    [LOAD_METHOD] = 10,
    [PRECALL] = 1,
    [CALL] = 4,
};

const uint8_t _PyOpcode_Deopt[256] = {
    [ASYNC_GEN_WRAP] = ASYNC_GEN_WRAP,
    [BEFORE_ASYNC_WITH] = BEFORE_ASYNC_WITH,
    [BEFORE_WITH] = BEFORE_WITH,
    [BINARY_OP] = BINARY_OP,
    [BINARY_OP_ADAPTIVE] = BINARY_OP,
    [BINARY_OP_ADD_FLOAT] = BINARY_OP,
    [BINARY_OP_ADD_INT] = BINARY_OP,
    [BINARY_OP_ADD_UNICODE] = BINARY_OP,
    [BINARY_OP_INPLACE_ADD_UNICODE] = BINARY_OP,
    [BINARY_OP_MULTIPLY_FLOAT] = BINARY_OP,
    [BINARY_OP_MULTIPLY_INT] = BINARY_OP,
    [BINARY_OP_SUBTRACT_FLOAT] = BINARY_OP,
    [BINARY_OP_SUBTRACT_INT] = BINARY_OP,
    [BINARY_SUBSCR] = BINARY_SUBSCR,
    [BINARY_SUBSCR_ADAPTIVE] = BINARY_SUBSCR,
    [BINARY_SUBSCR_DICT] = BINARY_SUBSCR,
    [BINARY_SUBSCR_GETITEM] = BINARY_SUBSCR,
    [BINARY_SUBSCR_LIST_INT] = BINARY_SUBSCR,
    [BINARY_SUBSCR_TUPLE_INT] = BINARY_SUBSCR,
    [BUILD_CONST_KEY_MAP] = BUILD_CONST_KEY_MAP,
    [BUILD_LIST] = BUILD_LIST,
    [BUILD_MAP] = BUILD_MAP,
    [BUILD_SET] = BUILD_SET,
    [BUILD_SLICE] = BUILD_SLICE,
    [BUILD_STRING] = BUILD_STRING,
    [BUILD_TUPLE] = BUILD_TUPLE,
    [CACHE] = CACHE,
    [CALL] = CALL,
    [CALL_ADAPTIVE] = CALL,
    [CALL_FUNCTION_EX] = CALL_FUNCTION_EX,
    [CALL_PY_EXACT_ARGS] = CALL,
    [CALL_PY_WITH_DEFAULTS] = CALL,
    [CHECK_EG_MATCH] = CHECK_EG_MATCH,
    [CHECK_EXC_MATCH] = CHECK_EXC_MATCH,
    [COMPARE_OP] = COMPARE_OP,
    [COMPARE_OP_ADAPTIVE] = COMPARE_OP,
    [COMPARE_OP_FLOAT_JUMP] = COMPARE_OP,
    [COMPARE_OP_INT_JUMP] = COMPARE_OP,
    [COMPARE_OP_STR_JUMP] = COMPARE_OP,
    [CONTAINS_OP] = CONTAINS_OP,
    [COPY] = COPY,
    [COPY_FREE_VARS] = COPY_FREE_VARS,
    [DELETE_ATTR] = DELETE_ATTR,
    [DELETE_DEREF] = DELETE_DEREF,
    [DELETE_FAST] = DELETE_FAST,
    [DELETE_GLOBAL] = DELETE_GLOBAL,
    [DELETE_NAME] = DELETE_NAME,
    [DELETE_SUBSCR] = DELETE_SUBSCR,
    [DICT_MERGE] = DICT_MERGE,
    [DICT_UPDATE] = DICT_UPDATE,
    [END_ASYNC_FOR] = END_ASYNC_FOR,
    [EXTENDED_ARG] = EXTENDED_ARG,
    [FORMAT_VALUE] = FORMAT_VALUE,
    [FOR_ITER] = FOR_ITER,
    [GET_AITER] = GET_AITER,
    [GET_ANEXT] = GET_ANEXT,
    [GET_AWAITABLE] = GET_AWAITABLE,
    [GET_ITER] = GET_ITER,
    [GET_LEN] = GET_LEN,
    [GET_YIELD_FROM_ITER] = GET_YIELD_FROM_ITER,
    [IMPORT_FROM] = IMPORT_FROM,
    [IMPORT_NAME] = IMPORT_NAME,
    [IMPORT_STAR] = IMPORT_STAR,
    [IS_OP] = IS_OP,
    [JUMP_BACKWARD] = JUMP_BACKWARD,
    [JUMP_BACKWARD_NO_INTERRUPT] = JUMP_BACKWARD_NO_INTERRUPT,
    [JUMP_BACKWARD_QUICK] = JUMP_BACKWARD,
    [JUMP_FORWARD] = JUMP_FORWARD,
    [JUMP_IF_FALSE_OR_POP] = JUMP_IF_FALSE_OR_POP,
    [JUMP_IF_TRUE_OR_POP] = JUMP_IF_TRUE_OR_POP,
    [KW_NAMES] = KW_NAMES,
    [LIST_APPEND] = LIST_APPEND,
    [LIST_EXTEND] = LIST_EXTEND,
    [LIST_TO_TUPLE] = LIST_TO_TUPLE,
    [LOAD_ASSERTION_ERROR] = LOAD_ASSERTION_ERROR,
    [LOAD_ATTR] = LOAD_ATTR,
    [LOAD_ATTR_ADAPTIVE] = LOAD_ATTR,
    [LOAD_ATTR_INSTANCE_VALUE] = LOAD_ATTR,
    [LOAD_ATTR_MODULE] = LOAD_ATTR,
    [LOAD_ATTR_SLOT] = LOAD_ATTR,
    [LOAD_ATTR_WITH_HINT] = LOAD_ATTR,
    [LOAD_BUILD_CLASS] = LOAD_BUILD_CLASS,
    [LOAD_CLASSDEREF] = LOAD_CLASSDEREF,
    [LOAD_CLOSURE] = LOAD_CLOSURE,
    [LOAD_CONST] = LOAD_CONST,
    [LOAD_CONST__LOAD_FAST] = LOAD_CONST,
    [LOAD_DEREF] = LOAD_DEREF,
    [LOAD_FAST] = LOAD_FAST,
    [LOAD_FAST__LOAD_CONST] = LOAD_FAST,
    [LOAD_FAST__LOAD_FAST] = LOAD_FAST,
    [LOAD_GLOBAL] = LOAD_GLOBAL,
    [LOAD_GLOBAL_ADAPTIVE] = LOAD_GLOBAL,
    [LOAD_GLOBAL_BUILTIN] = LOAD_GLOBAL,
    [LOAD_GLOBAL_MODULE] = LOAD_GLOBAL,
    [LOAD_METHOD] = LOAD_METHOD,
    [LOAD_METHOD_ADAPTIVE] = LOAD_METHOD,
    [LOAD_METHOD_CLASS] = LOAD_METHOD,
    [LOAD_METHOD_MODULE] = LOAD_METHOD,
    [LOAD_METHOD_NO_DICT] = LOAD_METHOD,
    [LOAD_METHOD_WITH_DICT] = LOAD_METHOD,
    [LOAD_METHOD_WITH_VALUES] = LOAD_METHOD,
    [LOAD_NAME] = LOAD_NAME,
    [MAKE_CELL] = MAKE_CELL,
    [MAKE_FUNCTION] = MAKE_FUNCTION,
    [MAP_ADD] = MAP_ADD,
    [MATCH_CLASS] = MATCH_CLASS,
    [MATCH_KEYS] = MATCH_KEYS,
    [MATCH_MAPPING] = MATCH_MAPPING,
    [MATCH_SEQUENCE] = MATCH_SEQUENCE,
    [NOP] = NOP,
    [POP_EXCEPT] = POP_EXCEPT,
    [POP_JUMP_BACKWARD_IF_FALSE] = POP_JUMP_BACKWARD_IF_FALSE,
    [POP_JUMP_BACKWARD_IF_NONE] = POP_JUMP_BACKWARD_IF_NONE,
    [POP_JUMP_BACKWARD_IF_NOT_NONE] = POP_JUMP_BACKWARD_IF_NOT_NONE,
    [POP_JUMP_BACKWARD_IF_TRUE] = POP_JUMP_BACKWARD_IF_TRUE,
    [POP_JUMP_FORWARD_IF_FALSE] = POP_JUMP_FORWARD_IF_FALSE,
    [POP_JUMP_FORWARD_IF_NONE] = POP_JUMP_FORWARD_IF_NONE,
    [POP_JUMP_FORWARD_IF_NOT_NONE] = POP_JUMP_FORWARD_IF_NOT_NONE,
    [POP_JUMP_FORWARD_IF_TRUE] = POP_JUMP_FORWARD_IF_TRUE,
    [POP_TOP] = POP_TOP,
    [PRECALL] = PRECALL,
    [PRECALL_ADAPTIVE] = PRECALL,
    [PRECALL_BOUND_METHOD] = PRECALL,
    [PRECALL_BUILTIN_CLASS] = PRECALL,
    [PRECALL_BUILTIN_FAST_WITH_KEYWORDS] = PRECALL,
    [PRECALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS] = PRECALL,
    [PRECALL_NO_KW_BUILTIN_FAST] = PRECALL,
    [PRECALL_NO_KW_BUILTIN_O] = PRECALL,
    [PRECALL_NO_KW_ISINSTANCE] = PRECALL,
    [PRECALL_NO_KW_LEN] = PRECALL,
    [PRECALL_NO_KW_LIST_APPEND] = PRECALL,
    [PRECALL_NO_KW_METHOD_DESCRIPTOR_FAST] = PRECALL,
    [PRECALL_NO_KW_METHOD_DESCRIPTOR_NOARGS] = PRECALL,
    [PRECALL_NO_KW_METHOD_DESCRIPTOR_O] = PRECALL,
    [PRECALL_NO_KW_STR_1] = PRECALL,
    [PRECALL_NO_KW_TUPLE_1] = PRECALL,
    [PRECALL_NO_KW_TYPE_1] = PRECALL,
    [PRECALL_PYFUNC] = PRECALL,
    [PREP_RERAISE_STAR] = PREP_RERAISE_STAR,
    [PRINT_EXPR] = PRINT_EXPR,
    [PUSH_EXC_INFO] = PUSH_EXC_INFO,
    [PUSH_NULL] = PUSH_NULL,
    [RAISE_VARARGS] = RAISE_VARARGS,
    [RERAISE] = RERAISE,
    [RESUME] = RESUME,
    [RESUME_QUICK] = RESUME,
    [RETURN_GENERATOR] = RETURN_GENERATOR,
    [RETURN_VALUE] = RETURN_VALUE,
    [SEND] = SEND,
    [SETUP_ANNOTATIONS] = SETUP_ANNOTATIONS,
    [SET_ADD] = SET_ADD,
    [SET_UPDATE] = SET_UPDATE,
    [STORE_ATTR] = STORE_ATTR,
    [STORE_ATTR_ADAPTIVE] = STORE_ATTR,
    [STORE_ATTR_INSTANCE_VALUE] = STORE_ATTR,
    [STORE_ATTR_SLOT] = STORE_ATTR,
    [STORE_ATTR_WITH_HINT] = STORE_ATTR,
    [STORE_DEREF] = STORE_DEREF,
    [STORE_FAST] = STORE_FAST,
    [STORE_FAST__LOAD_FAST] = STORE_FAST,
    [STORE_FAST__STORE_FAST] = STORE_FAST,
    [STORE_GLOBAL] = STORE_GLOBAL,
    [STORE_NAME] = STORE_NAME,
    [STORE_SUBSCR] = STORE_SUBSCR,
    [STORE_SUBSCR_ADAPTIVE] = STORE_SUBSCR,
    [STORE_SUBSCR_DICT] = STORE_SUBSCR,
    [STORE_SUBSCR_LIST_INT] = STORE_SUBSCR,
    [SWAP] = SWAP,
    [UNARY_INVERT] = UNARY_INVERT,
    [UNARY_NEGATIVE] = UNARY_NEGATIVE,
    [UNARY_NOT] = UNARY_NOT,
    [UNARY_POSITIVE] = UNARY_POSITIVE,
    [UNPACK_EX] = UNPACK_EX,
    [UNPACK_SEQUENCE] = UNPACK_SEQUENCE,
    [UNPACK_SEQUENCE_ADAPTIVE] = UNPACK_SEQUENCE,
    [UNPACK_SEQUENCE_LIST] = UNPACK_SEQUENCE,
    [UNPACK_SEQUENCE_TUPLE] = UNPACK_SEQUENCE,
    [UNPACK_SEQUENCE_TWO_TUPLE] = UNPACK_SEQUENCE,
    [WITH_EXCEPT_START] = WITH_EXCEPT_START,
    [YIELD_VALUE] = YIELD_VALUE,
};
#endif /* OPCODE_TABLES */

#define HAS_CONST(op) (false\
    || ((op) == 100) \
    || ((op) == 172) \
    )

#define NB_ADD                                   0
#define NB_AND                                   1
#define NB_FLOOR_DIVIDE                          2
#define NB_LSHIFT                                3
#define NB_MATRIX_MULTIPLY                       4
#define NB_MULTIPLY                              5
#define NB_REMAINDER                             6
#define NB_OR                                    7
#define NB_POWER                                 8
#define NB_RSHIFT                                9
#define NB_SUBTRACT                             10
#define NB_TRUE_DIVIDE                          11
#define NB_XOR                                  12
#define NB_INPLACE_ADD                          13
#define NB_INPLACE_AND                          14
#define NB_INPLACE_FLOOR_DIVIDE                 15
#define NB_INPLACE_LSHIFT                       16
#define NB_INPLACE_MATRIX_MULTIPLY              17
#define NB_INPLACE_MULTIPLY                     18
#define NB_INPLACE_REMAINDER                    19
#define NB_INPLACE_OR                           20
#define NB_INPLACE_POWER                        21
#define NB_INPLACE_RSHIFT                       22
#define NB_INPLACE_SUBTRACT                     23
#define NB_INPLACE_TRUE_DIVIDE                  24
#define NB_INPLACE_XOR                          25

#ifdef Py_DEBUG
static const char *const _PyOpcode_OpName[256] = {
    [CACHE] = "CACHE",
    [POP_TOP] = "POP_TOP",
    [PUSH_NULL] = "PUSH_NULL",
    [BINARY_OP_ADAPTIVE] = "BINARY_OP_ADAPTIVE",
    [BINARY_OP_ADD_FLOAT] = "BINARY_OP_ADD_FLOAT",
    [BINARY_OP_ADD_INT] = "BINARY_OP_ADD_INT",
    [BINARY_OP_ADD_UNICODE] = "BINARY_OP_ADD_UNICODE",
    [BINARY_OP_INPLACE_ADD_UNICODE] = "BINARY_OP_INPLACE_ADD_UNICODE",
    [BINARY_OP_MULTIPLY_FLOAT] = "BINARY_OP_MULTIPLY_FLOAT",
    [NOP] = "NOP",
    [UNARY_POSITIVE] = "UNARY_POSITIVE",
    [UNARY_NEGATIVE] = "UNARY_NEGATIVE",
    [UNARY_NOT] = "UNARY_NOT",
    [BINARY_OP_MULTIPLY_INT] = "BINARY_OP_MULTIPLY_INT",
    [BINARY_OP_SUBTRACT_FLOAT] = "BINARY_OP_SUBTRACT_FLOAT",
    [UNARY_INVERT] = "UNARY_INVERT",
    [BINARY_OP_SUBTRACT_INT] = "BINARY_OP_SUBTRACT_INT",
    [BINARY_SUBSCR_ADAPTIVE] = "BINARY_SUBSCR_ADAPTIVE",
    [BINARY_SUBSCR_DICT] = "BINARY_SUBSCR_DICT",
    [BINARY_SUBSCR_GETITEM] = "BINARY_SUBSCR_GETITEM",
    [BINARY_SUBSCR_LIST_INT] = "BINARY_SUBSCR_LIST_INT",
    [BINARY_SUBSCR_TUPLE_INT] = "BINARY_SUBSCR_TUPLE_INT",
    [CALL_ADAPTIVE] = "CALL_ADAPTIVE",
    [CALL_PY_EXACT_ARGS] = "CALL_PY_EXACT_ARGS",
    [CALL_PY_WITH_DEFAULTS] = "CALL_PY_WITH_DEFAULTS",
    [BINARY_SUBSCR] = "BINARY_SUBSCR",
    [COMPARE_OP_ADAPTIVE] = "COMPARE_OP_ADAPTIVE",
    [COMPARE_OP_FLOAT_JUMP] = "COMPARE_OP_FLOAT_JUMP",
    [COMPARE_OP_INT_JUMP] = "COMPARE_OP_INT_JUMP",
    [COMPARE_OP_STR_JUMP] = "COMPARE_OP_STR_JUMP",
    [GET_LEN] = "GET_LEN",
    [MATCH_MAPPING] = "MATCH_MAPPING",
    [MATCH_SEQUENCE] = "MATCH_SEQUENCE",
    [MATCH_KEYS] = "MATCH_KEYS",
    [JUMP_BACKWARD_QUICK] = "JUMP_BACKWARD_QUICK",
    [PUSH_EXC_INFO] = "PUSH_EXC_INFO",
    [CHECK_EXC_MATCH] = "CHECK_EXC_MATCH",
    [CHECK_EG_MATCH] = "CHECK_EG_MATCH",
    [LOAD_ATTR_ADAPTIVE] = "LOAD_ATTR_ADAPTIVE",
    [LOAD_ATTR_INSTANCE_VALUE] = "LOAD_ATTR_INSTANCE_VALUE",
    [LOAD_ATTR_MODULE] = "LOAD_ATTR_MODULE",
    [LOAD_ATTR_SLOT] = "LOAD_ATTR_SLOT",
    [LOAD_ATTR_WITH_HINT] = "LOAD_ATTR_WITH_HINT",
    [LOAD_CONST__LOAD_FAST] = "LOAD_CONST__LOAD_FAST",
    [LOAD_FAST__LOAD_CONST] = "LOAD_FAST__LOAD_CONST",
    [LOAD_FAST__LOAD_FAST] = "LOAD_FAST__LOAD_FAST",
    [LOAD_GLOBAL_ADAPTIVE] = "LOAD_GLOBAL_ADAPTIVE",
    [LOAD_GLOBAL_BUILTIN] = "LOAD_GLOBAL_BUILTIN",
    [LOAD_GLOBAL_MODULE] = "LOAD_GLOBAL_MODULE",
    [WITH_EXCEPT_START] = "WITH_EXCEPT_START",
    [GET_AITER] = "GET_AITER",
    [GET_ANEXT] = "GET_ANEXT",
    [BEFORE_ASYNC_WITH] = "BEFORE_ASYNC_WITH",
    [BEFORE_WITH] = "BEFORE_WITH",
    [END_ASYNC_FOR] = "END_ASYNC_FOR",
    [LOAD_METHOD_ADAPTIVE] = "LOAD_METHOD_ADAPTIVE",
    [LOAD_METHOD_CLASS] = "LOAD_METHOD_CLASS",
    [LOAD_METHOD_MODULE] = "LOAD_METHOD_MODULE",
    [LOAD_METHOD_NO_DICT] = "LOAD_METHOD_NO_DICT",
    [LOAD_METHOD_WITH_DICT] = "LOAD_METHOD_WITH_DICT",
    [STORE_SUBSCR] = "STORE_SUBSCR",
    [DELETE_SUBSCR] = "DELETE_SUBSCR",
    [LOAD_METHOD_WITH_VALUES] = "LOAD_METHOD_WITH_VALUES",
    [PRECALL_ADAPTIVE] = "PRECALL_ADAPTIVE",
    [PRECALL_BOUND_METHOD] = "PRECALL_BOUND_METHOD",
    [PRECALL_BUILTIN_CLASS] = "PRECALL_BUILTIN_CLASS",
    [PRECALL_BUILTIN_FAST_WITH_KEYWORDS] = "PRECALL_BUILTIN_FAST_WITH_KEYWORDS",
    [PRECALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS] = "PRECALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS",
    [GET_ITER] = "GET_ITER",
    [GET_YIELD_FROM_ITER] = "GET_YIELD_FROM_ITER",
    [PRINT_EXPR] = "PRINT_EXPR",
    [LOAD_BUILD_CLASS] = "LOAD_BUILD_CLASS",
    [PRECALL_NO_KW_BUILTIN_FAST] = "PRECALL_NO_KW_BUILTIN_FAST",
    [PRECALL_NO_KW_BUILTIN_O] = "PRECALL_NO_KW_BUILTIN_O",
    [LOAD_ASSERTION_ERROR] = "LOAD_ASSERTION_ERROR",
    [RETURN_GENERATOR] = "RETURN_GENERATOR",
    [PRECALL_NO_KW_ISINSTANCE] = "PRECALL_NO_KW_ISINSTANCE",
    [PRECALL_NO_KW_LEN] = "PRECALL_NO_KW_LEN",
    [PRECALL_NO_KW_LIST_APPEND] = "PRECALL_NO_KW_LIST_APPEND",
    [PRECALL_NO_KW_METHOD_DESCRIPTOR_FAST] = "PRECALL_NO_KW_METHOD_DESCRIPTOR_FAST",
    [PRECALL_NO_KW_METHOD_DESCRIPTOR_NOARGS] = "PRECALL_NO_KW_METHOD_DESCRIPTOR_NOARGS",
    [PRECALL_NO_KW_METHOD_DESCRIPTOR_O] = "PRECALL_NO_KW_METHOD_DESCRIPTOR_O",
    [LIST_TO_TUPLE] = "LIST_TO_TUPLE",
    [RETURN_VALUE] = "RETURN_VALUE",
    [IMPORT_STAR] = "IMPORT_STAR",
    [SETUP_ANNOTATIONS] = "SETUP_ANNOTATIONS",
    [YIELD_VALUE] = "YIELD_VALUE",
    [ASYNC_GEN_WRAP] = "ASYNC_GEN_WRAP",
    [PREP_RERAISE_STAR] = "PREP_RERAISE_STAR",
    [POP_EXCEPT] = "POP_EXCEPT",
    [STORE_NAME] = "STORE_NAME",
    [DELETE_NAME] = "DELETE_NAME",
    [UNPACK_SEQUENCE] = "UNPACK_SEQUENCE",
    [FOR_ITER] = "FOR_ITER",
    [UNPACK_EX] = "UNPACK_EX",
    [STORE_ATTR] = "STORE_ATTR",
    [DELETE_ATTR] = "DELETE_ATTR",
    [STORE_GLOBAL] = "STORE_GLOBAL",
    [DELETE_GLOBAL] = "DELETE_GLOBAL",
    [SWAP] = "SWAP",
    [LOAD_CONST] = "LOAD_CONST",
    [LOAD_NAME] = "LOAD_NAME",
    [BUILD_TUPLE] = "BUILD_TUPLE",
    [BUILD_LIST] = "BUILD_LIST",
    [BUILD_SET] = "BUILD_SET",
    [BUILD_MAP] = "BUILD_MAP",
    [LOAD_ATTR] = "LOAD_ATTR",
    [COMPARE_OP] = "COMPARE_OP",
    [IMPORT_NAME] = "IMPORT_NAME",
    [IMPORT_FROM] = "IMPORT_FROM",
    [JUMP_FORWARD] = "JUMP_FORWARD",
    [JUMP_IF_FALSE_OR_POP] = "JUMP_IF_FALSE_OR_POP",
    [JUMP_IF_TRUE_OR_POP] = "JUMP_IF_TRUE_OR_POP",
    [PRECALL_NO_KW_STR_1] = "PRECALL_NO_KW_STR_1",
    [POP_JUMP_FORWARD_IF_FALSE] = "POP_JUMP_FORWARD_IF_FALSE",
    [POP_JUMP_FORWARD_IF_TRUE] = "POP_JUMP_FORWARD_IF_TRUE",
    [LOAD_GLOBAL] = "LOAD_GLOBAL",
    [IS_OP] = "IS_OP",
    [CONTAINS_OP] = "CONTAINS_OP",
    [RERAISE] = "RERAISE",
    [COPY] = "COPY",
    [PRECALL_NO_KW_TUPLE_1] = "PRECALL_NO_KW_TUPLE_1",
    [BINARY_OP] = "BINARY_OP",
    [SEND] = "SEND",
    [LOAD_FAST] = "LOAD_FAST",
    [STORE_FAST] = "STORE_FAST",
    [DELETE_FAST] = "DELETE_FAST",
    [PRECALL_NO_KW_TYPE_1] = "PRECALL_NO_KW_TYPE_1",
    [POP_JUMP_FORWARD_IF_NOT_NONE] = "POP_JUMP_FORWARD_IF_NOT_NONE",
    [POP_JUMP_FORWARD_IF_NONE] = "POP_JUMP_FORWARD_IF_NONE",
    [RAISE_VARARGS] = "RAISE_VARARGS",
    [GET_AWAITABLE] = "GET_AWAITABLE",
    [MAKE_FUNCTION] = "MAKE_FUNCTION",
    [BUILD_SLICE] = "BUILD_SLICE",
    [JUMP_BACKWARD_NO_INTERRUPT] = "JUMP_BACKWARD_NO_INTERRUPT",
    [MAKE_CELL] = "MAKE_CELL",
    [LOAD_CLOSURE] = "LOAD_CLOSURE",
    [LOAD_DEREF] = "LOAD_DEREF",
    [STORE_DEREF] = "STORE_DEREF",
    [DELETE_DEREF] = "DELETE_DEREF",
    [JUMP_BACKWARD] = "JUMP_BACKWARD",
    [PRECALL_PYFUNC] = "PRECALL_PYFUNC",
    [CALL_FUNCTION_EX] = "CALL_FUNCTION_EX",
    [RESUME_QUICK] = "RESUME_QUICK",
    [EXTENDED_ARG] = "EXTENDED_ARG",
    [LIST_APPEND] = "LIST_APPEND",
    [SET_ADD] = "SET_ADD",
    [MAP_ADD] = "MAP_ADD",
    [LOAD_CLASSDEREF] = "LOAD_CLASSDEREF",
    [COPY_FREE_VARS] = "COPY_FREE_VARS",
    [STORE_ATTR_ADAPTIVE] = "STORE_ATTR_ADAPTIVE",
    [RESUME] = "RESUME",
    [MATCH_CLASS] = "MATCH_CLASS",
    [STORE_ATTR_INSTANCE_VALUE] = "STORE_ATTR_INSTANCE_VALUE",
    [STORE_ATTR_SLOT] = "STORE_ATTR_SLOT",
    [FORMAT_VALUE] = "FORMAT_VALUE",
    [BUILD_CONST_KEY_MAP] = "BUILD_CONST_KEY_MAP",
    [BUILD_STRING] = "BUILD_STRING",
    [STORE_ATTR_WITH_HINT] = "STORE_ATTR_WITH_HINT",
    [STORE_FAST__LOAD_FAST] = "STORE_FAST__LOAD_FAST",
    [LOAD_METHOD] = "LOAD_METHOD",
    [STORE_FAST__STORE_FAST] = "STORE_FAST__STORE_FAST",
    [LIST_EXTEND] = "LIST_EXTEND",
    [SET_UPDATE] = "SET_UPDATE",
    [DICT_MERGE] = "DICT_MERGE",
    [DICT_UPDATE] = "DICT_UPDATE",
    [PRECALL] = "PRECALL",
    [STORE_SUBSCR_ADAPTIVE] = "STORE_SUBSCR_ADAPTIVE",
    [STORE_SUBSCR_DICT] = "STORE_SUBSCR_DICT",
    [STORE_SUBSCR_LIST_INT] = "STORE_SUBSCR_LIST_INT",
    [UNPACK_SEQUENCE_ADAPTIVE] = "UNPACK_SEQUENCE_ADAPTIVE",
    [CALL] = "CALL",
    [KW_NAMES] = "KW_NAMES",
    [POP_JUMP_BACKWARD_IF_NOT_NONE] = "POP_JUMP_BACKWARD_IF_NOT_NONE",
    [POP_JUMP_BACKWARD_IF_NONE] = "POP_JUMP_BACKWARD_IF_NONE",
    [POP_JUMP_BACKWARD_IF_FALSE] = "POP_JUMP_BACKWARD_IF_FALSE",
    [POP_JUMP_BACKWARD_IF_TRUE] = "POP_JUMP_BACKWARD_IF_TRUE",
    [UNPACK_SEQUENCE_LIST] = "UNPACK_SEQUENCE_LIST",
    [UNPACK_SEQUENCE_TUPLE] = "UNPACK_SEQUENCE_TUPLE",
    [UNPACK_SEQUENCE_TWO_TUPLE] = "UNPACK_SEQUENCE_TWO_TUPLE",
    [180] = "<180>",
    [181] = "<181>",
    [182] = "<182>",
    [183] = "<183>",
    [184] = "<184>",
    [185] = "<185>",
    [186] = "<186>",
    [187] = "<187>",
    [188] = "<188>",
    [189] = "<189>",
    [190] = "<190>",
    [191] = "<191>",
    [192] = "<192>",
    [193] = "<193>",
    [194] = "<194>",
    [195] = "<195>",
    [196] = "<196>",
    [197] = "<197>",
    [198] = "<198>",
    [199] = "<199>",
    [200] = "<200>",
    [201] = "<201>",
    [202] = "<202>",
    [203] = "<203>",
    [204] = "<204>",
    [205] = "<205>",
    [206] = "<206>",
    [207] = "<207>",
    [208] = "<208>",
    [209] = "<209>",
    [210] = "<210>",
    [211] = "<211>",
    [212] = "<212>",
    [213] = "<213>",
    [214] = "<214>",
    [215] = "<215>",
    [216] = "<216>",
    [217] = "<217>",
    [218] = "<218>",
    [219] = "<219>",
    [220] = "<220>",
    [221] = "<221>",
    [222] = "<222>",
    [223] = "<223>",
    [224] = "<224>",
    [225] = "<225>",
    [226] = "<226>",
    [227] = "<227>",
    [228] = "<228>",
    [229] = "<229>",
    [230] = "<230>",
    [231] = "<231>",
    [232] = "<232>",
    [233] = "<233>",
    [234] = "<234>",
    [235] = "<235>",
    [236] = "<236>",
    [237] = "<237>",
    [238] = "<238>",
    [239] = "<239>",
    [240] = "<240>",
    [241] = "<241>",
    [242] = "<242>",
    [243] = "<243>",
    [244] = "<244>",
    [245] = "<245>",
    [246] = "<246>",
    [247] = "<247>",
    [248] = "<248>",
    [249] = "<249>",
    [250] = "<250>",
    [251] = "<251>",
    [252] = "<252>",
    [253] = "<253>",
    [254] = "<254>",
    [DO_TRACING] = "DO_TRACING",
};
#endif

#define HAS_ARG(op) ((op) >= HAVE_ARGUMENT)

/* Reserve some bytecodes for internal use in the compiler.
 * The value of 240 is arbitrary. */
#define IS_ARTIFICIAL(op) ((op) > 240)

#ifdef __cplusplus
}
#endif
#endif /* !Py_OPCODE_H */
