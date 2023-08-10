#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

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
  unsigned char *pixels;
  int32_t width;
  int32_t height;
  int32_t bytes_per_pixel;
} bitmap;

void bitmap_read(const char *source, unsigned char **pixels, int32_t *width, int32_t *height, int32_t *bytes_per_pixel)
{
  FILE *image = fopen(source, "rb");
  if(image == NULL)
  {
    printf("error opening bitmap file\n");
  }

  // get pixel information
  int32_t data_offset;
  fseek(image, DATA_OFFSET_OFFSET, SEEK_SET);
  fread(&data_offset, 4, 1, image);
  fseek(image, WIDTH_OFFSET, SEEK_SET);
  fread(width, 4, 1, image);
  fseek(image, HEIGHT_OFFSET, SEEK_SET);
  fread(height, 4, 1, image);
  int16_t bits_per_pixel;
  fseek(image, BITS_PER_PIXEL_OFFSET, SEEK_SET);
  fread(&bits_per_pixel, 2, 1, image);
  *bytes_per_pixel = ((int32_t) bits_per_pixel) / 8;

  // get data for each pixel
  int padded_row_size = (int)(4 * ceil((float)(*width) / 4.0f)) * (*bytes_per_pixel);
  int unpadded_row_size = (*width) * (*bytes_per_pixel);
  int total_size = unpadded_row_size * (*height);
  *pixels = (unsigned char*) malloc(total_size);
  unsigned char *current_row_pointer = *pixels + ((*height - 1) * unpadded_row_size);
  for(int i = 0; i < *height; i++)
  {
    fseek(image, data_offset + (i * padded_row_size), SEEK_SET);
    fread(current_row_pointer, 1, unpadded_row_size, image);
    current_row_pointer -= unpadded_row_size;
  }

  fclose(image);
}

uint32_t string_length(const char *string)
{
  uint32_t index = 0;
  while(string[index] != '\0')
  {
    index++;
  }
  return index;
}

int32_t map_value(int32_t value, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
  return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main(int argc, char **argv)
{
  bitmap data;
  bitmap_read("input.bmp", &data.pixels, &data.width, &data.height, &data.bytes_per_pixel);

  FILE *out = fopen("output.txt", "w");

  const char *map = " .,:;ox%#@";
  for(int32_t i = 0; i < data.height; i++)
  {
    for(int32_t j = 0; j < data.width; j++)
    {
      const uint32_t pixel_index = (i * data.width + j);
      uint32_t pixel;
      pixel  = data.pixels[pixel_index + 0];
      pixel += data.pixels[pixel_index + 1];
      pixel += data.pixels[pixel_index + 2];
      pixel /= 3;
      const uint32_t map_length = string_length(map);
      // translate color value to ascii value on map
      const int32_t char_index = map_value(pixel, 0, 255, 0, map_length);
      fputc(map[char_index], out);
    }
    fputc('\n', out);
  }

  return 0;
}
