/**
 * @file example_uyvy.c
 * @brief UYVY (YUV422) to JPEG encoding example
 * 
 * Demonstrates how to use the JPEG encoder with UYVY packed input format.
 * This format is commonly used by cameras and video capture devices.
 */

#include "jpeg_encoder.h"
#include <stdio.h>
#include <stdlib.h>

static void generate_test_uyvy(uint8_t *uyvy, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x += 2) {
            int idx = (y * width + x) * 2;
            uyvy[idx] = (y * 255) / height;
            uyvy[idx + 1] = (x * 255) / width;
            uyvy[idx + 2] = 128;
            uyvy[idx + 3] = ((x + 1) * 255) / width;
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
    
    width = (width + 1) & ~1;
    
    printf("Encoding %dx%d UYVY image with quality %d\n", width, height, quality);
    
    uint8_t *uyvy_data = malloc(width * height * 2);
    if (!uyvy_data) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    generate_test_uyvy(uyvy_data, width, height);
    
    uint8_t *jpeg_data = NULL;
    size_t jpeg_size = 0;
    
    int result = jpeg_encode_uyvy(uyvy_data, width, height, quality, &jpeg_data, &jpeg_size);
    
    if (result == 0) {
        FILE *fp = fopen("output_uyvy.jpg", "wb");
        if (fp) {
            fwrite(jpeg_data, 1, jpeg_size, fp);
            fclose(fp);
            printf("JPEG encoded successfully: %zu bytes\n", jpeg_size);
            printf("Output saved to output_uyvy.jpg\n");
        } else {
            fprintf(stderr, "Failed to write output file\n");
        }
        jpeg_free(jpeg_data);
    } else {
        fprintf(stderr, "JPEG encoding failed\n");
    }
    
    free(uyvy_data);
    return 0;
}
