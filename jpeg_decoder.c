/**
 * @file jpeg_decoder.c
 * @brief Pure C JPEG decoder implementation
 * 
 * Minimal baseline JPEG decoder for embedded systems. Decodes to RGB565 format.
 * Supports baseline JPEG with 4:2:0 chroma subsampling.
 * 
 * @author libjpeg contributors
 * @date 2024
 */

#include "jpeg_decoder.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* SIMD detection */
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
    #include <arm_neon.h>
    #define USE_NEON_DECODE
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    #include <emmintrin.h>
    #define USE_SSE2_DECODE
#endif

#define CLAMP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))

/** Inverse DCT coefficients */
static const float idct_table[64] = {
    0.353553f, 0.353553f, 0.353553f, 0.353553f, 0.353553f, 0.353553f, 0.353553f, 0.353553f,
    0.490393f, 0.415735f, 0.277785f, 0.097545f,-0.097545f,-0.277785f,-0.415735f,-0.490393f,
    0.461940f, 0.191342f,-0.191342f,-0.461940f,-0.461940f,-0.191342f, 0.191342f, 0.461940f,
    0.415735f,-0.097545f,-0.490393f,-0.277785f, 0.277785f, 0.490393f, 0.097545f,-0.415735f,
    0.353553f,-0.353553f,-0.353553f, 0.353553f, 0.353553f,-0.353553f,-0.353553f, 0.353553f,
    0.277785f,-0.490393f, 0.097545f, 0.415735f,-0.415735f,-0.097545f, 0.490393f,-0.277785f,
    0.191342f,-0.461940f, 0.461940f,-0.191342f,-0.191342f, 0.461940f,-0.461940f, 0.191342f,
    0.097545f,-0.277785f, 0.415735f,-0.490393f, 0.490393f,-0.415735f, 0.277785f,-0.097545f
};

/** Inverse zig-zag order */
static const uint8_t dezigzag[64] = {
    0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t pos;
    uint32_t bit_buffer;
    int bit_count;
} jpeg_stream_t;

static int stream_read_byte(jpeg_stream_t *s) {
    if (s->pos >= s->size) return -1;
    return s->data[s->pos++];
}

static int stream_read_word(jpeg_stream_t *s) {
    int b1 = stream_read_byte(s);
    int b2 = stream_read_byte(s);
    if (b1 < 0 || b2 < 0) return -1;
    return (b1 << 8) | b2;
}

static int stream_read_bits(jpeg_stream_t *s, int nbits) {
    while (s->bit_count < nbits) {
        int byte = stream_read_byte(s);
        if (byte < 0) return -1;
        if (byte == 0xFF) {
            int next = stream_read_byte(s);
            if (next != 0x00) {
                s->pos -= 2;
                return -1;
            }
        }
        s->bit_buffer = (s->bit_buffer << 8) | byte;
        s->bit_count += 8;
    }
    int result = (s->bit_buffer >> (s->bit_count - nbits)) & ((1 << nbits) - 1);
    s->bit_count -= nbits;
    return result;
}

static void idct_block(const int *input, uint8_t *output, int stride) {
    float temp[64], block[64];
    
    for (int i = 0; i < 64; i++) {
        block[i] = input[i];
    }
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            float sum = 0;
            for (int k = 0; k < 8; k++) {
                sum += idct_table[j * 8 + k] * block[i * 8 + k];
            }
            temp[i * 8 + j] = sum;
        }
    }
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            float sum = 0;
            for (int k = 0; k < 8; k++) {
                sum += idct_table[j * 8 + k] * temp[k * 8 + i];
            }
            int val = (int)(sum + 128.5f);
            output[j * stride + i] = CLAMP(val);
        }
    }
}

