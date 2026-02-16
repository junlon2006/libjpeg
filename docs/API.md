# API Documentation

## Overview

This library provides lightweight JPEG encoder and decoder suitable for embedded systems. It supports multiple input/output formats and optional SIMD optimizations.

---

## Encoder Functions

### jpeg_encode_rgb

```c
int jpeg_encode_rgb(const uint8_t *rgb_data, int width, int height, int quality, 
                    uint8_t **out_data, size_t *out_size);
```

Encodes RGB image data to JPEG format.

**Parameters:**
- `rgb_data`: Input RGB data in interleaved format (R,G,B,R,G,B,...)
- `width`: Image width in pixels
- `height`: Image height in pixels
- `quality`: Quality factor (1-100, where 100 is best quality)
- `out_data`: Pointer to receive allocated output buffer
- `out_size`: Pointer to receive output size in bytes

**Returns:**
- `0` on success
- `-1` on error (invalid parameters)

**Memory:**
- Output buffer is dynamically allocated
- Caller must free using `jpeg_free()`

**Example:**
```c
uint8_t *rgb = ...; // width * height * 3 bytes
uint8_t *jpeg_data = NULL;
size_t jpeg_size = 0;

if (jpeg_encode_rgb(rgb, 640, 480, 85, &jpeg_data, &jpeg_size) == 0) {
    // Use jpeg_data
    jpeg_free(jpeg_data);
}
```

---

### jpeg_encode_yuv420

```c
int jpeg_encode_yuv420(const uint8_t *y_plane, const uint8_t *u_plane, 
                       const uint8_t *v_plane, int width, int height, 
                       int quality, uint8_t **out_data, size_t *out_size);
```

Encodes YUV420 planar image to JPEG format.

**Parameters:**
- `y_plane`: Y (luminance) plane, size = width × height
- `u_plane`: U (Cb) plane, size = (width/2) × (height/2)
- `v_plane`: V (Cr) plane, size = (width/2) × (height/2)
- `width`: Image width in pixels (should be even)
- `height`: Image height in pixels (should be even)
- `quality`: Quality factor (1-100)
- `out_data`: Pointer to receive allocated output buffer
- `out_size`: Pointer to receive output size in bytes

**Returns:**
- `0` on success
- `-1` on error

**Notes:**
- More efficient than RGB encoding for video pipelines
- Best results with width/height as multiples of 16

**Example:**
```c
uint8_t *y = ...; // width * height
uint8_t *u = ...; // (width/2) * (height/2)
uint8_t *v = ...; // (width/2) * (height/2)
uint8_t *jpeg_data = NULL;
size_t jpeg_size = 0;

if (jpeg_encode_yuv420(y, u, v, 640, 480, 85, &jpeg_data, &jpeg_size) == 0) {
    jpeg_free(jpeg_data);
}
```

---

### jpeg_encode_uyvy

```c
int jpeg_encode_uyvy(const uint8_t *uyvy_data, int width, int height, 
                     int quality, uint8_t **out_data, size_t *out_size);
```

Encodes UYVY (YUV422) packed image to JPEG format.

**Parameters:**
- `uyvy_data`: Input UYVY data (U,Y0,V,Y1,U,Y2,V,Y3,...)
- `width`: Image width in pixels (must be even)
- `height`: Image height in pixels
- `quality`: Quality factor (1-100)
- `out_data`: Pointer to receive allocated output buffer
- `out_size`: Pointer to receive output size in bytes

**Returns:**
- `0` on success
- `-1` on error

**Notes:**
- Common format for cameras and capture devices
- Width must be even
- Input size = width × height × 2 bytes

**Example:**
```c
uint8_t *uyvy = ...; // width * height * 2 bytes
uint8_t *jpeg_data = NULL;
size_t jpeg_size = 0;

if (jpeg_encode_uyvy(uyvy, 640, 480, 85, &jpeg_data, &jpeg_size) == 0) {
    jpeg_free(jpeg_data);
}
```

---

### jpeg_free

```c
void jpeg_free(uint8_t *data);
```

Frees memory allocated by encoding functions.

**Parameters:**
- `data`: Pointer returned by `jpeg_encode_*` functions

