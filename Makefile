CC = gcc
CFLAGS = -Wall -O2 -std=c99 -Isrc
LDFLAGS = -lm

# SIMD optimization flags
ifeq ($(ARCH),arm)
    CFLAGS += -mfpu=neon -DUSE_SIMD_CONVERT -DUSE_SIMD_DCT
endif
ifeq ($(ARCH),aarch64)
    CFLAGS += -DUSE_SIMD_CONVERT -DUSE_SIMD_DCT
endif
ifeq ($(ARCH),mips)
    CFLAGS += -mmsa -DUSE_SIMD_CONVERT -DUSE_SIMD_DCT
endif
ifeq ($(ARCH),x86)
    CFLAGS += -msse2 -DUSE_SIMD_CONVERT -DUSE_SIMD_DCT
endif
ifeq ($(ARCH),x86_avx)
    CFLAGS += -mavx2 -DUSE_SIMD_CONVERT -DUSE_SIMD_DCT
endif

# Source files
SRC_DIR = src
EXAMPLE_DIR = examples
BUILD_DIR = build

# Targets
TARGET_RGB = $(BUILD_DIR)/jpeg_example
TARGET_YUV = $(BUILD_DIR)/jpeg_example_yuv
TARGET_UYVY = $(BUILD_DIR)/jpeg_example_uyvy
TARGET_DECODE = $(BUILD_DIR)/jpeg_example_decode

# Object files
ENCODER_OBJS = $(BUILD_DIR)/jpeg_encoder.o $(BUILD_DIR)/jpeg_simd.o
DECODER_OBJS = $(BUILD_DIR)/jpeg_decoder.o

all: $(BUILD_DIR) $(TARGET_RGB) $(TARGET_YUV) $(TARGET_UYVY) $(TARGET_DECODE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Encoder library objects
$(BUILD_DIR)/jpeg_encoder.o: $(SRC_DIR)/jpeg_encoder.c $(SRC_DIR)/jpeg_encoder.h $(SRC_DIR)/jpeg_simd.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/jpeg_encoder.c -o $@

$(BUILD_DIR)/jpeg_simd.o: $(SRC_DIR)/jpeg_simd.c $(SRC_DIR)/jpeg_simd.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/jpeg_simd.c -o $@

# Decoder library objects
$(BUILD_DIR)/jpeg_decoder.o: $(SRC_DIR)/jpeg_decoder.c $(SRC_DIR)/jpeg_decoder.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/jpeg_decoder.c -o $@

# Example executables
$(TARGET_RGB): $(ENCODER_OBJS) $(BUILD_DIR)/example.o
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_YUV): $(ENCODER_OBJS) $(BUILD_DIR)/example_yuv.o
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_UYVY): $(ENCODER_OBJS) $(BUILD_DIR)/example_uyvy.o
	$(CC) $^ -o $@ $(LDFLAGS)

$(TARGET_DECODE): $(DECODER_OBJS) $(BUILD_DIR)/example_decode.o
	$(CC) $^ -o $@ $(LDFLAGS)

# Example object files
$(BUILD_DIR)/example.o: $(EXAMPLE_DIR)/example.c $(SRC_DIR)/jpeg_encoder.h
	$(CC) $(CFLAGS) -c $(EXAMPLE_DIR)/example.c -o $@

$(BUILD_DIR)/example_yuv.o: $(EXAMPLE_DIR)/example_yuv.c $(SRC_DIR)/jpeg_encoder.h
	$(CC) $(CFLAGS) -c $(EXAMPLE_DIR)/example_yuv.c -o $@

$(BUILD_DIR)/example_uyvy.o: $(EXAMPLE_DIR)/example_uyvy.c $(SRC_DIR)/jpeg_encoder.h
	$(CC) $(CFLAGS) -c $(EXAMPLE_DIR)/example_uyvy.c -o $@

$(BUILD_DIR)/example_decode.o: $(EXAMPLE_DIR)/example_decode.c $(SRC_DIR)/jpeg_decoder.h
	$(CC) $(CFLAGS) -c $(EXAMPLE_DIR)/example_decode.c -o $@

clean:
	rm -rf $(BUILD_DIR) output.jpg output_yuv.jpg output_uyvy.jpg output.rgb565

.PHONY: all clean