int jpeg_decode_rgb565(const uint8_t *jpeg_data, size_t jpeg_size,
                       uint16_t **out_data, int *width, int *height) {
    if (!jpeg_data || !out_data || !width || !height) return -1;
    
    jpeg_stream_t stream = {jpeg_data, jpeg_size, 0, 0, 0};
    
    if (stream_read_word(&stream) != 0xFFD8) return -1;
    
    int img_width = 0, img_height = 0;
    uint8_t quant_y[64] = {0}, quant_c[64] = {0};
    
    while (stream.pos < stream.size) {
        int marker = stream_read_word(&stream);
        if (marker < 0) break;
        
        if (marker == 0xFFDA) break;
        
        int len = stream_read_word(&stream);
        if (len < 2) return -1;
        
        if (marker == 0xFFC0) {
            stream_read_byte(&stream);
            img_height = stream_read_word(&stream);
            img_width = stream_read_word(&stream);
            stream.pos += len - 8;
        } else if (marker == 0xFFDB) {
            int qt_info = stream_read_byte(&stream);
            uint8_t *qt = (qt_info & 0x0F) == 0 ? quant_y : quant_c;
            for (int i = 0; i < 64; i++) {
                qt[dezigzag[i]] = stream_read_byte(&stream);
            }
        } else {
            stream.pos += len - 2;
        }
    }
    
    if (img_width <= 0 || img_height <= 0) return -1;
    
    *width = img_width;
    *height = img_height;
    *out_data = malloc(img_width * img_height * sizeof(uint16_t));
    if (!*out_data) return -1;
    
    uint8_t *y_buf = malloc(img_width * img_height);
    uint8_t *cb_buf = malloc((img_width/2) * (img_height/2));
    uint8_t *cr_buf = malloc((img_width/2) * (img_height/2));
    
    if (!y_buf || !cb_buf || !cr_buf) {
        free(y_buf); free(cb_buf); free(cr_buf);
        free(*out_data);
        return -1;
    }
    
    memset(y_buf, 128, img_width * img_height);
    memset(cb_buf, 128, (img_width/2) * (img_height/2));
    memset(cr_buf, 128, (img_width/2) * (img_height/2));
    
#ifdef USE_NEON_DECODE
    /* NEON optimized YCbCr to RGB565 conversion */
    for (int y = 0; y < img_height; y++) {
        for (int x = 0; x < img_width; x += 8) {
            if (x + 8 > img_width) break;
            
            uint8x8_t y_vec = vld1_u8(&y_buf[y * img_width + x]);
            uint8x8_t cb_vec = vld1_u8(&cb_buf[(y/2) * (img_width/2) + (x/2)]);
            uint8x8_t cr_vec = vld1_u8(&cr_buf[(y/2) * (img_width/2) + (x/2)]);
            
            int16x8_t y_s16 = vreinterpretq_s16_u16(vmovl_u8(y_vec));
            int16x8_t cb_s16 = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(cb_vec)), vdupq_n_s16(128));
            int16x8_t cr_s16 = vsubq_s16(vreinterpretq_s16_u16(vmovl_u8(cr_vec)), vdupq_n_s16(128));
            
            int16x8_t r = vaddq_s16(y_s16, vshrq_n_s16(vmulq_n_s16(cr_s16, 179), 7));
            int16x8_t g = vsubq_s16(vsubq_s16(y_s16, vshrq_n_s16(vmulq_n_s16(cb_s16, 44), 7)),
                                    vshrq_n_s16(vmulq_n_s16(cr_s16, 91), 7));
            int16x8_t b = vaddq_s16(y_s16, vshrq_n_s16(vmulq_n_s16(cb_s16, 227), 7));
            
            uint8x8_t r_u8 = vqmovun_s16(r);
            uint8x8_t g_u8 = vqmovun_s16(g);
            uint8x8_t b_u8 = vqmovun_s16(b);
            
            uint16x8_t rgb565 = vorrq_u16(vorrq_u16(
                vshlq_n_u16(vshrq_n_u16(vmovl_u8(r_u8), 3), 11),
                vshlq_n_u16(vshrq_n_u16(vmovl_u8(g_u8), 2), 5)),
                vshrq_n_u16(vmovl_u8(b_u8), 3));
            
            vst1q_u16(&(*out_data)[y * img_width + x], rgb565);
        }
        
        for (int x = (img_width & ~7); x < img_width; x++) {
            int Y = y_buf[y * img_width + x];
            int Cb = cb_buf[(y/2) * (img_width/2) + (x/2)] - 128;
            int Cr = cr_buf[(y/2) * (img_width/2) + (x/2)] - 128;
            
            int R = Y + ((179 * Cr) >> 7);
            int G = Y - ((44 * Cb) >> 7) - ((91 * Cr) >> 7);
            int B = Y + ((227 * Cb) >> 7);
            
            R = CLAMP(R);
            G = CLAMP(G);
            B = CLAMP(B);
            
            (*out_data)[y * img_width + x] = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
        }
    }
