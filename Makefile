CC = gcc
CFLAGS = -Wall -O2 -std=c99
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

TARGET = jpeg_example
TARGET_YUV = jpeg_example_yuv
TARGET_UYVY = jpeg_example_uyvy
OBJS = jpeg_encoder.o jpeg_simd.o example.o
OBJS_YUV = jpeg_encoder.o jpeg_simd.o example_yuv.o
OBJS_UYVY = jpeg_encoder.o jpeg_simd.o example_uyvy.o

all: $(TARGET) $(TARGET_YUV) $(TARGET_UYVY)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(TARGET_YUV): $(OBJS_YUV)
	$(CC) $(OBJS_YUV) -o $(TARGET_YUV) $(LDFLAGS)

$(TARGET_UYVY): $(OBJS_UYVY)
	$(CC) $(OBJS_UYVY) -o $(TARGET_UYVY) $(LDFLAGS)

jpeg_encoder.o: jpeg_encoder.c jpeg_encoder.h jpeg_simd.h
	$(CC) $(CFLAGS) -c jpeg_encoder.c

jpeg_simd.o: jpeg_simd.c jpeg_simd.h
	$(CC) $(CFLAGS) -c jpeg_simd.c

example.o: example.c jpeg_encoder.h
	$(CC) $(CFLAGS) -c example.c

example_yuv.o: example_yuv.c jpeg_encoder.h
	$(CC) $(CFLAGS) -c example_yuv.c

example_uyvy.o: example_uyvy.c jpeg_encoder.h
	$(CC) $(CFLAGS) -c example_uyvy.c

clean:
	rm -f $(OBJS) $(OBJS_YUV) $(OBJS_UYVY) $(TARGET) $(TARGET_YUV) $(TARGET_UYVY) output.jpg output_yuv.jpg output_uyvy.jpg

.PHONY: all clean
