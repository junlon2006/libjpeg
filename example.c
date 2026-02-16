/**
 * @file example.c
 * @brief RGB to JPEG encoding example
 * 
 * Demonstrates how to use the JPEG encoder with RGB input format.
 * Generates a test gradient image and encodes it to JPEG.
 */

#include "jpeg_encoder.h"
#include <stdio.h>
#include <stdlib.h>

static void generate_test_image(uint8_t *rgb, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            rgb[idx] = (x * 255) / width;
            rgb[idx + 1] = (y * 255) / height;
            rgb[idx + 2] = 128;
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
    
    printf("Encoding %dx%d image with quality %d\n", width, height, quality);
    
    uint8_t *rgb_data = malloc(width * height * 3);
    if (!rgb_data) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    generate_test_image(rgb_data, width, height);
    
    uint8_t *jpeg_data = NULL;
    size_t jpeg_size = 0;
    
    int result = jpeg_encode_rgb(rgb_data, width, height, quality, &jpeg_data, &jpeg_size);
    
    if (result == 0) {
        FILE *fp = fopen("output.jpg", "wb");
        if (fp) {
            fwrite(jpeg_data, 1, jpeg_size, fp);
            fclose(fp);
            printf("JPEG encoded successfully: %zu bytes\n", jpeg_size);
            printf("Output saved to output.jpg\n");
        } else {
            fprintf(stderr, "Failed to write output file\n");
        }
        jpeg_free(jpeg_data);
    } else {
        fprintf(stderr, "JPEG encoding failed\n");
    }
    
    free(rgb_data);
    return 0;
}
