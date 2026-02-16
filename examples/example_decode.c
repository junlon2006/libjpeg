/**
 * @file example_decode.c
 * @brief JPEG to RGB565 decoding example
 * 
 * Demonstrates how to use the JPEG decoder to decode JPEG files to RGB565 format.
 */

#include "jpeg_decoder.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input.jpg>\n", argv[0]);
        return 1;
    }
    
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    size_t jpeg_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    uint8_t *jpeg_data = malloc(jpeg_size);
    if (!jpeg_data) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        return 1;
    }
    
    fread(jpeg_data, 1, jpeg_size, fp);
    fclose(fp);
    
    uint16_t *rgb565_data = NULL;
    int width, height;
    
    printf("Decoding JPEG...\n");
    int result = jpeg_decode_rgb565(jpeg_data, jpeg_size, &rgb565_data, &width, &height);
    
    if (result == 0) {
        printf("Decoded successfully: %dx%d\n", width, height);
        printf("Output size: %d bytes\n", width * height * 2);
        
        FILE *out = fopen("output.rgb565", "wb");
        if (out) {
            fwrite(rgb565_data, 2, width * height, out);
            fclose(out);
            printf("RGB565 data saved to output.rgb565\n");
        }
        
        jpeg_decoder_free(rgb565_data);
    } else {
        fprintf(stderr, "Decoding failed\n");
    }
    
    free(jpeg_data);
    return result;
}
