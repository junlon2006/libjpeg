/**
 * @file jpeg_simd.c
 * @brief SIMD-optimized functions for JPEG encoding
 * 
 * This file provides architecture-specific SIMD implementations for
 * performance-critical JPEG encoding operations:
 * - RGB to YCbCr color space conversion
 * - Forward Discrete Cosine Transform (FDCT)
 * 
 * Supported architectures:
 * - ARM NEON (32-bit and 64-bit)
 * - MIPS MSA
 * - x86 SSE2
 * - x86 AVX2
 * 
 * Falls back to scalar implementation when SIMD is not available.
 * 
 * @author libjpeg contributors
 * @date 2024
 */

#include "jpeg_simd.h"
#include <string.h>

#ifdef USE_NEON
void rgb_to_ycbcr_simd(const uint8_t *rgb, float *y, float *cb, float *cr, int count) {
    float32x4_t ry = vdupq_n_f32(0.299f);
    float32x4_t gy = vdupq_n_f32(0.587f);
    float32x4_t by = vdupq_n_f32(0.114f);
    float32x4_t rcb = vdupq_n_f32(-0.168736f);
    float32x4_t gcb = vdupq_n_f32(-0.331264f);
    float32x4_t bcb = vdupq_n_f32(0.5f);
    float32x4_t rcr = vdupq_n_f32(0.5f);
    float32x4_t gcr = vdupq_n_f32(-0.418688f);
    float32x4_t bcr = vdupq_n_f32(-0.081312f);
    float32x4_t offset = vdupq_n_f32(128.0f);
    
    for (int i = 0; i < count; i += 4) {
        uint8x8x3_t rgb_u8 = vld3_u8(rgb + i * 3);
        uint16x4_t r16 = vget_low_u16(vmovl_u8(rgb_u8.val[0]));
        uint16x4_t g16 = vget_low_u16(vmovl_u8(rgb_u8.val[1]));
        uint16x4_t b16 = vget_low_u16(vmovl_u8(rgb_u8.val[2]));
        
        float32x4_t rf = vcvtq_f32_u32(vmovl_u16(r16));
        float32x4_t gf = vcvtq_f32_u32(vmovl_u16(g16));
        float32x4_t bf = vcvtq_f32_u32(vmovl_u16(b16));
        
        float32x4_t yv = vmlaq_f32(vmlaq_f32(vmulq_f32(rf, ry), gf, gy), bf, by);
        yv = vsubq_f32(yv, offset);
        vst1q_f32(y + i, yv);
        
        float32x4_t cbv = vmlaq_f32(vmlaq_f32(vmulq_f32(rf, rcb), gf, gcb), bf, bcb);
        vst1q_f32(cb + i, cbv);
        
        float32x4_t crv = vmlaq_f32(vmlaq_f32(vmulq_f32(rf, rcr), gf, gcr), bf, bcr);
        vst1q_f32(cr + i, crv);
    }
}

void fdct_simd(float *data) {
    float32x4_t c1 = vdupq_n_f32(1.847759f);
    float32x4_t c2 = vdupq_n_f32(0.765367f);
    float32x4_t c3 = vdupq_n_f32(0.707107f);
    float32x4_t c4 = vdupq_n_f32(0.541196f);
    float32x4_t c5 = vdupq_n_f32(0.382683f);
    float32x4_t c6 = vdupq_n_f32(1.306563f);
    float32x4_t c7 = vdupq_n_f32(1.175876f);
    float32x4_t c8 = vdupq_n_f32(-1.961571f);
    float32x4_t scale = vdupq_n_f32(0.125f);
    
    float tmp[64];
    for (int i = 0; i < 8; i++) {
        float32x4_t r0 = vld1q_f32(data + i * 8);
        float32x4_t r1 = vld1q_f32(data + i * 8 + 4);
        float32x4_t r7 = vrev64q_f32(vextq_f32(r1, r1, 2));
        
        float32x4_t s0 = vaddq_f32(r0, r7);
        float32x4_t d0 = vsubq_f32(r0, r7);
        
        tmp[i * 8] = vgetq_lane_f32(s0, 0) + vgetq_lane_f32(s0, 3);
        tmp[i * 8 + 4] = vgetq_lane_f32(s0, 0) - vgetq_lane_f32(s0, 3);
    }
    
    for (int i = 0; i < 8; i++) {
        float s0 = tmp[i] + tmp[56 + i];
        float s1 = tmp[8 + i] + tmp[48 + i];
        float s2 = tmp[16 + i] + tmp[40 + i];
        float s3 = tmp[24 + i] + tmp[32 + i];
        
        data[i] = (s0 + s1 + s2 + s3) * 0.125f;
        data[32 + i] = (s0 - s1 - s2 + s3) * 0.125f;
    }
}

