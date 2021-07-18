static void *opcode_targets[256] = {
    &&_unknown_opcode,
    &&TARGET_POP_TOP,
    &&TARGET_ROT_TWO,
    &&TARGET_ROT_THREE,
    &&TARGET_DUP_TOP,
    &&TARGET_DUP_TOP_TWO,
    &&TARGET_ROT_FOUR,
    &&TARGET_BINARY_SUBSCR_ADAPTIVE,
    &&TARGET_BINARY_SUBSCR_LIST_INT,
    &&TARGET_NOP,
    &&TARGET_UNARY_POSITIVE,
    &&TARGET_UNARY_NEGATIVE,
    &&TARGET_UNARY_NOT,
    &&TARGET_BINARY_SUBSCR_TUPLE_INT,
    &&TARGET_BINARY_SUBSCR_DICT,
    &&TARGET_UNARY_INVERT,
    &&TARGET_BINARY_MATRIX_MULTIPLY,
    &&TARGET_INPLACE_MATRIX_MULTIPLY,
    &&TARGET_JUMP_ABSOLUTE_QUICK,
    &&TARGET_BINARY_POWER,
    &&TARGET_BINARY_MULTIPLY,
    &&TARGET_LOAD_ATTR_ADAPTIVE,
    &&TARGET_BINARY_MODULO,
    &&TARGET_BINARY_ADD,
    &&TARGET_BINARY_SUBTRACT,
    &&TARGET_BINARY_SUBSCR,
    &&TARGET_BINARY_FLOOR_DIVIDE,
    &&TARGET_BINARY_TRUE_DIVIDE,
    &&TARGET_INPLACE_FLOOR_DIVIDE,
    &&TARGET_INPLACE_TRUE_DIVIDE,
    &&TARGET_GET_LEN,
    &&TARGET_MATCH_MAPPING,
    &&TARGET_MATCH_SEQUENCE,
    &&TARGET_MATCH_KEYS,
    &&TARGET_COPY_DICT_WITHOUT_KEYS,
    &&TARGET_PUSH_EXC_INFO,
    &&TARGET_LOAD_ATTR_SPLIT_KEYS,
    &&TARGET_POP_EXCEPT_AND_RERAISE,
    &&TARGET_LOAD_ATTR_WITH_HINT,
    &&TARGET_LOAD_ATTR_SLOT,
    &&TARGET_LOAD_ATTR_MODULE,
    &&TARGET_LOAD_GLOBAL_ADAPTIVE,
    &&TARGET_LOAD_GLOBAL_MODULE,
    &&TARGET_LOAD_GLOBAL_BUILTIN,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&TARGET_WITH_EXCEPT_START,
    &&TARGET_GET_AITER,
    &&TARGET_GET_ANEXT,
    &&TARGET_BEFORE_ASYNC_WITH,
    &&TARGET_BEFORE_WITH,
    &&TARGET_END_ASYNC_FOR,
    &&TARGET_INPLACE_ADD,
    &&TARGET_INPLACE_SUBTRACT,
    &&TARGET_INPLACE_MULTIPLY,
    &&_unknown_opcode,
    &&TARGET_INPLACE_MODULO,
    &&TARGET_STORE_SUBSCR,
    &&TARGET_DELETE_SUBSCR,
    &&TARGET_BINARY_LSHIFT,
    &&TARGET_BINARY_RSHIFT,
    &&TARGET_BINARY_AND,
    &&TARGET_BINARY_XOR,
    &&TARGET_BINARY_OR,
    &&TARGET_INPLACE_POWER,
    &&TARGET_GET_ITER,
    &&TARGET_GET_YIELD_FROM_ITER,
    &&TARGET_PRINT_EXPR,
    &&TARGET_LOAD_BUILD_CLASS,
    &&TARGET_YIELD_FROM,
    &&TARGET_GET_AWAITABLE,
    &&TARGET_LOAD_ASSERTION_ERROR,
    &&TARGET_INPLACE_LSHIFT,
    &&TARGET_INPLACE_RSHIFT,
    &&TARGET_INPLACE_AND,
    &&TARGET_INPLACE_XOR,
    &&TARGET_INPLACE_OR,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&TARGET_LIST_TO_TUPLE,
    &&TARGET_RETURN_VALUE,
    &&TARGET_IMPORT_STAR,
    &&TARGET_SETUP_ANNOTATIONS,
    &&TARGET_YIELD_VALUE,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&TARGET_POP_EXCEPT,
    &&TARGET_STORE_NAME,
    &&TARGET_DELETE_NAME,
    &&TARGET_UNPACK_SEQUENCE,
    &&TARGET_FOR_ITER,
    &&TARGET_UNPACK_EX,
    &&TARGET_STORE_ATTR,
    &&TARGET_DELETE_ATTR,
    &&TARGET_STORE_GLOBAL,
    &&TARGET_DELETE_GLOBAL,
    &&TARGET_ROT_N,
    &&TARGET_LOAD_CONST,
    &&TARGET_LOAD_NAME,
    &&TARGET_BUILD_TUPLE,
    &&TARGET_BUILD_LIST,
    &&TARGET_BUILD_SET,
    &&TARGET_BUILD_MAP,
    &&TARGET_LOAD_ATTR,
    &&TARGET_COMPARE_OP,
    &&TARGET_IMPORT_NAME,
    &&TARGET_IMPORT_FROM,
    &&TARGET_JUMP_FORWARD,
    &&TARGET_JUMP_IF_FALSE_OR_POP,
    &&TARGET_JUMP_IF_TRUE_OR_POP,
    &&TARGET_JUMP_ABSOLUTE,
    &&TARGET_POP_JUMP_IF_FALSE,
    &&TARGET_POP_JUMP_IF_TRUE,
    &&TARGET_LOAD_GLOBAL,
    &&TARGET_IS_OP,
    &&TARGET_CONTAINS_OP,
    &&TARGET_RERAISE,
    &&_unknown_opcode,
    &&TARGET_JUMP_IF_NOT_EXC_MATCH,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&TARGET_LOAD_FAST,
    &&TARGET_STORE_FAST,
    &&TARGET_DELETE_FAST,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&TARGET_GEN_START,
    &&TARGET_RAISE_VARARGS,
    &&TARGET_CALL_FUNCTION,
    &&TARGET_MAKE_FUNCTION,
    &&TARGET_BUILD_SLICE,
    &&_unknown_opcode,
    &&TARGET_MAKE_CELL,
    &&TARGET_LOAD_CLOSURE,
    &&TARGET_LOAD_DEREF,
    &&TARGET_STORE_DEREF,
    &&TARGET_DELETE_DEREF,
    &&_unknown_opcode,
    &&TARGET_CALL_FUNCTION_KW,
    &&TARGET_CALL_FUNCTION_EX,
    &&_unknown_opcode,
    &&TARGET_EXTENDED_ARG,
    &&TARGET_LIST_APPEND,
    &&TARGET_SET_ADD,
    &&TARGET_MAP_ADD,
    &&TARGET_LOAD_CLASSDEREF,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&TARGET_MATCH_CLASS,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&TARGET_FORMAT_VALUE,
    &&TARGET_BUILD_CONST_KEY_MAP,
    &&TARGET_BUILD_STRING,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&TARGET_LOAD_METHOD,
    &&TARGET_CALL_METHOD,
    &&TARGET_LIST_EXTEND,
    &&TARGET_SET_UPDATE,
    &&TARGET_DICT_MERGE,
    &&TARGET_DICT_UPDATE,
    &&TARGET_CALL_METHOD_KW,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode,
    &&_unknown_opcode
};
