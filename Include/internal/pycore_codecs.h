#ifndef Py_INTERNAL_CODECS_H
#define Py_INTERNAL_CODECS_H
#ifdef __cplusplus
extern "C" {
#endif

extern PyObject* _PyCodec_Lookup(const char *encoding);

/* Text codec specific encoding and decoding API.

   Checks the encoding against a list of codecs which do not
   implement a str<->bytes encoding before attempting the
   operation.

   Please note that these APIs are internal and should not
   be used in Python C extensions.

   XXX (ncoghlan): should we make these, or something like them, public
   in Python 3.5+?

 */
extern PyObject* _PyCodec_LookupTextEncoding(
   const char *encoding,
   const char *alternate_command);

extern PyObject* _PyCodec_EncodeText(
   PyObject *object,
   const char *encoding,
   const char *errors);

extern PyObject* _PyCodec_DecodeText(
   PyObject *object,
   const char *encoding,
   const char *errors);

/* These two aren't actually text encoding specific, but _io.TextIOWrapper
 * is the only current API consumer.
 */
extern PyObject* _PyCodecInfo_GetIncrementalDecoder(
   PyObject *codec_info,
   const char *errors);

extern PyObject* _PyCodecInfo_GetIncrementalEncoder(
   PyObject *codec_info,
   const char *errors);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODECS_H */