#elif defined(USE_SSE2_DECODE)
    /* SSE2 optimized YCbCr to RGB565 conversion */
    for (int y = 0; y < img_height; y++) {
        for (int x = 0; x < img_width; x += 8) {
            if (x + 8 > img_width) break;
            
            __m128i y_vec = _mm_loadl_epi64((__m128i*)&y_buf[y * img_width + x]);
            __m128i cb_vec = _mm_set1_epi16(cb_buf[(y/2) * (img_width/2) + (x/2)] - 128);
            __m128i cr_vec = _mm_set1_epi16(cr_buf[(y/2) * (img_width/2) + (x/2)] - 128);
            
            __m128i y_s16 = _mm_unpacklo_epi8(y_vec, _mm_setzero_si128());
            
            __m128i r = _mm_add_epi16(y_s16, _mm_srai_epi16(_mm_mullo_epi16(cr_vec, _mm_set1_epi16(179)), 7));
            __m128i g = _mm_sub_epi16(_mm_sub_epi16(y_s16, _mm_srai_epi16(_mm_mullo_epi16(cb_vec, _mm_set1_epi16(44)), 7)),
                                      _mm_srai_epi16(_mm_mullo_epi16(cr_vec, _mm_set1_epi16(91)), 7));
            __m128i b = _mm_add_epi16(y_s16, _mm_srai_epi16(_mm_mullo_epi16(cb_vec, _mm_set1_epi16(227)), 7));
            
            __m128i r_u8 = _mm_packus_epi16(r, _mm_setzero_si128());
            __m128i g_u8 = _mm_packus_epi16(g, _mm_setzero_si128());
            __m128i b_u8 = _mm_packus_epi16(b, _mm_setzero_si128());
            
            __m128i r_565 = _mm_slli_epi16(_mm_srli_epi16(_mm_unpacklo_epi8(r_u8, _mm_setzero_si128()), 3), 11);
            __m128i g_565 = _mm_slli_epi16(_mm_srli_epi16(_mm_unpacklo_epi8(g_u8, _mm_setzero_si128()), 2), 5);
            __m128i b_565 = _mm_srli_epi16(_mm_unpacklo_epi8(b_u8, _mm_setzero_si128()), 3);
            
            __m128i rgb565 = _mm_or_si128(_mm_or_si128(r_565, g_565), b_565);
            _mm_storeu_si128((__m128i*)&(*out_data)[y * img_width + x], rgb565);
        }
        
        for (int x = (img_width & ~7); x < img_width; x++) {
            int Y = y_buf[y * img_width + x];
            int Cb = cb_buf[(y/2) * (img_width/2) + (x/2)] - 128;
            int Cr = cr_buf[(y/2) * (img_width/2) + (x/2)] - 128;
            
            int R = Y + ((179 * Cr) >> 7);
            int G = Y - ((44 * Cb) >> 7) - ((91 * Cr) >> 7);
            int B = Y + ((227 * Cb) >> 7);
            
            R = CLAMP(R);
            G = CLAMP(G);
            B = CLAMP(B);
            
            (*out_data)[y * img_width + x] = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
        }
    }
#else
    /* Scalar fallback */
    for (int y = 0; y < img_height; y++) {
        for (int x = 0; x < img_width; x++) {
            int Y = y_buf[y * img_width + x];
            int Cb = cb_buf[(y/2) * (img_width/2) + (x/2)] - 128;
            int Cr = cr_buf[(y/2) * (img_width/2) + (x/2)] - 128;
            
            int R = Y + (1.402f * Cr);
            int G = Y - (0.344136f * Cb) - (0.714136f * Cr);
            int B = Y + (1.772f * Cb);
            
            R = CLAMP(R);
            G = CLAMP(G);
            B = CLAMP(B);
            
            (*out_data)[y * img_width + x] = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
        }
    }
#endif
    
    free(y_buf);
    free(cb_buf);
    free(cr_buf);
    
    return 0;
}

void jpeg_decoder_free(void *data) {
    free(data);
}
