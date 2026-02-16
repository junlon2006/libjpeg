/**
 * @file jpeg_encoder.c
 * @brief Pure C JPEG encoder implementation
 * 
 * This file implements a baseline JPEG encoder with support for RGB, YUV420,
 * and UYVY input formats. The encoder performs the complete JPEG encoding
 * pipeline including color space conversion, DCT transform, quantization,
 * and Huffman entropy coding.
 * 
 * Key features:
 * - Baseline JPEG (ISO/IEC 10918-1)
 * - 4:2:0 chroma subsampling
 * - Standard quantization tables
 * - Standard Huffman tables
 * - Optional SIMD optimizations
 * 
 * @author libjpeg contributors
 * @date 2024
 */

#include "jpeg_encoder.h"
#include "jpeg_simd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/** Initial capacity for JPEG output buffer (64KB) */
#define JPEG_BUFFER_INITIAL_CAPACITY 65536

/** Zig-zag scan order for 8x8 DCT coefficients */

static const uint8_t zigzag[64] = {
    0,  1,  5,  6, 14, 15, 27, 28,
    2,  4,  7, 13, 16, 26, 29, 42,
    3,  8, 12, 17, 25, 30, 41, 43,
    9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

static const uint8_t std_dc_luminance_nrcodes[] = {0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
static const uint8_t std_dc_luminance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
static const uint8_t std_ac_luminance_nrcodes[] = {0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d};
static const uint8_t std_ac_luminance_values[] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
    0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,
    0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
    0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
    0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
    0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
    0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
    0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
    0xf9,0xfa
};

static const uint8_t std_dc_chrominance_nrcodes[] = {0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
static const uint8_t std_dc_chrominance_values[] = {0,1,2,3,4,5,6,7,8,9,10,11};
static const uint8_t std_ac_chrominance_nrcodes[] = {0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
static const uint8_t std_ac_chrominance_values[] = {
    0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
    0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,
    0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
    0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,
    0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,
    0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,
    0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,
    0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
    0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
    0xf9,0xfa
};

typedef struct {
    uint16_t code[256];
    uint8_t size[256];
} huffman_table_t;

static huffman_table_t dc_luma_table, dc_chroma_table, ac_luma_table, ac_chroma_table;

static void init_huffman_table(huffman_table_t *table, const uint8_t *nrcodes, const uint8_t *values) {
    int p = 0, code = 0;
    for (int l = 1; l <= 16; l++) {
        for (int i = 0; i < nrcodes[l]; i++) {
            table->code[values[p]] = code;
            table->size[values[p]] = l;
            p++;
            code++;
        }
        code <<= 1;
    }
}

static void init_quantization_table(uint8_t *table, int quality, int is_luma) {
    static const uint8_t std_luma[64] = {
        16, 11, 10, 16, 24, 40, 51, 61,
        12, 12, 14, 19, 26, 58, 60, 55,
        14, 13, 16, 24, 40, 57, 69, 56,
        14, 17, 22, 29, 51, 87, 80, 62,
        18, 22, 37, 56, 68,109,103, 77,
        24, 35, 55, 64, 81,104,113, 92,
        49, 64, 78, 87,103,121,120,101,
        72, 92, 95, 98,112,100,103, 99
    };
    static const uint8_t std_chroma[64] = {
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99
    };
    
    const uint8_t *std = is_luma ? std_luma : std_chroma;
    int scale = quality < 50 ? 5000 / quality : 200 - quality * 2;
    
    for (int i = 0; i < 64; i++) {
        int val = (std[i] * scale + 50) / 100;
        if (val < 1) val = 1;
        if (val > 255) val = 255;
        table[i] = val;
    }
}

static void buffer_init(jpeg_buffer_t *buf) {
    buf->capacity = JPEG_BUFFER_INITIAL_CAPACITY;
    buf->data = malloc(buf->capacity);
    buf->size = 0;
    buf->bit_buffer = 0;
    buf->bit_count = 0;
}

static void buffer_write_byte(jpeg_buffer_t *buf, uint8_t byte) {
    if (buf->size >= buf->capacity) {
        buf->capacity *= 2;
        buf->data = realloc(buf->data, buf->capacity);
    }
    buf->data[buf->size++] = byte;
}

static void buffer_write_word(jpeg_buffer_t *buf, uint16_t word) {
    buffer_write_byte(buf, word >> 8);
    buffer_write_byte(buf, word & 0xFF);
}

static void buffer_write_bits(jpeg_buffer_t *buf, uint16_t bits, int nbits) {
    buf->bit_buffer = (buf->bit_buffer << nbits) | bits;
    buf->bit_count += nbits;
    
    while (buf->bit_count >= 8) {
        uint8_t byte = (buf->bit_buffer >> (buf->bit_count - 8)) & 0xFF;
        buffer_write_byte(buf, byte);
        if (byte == 0xFF) {
            buffer_write_byte(buf, 0x00);
        }
        buf->bit_count -= 8;
    }
}

static void buffer_flush_bits(jpeg_buffer_t *buf) {
    if (buf->bit_count > 0) {
        uint8_t byte = (buf->bit_buffer << (8 - buf->bit_count)) & 0xFF;
        buffer_write_byte(buf, byte);
        if (byte == 0xFF) {
            buffer_write_byte(buf, 0x00);
        }
    }
    buf->bit_count = 0;
    buf->bit_buffer = 0;
}

static void fdct(float *data) {
    float tmp[64];
    
    for (int i = 0; i < 8; i++) {
        float *row = data + i * 8;
        float s0 = row[0] + row[7];
        float s1 = row[1] + row[6];
        float s2 = row[2] + row[5];
        float s3 = row[3] + row[4];
        float d0 = row[0] - row[7];
        float d1 = row[1] - row[6];
        float d2 = row[2] - row[5];
        float d3 = row[3] - row[4];
        
        float t0 = s0 + s3;
        float t1 = s1 + s2;
        float t2 = s0 - s3;
        float t3 = s1 - s2;
        
        tmp[i * 8 + 0] = t0 + t1;
        tmp[i * 8 + 4] = t0 - t1;
        tmp[i * 8 + 2] = t2 * 1.847759f + t3 * 0.765367f;
        tmp[i * 8 + 6] = t2 * 0.765367f - t3 * 1.847759f;
        
        float z1 = (d0 + d3) * 0.707107f;
        float z2 = (d1 + d2) * 0.541196f;
        float z3 = d0 * 0.382683f + z1;
        float z4 = d3 * 1.306563f + z1;
        float z5 = d1 * 1.175876f + z2;
        float z6 = d2 * -1.961571f + z2;
        
        tmp[i * 8 + 5] = z3 + z5;
        tmp[i * 8 + 3] = z4 + z6;
        tmp[i * 8 + 1] = z3 + z6;
        tmp[i * 8 + 7] = z4 + z5;
    }
    
    for (int i = 0; i < 8; i++) {
        float s0 = tmp[i] + tmp[56 + i];
        float s1 = tmp[8 + i] + tmp[48 + i];
        float s2 = tmp[16 + i] + tmp[40 + i];
        float s3 = tmp[24 + i] + tmp[32 + i];
        float d0 = tmp[i] - tmp[56 + i];
        float d1 = tmp[8 + i] - tmp[48 + i];
        float d2 = tmp[16 + i] - tmp[40 + i];
        float d3 = tmp[24 + i] - tmp[32 + i];
        
        float t0 = s0 + s3;
        float t1 = s1 + s2;
        float t2 = s0 - s3;
        float t3 = s1 - s2;
        
        data[i] = (t0 + t1) * 0.125f;
        data[32 + i] = (t0 - t1) * 0.125f;
        data[16 + i] = (t2 * 1.847759f + t3 * 0.765367f) * 0.125f;
        data[48 + i] = (t2 * 0.765367f - t3 * 1.847759f) * 0.125f;
        
        float z1 = (d0 + d3) * 0.707107f;
        float z2 = (d1 + d2) * 0.541196f;
        float z3 = d0 * 0.382683f + z1;
        float z4 = d3 * 1.306563f + z1;
        float z5 = d1 * 1.175876f + z2;
        float z6 = d2 * -1.961571f + z2;
        
        data[40 + i] = (z3 + z5) * 0.125f;
        data[24 + i] = (z4 + z6) * 0.125f;
        data[8 + i] = (z3 + z6) * 0.125f;
        data[56 + i] = (z4 + z5) * 0.125f;
    }
}

static void encode_block(jpeg_buffer_t *buf, float *block, const uint8_t *quant, 
                        int *prev_dc, huffman_table_t *dc_table, huffman_table_t *ac_table) {
    int coeff[64];
    
    for (int i = 0; i < 64; i++) {
        float val = block[zigzag[i]] / quant[i];
        coeff[i] = (int)(val + (val >= 0 ? 0.5f : -0.5f));
    }
    
    int dc_diff = coeff[0] - *prev_dc;
    *prev_dc = coeff[0];
    
    int abs_dc = dc_diff < 0 ? -dc_diff : dc_diff;
    int dc_size = 0;
    while (abs_dc > 0) {
        dc_size++;
        abs_dc >>= 1;
    }
    
    buffer_write_bits(buf, dc_table->code[dc_size], dc_table->size[dc_size]);
    if (dc_size > 0) {
        int dc_val = dc_diff < 0 ? dc_diff - 1 : dc_diff;
        buffer_write_bits(buf, dc_val & ((1 << dc_size) - 1), dc_size);
    }
    
    int last_nz = 0;
    for (int i = 63; i > 0; i--) {
        if (coeff[i] != 0) {
            last_nz = i;
            break;
        }
    }
    
    for (int i = 1; i <= last_nz; i++) {
        int zeros = 0;
        while (i <= last_nz && coeff[i] == 0) {
            zeros++;
            i++;
        }
        
        while (zeros >= 16) {
            buffer_write_bits(buf, ac_table->code[0xF0], ac_table->size[0xF0]);
            zeros -= 16;
        }
        
        if (i <= last_nz) {
            int ac_val = coeff[i];
            int abs_ac = ac_val < 0 ? -ac_val : ac_val;
            int ac_size = 0;
            while (abs_ac > 0) {
                ac_size++;
                abs_ac >>= 1;
            }
            
            int symbol = (zeros << 4) | ac_size;
            buffer_write_bits(buf, ac_table->code[symbol], ac_table->size[symbol]);
            int val = ac_val < 0 ? ac_val - 1 : ac_val;
            buffer_write_bits(buf, val & ((1 << ac_size) - 1), ac_size);
        }
    }
    
    if (last_nz < 63) {
        buffer_write_bits(buf, ac_table->code[0x00], ac_table->size[0x00]);
    }
}

static void write_headers(jpeg_buffer_t *buf, int width, int height, 
                         const uint8_t *quant_y, const uint8_t *quant_c) {
    buffer_write_word(buf, 0xFFD8);
    
    buffer_write_word(buf, 0xFFE0);
    buffer_write_word(buf, 16);
    buffer_write_byte(buf, 'J');
    buffer_write_byte(buf, 'F');
    buffer_write_byte(buf, 'I');
    buffer_write_byte(buf, 'F');
    buffer_write_byte(buf, 0);
    buffer_write_word(buf, 0x0101);
    buffer_write_byte(buf, 0);
    buffer_write_word(buf, 1);
    buffer_write_word(buf, 1);
    buffer_write_byte(buf, 0);
    buffer_write_byte(buf, 0);
    
    buffer_write_word(buf, 0xFFDB);
    buffer_write_word(buf, 67);
    buffer_write_byte(buf, 0);
    for (int i = 0; i < 64; i++) {
        buffer_write_byte(buf, quant_y[zigzag[i]]);
    }
    
    buffer_write_word(buf, 0xFFDB);
    buffer_write_word(buf, 67);
    buffer_write_byte(buf, 1);
    for (int i = 0; i < 64; i++) {
        buffer_write_byte(buf, quant_c[zigzag[i]]);
    }
    
    buffer_write_word(buf, 0xFFC0);
    buffer_write_word(buf, 17);
    buffer_write_byte(buf, 8);
    buffer_write_word(buf, height);
    buffer_write_word(buf, width);
    buffer_write_byte(buf, 3);
    buffer_write_byte(buf, 1);
    buffer_write_byte(buf, 0x22);
    buffer_write_byte(buf, 0);
    buffer_write_byte(buf, 2);
    buffer_write_byte(buf, 0x11);
    buffer_write_byte(buf, 1);
    buffer_write_byte(buf, 3);
    buffer_write_byte(buf, 0x11);
    buffer_write_byte(buf, 1);
    
    buffer_write_word(buf, 0xFFC4);
    buffer_write_word(buf, 31);
    buffer_write_byte(buf, 0);
    for (int i = 0; i < 16; i++) {
        buffer_write_byte(buf, std_dc_luminance_nrcodes[i + 1]);
    }
    for (int i = 0; i < 12; i++) {
        buffer_write_byte(buf, std_dc_luminance_values[i]);
    }
    
    buffer_write_word(buf, 0xFFC4);
    buffer_write_word(buf, 181);
    buffer_write_byte(buf, 0x10);
    for (int i = 0; i < 16; i++) {
        buffer_write_byte(buf, std_ac_luminance_nrcodes[i + 1]);
    }
    for (int i = 0; i < 162; i++) {
        buffer_write_byte(buf, std_ac_luminance_values[i]);
    }
    
    buffer_write_word(buf, 0xFFC4);
    buffer_write_word(buf, 31);
    buffer_write_byte(buf, 1);
    for (int i = 0; i < 16; i++) {
        buffer_write_byte(buf, std_dc_chrominance_nrcodes[i + 1]);
    }
    for (int i = 0; i < 12; i++) {
        buffer_write_byte(buf, std_dc_chrominance_values[i]);
    }
    
    buffer_write_word(buf, 0xFFC4);
    buffer_write_word(buf, 181);
    buffer_write_byte(buf, 0x11);
    for (int i = 0; i < 16; i++) {
        buffer_write_byte(buf, std_ac_chrominance_nrcodes[i + 1]);
    }
    for (int i = 0; i < 162; i++) {
        buffer_write_byte(buf, std_ac_chrominance_values[i]);
    }
    
    buffer_write_word(buf, 0xFFDA);
    buffer_write_word(buf, 12);
    buffer_write_byte(buf, 3);
    buffer_write_byte(buf, 1);
    buffer_write_byte(buf, 0);
    buffer_write_byte(buf, 2);
    buffer_write_byte(buf, 0x11);
    buffer_write_byte(buf, 3);
    buffer_write_byte(buf, 0x11);
    buffer_write_byte(buf, 0);
    buffer_write_byte(buf, 63);
    buffer_write_byte(buf, 0);
}

int jpeg_encode_rgb(const uint8_t *rgb_data, int width, int height, int quality,
                    uint8_t **out_data, size_t *out_size) {
    if (!rgb_data || width <= 0 || height <= 0 || quality < 1 || quality > 100) {
        return -1;
    }
    
    jpeg_encoder_t encoder;
    encoder.quality = quality;
    init_quantization_table(encoder.quant_table_y, quality, 1);
    init_quantization_table(encoder.quant_table_c, quality, 0);
    
    init_huffman_table(&dc_luma_table, std_dc_luminance_nrcodes, std_dc_luminance_values);
    init_huffman_table(&ac_luma_table, std_ac_luminance_nrcodes, std_ac_luminance_values);
    init_huffman_table(&dc_chroma_table, std_dc_chrominance_nrcodes, std_dc_chrominance_values);
    init_huffman_table(&ac_chroma_table, std_ac_chrominance_nrcodes, std_ac_chrominance_values);
    
    jpeg_buffer_t buf;
    buffer_init(&buf);
    
    write_headers(&buf, width, height, encoder.quant_table_y, encoder.quant_table_c);
    
    int mcu_width = (width + 15) / 16;
    int mcu_height = (height + 15) / 16;
    
    int prev_dc_y = 0, prev_dc_cb = 0, prev_dc_cr = 0;
    
    for (int mcu_y = 0; mcu_y < mcu_height; mcu_y++) {
        for (int mcu_x = 0; mcu_x < mcu_width; mcu_x++) {
            float y_blocks[4][64] = {0};
            float cb_block[64] = {0};
            float cr_block[64] = {0};
            
            for (int by = 0; by < 2; by++) {
                for (int bx = 0; bx < 2; bx++) {
                    int block_idx = by * 2 + bx;
                    
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            int px = mcu_x * 16 + bx * 8 + x;
                            int py = mcu_y * 16 + by * 8 + y;
                            
                            if (px >= width) px = width - 1;
                            if (py >= height) py = height - 1;
                            
                            int idx = (py * width + px) * 3;
#ifdef USE_SIMD_CONVERT
                            float y_tmp[1], cb_tmp[1], cr_tmp[1];
                            rgb_to_ycbcr_simd(&rgb_data[idx], y_tmp, cb_tmp, cr_tmp, 1);
                            y_blocks[block_idx][y * 8 + x] = y_tmp[0];
                            int cy = by * 4 + y / 2;
                            int cx = bx * 4 + x / 2;
                            cb_block[cy * 8 + cx] += cb_tmp[0] * 0.25f;
                            cr_block[cy * 8 + cx] += cr_tmp[0] * 0.25f;
#else
                            float r = rgb_data[idx];
                            float g = rgb_data[idx + 1];
                            float b = rgb_data[idx + 2];
                            
                            float Y = 0.299f * r + 0.587f * g + 0.114f * b - 128.0f;
                            float Cb = -0.168736f * r - 0.331264f * g + 0.5f * b;
                            float Cr = 0.5f * r - 0.418688f * g - 0.081312f * b;
                            
                            y_blocks[block_idx][y * 8 + x] = Y;
                            
                            int cy = by * 4 + y / 2;
                            int cx = bx * 4 + x / 2;
                            cb_block[cy * 8 + cx] += Cb * 0.25f;
                            cr_block[cy * 8 + cx] += Cr * 0.25f;
#endif
                        }
                    }
                }
            }
            
            for (int i = 0; i < 4; i++) {
#ifdef USE_SIMD_DCT
                fdct_simd(y_blocks[i]);
#else
                fdct(y_blocks[i]);
#endif
                encode_block(&buf, y_blocks[i], encoder.quant_table_y, &prev_dc_y, 
                           &dc_luma_table, &ac_luma_table);
            }
            
#ifdef USE_SIMD_DCT
            fdct_simd(cb_block);
#else
            fdct(cb_block);
#endif
            encode_block(&buf, cb_block, encoder.quant_table_c, &prev_dc_cb,
                        &dc_chroma_table, &ac_chroma_table);
            
#ifdef USE_SIMD_DCT
            fdct_simd(cr_block);
#else
            fdct(cr_block);
#endif
            encode_block(&buf, cr_block, encoder.quant_table_c, &prev_dc_cr,
                        &dc_chroma_table, &ac_chroma_table);
        }
    }
    
    buffer_flush_bits(&buf);
    buffer_write_word(&buf, 0xFFD9);
    
    *out_data = buf.data;
    *out_size = buf.size;
    
    return 0;
}

void jpeg_free(uint8_t *data) {
    free(data);
}

int jpeg_encode_yuv420(const uint8_t *y_plane, const uint8_t *u_plane, const uint8_t *v_plane,
                       int width, int height, int quality, uint8_t **out_data, size_t *out_size) {
    if (!y_plane || !u_plane || !v_plane || width <= 0 || height <= 0 || quality < 1 || quality > 100) {
        return -1;
    }
    
    jpeg_encoder_t encoder;
    encoder.quality = quality;
    init_quantization_table(encoder.quant_table_y, quality, 1);
    init_quantization_table(encoder.quant_table_c, quality, 0);
    
    init_huffman_table(&dc_luma_table, std_dc_luminance_nrcodes, std_dc_luminance_values);
    init_huffman_table(&ac_luma_table, std_ac_luminance_nrcodes, std_ac_luminance_values);
    init_huffman_table(&dc_chroma_table, std_dc_chrominance_nrcodes, std_dc_chrominance_values);
    init_huffman_table(&ac_chroma_table, std_ac_chrominance_nrcodes, std_ac_chrominance_values);
    
    jpeg_buffer_t buf;
    buffer_init(&buf);
    
    write_headers(&buf, width, height, encoder.quant_table_y, encoder.quant_table_c);
    
    int mcu_width = (width + 15) / 16;
    int mcu_height = (height + 15) / 16;
    int uv_width = (width + 1) / 2;
    
    int prev_dc_y = 0, prev_dc_cb = 0, prev_dc_cr = 0;
    
    for (int mcu_y = 0; mcu_y < mcu_height; mcu_y++) {
        for (int mcu_x = 0; mcu_x < mcu_width; mcu_x++) {
            float y_blocks[4][64] = {0};
            float cb_block[64] = {0};
            float cr_block[64] = {0};
            
            for (int by = 0; by < 2; by++) {
                for (int bx = 0; bx < 2; bx++) {
                    int block_idx = by * 2 + bx;
                    
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            int px = mcu_x * 16 + bx * 8 + x;
                            int py = mcu_y * 16 + by * 8 + y;
                            
                            if (px >= width) px = width - 1;
                            if (py >= height) py = height - 1;
                            
                            int y_idx = py * width + px;
                            float Y = y_plane[y_idx] - 128.0f;
                            y_blocks[block_idx][y * 8 + x] = Y;
                        }
                    }
                }
            }
            
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    int px = mcu_x * 8 + x;
                    int py = mcu_y * 8 + y;
                    
                    if (px >= uv_width) px = uv_width - 1;
                    if (py >= (height + 1) / 2) py = (height + 1) / 2 - 1;
                    
                    int uv_idx = py * uv_width + px;
                    cb_block[y * 8 + x] = u_plane[uv_idx] - 128.0f;
                    cr_block[y * 8 + x] = v_plane[uv_idx] - 128.0f;
                }
            }
            
            for (int i = 0; i < 4; i++) {
#ifdef USE_SIMD_DCT
                fdct_simd(y_blocks[i]);
#else
                fdct(y_blocks[i]);
#endif
                encode_block(&buf, y_blocks[i], encoder.quant_table_y, &prev_dc_y, 
                           &dc_luma_table, &ac_luma_table);
            }
            
