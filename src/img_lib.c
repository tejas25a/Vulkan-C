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

#include "img_lib.h"

#define PNGflag 0x0001
#define BMPflag 0x0002

fileHead getHeadDetails(FILE *file) {
  fileHead fHead = {0};

  fread(fHead.data, 1, 54, file);

  if (fHead.data[0] == 0x42 && fHead.data[1] == 0x4D) {
    fHead.flag = BMPflag;
    printf("BMP detected\n");

    fHead.width = (uint32_t)fHead.data[18] | (uint32_t)fHead.data[19] << 8 |
                  (uint32_t)fHead.data[20] << 16 |
                  (uint32_t)fHead.data[21] << 24;

    fHead.height =
        (int32_t)((uint32_t)fHead.data[22] | (uint32_t)fHead.data[23] << 8 |
                  (uint32_t)fHead.data[24] << 16 |
                  (uint32_t)fHead.data[25] << 24);

    fHead.pixelDataOffset =
        (uint32_t)fHead.data[10] | (uint32_t)fHead.data[11] << 8 |
        (uint32_t)fHead.data[12] << 16 | (uint32_t)fHead.data[13] << 24;

    fHead.imageSize = (uint32_t)fHead.data[34] | (uint32_t)fHead.data[35] << 8 |
                      (uint32_t)fHead.data[36] << 16 |
                      (uint32_t)fHead.data[37] << 24;

    fHead.bpp = (uint32_t)fHead.data[28] | (uint32_t)fHead.data[29] << 8;

    fHead.compression =
        (uint32_t)fHead.data[30] | (uint32_t)fHead.data[31] << 8 |
        (uint32_t)fHead.data[32] << 16 | (uint32_t)fHead.data[33] << 24;

    fHead.colorTableSize =
        (uint32_t)fHead.data[46] | (uint32_t)fHead.data[47] << 8 |
        (uint32_t)fHead.data[48] << 16 | (uint32_t)fHead.data[49] << 24;

  } else if (fHead.data[0] == 0x89 && fHead.data[1] == 0x50 &&
             fHead.data[2] == 0x4E && fHead.data[3] == 0x47 &&
             fHead.data[4] == 0x0D && fHead.data[5] == 0x0A &&
             fHead.data[6] == 0x1A && fHead.data[7] == 0x0A) {
    printf("PNG detected\n");
    fHead.flag = PNGflag;
  } else {
    printf("Unsupported file type! Header bytes:\n");
    for (int i = 0; i < 16; i++) {
      printf("|%02x", fHead.data[i]);
    }
    printf("\n");
  }

  return fHead;
}

