/**
 * @file jpeg_encoder.h
 * @brief Lightweight JPEG encoder for embedded systems
 * 
 * This is a pure C implementation of a baseline JPEG encoder with no external
 * dependencies (only standard C library). Supports RGB, YUV420, and UYVY input
 * formats with optional SIMD optimizations for ARM NEON, AArch64, MIPS MSA,
 * and x86 SSE/AVX.
 */

#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internal buffer structure for JPEG bitstream generation
 * 
 * This structure manages the output buffer and bit-level operations
 * required for JPEG entropy coding.
 */
typedef struct {
    uint8_t *data;          /**< Output buffer pointer */
    size_t size;            /**< Current size of data in buffer */
    size_t capacity;        /**< Total allocated capacity */
    uint32_t bit_buffer;    /**< Bit accumulator for entropy coding */
    int bit_count;          /**< Number of bits in bit_buffer */
} jpeg_buffer_t;

/**
 * @brief JPEG encoder configuration and state
 * 
 * Contains quality settings and quantization tables for Y and C components.
 */
typedef struct {
    int quality;                /**< Quality factor (1-100) */
    uint8_t quant_table_y[64];  /**< Quantization table for Y (luminance) */
    uint8_t quant_table_c[64];  /**< Quantization table for Cb/Cr (chrominance) */
} jpeg_encoder_t;

/**
 * @brief Encode RGB image to JPEG format
 * 
 * Converts RGB image data to JPEG baseline format with 4:2:0 chroma subsampling.
 * The function performs color space conversion (RGB to YCbCr), DCT transform,
 * quantization, and Huffman encoding.
 * 
 * @param rgb_data Input RGB data in interleaved format (R,G,B,R,G,B,...)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param quality Quality factor (1-100, where 100 is best quality)
 * @param out_data Pointer to receive allocated output buffer (caller must free with jpeg_free)
 * @param out_size Pointer to receive output JPEG size in bytes
 * @return 0 on success, -1 on error
 * 
 * @note The output buffer is dynamically allocated and must be freed using jpeg_free()
 * @note Input size must be width * height * 3 bytes
 */
int jpeg_encode_rgb(const uint8_t *rgb_data, int width, int height, int quality, 
                    uint8_t **out_data, size_t *out_size);

/**
 * @brief Encode YUV420 planar image to JPEG format
 * 
 * Encodes YUV420 planar format directly to JPEG, avoiding RGB to YUV conversion.
 * This is more efficient for video processing pipelines that already have YUV data.
 * 
 * @param y_plane Y (luminance) plane, size = width * height
 * @param u_plane U (Cb) plane, size = (width/2) * (height/2)
 * @param v_plane V (Cr) plane, size = (width/2) * (height/2)
 * @param width Image width in pixels (should be even)
 * @param height Image height in pixels (should be even)
 * @param quality Quality factor (1-100, where 100 is best quality)
 * @param out_data Pointer to receive allocated output buffer (caller must free with jpeg_free)
 * @param out_size Pointer to receive output JPEG size in bytes
 * @return 0 on success, -1 on error
 * 
 * @note The output buffer is dynamically allocated and must be freed using jpeg_free()
 * @note For best results, width and height should be multiples of 16
 */
int jpeg_encode_yuv420(const uint8_t *y_plane, const uint8_t *u_plane, const uint8_t *v_plane,
                       int width, int height, int quality, uint8_t **out_data, size_t *out_size);

/**
 * @brief Encode UYVY (YUV422) packed image to JPEG format
 * 
 * Encodes UYVY packed format (U,Y,V,Y interleaved) to JPEG. This format is
 * commonly used by cameras and video capture devices.
 * 
 * @param uyvy_data Input UYVY data in packed format (U,Y0,V,Y1,U,Y2,V,Y3,...)
 * @param width Image width in pixels (must be even)
 * @param height Image height in pixels
 * @param quality Quality factor (1-100, where 100 is best quality)
 * @param out_data Pointer to receive allocated output buffer (caller must free with jpeg_free)
 * @param out_size Pointer to receive output JPEG size in bytes
 * @return 0 on success, -1 on error
 * 
 * @note The output buffer is dynamically allocated and must be freed using jpeg_free()
 * @note Input size must be width * height * 2 bytes
 * @note Width must be even (UYVY format requirement)
 */
int jpeg_encode_uyvy(const uint8_t *uyvy_data, int width, int height, int quality,
                     uint8_t **out_data, size_t *out_size);

/**
 * @brief Free JPEG output buffer
 * 
 * Releases memory allocated by jpeg_encode_* functions.
 * 
 * @param data Pointer to buffer returned by jpeg_encode_* functions
 */
void jpeg_free(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
