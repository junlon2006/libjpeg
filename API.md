# API Documentation

## Overview

This library provides a lightweight JPEG encoder suitable for embedded systems. It supports multiple input formats and optional SIMD optimizations.

## Core Functions

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