// always returns RGBA pixel data — caller must free()
unsigned char *getPixelData(FILE *file, fileHead *fHead) {
  if (fHead->flag != BMPflag) {
    fprintf(stderr, "Only BMP is currently supported\n");
    return NULL;
  }

  uint32_t absHeight = (fHead->height < 0) ? (uint32_t)(-fHead->height)
                                           : (uint32_t)(fHead->height);
  int bottomUp = (fHead->height > 0); // positive height = bottom-up

  fseek(file, fHead->pixelDataOffset, SEEK_SET);

  // output is always RGBA
  uint32_t outRowSize = fHead->width * 4;
  uint32_t bufferSize = outRowSize * absHeight;
  uint8_t *rgba = malloc(bufferSize);
  if (!rgba) {
    fprintf(stderr, "Failed to allocate pixel data!\n");
    return NULL;
  }

  if (fHead->bpp == 24) {
    uint32_t srcRowSize = fHead->width * 3;
    uint32_t padding = (4 - srcRowSize % 4) % 4;
    uint8_t *row = malloc(srcRowSize);
    if (!row) {
      free(rgba);
      return NULL;
    }

    for (uint32_t i = 0; i < absHeight; i++) {
      fread(row, 1, srcRowSize, file);
      fseek(file, padding, SEEK_CUR);

      uint32_t destRow = bottomUp ? (absHeight - 1 - i) : i;
      for (uint32_t j = 0; j < fHead->width; j++) {
        rgba[destRow * outRowSize + j * 4 + 0] = row[j * 3 + 2]; // R
        rgba[destRow * outRowSize + j * 4 + 1] = row[j * 3 + 1]; // G
        rgba[destRow * outRowSize + j * 4 + 2] = row[j * 3 + 0]; // B
        rgba[destRow * outRowSize + j * 4 + 3] = 255;            // A
      }
    }
    free(row);

  } else if (fHead->bpp == 32) {
    uint32_t srcRowSize = fHead->width * 4;
    uint32_t padding = (4 - srcRowSize % 4) % 4;
    uint8_t *row = malloc(srcRowSize);
    if (!row) {
      free(rgba);
      return NULL;
    }

    for (uint32_t i = 0; i < absHeight; i++) {
      fread(row, 1, srcRowSize, file);
      fseek(file, padding, SEEK_CUR);

      uint32_t destRow = bottomUp ? (absHeight - 1 - i) : i;
      for (uint32_t j = 0; j < fHead->width; j++) {
        rgba[destRow * outRowSize + j * 4 + 0] = row[j * 4 + 2]; // R
        rgba[destRow * outRowSize + j * 4 + 1] = row[j * 4 + 1]; // G
        rgba[destRow * outRowSize + j * 4 + 2] = row[j * 4 + 0]; // B
        rgba[destRow * outRowSize + j * 4 + 3] = row[j * 4 + 3]; // A
      }
    }
    free(row);

  } else if (fHead->bpp == 8) {
    // 8-bit indexed with palette
    // palette starts at offset 54 (after BITMAPFILEHEADER + BITMAPINFOHEADER)
    fseek(file, 54, SEEK_SET);
    uint8_t palette[256][4];
    fread(palette, 4, 256, file);

    uint32_t srcRowSize = fHead->width;
    uint32_t padding = (4 - srcRowSize % 4) % 4;
    uint8_t *row = malloc(srcRowSize);
    if (!row) {
      free(rgba);
      return NULL;
    }

    fseek(file, fHead->pixelDataOffset, SEEK_SET);
    for (uint32_t i = 0; i < absHeight; i++) {
      fread(row, 1, srcRowSize, file);
      fseek(file, padding, SEEK_CUR);

      uint32_t destRow = bottomUp ? (absHeight - 1 - i) : i;
      for (uint32_t j = 0; j < fHead->width; j++) {
        uint8_t idx = row[j];
        rgba[destRow * outRowSize + j * 4 + 0] = palette[idx][2]; // R
        rgba[destRow * outRowSize + j * 4 + 1] = palette[idx][1]; // G
        rgba[destRow * outRowSize + j * 4 + 2] = palette[idx][0]; // B
        rgba[destRow * outRowSize + j * 4 + 3] = 255;             // A
      }
    }
    free(row);

  } else {
    fprintf(stderr, "Unsupported BMP bit depth: %u\n", fHead->bpp);
    free(rgba);
    return NULL;
  }

  return rgba;
}

Image loadImage(const char *filename) {
  Image img = {0};

  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Failed to open image: %s\n", filename);
    return img;
  }

  fileHead fHead = getHeadDetails(file);
  img.pixels = getPixelData(file, &fHead);
  img.width = fHead.width;
  img.height =
      (fHead.height < 0) ? (uint32_t)(-fHead.height) : (uint32_t)(fHead.height);
  img.size = img.width * img.height * 4;

  fclose(file);
  return img;
}

void freeImage(Image *img) {
  free(img->pixels);
  img->pixels = NULL;
  img->width = 0;
  img->height = 0;
  img->size = 0;
}

#ifdef IMG_LIB_MAIN
int main() {
  Image img = loadImage("texture/texture.bmp");

  if (!img.pixels) {
    fprintf(stderr, "Failed to load image\n");
    return 1;
  }

  printf("Width:  %u\n", img.width);
  printf("Height: %u\n", img.height);
  printf("Size:   %u bytes\n", img.size);

  printf("First 5 pixels (RGBA):\n");
  for (uint32_t i = 0; i < 5 && i < img.width * img.height; i++) {
    printf("  [%u] R:%3u G:%3u B:%3u A:%3u\n", i, img.pixels[i * 4 + 0],
           img.pixels[i * 4 + 1], img.pixels[i * 4 + 2], img.pixels[i * 4 + 3]);
  }

  freeImage(&img);
  return 0;
}
#endif