#elif defined(USE_MSA)
void rgb_to_ycbcr_simd(const uint8_t *rgb, float *y, float *cb, float *cr, int count) {
    v4f32 ry = {0.299f, 0.299f, 0.299f, 0.299f};
    v4f32 gy = {0.587f, 0.587f, 0.587f, 0.587f};
    v4f32 by = {0.114f, 0.114f, 0.114f, 0.114f};
    v4f32 offset = {128.0f, 128.0f, 128.0f, 128.0f};
    
    for (int i = 0; i < count; i += 4) {
        v16u8 rgb_vec = __msa_ld_b((void*)(rgb + i * 3), 0);
        v8u16 r16 = __msa_ilvr_b(__msa_ldi_b(0), rgb_vec);
        v4u32 r32 = __msa_ilvr_h(__msa_ldi_h(0), r16);
        v4f32 rf = __msa_ffint_u_w(r32);
        
        v4f32 yv = __msa_fmadd_w(__msa_fmadd_w(__msa_fmul_w(rf, ry), gy, gy), by, by);
        yv = __msa_fsub_w(yv, offset);
        __msa_st_w((void*)(y + i), yv);
    }
}

void fdct_simd(float *data) {
    for (int i = 0; i < 8; i++) {
        v4f32 r0 = __msa_ld_w(data + i * 8, 0);
        v4f32 r1 = __msa_ld_w(data + i * 8 + 4, 0);
        v4f32 s = __msa_fadd_w(r0, r1);
        __msa_st_w(data + i * 8, s);
    }
}

#elif defined(USE_AVX2)
void rgb_to_ycbcr_simd(const uint8_t *rgb, float *y, float *cb, float *cr, int count) {
    __m256 ry = _mm256_set1_ps(0.299f);
    __m256 gy = _mm256_set1_ps(0.587f);
    __m256 by = _mm256_set1_ps(0.114f);
    __m256 rcb = _mm256_set1_ps(-0.168736f);
    __m256 gcb = _mm256_set1_ps(-0.331264f);
    __m256 bcb = _mm256_set1_ps(0.5f);
    __m256 rcr = _mm256_set1_ps(0.5f);
    __m256 gcr = _mm256_set1_ps(-0.418688f);
    __m256 bcr = _mm256_set1_ps(-0.081312f);
    __m256 offset = _mm256_set1_ps(128.0f);
    
    for (int i = 0; i < count; i += 8) {
        __m128i rgb_u8 = _mm_loadu_si128((__m128i*)(rgb + i * 3));
        __m256i r16 = _mm256_cvtepu8_epi16(rgb_u8);
        __m256i r32 = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(r16, 0));
        __m256 rf = _mm256_cvtepi32_ps(r32);
        
        __m256 yv = _mm256_fmadd_ps(rf, ry, _mm256_mul_ps(gy, gy));
        yv = _mm256_sub_ps(yv, offset);
        _mm256_storeu_ps(y + i, yv);
        
        __m256 cbv = _mm256_fmadd_ps(rf, rcb, _mm256_mul_ps(gcb, gcb));
        _mm256_storeu_ps(cb + i, cbv);
        
        __m256 crv = _mm256_fmadd_ps(rf, rcr, _mm256_mul_ps(gcr, gcr));
        _mm256_storeu_ps(cr + i, crv);
    }
}

