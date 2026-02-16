/**
 * @file jpeg_simd.h
 * @brief SIMD-optimized functions for JPEG encoding
 * 
 * Provides vectorized implementations of color space conversion and DCT
 * transform for various architectures including ARM NEON, AArch64, MIPS MSA,
 * and x86 SSE/AVX. Falls back to scalar implementation when SIMD is not available.
 */

#ifndef JPEG_SIMD_H
#define JPEG_SIMD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Detect SIMD capabilities */
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
    #define USE_NEON
#elif defined(__aarch64__)
    #include <arm_neon.h>
    #define USE_NEON
#elif defined(__mips_msa)
    #include <msa.h>
    #define USE_MSA
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    #include <emmintrin.h>
    #define USE_SSE2
    #ifdef __AVX2__
        #include <immintrin.h>
        #define USE_AVX2
    #endif
#endif

/**
 * @brief SIMD-optimized RGB to YCbCr color space conversion
 * 
 * Converts RGB pixels to YCbCr color space using SIMD instructions when available.
 * Falls back to scalar implementation on platforms without SIMD support.
 * 
 * @param rgb Input RGB data (interleaved R,G,B format)
 * @param y Output Y (luminance) values
 * @param cb Output Cb (blue chroma) values
 * @param cr Output Cr (red chroma) values
 * @param count Number of pixels to convert
 * 
 * @note For optimal performance, count should be a multiple of the SIMD vector size
 */
void rgb_to_ycbcr_simd(const uint8_t *rgb, float *y, float *cb, float *cr, int count);

/**
 * @brief SIMD-optimized Forward Discrete Cosine Transform (FDCT)
 * 
 * Performs 8x8 FDCT using SIMD instructions when available. This is a key
 * operation in JPEG compression that transforms spatial domain data to
 * frequency domain.
 * 
 * @param data Input/output 8x8 block of float values (64 elements)
 * 
 * @note Input data is modified in-place
 * @note Data should be 16-byte aligned for best SIMD performance
 */
void fdct_simd(float *data);

#ifdef __cplusplus
}
#endif

#endif
