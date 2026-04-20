/* Written by Tejas Sethi
 * https://github.com/tejas25a/
 *
 * Supported file types - BMP
 */
#ifndef IMG_LIB
#define IMG_LIB

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#endif /* ifndef IMG_LIB */

#define PNGflag 0x0001
#define BMPflag 0x0002

typedef struct fileHead {
  uint8_t data[54];
  uint16_t flag;
  uint32_t width;
  int32_t height; // signed — negative means top-down BMP
  uint32_t pixelDataOffset;
  uint32_t imageSize;
  uint32_t bpp;
  uint32_t compression;
  uint32_t colorTableSize;
} fileHead;

typedef struct Image {
  unsigned char *pixels; // always RGBA, 4 bytes per pixel
  uint32_t width;
  uint32_t height;
  uint32_t size; // width * height * 4
} Image;

fileHead getHeadDetails(FILE *file);

// always returns RGBA pixel data — caller must free()
unsigned char *getPixelData(FILE *file, fileHead *fHead);

Image loadImage(const char *filename);

void freeImage(Image *img);