**Example:**
```c
uint8_t *jpeg_data = NULL;
size_t jpeg_size = 0;

jpeg_encode_rgb(rgb, w, h, q, &jpeg_data, &jpeg_size);
// Use jpeg_data...
jpeg_free(jpeg_data);
```

---

## Quality Parameter

The quality parameter (1-100) controls the trade-off between file size and image quality:

- **1-50**: Low quality, high compression, small files
- **50-75**: Medium quality, good for web images
- **75-90**: High quality, suitable for most applications
- **90-100**: Very high quality, large files

**Recommended values:**
- Web thumbnails: 60-70
- Web images: 75-85
- Print quality: 90-95
- Archival: 95-100

---

## Error Handling

All encoding functions return:
- `0` on success
- `-1` on error

Common error conditions:
- NULL input pointers
- Invalid dimensions (width/height ≤ 0)
- Invalid quality (< 1 or > 100)
- Memory allocation failure

**Example:**
```c
int result = jpeg_encode_rgb(rgb, width, height, quality, &out, &size);
if (result != 0) {
    fprintf(stderr, "Encoding failed\n");
    return -1;
}
```

---

## Thread Safety

The encoder is **not thread-safe**. Each thread should use separate encoder instances or external synchronization.

---

## Memory Management

- All output buffers are allocated with `malloc()`
- Initial buffer size: 64KB (configurable via `JPEG_BUFFER_INITIAL_CAPACITY`)
- Buffer grows automatically as needed (doubles each time)
- Always call `jpeg_free()` to release memory

---

## Performance Tips

1. **Use YUV formats** when available (avoids color conversion)
2. **Enable SIMD** by compiling with appropriate flags
3. **Align dimensions** to multiples of 16 for best performance
4. **Reuse buffers** in video encoding scenarios
5. **Choose appropriate quality** (85 is often optimal)

---

## SIMD Optimization

Enable SIMD optimizations at compile time:

```bash
# ARM NEON
make ARCH=arm

# AArch64
make ARCH=aarch64

# MIPS MSA
make ARCH=mips

# x86 SSE2
make ARCH=x86

# x86 AVX2
make ARCH=x86_avx
```

SIMD provides 2-4x speedup for:
- RGB to YCbCr conversion
- DCT transform

---

## Limitations

- Baseline JPEG only (no progressive)
- Fixed 4:2:0 chroma subsampling
- Standard Huffman tables (not optimized)
- No restart markers
- No EXIF metadata support

---

## Decoder Functions

### jpeg_decode_rgb565

```c
int jpeg_decode_rgb565(const uint8_t *jpeg_data, size_t jpeg_size,
                       uint16_t **out_data, int *width, int *height);
```

Decodes JPEG image to RGB565 format.

**Parameters:**
- `jpeg_data`: Input JPEG data buffer
- `jpeg_size`: Size of JPEG data in bytes
- `out_data`: Pointer to receive allocated RGB565 output buffer
- `width`: Pointer to receive image width
- `height`: Pointer to receive image height

**Returns:**
- `0` on success
- `-1` on error

**Memory:**
- Output buffer is dynamically allocated
- Caller must free using `jpeg_decoder_free()`

**RGB565 Format:**
- 16 bits per pixel: `[RRRRR GGGGGG BBBBB]`
- 5 bits for Red, 6 bits for Green, 5 bits for Blue
- Little-endian byte order
- Output size = width × height × 2 bytes

**Example:**
```c
#include "jpeg_decoder.h"

uint8_t *jpeg_data = ...; // JPEG file data
size_t jpeg_size = ...;   // JPEG file size
uint16_t *rgb565 = NULL;
int width, height;

if (jpeg_decode_rgb565(jpeg_data, jpeg_size, &rgb565, &width, &height) == 0) {
    // Use RGB565 data for display
    // Format: pixel = (R>>3)<<11 | (G>>2)<<5 | (B>>3)
    jpeg_decoder_free(rgb565);
}
```

---

### jpeg_decoder_free

```c
void jpeg_decoder_free(void *data);
```

Frees memory allocated by decoder.

**Parameters:**
- `data`: Pointer returned by `jpeg_decode_rgb565()`

**Example:**
```c
uint16_t *rgb565 = NULL;
int width, height;

jpeg_decode_rgb565(jpeg_data, jpeg_size, &rgb565, &width, &height);
// Use rgb565...
jpeg_decoder_free(rgb565);
```

