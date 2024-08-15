// This file is generated by Tools/cases_generator/uop_id_generator.py
// from:
//   Python/bytecodes.c
// Do not edit!

#ifndef Py_CORE_UOP_IDS_H
#define Py_CORE_UOP_IDS_H
#ifdef __cplusplus
extern "C" {
#endif

#define _EXIT_TRACE 300
#define _SET_IP 301
#define _BINARY_OP 302
#define _BINARY_OP_ADD_FLOAT 303
#define _BINARY_OP_ADD_INT 304
#define _BINARY_OP_ADD_UNICODE 305
#define _BINARY_OP_INPLACE_ADD_UNICODE 306
#define _BINARY_OP_MULTIPLY_FLOAT 307
#define _BINARY_OP_MULTIPLY_INT 308
#define _BINARY_OP_SUBTRACT_FLOAT 309
#define _BINARY_OP_SUBTRACT_INT 310
#define _BINARY_SLICE BINARY_SLICE
#define _BINARY_SUBSCR 311
#define _BINARY_SUBSCR_CHECK_FUNC 312
#define _BINARY_SUBSCR_DICT BINARY_SUBSCR_DICT
#define _BINARY_SUBSCR_INIT_CALL 313
#define _BINARY_SUBSCR_LIST_INT BINARY_SUBSCR_LIST_INT
#define _BINARY_SUBSCR_STR_INT BINARY_SUBSCR_STR_INT
#define _BINARY_SUBSCR_TUPLE_INT BINARY_SUBSCR_TUPLE_INT
#define _BUILD_LIST BUILD_LIST
#define _BUILD_MAP BUILD_MAP
#define _BUILD_SET BUILD_SET
#define _BUILD_SLICE BUILD_SLICE
#define _BUILD_STRING BUILD_STRING
#define _BUILD_TUPLE BUILD_TUPLE
#define _CALL_ALLOC_AND_ENTER_INIT CALL_ALLOC_AND_ENTER_INIT
#define _CALL_BUILTIN_CLASS 314
#define _CALL_BUILTIN_FAST 315
#define _CALL_BUILTIN_FAST_WITH_KEYWORDS 316
#define _CALL_BUILTIN_O 317
#define _CALL_INTRINSIC_1 CALL_INTRINSIC_1
#define _CALL_INTRINSIC_2 CALL_INTRINSIC_2
#define _CALL_ISINSTANCE CALL_ISINSTANCE
#define _CALL_LEN CALL_LEN
#define _CALL_LIST_APPEND CALL_LIST_APPEND
#define _CALL_METHOD_DESCRIPTOR_FAST 318
#define _CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS 319
#define _CALL_METHOD_DESCRIPTOR_NOARGS 320
#define _CALL_METHOD_DESCRIPTOR_O 321
#define _CALL_NON_PY_GENERAL 322
#define _CALL_STR_1 323
#define _CALL_TUPLE_1 324
#define _CALL_TYPE_1 CALL_TYPE_1
#define _CHECK_ATTR_CLASS 325
#define _CHECK_ATTR_METHOD_LAZY_DICT 326
#define _CHECK_ATTR_MODULE 327
#define _CHECK_ATTR_WITH_HINT 328
#define _CHECK_CALL_BOUND_METHOD_EXACT_ARGS 329
#define _CHECK_EG_MATCH CHECK_EG_MATCH
#define _CHECK_EXC_MATCH CHECK_EXC_MATCH
#define _CHECK_FUNCTION 330
#define _CHECK_FUNCTION_EXACT_ARGS 331
#define _CHECK_FUNCTION_VERSION 332
#define _CHECK_IS_NOT_PY_CALLABLE 333
#define _CHECK_MANAGED_OBJECT_HAS_VALUES 334
#define _CHECK_METHOD_VERSION 335
#define _CHECK_PEP_523 336
#define _CHECK_PERIODIC 337
#define _CHECK_PERIODIC_IF_NOT_YIELD_FROM 338
#define _CHECK_STACK_SPACE 339
#define _CHECK_STACK_SPACE_OPERAND 340
#define _CHECK_VALIDITY 341
#define _CHECK_VALIDITY_AND_SET_IP 342
#define _COMPARE_OP 343
#define _COMPARE_OP_FLOAT 344
#define _COMPARE_OP_INT 345
#define _COMPARE_OP_STR 346
#define _CONTAINS_OP 347
#define _CONTAINS_OP_DICT CONTAINS_OP_DICT
#define _CONTAINS_OP_SET CONTAINS_OP_SET
#define _CONVERT_VALUE CONVERT_VALUE
#define _COPY COPY
#define _COPY_FREE_VARS COPY_FREE_VARS
#define _DELETE_ATTR DELETE_ATTR
#define _DELETE_DEREF DELETE_DEREF
#define _DELETE_FAST DELETE_FAST
#define _DELETE_GLOBAL DELETE_GLOBAL
#define _DELETE_NAME DELETE_NAME
#define _DELETE_SUBSCR DELETE_SUBSCR
#define _DEOPT 348
#define _DICT_MERGE DICT_MERGE
#define _DICT_UPDATE DICT_UPDATE
#define _DO_CALL 349
#define _DO_CALL_KW 350
#define _DYNAMIC_EXIT 351
#define _END_SEND END_SEND
#define _ERROR_POP_N 352
#define _EXIT_INIT_CHECK EXIT_INIT_CHECK
#define _EXPAND_METHOD 353
#define _FATAL_ERROR 354
#define _FORMAT_SIMPLE FORMAT_SIMPLE
#define _FORMAT_WITH_SPEC FORMAT_WITH_SPEC
#define _FOR_ITER 355
#define _FOR_ITER_GEN_FRAME 356
#define _FOR_ITER_TIER_TWO 357
#define _GET_AITER GET_AITER
#define _GET_ANEXT GET_ANEXT
#define _GET_AWAITABLE GET_AWAITABLE
#define _GET_ITER GET_ITER
#define _GET_LEN GET_LEN
#define _GET_YIELD_FROM_ITER GET_YIELD_FROM_ITER
#define _GUARD_BOTH_FLOAT 358
#define _GUARD_BOTH_INT 359
#define _GUARD_BOTH_UNICODE 360
#define _GUARD_BUILTINS_VERSION 361
#define _GUARD_DORV_NO_DICT 362
#define _GUARD_DORV_VALUES_INST_ATTR_FROM_DICT 363
#define _GUARD_GLOBALS_VERSION 364
#define _GUARD_IS_FALSE_POP 365
#define _GUARD_IS_NONE_POP 366
#define _GUARD_IS_NOT_NONE_POP 367
#define _GUARD_IS_TRUE_POP 368
#define _GUARD_KEYS_VERSION 369
#define _GUARD_NOS_FLOAT 370
#define _GUARD_NOS_INT 371
#define _GUARD_NOT_EXHAUSTED_LIST 372
#define _GUARD_NOT_EXHAUSTED_RANGE 373
#define _GUARD_NOT_EXHAUSTED_TUPLE 374
#define _GUARD_TOS_FLOAT 375
#define _GUARD_TOS_INT 376
#define _GUARD_TYPE_VERSION 377
#define _IMPORT_FROM IMPORT_FROM
#define _IMPORT_NAME IMPORT_NAME
#define _INIT_CALL_BOUND_METHOD_EXACT_ARGS 378
#define _INIT_CALL_PY_EXACT_ARGS 379
#define _INIT_CALL_PY_EXACT_ARGS_0 380
#define _INIT_CALL_PY_EXACT_ARGS_1 381
#define _INIT_CALL_PY_EXACT_ARGS_2 382
#define _INIT_CALL_PY_EXACT_ARGS_3 383
#define _INIT_CALL_PY_EXACT_ARGS_4 384
#define _INSTRUMENTED_CALL_FUNCTION_EX INSTRUMENTED_CALL_FUNCTION_EX
#define _INSTRUMENTED_CALL_KW INSTRUMENTED_CALL_KW
#define _INSTRUMENTED_FOR_ITER INSTRUMENTED_FOR_ITER
#define _INSTRUMENTED_INSTRUCTION INSTRUMENTED_INSTRUCTION
#define _INSTRUMENTED_JUMP_FORWARD INSTRUMENTED_JUMP_FORWARD
#define _INSTRUMENTED_LINE INSTRUMENTED_LINE
#define _INSTRUMENTED_LOAD_SUPER_ATTR INSTRUMENTED_LOAD_SUPER_ATTR
#define _INSTRUMENTED_NOT_TAKEN INSTRUMENTED_NOT_TAKEN
#define _INSTRUMENTED_POP_JUMP_IF_FALSE INSTRUMENTED_POP_JUMP_IF_FALSE
#define _INSTRUMENTED_POP_JUMP_IF_NONE INSTRUMENTED_POP_JUMP_IF_NONE
#define _INSTRUMENTED_POP_JUMP_IF_NOT_NONE INSTRUMENTED_POP_JUMP_IF_NOT_NONE
#define _INSTRUMENTED_POP_JUMP_IF_TRUE INSTRUMENTED_POP_JUMP_IF_TRUE
#define _INTERNAL_INCREMENT_OPT_COUNTER 385
#define _IS_NONE 386
#define _IS_OP IS_OP
#define _ITER_CHECK_LIST 387
#define _ITER_CHECK_RANGE 388
#define _ITER_CHECK_TUPLE 389
#define _ITER_JUMP_LIST 390
#define _ITER_JUMP_RANGE 391
#define _ITER_JUMP_TUPLE 392
#define _ITER_NEXT_LIST 393
#define _ITER_NEXT_RANGE 394
#define _ITER_NEXT_TUPLE 395
#define _JUMP_TO_TOP 396
#define _LIST_APPEND LIST_APPEND
#define _LIST_EXTEND LIST_EXTEND
#define _LOAD_ATTR 397
#define _LOAD_ATTR_CLASS 398
#define _LOAD_ATTR_CLASS_0 399
#define _LOAD_ATTR_CLASS_1 400
#define _LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN
#define _LOAD_ATTR_INSTANCE_VALUE 401
#define _LOAD_ATTR_INSTANCE_VALUE_0 402
#define _LOAD_ATTR_INSTANCE_VALUE_1 403
#define _LOAD_ATTR_METHOD_LAZY_DICT 404
#define _LOAD_ATTR_METHOD_NO_DICT 405
#define _LOAD_ATTR_METHOD_WITH_VALUES 406
#define _LOAD_ATTR_MODULE 407
#define _LOAD_ATTR_NONDESCRIPTOR_NO_DICT 408
#define _LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES 409
#define _LOAD_ATTR_PROPERTY_FRAME 410
#define _LOAD_ATTR_SLOT 411
#define _LOAD_ATTR_SLOT_0 412
#define _LOAD_ATTR_SLOT_1 413
#define _LOAD_ATTR_WITH_HINT 414
#define _LOAD_BUILD_CLASS LOAD_BUILD_CLASS
#define _LOAD_COMMON_CONSTANT LOAD_COMMON_CONSTANT
#define _LOAD_CONST LOAD_CONST
#define _LOAD_CONST_INLINE 415
#define _LOAD_CONST_INLINE_BORROW 416
#define _LOAD_CONST_INLINE_BORROW_WITH_NULL 417
#define _LOAD_CONST_INLINE_WITH_NULL 418
#define _LOAD_DEREF LOAD_DEREF
#define _LOAD_FAST 419
#define _LOAD_FAST_0 420
#define _LOAD_FAST_1 421
#define _LOAD_FAST_2 422
#define _LOAD_FAST_3 423
#define _LOAD_FAST_4 424
#define _LOAD_FAST_5 425
#define _LOAD_FAST_6 426
#define _LOAD_FAST_7 427
#define _LOAD_FAST_AND_CLEAR LOAD_FAST_AND_CLEAR
#define _LOAD_FAST_CHECK LOAD_FAST_CHECK
#define _LOAD_FAST_LOAD_FAST LOAD_FAST_LOAD_FAST
#define _LOAD_FROM_DICT_OR_DEREF LOAD_FROM_DICT_OR_DEREF
#define _LOAD_FROM_DICT_OR_GLOBALS LOAD_FROM_DICT_OR_GLOBALS
#define _LOAD_GLOBAL 428
#define _LOAD_GLOBAL_BUILTINS 429
#define _LOAD_GLOBAL_MODULE 430
#define _LOAD_LOCALS LOAD_LOCALS
#define _LOAD_NAME LOAD_NAME
#define _LOAD_SPECIAL LOAD_SPECIAL
#define _LOAD_SUPER_ATTR_ATTR LOAD_SUPER_ATTR_ATTR
#define _LOAD_SUPER_ATTR_METHOD LOAD_SUPER_ATTR_METHOD
#define _MAKE_CELL MAKE_CELL
#define _MAKE_FUNCTION MAKE_FUNCTION
#define _MAP_ADD MAP_ADD
#define _MATCH_CLASS MATCH_CLASS
#define _MATCH_KEYS MATCH_KEYS
#define _MATCH_MAPPING MATCH_MAPPING
#define _MATCH_SEQUENCE MATCH_SEQUENCE
#define _MAYBE_EXPAND_METHOD 431
#define _MONITOR_CALL 432
#define _MONITOR_JUMP_BACKWARD 433
#define _MONITOR_RESUME 434
#define _NOP NOP
#define _POP_EXCEPT POP_EXCEPT
#define _POP_JUMP_IF_FALSE 435
#define _POP_JUMP_IF_TRUE 436
#define _POP_TOP POP_TOP
#define _POP_TOP_LOAD_CONST_INLINE_BORROW 437
#define _PUSH_EXC_INFO PUSH_EXC_INFO
#define _PUSH_FRAME 438
#define _PUSH_NULL PUSH_NULL
#define _PY_FRAME_GENERAL 439
#define _QUICKEN_RESUME 440
#define _REPLACE_WITH_TRUE 441
#define _RESUME_CHECK RESUME_CHECK
#define _RETURN_GENERATOR RETURN_GENERATOR
#define _RETURN_VALUE RETURN_VALUE
#define _SAVE_RETURN_OFFSET 442
#define _SEND 443
#define _SEND_GEN_FRAME 444
#define _SETUP_ANNOTATIONS SETUP_ANNOTATIONS
#define _SET_ADD SET_ADD
#define _SET_FUNCTION_ATTRIBUTE SET_FUNCTION_ATTRIBUTE
#define _SET_UPDATE SET_UPDATE
#define _START_EXECUTOR 445
#define _STORE_ATTR 446
#define _STORE_ATTR_INSTANCE_VALUE 447
#define _STORE_ATTR_SLOT 448
#define _STORE_ATTR_WITH_HINT 449
#define _STORE_DEREF STORE_DEREF
#define _STORE_FAST 450
#define _STORE_FAST_0 451
#define _STORE_FAST_1 452
#define _STORE_FAST_2 453
#define _STORE_FAST_3 454
#define _STORE_FAST_4 455
#define _STORE_FAST_5 456
#define _STORE_FAST_6 457
#define _STORE_FAST_7 458
#define _STORE_FAST_LOAD_FAST STORE_FAST_LOAD_FAST
#define _STORE_FAST_STORE_FAST STORE_FAST_STORE_FAST
#define _STORE_GLOBAL STORE_GLOBAL
#define _STORE_NAME STORE_NAME
#define _STORE_SLICE STORE_SLICE
#define _STORE_SUBSCR 459
#define _STORE_SUBSCR_DICT STORE_SUBSCR_DICT
#define _STORE_SUBSCR_LIST_INT STORE_SUBSCR_LIST_INT
#define _SWAP SWAP
#define _TIER2_RESUME_CHECK 460
#define _TO_BOOL 461
#define _TO_BOOL_BOOL TO_BOOL_BOOL
#define _TO_BOOL_INT TO_BOOL_INT
#define _TO_BOOL_LIST TO_BOOL_LIST
#define _TO_BOOL_NONE TO_BOOL_NONE
#define _TO_BOOL_STR TO_BOOL_STR
#define _UNARY_INVERT UNARY_INVERT
#define _UNARY_NEGATIVE UNARY_NEGATIVE
#define _UNARY_NOT UNARY_NOT
#define _UNPACK_EX UNPACK_EX
#define _UNPACK_SEQUENCE 462
#define _UNPACK_SEQUENCE_LIST UNPACK_SEQUENCE_LIST
#define _UNPACK_SEQUENCE_TUPLE UNPACK_SEQUENCE_TUPLE
#define _UNPACK_SEQUENCE_TWO_TUPLE UNPACK_SEQUENCE_TWO_TUPLE
#define _WITH_EXCEPT_START WITH_EXCEPT_START
#define _YIELD_VALUE YIELD_VALUE
#define __DO_CALL_FUNCTION_EX _DO_CALL_FUNCTION_EX
#define MAX_UOP_ID 462

#ifdef __cplusplus
}
#endif
#endif /* !Py_CORE_UOP_IDS_H */
