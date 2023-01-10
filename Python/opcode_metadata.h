// This file is generated by Tools/cases_generator/generate_cases.py --metadata
// from Python/bytecodes.c
// Do not edit!
enum Direction { DIR_NONE, DIR_READ, DIR_WRITE };
enum InstructionFormat { INSTR_FMT_IB, INSTR_FMT_IBC, INSTR_FMT_IBC0, INSTR_FMT_IBC000, INSTR_FMT_IBC0IB, INSTR_FMT_IBIB };
static const struct {
    short n_popped;
    short n_pushed;
    enum Direction dir_op1;
    enum Direction dir_op2;
    enum Direction dir_op3;
    bool valid_entry;
    enum InstructionFormat instr_format;
} _PyOpcode_opcode_metadata[256] = {
    [NOP] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [RESUME] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_CLOSURE] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_FAST_CHECK] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_FAST] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_CONST] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [STORE_FAST] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_FAST__LOAD_FAST] = { 0, 2, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBIB },
    [LOAD_FAST__LOAD_CONST] = { 0, 2, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBIB },
    [STORE_FAST__LOAD_FAST] = { 1, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBIB },
    [STORE_FAST__STORE_FAST] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBIB },
    [LOAD_CONST__LOAD_FAST] = { 0, 2, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBIB },
    [POP_TOP] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [PUSH_NULL] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [END_FOR] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [UNARY_NEGATIVE] = { 1, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [UNARY_NOT] = { 1, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [UNARY_INVERT] = { 1, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BINARY_OP_MULTIPLY_INT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [BINARY_OP_MULTIPLY_FLOAT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [BINARY_OP_SUBTRACT_INT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [BINARY_OP_SUBTRACT_FLOAT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [BINARY_OP_ADD_UNICODE] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [BINARY_OP_INPLACE_ADD_UNICODE] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BINARY_OP_ADD_FLOAT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [BINARY_OP_ADD_INT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [BINARY_SUBSCR] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [BINARY_SLICE] = { 3, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [STORE_SLICE] = { 4, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BINARY_SUBSCR_LIST_INT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [BINARY_SUBSCR_TUPLE_INT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [BINARY_SUBSCR_DICT] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [BINARY_SUBSCR_GETITEM] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [LIST_APPEND] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [SET_ADD] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [STORE_SUBSCR] = { 3, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [STORE_SUBSCR_LIST_INT] = { 3, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [STORE_SUBSCR_DICT] = { 3, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [DELETE_SUBSCR] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_INTRINSIC_1] = { 1, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [RAISE_VARARGS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [INTERPRETER_EXIT] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [RETURN_VALUE] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [GET_AITER] = { 1, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [GET_ANEXT] = { 1, 2, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [GET_AWAITABLE] = { 1, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [SEND] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [YIELD_VALUE] = { 1, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [POP_EXCEPT] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [RERAISE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [PREP_RERAISE_STAR] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [END_ASYNC_FOR] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CLEANUP_THROW] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ASSERTION_ERROR] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_BUILD_CLASS] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [STORE_NAME] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [DELETE_NAME] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [UNPACK_SEQUENCE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [UNPACK_SEQUENCE_TWO_TUPLE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [UNPACK_SEQUENCE_TUPLE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [UNPACK_SEQUENCE_LIST] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [UNPACK_EX] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [STORE_ATTR] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [DELETE_ATTR] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [STORE_GLOBAL] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [DELETE_GLOBAL] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_NAME] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_GLOBAL] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_GLOBAL_MODULE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_GLOBAL_BUILTIN] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [DELETE_FAST] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [MAKE_CELL] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [DELETE_DEREF] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_CLASSDEREF] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_DEREF] = { 0, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [STORE_DEREF] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [COPY_FREE_VARS] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BUILD_STRING] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BUILD_TUPLE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BUILD_LIST] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LIST_EXTEND] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [SET_UPDATE] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BUILD_SET] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BUILD_MAP] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [SETUP_ANNOTATIONS] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BUILD_CONST_KEY_MAP] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [DICT_UPDATE] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [DICT_MERGE] = { 1, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [MAP_ADD] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_INSTANCE_VALUE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_MODULE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_WITH_HINT] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_SLOT] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_CLASS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_PROPERTY] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [STORE_ATTR_INSTANCE_VALUE] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [STORE_ATTR_WITH_HINT] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [STORE_ATTR_SLOT] = { 2, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC000 },
    [COMPARE_OP] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC0 },
    [COMPARE_OP_FLOAT_JUMP] = { 3, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC0IB },
    [COMPARE_OP_INT_JUMP] = { 3, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC0IB },
    [COMPARE_OP_STR_JUMP] = { 3, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC0IB },
    [IS_OP] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CONTAINS_OP] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CHECK_EG_MATCH] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CHECK_EXC_MATCH] = { 2, 2, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [IMPORT_NAME] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [IMPORT_FROM] = { 1, 2, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [JUMP_FORWARD] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [JUMP_BACKWARD] = { 0, 0, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [POP_JUMP_IF_FALSE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [POP_JUMP_IF_TRUE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [POP_JUMP_IF_NOT_NONE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [POP_JUMP_IF_NONE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [JUMP_IF_FALSE_OR_POP] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [JUMP_IF_TRUE_OR_POP] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [JUMP_BACKWARD_NO_INTERRUPT] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [GET_LEN] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [MATCH_CLASS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [MATCH_MAPPING] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [MATCH_SEQUENCE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [MATCH_KEYS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [GET_ITER] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [GET_YIELD_FROM_ITER] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [FOR_ITER] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [FOR_ITER_LIST] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [FOR_ITER_TUPLE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [FOR_ITER_RANGE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [FOR_ITER_GEN] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BEFORE_ASYNC_WITH] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BEFORE_WITH] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [WITH_EXCEPT_START] = { 4, 5, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [PUSH_EXC_INFO] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_METHOD_WITH_VALUES] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_METHOD_NO_DICT] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [LOAD_ATTR_METHOD_LAZY_DICT] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_BOUND_METHOD_EXACT_ARGS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [KW_NAMES] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_PY_EXACT_ARGS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_PY_WITH_DEFAULTS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_TYPE_1] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_STR_1] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_TUPLE_1] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_BUILTIN_CLASS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_BUILTIN_O] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_BUILTIN_FAST] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_BUILTIN_FAST_WITH_KEYWORDS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_LEN] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_ISINSTANCE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_LIST_APPEND] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_METHOD_DESCRIPTOR_O] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_METHOD_DESCRIPTOR_NOARGS] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_NO_KW_METHOD_DESCRIPTOR_FAST] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CALL_FUNCTION_EX] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [MAKE_FUNCTION] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [RETURN_GENERATOR] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BUILD_SLICE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [FORMAT_VALUE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [COPY] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [BINARY_OP] = { 2, 1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IBC },
    [SWAP] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [EXTENDED_ARG] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
    [CACHE] = { -1, -1, DIR_NONE, DIR_NONE, DIR_NONE, true, INSTR_FMT_IB },
};