#ifdef USE_SIMD_DCT
            fdct_simd(cb_block);
#else
            fdct(cb_block);
#endif
            encode_block(&buf, cb_block, encoder.quant_table_c, &prev_dc_cb,
                        &dc_chroma_table, &ac_chroma_table);
            
#ifdef USE_SIMD_DCT
            fdct_simd(cr_block);
#else
            fdct(cr_block);
#endif
            encode_block(&buf, cr_block, encoder.quant_table_c, &prev_dc_cr,
                        &dc_chroma_table, &ac_chroma_table);
        }
    }
    
    buffer_flush_bits(&buf);
    buffer_write_word(&buf, 0xFFD9);
    
    *out_data = buf.data;
    *out_size = buf.size;
    
    return 0;
}

int jpeg_encode_uyvy(const uint8_t *uyvy_data, int width, int height, int quality,
                     uint8_t **out_data, size_t *out_size) {
    if (!uyvy_data || width <= 0 || height <= 0 || quality < 1 || quality > 100) {
        return -1;
    }
    
    jpeg_encoder_t encoder;
    encoder.quality = quality;
    init_quantization_table(encoder.quant_table_y, quality, 1);
    init_quantization_table(encoder.quant_table_c, quality, 0);
    
    init_huffman_table(&dc_luma_table, std_dc_luminance_nrcodes, std_dc_luminance_values);
    init_huffman_table(&ac_luma_table, std_ac_luminance_nrcodes, std_ac_luminance_values);
    init_huffman_table(&dc_chroma_table, std_dc_chrominance_nrcodes, std_dc_chrominance_values);
    init_huffman_table(&ac_chroma_table, std_ac_chrominance_nrcodes, std_ac_chrominance_values);
    
    jpeg_buffer_t buf;
    buffer_init(&buf);
    
    write_headers(&buf, width, height, encoder.quant_table_y, encoder.quant_table_c);
    
    int mcu_width = (width + 15) / 16;
    int mcu_height = (height + 15) / 16;
    
    int prev_dc_y = 0, prev_dc_cb = 0, prev_dc_cr = 0;
    
    for (int mcu_y = 0; mcu_y < mcu_height; mcu_y++) {
        for (int mcu_x = 0; mcu_x < mcu_width; mcu_x++) {
            float y_blocks[4][64] = {0};
            float cb_block[64] = {0};
            float cr_block[64] = {0};
            
            for (int by = 0; by < 2; by++) {
                for (int bx = 0; bx < 2; bx++) {
                    int block_idx = by * 2 + bx;
                    
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            int px = mcu_x * 16 + bx * 8 + x;
                            int py = mcu_y * 16 + by * 8 + y;
                            
                            if (px >= width) px = width - 1;
                            if (py >= height) py = height - 1;
                            
                            int uyvy_idx = (py * width + (px & ~1)) * 2;
                            float Y = (px & 1) ? uyvy_data[uyvy_idx + 3] : uyvy_data[uyvy_idx + 1];
                            float U = uyvy_data[uyvy_idx];
                            float V = uyvy_data[uyvy_idx + 2];
                            
                            y_blocks[block_idx][y * 8 + x] = Y - 128.0f;
                            
                            int cy = by * 4 + y / 2;
                            int cx = bx * 4 + x / 2;
                            cb_block[cy * 8 + cx] += (U - 128.0f) * 0.25f;
                            cr_block[cy * 8 + cx] += (V - 128.0f) * 0.25f;
                        }
                    }
                }
            }
            
            for (int i = 0; i < 4; i++) {
#ifdef USE_SIMD_DCT
                fdct_simd(y_blocks[i]);
#else
                fdct(y_blocks[i]);
#endif
                encode_block(&buf, y_blocks[i], encoder.quant_table_y, &prev_dc_y, 
                           &dc_luma_table, &ac_luma_table);
            }
            
#ifdef USE_SIMD_DCT
            fdct_simd(cb_block);
#else
            fdct(cb_block);
#endif
            encode_block(&buf, cb_block, encoder.quant_table_c, &prev_dc_cb,
                        &dc_chroma_table, &ac_chroma_table);
            
#ifdef USE_SIMD_DCT
            fdct_simd(cr_block);
#else
            fdct(cr_block);
#endif
            encode_block(&buf, cr_block, encoder.quant_table_c, &prev_dc_cr,
                        &dc_chroma_table, &ac_chroma_table);
        }
    }
    
    buffer_flush_bits(&buf);
    buffer_write_word(&buf, 0xFFD9);
    
    *out_data = buf.data;
    *out_size = buf.size;
    
    return 0;
}
