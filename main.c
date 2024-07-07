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

#define PI 3.1415926
#define EDGE_THRESHOLD 350

typedef struct
{
  byte *pixels;
  u32 width;
  u32 height;
  u32 bytes_per_pixel;
} bitmap;

typedef struct
{
  f32 *x;
  f32 *y;
} gradient;

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

void static gradient_destroy(gradient *g)
{
  free(g->x);
  free(g->y);
  g->x = NULL;
  g->y = NULL;
}

u32 static inline string_length(const char *string)
{
  u32 index = 0;

  while (string[index] != '\0') index++;

  return index;
}

i32 static inline map_char_index(i32 pixel, u32 chars_length)
{
  return pixel * (chars_length - 1) / 255;
}

i32 static inline boundary_check(const i32 x, const i32 y, const u32 width, const u32 height)
{
  if (x > width) return 0;
  else if (x < 0) return 0;
  else if (y > height) return 0;
  else if (y < 0) return 0;
  else if (y * width + x > width * height) return 0;
  else return 1;
}

void static sobel(gradient *g, const byte *grayscale, const bitmap data)
{
  /* fancy differential equation shit i looked up */
  g->x = malloc(sizeof(f32) * data.width * data.height);
  g->y = malloc(sizeof(f32) * data.width * data.height);

  i32 index;
  i32 square[9];

  for (i32 i = 0; i < data.height; i++)
  {
    for (i32 j = 0; j < data.width; j++)
    {
      index = 0;

      // record values in 3x3 square
      for (i32 m = -1; m <= 1; m++)
      {
        for (i32 n = -1; n <= 1; n++)
        {
          if (!boundary_check(j + n, i + m, data.width, data.height)) continue;
          square[index++] = grayscale[(i + m) * data.width + (j + n)];
        }
      }
      g->x[i * data.width + j] = abs(square[2] + 2*square[5] + square[8] - square[0] - 2*square[3] - square[6]);
      g->y[i * data.width + j] = abs(square[6] + 2*square[7] + square[8] - square[0] - 2*square[1] - square[2]);
    }
  }
}

char get_edge_char(f32 gx, f32 gy)
{
    f32 magnitude = sqrt(gx * gx + gy * gy);
    if (magnitude < EDGE_THRESHOLD) return ' ';

    f32 angle = atan2(gy, gx) * 180.0f / PI;

    if (angle < 0) angle += 360;

    if ((angle >= 0 && angle < 22.5) || (angle >= 157.5 && angle < 202.5) || (angle >= 337.5 && angle < 360))
    {
        return '|';  // vertical edge
    }
    else if ((angle >= 22.5 && angle < 67.5) || (angle >= 202.5 && angle < 247.5))
    {
        return '\\'; // descending diagonal edge
    }
    else if ((angle >= 67.5 && angle < 112.5) || (angle >= 247.5 && angle < 292.5))
    {
        return '_';  // horizontal edge
    }
    else if ((angle >= 112.5 && angle < 157.5) || (angle >= 292.5 && angle < 337.5))
    {
        return '/';  // ascending diagonal edge
    }

    return ' ';
}

i32 main(i32 argc, char **argv)
{
  bitmap data;
  gradient g;
  i32 output;
  
  output = bitmap_read("input.bmp", &data);
  if (output > 0)
  {
    printf("error reading bitmap\n");
    return 1;
  }

  /* get grayscale */
  byte grayscale[data.width * data.height];

  for (u32 i = 0; i < data.width * data.height; i++)
  {
    grayscale[i] = 0;
  }

  for (i32 i = 0; i < data.height; i++)
  {
    for (i32 j = 0; j < data.width; j++)
    {
      const u32 index = i * data.width + j;
      
      // 32 bits cuz 8 x 3 close to 32
      u32 pixel = 0;
      pixel  = data.pixels[index + 0];
      pixel += data.pixels[index + 1];
      pixel += data.pixels[index + 2];
      pixel /= 3;

      grayscale[index] = pixel;
    }
  }

  char buffer[data.width * data.height];
  const char *map = " .;coPO?@#";

  /* first pass of ascii chars */
  for (i32 i = 0; i < data.height; i++)
  {
    for (i32 j = 0; j < data.width; j++)
    {
      const u32 index = i * data.height + j;
      const u32 length = string_length(map);
      const u32 map_index = map_char_index(grayscale[index], length);
      buffer[index] = map[map_index];
    }
  }

  /* get gradient */
  sobel(&g, grayscale, data);

  /* second pass with edges */
  for (i32 i = 0; i < data.height; i++)
  {
    for (i32 j = 0; j < data.width; j++)
    {
      const i32 index = i * data.width + j;
      const char ascii = get_edge_char(g.x[index], g.y[index]);
      if (ascii == ' ') continue;
      buffer[index] = ascii;
    }
  }

  /* write buffer to file */
  FILE *out = fopen("output.txt", "w");
  if (out == NULL)
  {
    printf("error opening output file\n");
    return 1;
  }

  for (i32 i = 0; i < data.height; i++)
  {
    for (i32 j = 0; j < data.width; j++)
    {
      const i32 index = i * data.width + j;

      (void)fputc(buffer[index], out);
    }
    (void)fputc('\n', out);
  }

  fclose(out);

  gradient_destroy(&g);
  bitmap_destroy(&data);

  return 0;
}