---

## Decoder Features

### Supported JPEG Features

- ✅ Baseline JPEG (ISO/IEC 10918-1)
- ✅ 4:2:0 chroma subsampling
- ✅ Standard quantization tables
- ✅ Standard Huffman tables
- ✅ SIMD optimization (NEON, SSE2)

### Decoder Limitations

- Baseline JPEG only (no progressive)
- No restart markers support
- No EXIF parsing
- Fixed RGB565 output format

### SIMD Optimization

The decoder uses SIMD instructions for:
- **YCbCr to RGB conversion** - Vectorized color space conversion
- **RGB565 packing** - Parallel bit operations

**Performance:**
- SIMD provides 2-3× speedup
- Processes 8 pixels in parallel
- Uses fixed-point arithmetic

**Enable SIMD:**
```bash
# ARM NEON
make ARCH=arm

# x86 SSE2
make ARCH=x86
```

---

## Complete Example

### Encode and Decode Workflow

```c
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"

// Step 1: Encode RGB to JPEG
uint8_t *rgb_input = ...; // width * height * 3
uint8_t *jpeg_data = NULL;
size_t jpeg_size = 0;

jpeg_encode_rgb(rgb_input, 640, 480, 85, &jpeg_data, &jpeg_size);

// Step 2: Decode JPEG to RGB565
uint16_t *rgb565_output = NULL;
int width, height;

jpeg_decode_rgb565(jpeg_data, jpeg_size, &rgb565_output, &width, &height);

// Step 3: Use RGB565 for display
for (int i = 0; i < width * height; i++) {
    uint16_t pixel = rgb565_output[i];
    uint8_t r = (pixel >> 11) & 0x1F;  // 5 bits
    uint8_t g = (pixel >> 5) & 0x3F;   // 6 bits
    uint8_t b = pixel & 0x1F;          // 5 bits
    // Display pixel...
}

// Step 4: Cleanup
jpeg_decoder_free(rgb565_output);
jpeg_free(jpeg_data);
```

---

## RGB565 Format Details

### Bit Layout

```
Bit:  15 14 13 12 11 | 10  9  8  7  6 | 5  4  3  2  1  0
      R  R  R  R  R  | G  G  G  G  G  G | B  B  B  B  B
```

### Conversion Formulas

**RGB888 to RGB565:**
```c
uint16_t rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
```

**RGB565 to RGB888:**
```c
uint8_t r = ((rgb565 >> 11) & 0x1F) << 3;
uint8_t g = ((rgb565 >> 5) & 0x3F) << 2;
uint8_t b = (rgb565 & 0x1F) << 3;
```

### Common Use Cases

- **Embedded displays** - Many LCD controllers use RGB565
- **Framebuffers** - Reduced memory footprint (2 bytes vs 3)
- **Real-time video** - Lower bandwidth requirements
- **Memory-constrained systems** - 33% less memory than RGB888

---

## Error Handling

### Decoder Errors

Common error conditions:
- Invalid JPEG format
- Corrupted data
- Unsupported JPEG features
- Memory allocation failure

**Example:**
```c
int result = jpeg_decode_rgb565(jpeg_data, jpeg_size, &out, &w, &h);
if (result != 0) {
    fprintf(stderr, "Decoding failed\n");
    // Check JPEG file validity
    // Verify sufficient memory
    return -1;
}
```

---

## Performance Comparison

### Encoding Performance

| Resolution | Quality | Output Size | Time (scalar) | Time (SIMD) |
|------------|---------|-------------|---------------|-------------|
| 640×480    | 85      | ~50-100KB   | ~50ms         | ~15ms       |
| 1280×720   | 85      | ~150-250KB  | ~150ms        | ~45ms       |
| 1920×1080  | 85      | ~300-500KB  | ~350ms        | ~100ms      |

### Decoding Performance

| Resolution | Input Size | Time (scalar) | Time (SIMD) |
|------------|------------|---------------|-------------|
| 640×480    | ~50KB      | ~30ms         | ~12ms       |
| 1280×720   | ~150KB     | ~90ms         | ~35ms       |
| 1920×1080  | ~300KB     | ~200ms        | ~75ms       |

*Note: Times are approximate and vary by platform*
