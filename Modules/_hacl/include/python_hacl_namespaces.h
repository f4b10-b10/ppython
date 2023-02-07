#ifndef _PYTHON_HACL_NAMESPACES_H
#define _PYTHON_HACL_NAMESPACES_H

/*
 * C's excuse for namespaces: Use globally unique names to avoid linkage
 * conflicts with builds linking or dynamically loading other code potentially
 * using HACL* libraries.
 */

#define Hacl_Streaming_SHA2_state_sha2_224_s python_hashlib_Hacl_Streaming_SHA2_state_sha2_224_s
#define Hacl_Streaming_SHA2_state_sha2_224 python_hashlib_Hacl_Streaming_SHA2_state_sha2_224
#define Hacl_Streaming_SHA2_state_sha2_256 python_hashlib_Hacl_Streaming_SHA2_state_sha2_256
#define Hacl_Streaming_SHA2_create_in_256 python_hashlib_Hacl_Streaming_SHA2_create_in_256
#define Hacl_Streaming_SHA2_create_in_224 python_hashlib_Hacl_Streaming_SHA2_create_in_224
#define Hacl_Streaming_SHA2_copy_256 python_hashlib_Hacl_Streaming_SHA2_copy_256
#define Hacl_Streaming_SHA2_copy_224 python_hashlib_Hacl_Streaming_SHA2_copy_224
#define Hacl_Streaming_SHA2_init_256 python_hashlib_Hacl_Streaming_SHA2_init_256
#define Hacl_Streaming_SHA2_init_224 python_hashlib_Hacl_Streaming_SHA2_init_224
#define Hacl_Streaming_SHA2_update_256 python_hashlib_Hacl_Streaming_SHA2_update_256
#define Hacl_Streaming_SHA2_update_224 python_hashlib_Hacl_Streaming_SHA2_update_224
#define Hacl_Streaming_SHA2_finish_256 python_hashlib_Hacl_Streaming_SHA2_finish_256
#define Hacl_Streaming_SHA2_finish_224 python_hashlib_Hacl_Streaming_SHA2_finish_224
#define Hacl_Streaming_SHA2_free_256 python_hashlib_Hacl_Streaming_SHA2_free_256
#define Hacl_Streaming_SHA2_free_224 python_hashlib_Hacl_Streaming_SHA2_free_224
#define Hacl_Streaming_SHA2_sha256 python_hashlib_Hacl_Streaming_SHA2_sha256
#define Hacl_Streaming_SHA2_sha224 python_hashlib_Hacl_Streaming_SHA2_sha224

#endif  // _PYTHON_HACL_NAMESPACES_H
