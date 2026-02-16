/**
 * @file jpeg_decoder.h
 * @brief Lightweight JPEG decoder for embedded systems
 * 
 * This is a pure C implementation of a baseline JPEG decoder with no external
 * dependencies (only standard C library). Outputs RGB565 format suitable for
 * embedded displays.
 */

#ifndef JPEG_DECODER_H
#define JPEG_DECODER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Decode JPEG image to RGB565 format
 * 
 * Decodes a JPEG image to RGB565 packed format (16-bit per pixel).
 * This format is commonly used in embedded displays and framebuffers.
 * RGB565 format: [RRRRR GGGGGG BBBBB] in little-endian order.
 * 
 * @param jpeg_data Input JPEG data buffer
 * @param jpeg_size Size of JPEG data in bytes
 * @param out_data Pointer to receive allocated RGB565 output buffer
 * @param width Pointer to receive image width
 * @param height Pointer to receive image height
 * @return 0 on success, -1 on error
 * 
 * @note Output format: RGB565 packed (5 bits R, 6 bits G, 5 bits B)
 * @note Output size = width × height × 2 bytes
 * @note Caller must free output buffer using jpeg_decoder_free()
 */
int jpeg_decode_rgb565(const uint8_t *jpeg_data, size_t jpeg_size,
                       uint16_t **out_data, int *width, int *height);

/**
 * @brief Free decoder output buffer
 * 
 * Releases memory allocated by jpeg_decode_rgb565().
 * 
 * @param data Pointer to buffer returned by jpeg_decode_rgb565()
 */
void jpeg_decoder_free(void *data);

#ifdef __cplusplus
}
#endif

#endif
