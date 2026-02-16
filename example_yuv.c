/**
 * @file example_yuv.c
 * @brief YUV420 to JPEG encoding example
 * 
 * Demonstrates how to use the JPEG encoder with YUV420 planar input format.
 * This is more efficient for video processing pipelines.
 */

#include "jpeg_encoder.h"
#include <stdio.h>
#include <stdlib.h>

static void generate_test_yuv420(uint8_t *y, uint8_t *u, uint8_t *v, int width, int height) {
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            y[py * width + px] = (px * 255) / width;
        }
    }
    
    int uv_width = (width + 1) / 2;
    int uv_height = (height + 1) / 2;
    
    for (int py = 0; py < uv_height; py++) {
        for (int px = 0; px < uv_width; px++) {
            u[py * uv_width + px] = (py * 255) / uv_height;
            v[py * uv_width + px] = 128;
        }
    }
}

int main(int argc, char *argv[]) {
    int width = 640;
    int height = 480;
    int quality = 85;
    
    if (argc > 1) width = atoi(argv[1]);
    if (argc > 2) height = atoi(argv[2]);
    if (argc > 3) quality = atoi(argv[3]);
    
    printf("Encoding %dx%d YUV420 image with quality %d\n", width, height, quality);
    
    int uv_width = (width + 1) / 2;
    int uv_height = (height + 1) / 2;
    
    uint8_t *y_plane = malloc(width * height);
    uint8_t *u_plane = malloc(uv_width * uv_height);
    uint8_t *v_plane = malloc(uv_width * uv_height);
    
    if (!y_plane || !u_plane || !v_plane) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    generate_test_yuv420(y_plane, u_plane, v_plane, width, height);
    
    uint8_t *jpeg_data = NULL;
    size_t jpeg_size = 0;
    
    int result = jpeg_encode_yuv420(y_plane, u_plane, v_plane, width, height, quality, 
                                     &jpeg_data, &jpeg_size);
    
    if (result == 0) {
        FILE *fp = fopen("output_yuv.jpg", "wb");
        if (fp) {
            fwrite(jpeg_data, 1, jpeg_size, fp);
            fclose(fp);
            printf("JPEG encoded successfully: %zu bytes\n", jpeg_size);
            printf("Output saved to output_yuv.jpg\n");
        } else {
            fprintf(stderr, "Failed to write output file\n");
        }
        jpeg_free(jpeg_data);
    } else {
        fprintf(stderr, "JPEG encoding failed\n");
    }
    
    free(y_plane);
    free(u_plane);
    free(v_plane);
    
    return 0;
}