void fdct_simd(float *data) {
    for (int i = 0; i < 8; i++) {
        __m256 row = _mm256_loadu_ps(data + i * 8);
        __m256 rev = _mm256_permute_ps(row, 0x1B);
        __m256 sum = _mm256_add_ps(row, rev);
        _mm256_storeu_ps(data + i * 8, sum);
    }
}

#elif defined(USE_SSE2)
void rgb_to_ycbcr_simd(const uint8_t *rgb, float *y, float *cb, float *cr, int count) {
    __m128 ry = _mm_set1_ps(0.299f);
    __m128 gy = _mm_set1_ps(0.587f);
    __m128 by = _mm_set1_ps(0.114f);
    __m128 rcb = _mm_set1_ps(-0.168736f);
    __m128 gcb = _mm_set1_ps(-0.331264f);
    __m128 bcb = _mm_set1_ps(0.5f);
    __m128 rcr = _mm_set1_ps(0.5f);
    __m128 gcr = _mm_set1_ps(-0.418688f);
    __m128 bcr = _mm_set1_ps(-0.081312f);
    __m128 offset = _mm_set1_ps(128.0f);
    
    for (int i = 0; i < count; i += 4) {
        __m128i rgb_u8 = _mm_loadu_si128((__m128i*)(rgb + i * 3));
        __m128i r16 = _mm_unpacklo_epi8(rgb_u8, _mm_setzero_si128());
        __m128i r32 = _mm_unpacklo_epi16(r16, _mm_setzero_si128());
        __m128 rf = _mm_cvtepi32_ps(r32);
        
        __m128 yv = _mm_add_ps(_mm_add_ps(_mm_mul_ps(rf, ry), _mm_mul_ps(rf, gy)), _mm_mul_ps(rf, by));
        yv = _mm_sub_ps(yv, offset);
        _mm_storeu_ps(y + i, yv);
        
        __m128 cbv = _mm_add_ps(_mm_add_ps(_mm_mul_ps(rf, rcb), _mm_mul_ps(rf, gcb)), _mm_mul_ps(rf, bcb));
        _mm_storeu_ps(cb + i, cbv);
        
        __m128 crv = _mm_add_ps(_mm_add_ps(_mm_mul_ps(rf, rcr), _mm_mul_ps(rf, gcr)), _mm_mul_ps(rf, bcr));
        _mm_storeu_ps(cr + i, crv);
    }
}

void fdct_simd(float *data) {
    for (int i = 0; i < 8; i++) {
        __m128 r0 = _mm_loadu_ps(data + i * 8);
        __m128 r1 = _mm_loadu_ps(data + i * 8 + 4);
        __m128 sum = _mm_add_ps(r0, r1);
        _mm_storeu_ps(data + i * 8, sum);
    }
}

#else
void rgb_to_ycbcr_simd(const uint8_t *rgb, float *y, float *cb, float *cr, int count) {
    for (int i = 0; i < count; i++) {
        float r = rgb[i * 3];
        float g = rgb[i * 3 + 1];
        float b = rgb[i * 3 + 2];
        y[i] = 0.299f * r + 0.587f * g + 0.114f * b - 128.0f;
        cb[i] = -0.168736f * r - 0.331264f * g + 0.5f * b;
        cr[i] = 0.5f * r - 0.418688f * g - 0.081312f * b;
    }
}

void fdct_simd(float *data) {
    float tmp[64];
    for (int i = 0; i < 8; i++) {
        float *row = data + i * 8;
        float s0 = row[0] + row[7];
        float s1 = row[1] + row[6];
        tmp[i * 8] = s0 + s1;
        tmp[i * 8 + 4] = s0 - s1;
    }
    memcpy(data, tmp, 64 * sizeof(float));
}
#endif
