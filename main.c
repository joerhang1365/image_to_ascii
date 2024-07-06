#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef uint16_t u16;
typedef int16_t  i16;
typedef int32_t  i32;
typedef uint32_t u32;
typedef float    f32;
typedef unsigned char byte;

#define DATA_OFFSET_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPERSION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0

typedef struct
{
  byte *pixels;
  u32 width;
  u32 height;
  u32 bytes_per_pixel;
} bitmap;

i32 static bitmap_read(const char *source, bitmap *bmp)
{
  i32 success = 0;

  FILE *image = fopen(source, "rb");
  if (image == NULL)
  {
    printf("error opening bitmap file\n");
    success = 1;
  }

  // get pixel information
  i32 data_offset = 0;
  u32 count = 0;

  (void)fseek(image, DATA_OFFSET_OFFSET, SEEK_SET);
  count = fread(&data_offset, 4, 1, image);
  if (count == 0)
  {
    printf("error reading data_offset\n");
    success = 1;
  }

  (void)fseek(image, WIDTH_OFFSET, SEEK_SET);
  count = fread(&bmp->width, 4, 1, image);
  if (count == 0)
  {
    printf("error reading width\n");
    success = 1;
  }

  (void)fseek(image, HEIGHT_OFFSET, SEEK_SET);
  count = fread(&bmp->height, 4, 1, image);
  if (count == 0)
  {
    printf("error reading height\n");
    success = 1;
  }

  u16 bits_per_pixel;
  (void)fseek(image, BITS_PER_PIXEL_OFFSET, SEEK_SET);
  count = fread(&bits_per_pixel, 2, 1, image);
  if (count == 0)
  {
    printf("error reading bits per pixel\n");
    success = 1;
  }
  bmp->bytes_per_pixel = (u32) bits_per_pixel / 8;

  // get data for each pixel
  u32 padded_row_size = (u32)(4 * ceil((f32)(bmp->width) / 4.0f)) * (bmp->bytes_per_pixel);
  u32 unpadded_row_size = (bmp->width) * (bmp->bytes_per_pixel);
  u32 total_size = unpadded_row_size * (bmp->height);
  bmp->pixels = (byte*) malloc(total_size);
  if (bmp->pixels == NULL)
  {
    printf("error allocating memory to pixels\n");
    success = 1;
  }

  byte *current_row_pointer = bmp->pixels + ((bmp->height - 1) * unpadded_row_size);
  for (u32 i = 0; i < bmp->height; i++)
  {
    (void)fseek(image, data_offset + (i * padded_row_size), SEEK_SET);
    count = fread(current_row_pointer, 1, unpadded_row_size, image);
    if (count == 0)
    {
      printf("error reading current_row_pointer\n");
      success = 1;
    }
    current_row_pointer -= unpadded_row_size;
  }

  fclose(image);

  return success;
}

void static bitmap_destroy(bitmap *bitmap)
{
  free(bitmap->pixels);
  bitmap->pixels = NULL;
}

u32 static inline string_length(const char *string)
{
  u32 index = 0;

  while (string[index] != '\0')
  {
    index++;
  }

  return index;
}

i32 static inline map_char(i32 pixel, u32 chars_length)
{
  return pixel * (chars_length - 1) / 255;
}

i32 main(i32 argc, char **argv)
{
  bitmap data;
  i32 output;
  
  output = bitmap_read("input.bmp", &data);
  if (output > 0)
  {
    printf("error reading bitmap\n");
    return 1;
  }

  FILE *out = fopen("output.txt", "w");
  if (out == NULL)
  {
    printf("error opening output file\n");
    return 1;
  }

  const char *map = " .;coPO?@#";

  for (i32 i = 0; i < data.height; i++)
  {
    for (i32 j = 0; j < data.width; j++)
    {
      const u32 pixel_index = (i * data.width + j);
      
      u32 pixel;
      pixel  = data.pixels[pixel_index + 0];
      pixel += data.pixels[pixel_index + 1];
      pixel += data.pixels[pixel_index + 2];
      pixel /= 3;

      const u32 chars_length = string_length(map);
      // translate color value to ascii value on map
      const i32 char_index = map_char(pixel, chars_length);
      (void)fputc(map[char_index], out);
    }
    (void)fputc('\n', out);
  }

  bitmap_destroy(&data);

  return 0;
}
