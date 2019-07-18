/*
 *  LZ4 - Fast LZ compression algorithm
 *  Header File
 *  Copyright (C) 2011-2017, Yann Collet.

   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
    - LZ4 homepage : http://www.lz4.org
    - LZ4 source repository : https://github.com/lz4/lz4
*/
#if defined (__cplusplus)
extern "C" {
#endif

#ifndef LZ4_H_2983827168210
#define LZ4_H_2983827168210

/* --- Dependency --- */
#include <stddef.h>   /* size_t */


/**
  Introduction

  LZ4 is lossless compression algorithm, providing compression speed at 400 MB/s per core,
  scalable with multi-cores CPU. It features an extremely fast decoder, with speed in
  multiple GB/s per core, typically reaching RAM speed limits on multi-core systems.

  The LZ4 compression library provides in-memory compression and decompression functions.
  Compression can be done in:
    - a single step (described as Simple Functions)
    - a single step, reusing a context (described in Advanced Functions)
    - unbounded multiple steps (described as Streaming compression)

  lz4.h provides block compression functions. It gives full buffer control to user.
  Decompressing an lz4-compressed block also requires metadata (such as compressed size).
  Each application is free to encode such metadata in whichever way it wants.

  An additional format, called LZ4 frame specification (doc/lz4_Frame_format.md),
  take care of encoding standard metadata alongside LZ4-compressed blocks.
  If your application requires interoperability, it's recommended to use it.
  A library is provided to take care of it, see lz4frame.h.
*/

/*^***************************************************************
*  Export parameters
*****************************************************************/
/*
*  LZ4_DLL_EXPORT :
*  Enable exporting of functions when building a Windows DLL
*  LZ4LIB_VISIBILITY :
*  Control library symbols visibility.
*/
#ifndef LZ4LIB_VISIBILITY
#  if defined(__GNUC__) && (__GNUC__ >= 4)
#    define LZ4LIB_VISIBILITY __attribute__ ((visibility ("default")))
#  else
#    define LZ4LIB_VISIBILITY
#  endif
#endif
#if defined(LZ4_DLL_EXPORT) && (LZ4_DLL_EXPORT==1)
#  define LZ4LIB_API __declspec(dllexport) LZ4LIB_VISIBILITY
#elif defined(LZ4_DLL_IMPORT) && (LZ4_DLL_IMPORT==1)
#  define LZ4LIB_API __declspec(dllimport) LZ4LIB_VISIBILITY /* It isn't required but allows to generate better code, saving a function pointer load from the IAT and an indirect jump.*/
#else
#  define LZ4LIB_API LZ4LIB_VISIBILITY
#endif

/*------   Version   ------*/
#define LZ4_VERSION_MAJOR    1    /* for breaking interface changes  */
#define LZ4_VERSION_MINOR    8    /* for new (non-breaking) interface capabilities */
#define LZ4_VERSION_RELEASE  2    /* for tweaks, bug-fixes, or development */

#define LZ4_VERSION_NUMBER (LZ4_VERSION_MAJOR *100*100 + LZ4_VERSION_MINOR *100 + LZ4_VERSION_RELEASE)

#define LZ4_LIB_VERSION LZ4_VERSION_MAJOR.LZ4_VERSION_MINOR.LZ4_VERSION_RELEASE
#define LZ4_QUOTE(str) #str
#define LZ4_EXPAND_AND_QUOTE(str) LZ4_QUOTE(str)
#define LZ4_VERSION_STRING LZ4_EXPAND_AND_QUOTE(LZ4_LIB_VERSION)

LZ4LIB_API int LZ4_versionNumber (void);  /**< library version number; useful to check dll version */
LZ4LIB_API const char* LZ4_versionString (void);   /**< library version string; unseful to check dll version */


/*-************************************
*  Tuning parameter
**************************************/
/*!
 * LZ4_MEMORY_USAGE :
 * Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
 * Increasing memory usage improves compression ratio
 * Reduced memory usage may improve speed, thanks to cache effect
 * Default value is 14, for 16KB, which nicely fits into Intel x86 L1 cache
 */
#ifndef LZ4_MEMORY_USAGE
# define LZ4_MEMORY_USAGE 14
#endif

/*-************************************
*  Simple Functions
**************************************/
/*! LZ4_compress_default() :
    Compresses 'srcSize' bytes from buffer 'src'
    into already allocated 'dst' buffer of size 'dstCapacity'.
    Compression is guaranteed to succeed if 'dstCapacity' >= LZ4_compressBound(srcSize).
    It also runs faster, so it's a recommended setting.
    If the function cannot compress 'src' into a more limited 'dst' budget,
    compression stops *immediately*, and the function result is zero.
    Note : as a consequence, 'dst' content is not valid.
    Note 2 : This function is protected against buffer overflow scenarios (never writes outside 'dst' buffer, nor read outside 'source' buffer).
        srcSize : max supported value is LZ4_MAX_INPUT_SIZE.
        dstCapacity : size of buffer 'dst' (which must be already allocated)
        return  : the number of bytes written into buffer 'dst' (necessarily <= dstCapacity)
                  or 0 if compression fails */
LZ4LIB_API int LZ4_compress_default(const char* src, char* dst, int srcSize, int dstCapacity);

/*! LZ4_decompress_safe() :
    compressedSize : is the exact complete size of the compressed block.
    dstCapacity : is the size of destination buffer, which must be already allocated.
    return : the number of bytes decompressed into destination buffer (necessarily <= dstCapacity)
             If destination buffer is not large enough, decoding will stop and output an error code (negative value).
             If the source stream is detected malformed, the function will stop decoding and return a negative result.
             This function is protected against malicious data packets.
*/
LZ4LIB_API int LZ4_decompress_safe (const char* src, char* dst, int compressedSize, int dstCapacity);


/*-************************************
*  Advanced Functions
**************************************/
#define LZ4_MAX_INPUT_SIZE        0x7E000000   /* 2 113 929 216 bytes */
#define LZ4_COMPRESSBOUND(isize)  ((unsigned)(isize) > (unsigned)LZ4_MAX_INPUT_SIZE ? 0 : (isize) + ((isize)/255) + 16)

/*!
LZ4_compressBound() :
    Provides the maximum size that LZ4 compression may output in a "worst case" scenario (input data not compressible)
    This function is primarily useful for memory allocation purposes (destination buffer size).
    Macro LZ4_COMPRESSBOUND() is also provided for compilation-time evaluation (stack memory allocation for example).
    Note that LZ4_compress_default() compresses faster when dstCapacity is >= LZ4_compressBound(srcSize)
        inputSize  : max supported value is LZ4_MAX_INPUT_SIZE
        return : maximum output size in a "worst case" scenario
              or 0, if input size is incorrect (too large or negative)
*/
LZ4LIB_API int LZ4_compressBound(int inputSize);

/*!
LZ4_compress_fast() :
    Same as LZ4_compress_default(), but allows selection of "acceleration" factor.
    The larger the acceleration value, the faster the algorithm, but also the lesser the compression.
    It's a trade-off. It can be fine tuned, with each successive value providing roughly +~3% to speed.
    An acceleration value of "1" is the same as regular LZ4_compress_default()
    Values <= 0 will be replaced by ACCELERATION_DEFAULT (currently == 1, see lz4.c).
*/
LZ4LIB_API int LZ4_compress_fast (const char* src, char* dst, int srcSize, int dstCapacity, int acceleration);


/*!
LZ4_compress_fast_extState() :
    Same compression function, just using an externally allocated memory space to store compression state.
    Use LZ4_sizeofState() to know how much memory must be allocated,
    and allocate it on 8-bytes boundaries (using malloc() typically).
    Then, provide it as 'void* state' to compression function.
*/
LZ4LIB_API int LZ4_sizeofState(void);
LZ4LIB_API int LZ4_compress_fast_extState (void* state, const char* src, char* dst, int srcSize, int dstCapacity, int acceleration);


/*!
LZ4_compress_destSize() :
    Reverse the logic : compresses as much data as possible from 'src' buffer
    into already allocated buffer 'dst' of size 'targetDestSize'.
    This function either compresses the entire 'src' content into 'dst' if it's large enough,
    or fill 'dst' buffer completely with as much data as possible from 'src'.
        *srcSizePtr : will be modified to indicate how many bytes where read from 'src' to fill 'dst'.
                      New value is necessarily <= old value.
        return : Nb bytes written into 'dst' (necessarily <= targetDestSize)
                 or 0 if compression fails
*/
LZ4LIB_API int LZ4_compress_destSize (const char* src, char* dst, int* srcSizePtr, int targetDstSize);


/*!
LZ4_decompress_fast() : **unsafe!**
This function is a bit faster than LZ4_decompress_safe(),
but doesn't provide any security guarantee.
    originalSize : is the uncompressed size to regenerate
                   Destination buffer must be already allocated, and its size must be >= 'originalSize' bytes.
    return : number of bytes read from source buffer (== compressed size).
             If the source stream is detected malformed, the function stops decoding and return a negative result.
    note : This function respects memory boundaries for *properly formed* compressed data.
           However, it does not provide any protection against malicious input.
           It also doesn't know 'src' size, and implies it's >= compressed size.
           Use this function in trusted environment **only**.
*/
LZ4LIB_API int LZ4_decompress_fast (const char* src, char* dst, int originalSize);

/*!
LZ4_decompress_safe_partial() :
    This function decompress a compressed block of size 'srcSize' at position 'src'
    into destination buffer 'dst' of size 'dstCapacity'.
    The function will decompress a minimum of 'targetOutputSize' bytes, and stop after that.
    However, it's not accurate, and may write more than 'targetOutputSize' (but always <= dstCapacity).
   @return : the number of bytes decoded in the destination buffer (necessarily <= dstCapacity)
        Note : this number can also be < targetOutputSize, if compressed block contains less data.
            Therefore, always control how many bytes were decoded.
            If source stream is detected malformed, function returns a negative result.
            This function is protected against malicious data packets.
*/
LZ4LIB_API int LZ4_decompress_safe_partial (const char* src, char* dst, int srcSize, int targetOutputSize, int dstCapacity);


/*-*********************************************
*  Streaming Compression Functions
***********************************************/
typedef union LZ4_stream_u LZ4_stream_t;  /* incomplete type (defined later) */

/*! LZ4_createStream() and LZ4_freeStream() :
 *  LZ4_createStream() will allocate and initialize an `LZ4_stream_t` structure.
 *  LZ4_freeStream() releases its memory.
 */
LZ4LIB_API LZ4_stream_t* LZ4_createStream(void);
LZ4LIB_API int           LZ4_freeStream (LZ4_stream_t* streamPtr);

/*! LZ4_resetStream() :
 *  An LZ4_stream_t structure can be allocated once and re-used multiple times.
 *  Use this function to start compressing a new stream.
 */
LZ4LIB_API void LZ4_resetStream (LZ4_stream_t* streamPtr);

/*! LZ4_loadDict() :
 *  Use this function to load a static dictionary into LZ4_stream_t.
 *  Any previous data will be forgotten, only 'dictionary' will remain in memory.
 *  Loading a size of 0 is allowed, and is the same as reset.
 * @return : dictionary size, in bytes (necessarily <= 64 KB)
 */
LZ4LIB_API int LZ4_loadDict (LZ4_stream_t* streamPtr, const char* dictionary, int dictSize);

/*! LZ4_compress_fast_continue() :
 *  Compress 'src' content using data from previously compressed blocks, for better compression ratio.
 *  'dst' buffer must be already allocated.
 *  If dstCapacity >= LZ4_compressBound(srcSize), compression is guaranteed to succeed, and runs faster.
 *
 *  Important : The previous 64KB of compressed data is assumed to remain present and unmodified in memory!
 *
 *  Special 1 : When input is a double-buffer, they can have any size, including < 64 KB.
 *              Make sure that buffers are separated by at least one byte.
 *              This way, each block only depends on previous block.
 *  Special 2 : If input buffer is a ring-buffer, it can have any size, including < 64 KB.
 *
 * @return : size of compressed block
 *           or 0 if there is an error (typically, cannot fit into 'dst').
 *  After an error, the stream status is invalid, it can only be reset or freed.
 */
LZ4LIB_API int LZ4_compress_fast_continue (LZ4_stream_t* streamPtr, const char* src, char* dst, int srcSize, int dstCapacity, int acceleration);

/*! LZ4_saveDict() :
 *  If last 64KB data cannot be guaranteed to remain available at its current memory location,
 *  save it into a safer place (char* safeBuffer).
 *  This is schematically equivalent to a memcpy() followed by LZ4_loadDict(),
 *  but is much faster, because LZ4_saveDict() doesn't need to rebuild tables.
 * @return : saved dictionary size in bytes (necessarily <= maxDictSize), or 0 if error.
 */
LZ4LIB_API int LZ4_saveDict (LZ4_stream_t* streamPtr, char* safeBuffer, int maxDictSize);


/*-**********************************************
*  Streaming Decompression Functions
*  Bufferless synchronous API
************************************************/
typedef union LZ4_streamDecode_u LZ4_streamDecode_t;   /* incomplete type (defined later) */

/*! LZ4_createStreamDecode() and LZ4_freeStreamDecode() :
 *  creation / destruction of streaming decompression tracking structure.
 *  A tracking structure can be re-used multiple times sequentially. */
LZ4LIB_API LZ4_streamDecode_t* LZ4_createStreamDecode(void);
LZ4LIB_API int                 LZ4_freeStreamDecode (LZ4_streamDecode_t* LZ4_stream);

/*! LZ4_setStreamDecode() :
 *  An LZ4_streamDecode_t structure can be allocated once and re-used multiple times.
 *  Use this function to start decompression of a new stream of blocks.
 *  A dictionary can optionnally be set. Use NULL or size 0 for a reset order.
 * @return : 1 if OK, 0 if error
 */
LZ4LIB_API int LZ4_setStreamDecode (LZ4_streamDecode_t* LZ4_streamDecode, const char* dictionary, int dictSize);

/*! LZ4_decompress_*_continue() :
 *  These decoding functions allow decompression of consecutive blocks in "streaming" mode.
 *  A block is an unsplittable entity, it must be presented entirely to a decompression function.
 *  Decompression functions only accept one block at a time.
 *  The last 64KB of previously decoded data *must* remain available and unmodified at the memory position where they were decoded.
 *  If less than 64KB of data has been decoded all the data must be present.
 *
 *  Special : if application sets a ring buffer for decompression, it must respect one of the following conditions :
 *  - Exactly same size as encoding buffer, with same update rule (block boundaries at same positions)
 *    In which case, the decoding & encoding ring buffer can have any size, including very small ones ( < 64 KB).
 *  - Larger than encoding buffer, by a minimum of maxBlockSize more bytes.
 *    maxBlockSize is implementation dependent. It's the maximum size of any single block.
 *    In which case, encoding and decoding buffers do not need to be synchronized,
 *    and encoding ring buffer can have any size, including small ones ( < 64 KB).
 *  - _At least_ 64 KB + 8 bytes + maxBlockSize.
 *    In which case, encoding and decoding buffers do not need to be synchronized,
 *    and encoding ring buffer can have any size, including larger than decoding buffer.
 *  Whenever these conditions are not possible, save the last 64KB of decoded data into a safe buffer,
 *  and indicate where it is saved using LZ4_setStreamDecode() before decompressing next block.
*/
LZ4LIB_API int LZ4_decompress_safe_continue (LZ4_streamDecode_t* LZ4_streamDecode, const char* src, char* dst, int srcSize, int dstCapacity);
LZ4LIB_API int LZ4_decompress_fast_continue (LZ4_streamDecode_t* LZ4_streamDecode, const char* src, char* dst, int originalSize);


/*! LZ4_decompress_*_usingDict() :
 *  These decoding functions work the same as
 *  a combination of LZ4_setStreamDecode() followed by LZ4_decompress_*_continue()
 *  They are stand-alone, and don't need an LZ4_streamDecode_t structure.
 */
LZ4LIB_API int LZ4_decompress_safe_usingDict (const char* src, char* dst, int srcSize, int dstCapcity, const char* dictStart, int dictSize);
LZ4LIB_API int LZ4_decompress_fast_usingDict (const char* src, char* dst, int originalSize, const char* dictStart, int dictSize);


/*^**********************************************
 * !!!!!!   STATIC LINKING ONLY   !!!!!!
 ***********************************************/

/*-************************************
 *  Unstable declarations
 **************************************
 * Declarations in this section should be considered unstable.
 * Use at your own peril, etc., etc.
 * They may be removed in the future.
 * Their signatures may change.
 **************************************/

#ifdef LZ4_STATIC_LINKING_ONLY

/*! LZ4_resetStream_fast() :
 *  When an LZ4_stream_t is known to be in a internally coherent state,
 *  it can often be prepared for a new compression with almost no work, only
 *  sometimes falling back to the full, expensive reset that is always required
 *  when the stream is in an indeterminate state (i.e., the reset performed by
 *  LZ4_resetStream()).
 *
 *  LZ4_streams are guaranteed to be in a valid state when:
 *  - returned from LZ4_createStream()
 *  - reset by LZ4_resetStream()
 *  - memset(stream, 0, sizeof(LZ4_stream_t))
 *  - the stream was in a valid state and was reset by LZ4_resetStream_fast()
 *  - the stream was in a valid state and was then used in any compression call
 *    that returned success
 *  - the stream was in an indeterminate state and was used in a compression
 *    call that fully reset the state (LZ4_compress_fast_extState()) and that
 *    returned success
 */
LZ4LIB_API void LZ4_resetStream_fast (LZ4_stream_t* streamPtr);

/*! LZ4_compress_fast_extState_fastReset() :
 *  A variant of LZ4_compress_fast_extState().
 *
 *  Using this variant avoids an expensive initialization step. It is only safe
 *  to call if the state buffer is known to be correctly initialized already
 *  (see above comment on LZ4_resetStream_fast() for a definition of "correctly
 *  initialized"). From a high level, the difference is that this function
 *  initializes the provided state with a call to LZ4_resetStream_fast() while
 *  LZ4_compress_fast_extState() starts with a call to LZ4_resetStream().
 */
LZ4LIB_API int LZ4_compress_fast_extState_fastReset (void* state, const char* src, char* dst, int srcSize, int dstCapacity, int acceleration);

/*! LZ4_attach_dictionary() :
 *  This is an experimental API that allows for the efficient use of a
 *  static dictionary many times.
 *
 *  Rather than re-loading the dictionary buffer into a working context before
 *  each compression, or copying a pre-loaded dictionary's LZ4_stream_t into a
 *  working LZ4_stream_t, this function introduces a no-copy setup mechanism,
 *  in which the working stream references the dictionary stream in-place.
 *
 *  Several assumptions are made about the state of the dictionary stream.
 *  Currently, only streams which have been prepared by LZ4_loadDict() should
 *  be expected to work.
 *
 *  Alternatively, the provided dictionary stream pointer may be NULL, in which
 *  case any existing dictionary stream is unset.
 *
 *  If a dictionary is provided, it replaces any pre-existing stream history.
 *  The dictionary contents are the only history that can be referenced and
 *  logically immediately precede the data compressed in the first subsequent
 *  compression call.
 *
 *  The dictionary will only remain attached to the working stream through the
 *  first compression call, at the end of which it is cleared. The dictionary
 *  stream (and source buffer) must remain in-place / accessible / unchanged
 *  through the completion of the first compression call on the stream.
 */
LZ4LIB_API void LZ4_attach_dictionary(LZ4_stream_t *working_stream, const LZ4_stream_t *dictionary_stream);

#endif

/*-************************************
 *  Private definitions
 **************************************
 * Do not use these definitions.
 * They are exposed to allow static allocation of `LZ4_stream_t` and `LZ4_streamDecode_t`.
 * Using these definitions will expose code to API and/or ABI break in future versions of the library.
 **************************************/
#define LZ4_HASHLOG   (LZ4_MEMORY_USAGE-2)
#define LZ4_HASHTABLESIZE (1 << LZ4_MEMORY_USAGE)
#define LZ4_HASH_SIZE_U32 (1 << LZ4_HASHLOG)       /* required as macro for static allocation */

#if defined(__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
#include <stdint.h>

typedef struct LZ4_stream_t_internal LZ4_stream_t_internal;
struct LZ4_stream_t_internal {
    uint32_t hashTable[LZ4_HASH_SIZE_U32];
    uint32_t currentOffset;
    uint16_t initCheck;
    uint16_t tableType;
    const uint8_t* dictionary;
    const LZ4_stream_t_internal* dictCtx;
    uint32_t dictSize;
};

typedef struct {
    const uint8_t* externalDict;
    size_t extDictSize;
    const uint8_t* prefixEnd;
    size_t prefixSize;
} LZ4_streamDecode_t_internal;

#else

typedef struct LZ4_stream_t_internal LZ4_stream_t_internal;
struct LZ4_stream_t_internal {
    unsigned int hashTable[LZ4_HASH_SIZE_U32];
    unsigned int currentOffset;
    unsigned short initCheck;
    unsigned short tableType;
    const unsigned char* dictionary;
    const LZ4_stream_t_internal* dictCtx;
    unsigned int dictSize;
};

typedef struct {
    const unsigned char* externalDict;
    size_t extDictSize;
    const unsigned char* prefixEnd;
    size_t prefixSize;
} LZ4_streamDecode_t_internal;

#endif

/*!
 * LZ4_stream_t :
 * information structure to track an LZ4 stream.
 * init this structure before first use.
 * note : only use in association with static linking !
 *        this definition is not API/ABI safe,
 *        it may change in a future version !
 */
#define LZ4_STREAMSIZE_U64 ((1 << (LZ4_MEMORY_USAGE-3)) + 4)
#define LZ4_STREAMSIZE     (LZ4_STREAMSIZE_U64 * sizeof(unsigned long long))
union LZ4_stream_u {
    unsigned long long table[LZ4_STREAMSIZE_U64];
    LZ4_stream_t_internal internal_donotuse;
} ;  /* previously typedef'd to LZ4_stream_t */


/*!
 * LZ4_streamDecode_t :
 * information structure to track an LZ4 stream during decompression.
 * init this structure  using LZ4_setStreamDecode (or memset()) before first use
 * note : only use in association with static linking !
 *        this definition is not API/ABI safe,
 *        and may change in a future version !
 */
#define LZ4_STREAMDECODESIZE_U64  4
#define LZ4_STREAMDECODESIZE     (LZ4_STREAMDECODESIZE_U64 * sizeof(unsigned long long))
union LZ4_streamDecode_u {
    unsigned long long table[LZ4_STREAMDECODESIZE_U64];
    LZ4_streamDecode_t_internal internal_donotuse;
} ;   /* previously typedef'd to LZ4_streamDecode_t */


/*-************************************
*  Obsolete Functions
**************************************/

/*! Deprecation warnings
   Should deprecation warnings be a problem,
   it is generally possible to disable them,
   typically with -Wno-deprecated-declarations for gcc
   or _CRT_SECURE_NO_WARNINGS in Visual.
   Otherwise, it's also possible to define LZ4_DISABLE_DEPRECATE_WARNINGS */
#ifdef LZ4_DISABLE_DEPRECATE_WARNINGS
#  define LZ4_DEPRECATED(message)   /* disable deprecation warnings */
#else
#  define LZ4_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#  if defined (__cplusplus) && (__cplusplus >= 201402) /* C++14 or greater */
#    define LZ4_DEPRECATED(message) [[deprecated(message)]]
#  elif (LZ4_GCC_VERSION >= 405) || defined(__clang__)
#    define LZ4_DEPRECATED(message) __attribute__((deprecated(message)))
#  elif (LZ4_GCC_VERSION >= 301)
#    define LZ4_DEPRECATED(message) __attribute__((deprecated))
#  elif defined(_MSC_VER)
#    define LZ4_DEPRECATED(message) __declspec(deprecated(message))
#  else
#    pragma message("WARNING: You need to implement LZ4_DEPRECATED for this compiler")
#    define LZ4_DEPRECATED(message)
#  endif
#endif /* LZ4_DISABLE_DEPRECATE_WARNINGS */

/* Obsolete compression functions */
LZ4_DEPRECATED("use LZ4_compress_default() instead") LZ4LIB_API int LZ4_compress               (const char* source, char* dest, int sourceSize);
LZ4_DEPRECATED("use LZ4_compress_default() instead") LZ4LIB_API int LZ4_compress_limitedOutput (const char* source, char* dest, int sourceSize, int maxOutputSize);
LZ4_DEPRECATED("use LZ4_compress_fast_extState() instead") LZ4LIB_API int LZ4_compress_withState               (void* state, const char* source, char* dest, int inputSize);
LZ4_DEPRECATED("use LZ4_compress_fast_extState() instead") LZ4LIB_API int LZ4_compress_limitedOutput_withState (void* state, const char* source, char* dest, int inputSize, int maxOutputSize);
LZ4_DEPRECATED("use LZ4_compress_fast_continue() instead") LZ4LIB_API int LZ4_compress_continue                (LZ4_stream_t* LZ4_streamPtr, const char* source, char* dest, int inputSize);
LZ4_DEPRECATED("use LZ4_compress_fast_continue() instead") LZ4LIB_API int LZ4_compress_limitedOutput_continue  (LZ4_stream_t* LZ4_streamPtr, const char* source, char* dest, int inputSize, int maxOutputSize);

/* Obsolete decompression functions */
LZ4_DEPRECATED("use LZ4_decompress_fast() instead") LZ4LIB_API int LZ4_uncompress (const char* source, char* dest, int outputSize);
LZ4_DEPRECATED("use LZ4_decompress_safe() instead") LZ4LIB_API int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize);

