# Pure C JPEG Encoder / 纯C版本JPEG编码器

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C99](https://img.shields.io/badge/std-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)

[English](#english) | [中文](#chinese)

---

<a name="english"></a>
## English

### Overview

A lightweight, pure C implementation of a JPEG encoder designed for embedded systems. No external dependencies except the standard C library.

### Features

- ✅ **Pure C99** - No external dependencies
- ✅ **Multiple input formats** - RGB, YUV420 planar, UYVY packed
- ✅ **SIMD optimized** - ARM NEON, AArch64, MIPS MSA, x86 SSE/AVX
- ✅ **Baseline JPEG** - Complete encoding pipeline
- ✅ **RGB565 decoder** - Decode to embedded display format
- ✅ **4:2:0 subsampling** - Standard chroma subsampling
- ✅ **Configurable quality** - 1-100 quality parameter
- ✅ **Low memory footprint** - Suitable for embedded systems
- ✅ **C++ compatible** - Extern "C" linkage

### Quick Start

#### Build

```bash
# Default build (no SIMD)
make

# ARM NEON optimization
make ARCH=arm

# AArch64 optimization
make ARCH=aarch64

# MIPS MSA optimization
make ARCH=mips

# x86 SSE2 optimization
make ARCH=x86

# x86 AVX2 optimization
make ARCH=x86_avx
```

#### Run Examples

```bash
# RGB encoding
./jpeg_example 640 480 85

# YUV420 encoding
./jpeg_example_yuv 640 480 85

# UYVY encoding
./jpeg_example_uyvy 640 480 85

# JPEG decoding
./jpeg_example_decode input.jpg
```

### API Usage

#### RGB Encoding

```c
#include "jpeg_encoder.h"

uint8_t *rgb_data = ...; // width * height * 3 bytes
uint8_t *jpeg_data = NULL;
size_t jpeg_size = 0;

int result = jpeg_encode_rgb(rgb_data, 640, 480, 85, &jpeg_data, &jpeg_size);
if (result == 0) {
    FILE *fp = fopen("output.jpg", "wb");
    fwrite(jpeg_data, 1, jpeg_size, fp);
    fclose(fp);
    jpeg_free(jpeg_data);
}
```

#### YUV420 Encoding

```c
uint8_t *y_plane = ...; // width * height
uint8_t *u_plane = ...; // (width/2) * (height/2)
uint8_t *v_plane = ...; // (width/2) * (height/2)

int result = jpeg_encode_yuv420(y_plane, u_plane, v_plane, 
                                640, 480, 85, &jpeg_data, &jpeg_size);
```

#### UYVY Encoding

```c
uint8_t *uyvy_data = ...; // width * height * 2 bytes

int result = jpeg_encode_uyvy(uyvy_data, 640, 480, 85, &jpeg_data, &jpeg_size);
```

#### JPEG Decoding

```c
#include "jpeg_decoder.h"

uint8_t *jpeg_data = ...; // JPEG file data
size_t jpeg_size = ...;   // JPEG file size
uint16_t *rgb565 = NULL;
int width, height;

int result = jpeg_decode_rgb565(jpeg_data, jpeg_size, &rgb565, &width, &height);
if (result == 0) {
    // Use RGB565 data (16-bit per pixel)
    jpeg_decoder_free(rgb565);
}
```

### Documentation

- 📖 [API Documentation](API.md) - Detailed API reference
- 🤝 [Contributing Guide](CONTRIBUTING.md) - How to contribute

### Technical Details

#### Encoding Pipeline

1. **Color space conversion** - RGB → YCbCr
2. **Chroma subsampling** - 4:2:0 (4 Y blocks, 1 Cb, 1 Cr per 16×16 MCU)
3. **DCT transform** - 8×8 block Discrete Cosine Transform
4. **Quantization** - Standard tables scaled by quality
5. **Zig-zag scan** - Convert 8×8 to 1D sequence
6. **Huffman coding** - Entropy encoding with standard tables

#### Memory Usage

- Encoder structure: ~200 bytes
- Output buffer: Initial 64KB, grows as needed
- Working memory: ~2KB per MCU

#### Performance

- SIMD provides 2-4× speedup
- Typical encoding: 640×480 @ 85 quality ≈ 50-100KB output
- Embedded systems: Tested on ARM Cortex-A series

### Limitations

- Baseline JPEG only (no progressive)
- Fixed 4:2:0 chroma subsampling
- Standard Huffman tables (not optimized per image)
- No EXIF metadata support

### License

See [LICENSE](LICENSE) file for details.

### Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) first.

---

<a name="chinese"></a>
## 中文

### 概述

适用于嵌入式系统的轻量级JPEG编码器，纯C实现，无第三方库依赖。

### 特性

- ✅ **纯C99实现** - 无外部依赖（仅需标准C库）
- ✅ **多种输入格式** - 支持RGB、YUV420平面、UYVY打包
- ✅ **SIMD优化** - 支持ARM NEON、AArch64、MIPS MSA、x86 SSE/AVX
- ✅ **完整JPEG编码** - 实现基线JPEG编码流程
- ✅ **RGB565解码器** - 解码为嵌入式显示格式
- ✅ **4:2:0子采样** - 标准色度子采样
- ✅ **可配置质量** - 质量参数1-100
- ✅ **低内存占用** - 适合嵌入式系统
- ✅ **C++兼容** - 支持extern "C"链接

### 快速开始

#### 编译

```bash
# 默认编译（无SIMD优化）
make

# ARM NEON优化
make ARCH=arm

# AArch64优化
make ARCH=aarch64

# MIPS MSA优化
make ARCH=mips

# x86 SSE2优化
make ARCH=x86

# x86 AVX2优化
make ARCH=x86_avx
```

#### 运行示例

```bash
# RGB编码
./jpeg_example 640 480 85

# YUV420编码
./jpeg_example_yuv 640 480 85

# UYVY编码
./jpeg_example_uyvy 640 480 85

# JPEG解码
./jpeg_example_decode input.jpg
```

### API使用

#### RGB编码

```c
#include "jpeg_encoder.h"

// 准备RGB数据（每像素3字节：R,G,B）
uint8_t *rgb_data = ...; // width * height * 3 字节
uint8_t *jpeg_data = NULL;
size_t jpeg_size = 0;

int result = jpeg_encode_rgb(rgb_data, 640, 480, 85, &jpeg_data, &jpeg_size);
if (result == 0) {
    FILE *fp = fopen("output.jpg", "wb");
    fwrite(jpeg_data, 1, jpeg_size, fp);
    fclose(fp);
    jpeg_free(jpeg_data);
}
```

#### YUV420编码

```c
// 准备YUV420数据
uint8_t *y_plane = ...; // width * height 字节
uint8_t *u_plane = ...; // (width/2) * (height/2) 字节
uint8_t *v_plane = ...; // (width/2) * (height/2) 字节

int result = jpeg_encode_yuv420(y_plane, u_plane, v_plane, 
                                640, 480, 85, &jpeg_data, &jpeg_size);
```

#### UYVY编码

```c
// 准备UYVY数据（YUV422打包格式）
uint8_t *uyvy_data = ...; // width * height * 2 字节

int result = jpeg_encode_uyvy(uyvy_data, 640, 480, 85, &jpeg_data, &jpeg_size);
```

#### JPEG解码

```c
#include "jpeg_decoder.h"

// 准备JPEG数据
uint8_t *jpeg_data = ...; // JPEG文件数据
size_t jpeg_size = ...;   // JPEG文件大小
uint16_t *rgb565 = NULL;
int width, height;

int result = jpeg_decode_rgb565(jpeg_data, jpeg_size, &rgb565, &width, &height);
if (result == 0) {
    // 使用RGB565数据（16位/像素）
    jpeg_decoder_free(rgb565);
}
```

### 文档

- 📖 [API文档](API.md) - 详细API参考
- 🤝 [贡献指南](CONTRIBUTING.md) - 如何贡献代码

### 技术实现

#### 编码流程

1. **颜色空间转换** - RGB → YCbCr
2. **色度子采样** - 4:2:0（每个16x16 MCU包含4个Y块，1个Cb块，1个Cr块）
3. **DCT变换** - 8x8块的离散余弦变换
4. **量化** - 使用标准量化表，根据质量参数缩放
5. **Zig-zag扫描** - 将8x8块转换为1D序列
6. **霍夫曼编码** - 使用标准霍夫曼表进行熵编码

#### 内存使用

- 编码器结构：约200字节
- 输出缓冲区：初始64KB，按需增长
- 临时工作内存：每个MCU约2KB

#### 性能

- SIMD优化提供2-4倍加速
- 典型编码：640×480 @ 85质量 ≈ 50-100KB输出
- 嵌入式系统：已在ARM Cortex-A系列测试

### 限制

- 仅支持基线JPEG（不支持渐进式）
- 固定4:2:0色度子采样
- 使用标准霍夫曼表（不针对每张图片优化）
- 不支持EXIF元数据

### 许可证

详见 [LICENSE](LICENSE) 文件。

### 贡献

欢迎贡献！请先阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。

### 清理

```bash
make clean
```