/* Obsolete streaming functions; degraded functionality; do not use!
 *
 * In order to perform streaming compression, these functions depended on data
 * that is no longer tracked in the state. They have been preserved as well as
 * possible: using them will still produce a correct output. However, they don't
 * actually retain any history between compression calls. The compression ratio
 * achieved will therefore be no better than compressing each chunk
 * independently.
 */
LZ4_DEPRECATED("Use LZ4_createStream() instead") LZ4LIB_API void* LZ4_create (char* inputBuffer);
LZ4_DEPRECATED("Use LZ4_createStream() instead") LZ4LIB_API int   LZ4_sizeofStreamState(void);
LZ4_DEPRECATED("Use LZ4_resetStream() instead") LZ4LIB_API  int   LZ4_resetStreamState(void* state, char* inputBuffer);
LZ4_DEPRECATED("Use LZ4_saveDict() instead") LZ4LIB_API     char* LZ4_slideInputBuffer (void* state);

/* Obsolete streaming decoding functions */
LZ4_DEPRECATED("use LZ4_decompress_safe_usingDict() instead") LZ4LIB_API int LZ4_decompress_safe_withPrefix64k (const char* src, char* dst, int compressedSize, int maxDstSize);
LZ4_DEPRECATED("use LZ4_decompress_fast_usingDict() instead") LZ4LIB_API int LZ4_decompress_fast_withPrefix64k (const char* src, char* dst, int originalSize);

#endif /* LZ4_H_2983827168210 */


#if defined (__cplusplus)
}
#endif
