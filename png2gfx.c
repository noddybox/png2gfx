/* png2gfx - convert PNG to the simple bitmap format used by espec and ezx81
   Copyright (C) 2026 Ian Cowburn <ianc@noddybox.co.uk>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <png.h>

#include "gfx-bitmap.h"

static struct
{
    int         r,g,b;
} bmpix[eGFX_Num_Colours]=
{
    {0x00,0x00,0x00},        /* BLACK */
    {0x00,0x00,0xff},        /* BLUE */
    {0xff,0x00,0x00},        /* RED */
    {0xff,0x00,0xff},        /* MAGENTA */
    {0x00,0xff,0x00},        /* GREEN */
    {0x00,0xff,0xff},        /* CYAN */
    {0xff,0xff,0x00},        /* YELLOW */
    {0xff,0xff,0xff},        /* WHITE */
    {0x60,0x60,0x60},        /* GREY */
};

#define HEADER_SIZE (8)

static const char *name;

static const char *Basename(const char *p)
{
    const char *base = strrchr(p, '/');

    return base ? base + 1 : p;
}

static int ConvertFromPNG(const char *input, const char *output)
{
    FILE *in;
    unsigned char header[HEADER_SIZE];

    if (!(in = fopen(input, "rb")))
    {
    	perror(input);
	return EXIT_FAILURE;
    }

    if (fread(header, 1, HEADER_SIZE, in) != HEADER_SIZE)
    {
    	fprintf(stderr, "%s: unable to read header from '%s'\n", name, input);
	fclose(in);
	return EXIT_FAILURE;
    }

    if (png_sig_cmp(header, 0, HEADER_SIZE))
    {
    	fprintf(stderr, "%s: '%s' not a PNG file\n", name, input);
	fclose(in);
	return EXIT_FAILURE;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					     NULL, NULL, NULL);

    if (!png)
    {
    	fprintf(stderr, "%s: unable to allocate PNG structures\n", name);
	fclose(in);
	return EXIT_FAILURE;
    }

    png_infop info = png_create_info_struct(png);

    if (!info)
    {
    	fprintf(stderr, "%s: unable to allocate PNG structures\n", name);
	fclose(in);
	return EXIT_FAILURE;
    }

    png_init_io(png, in);
    png_set_sig_bytes(png, HEADER_SIZE);

    png_read_png(png, info,
    		 PNG_TRANSFORM_EXPAND|PNG_TRANSFORM_STRIP_16|
		 PNG_TRANSFORM_STRIP_ALPHA|PNG_TRANSFORM_PACKING,
		 NULL);

    fclose(in);

    png_uint_32 width;
    png_uint_32 height;
    int depth;
    int colour_type;

    png_get_IHDR(png, info, &width, &height, &depth, &colour_type,
    		 NULL, NULL, NULL);

    switch (colour_type)
    {
    	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	    fprintf(stderr, "%s: grey scale image not supported\n", name);
	    fclose(in);
	    return EXIT_FAILURE;

	default:
	    break;
    }

    png_bytepp rows = png_get_rows(png, info);

    GFX_Bitmap bitmap = {0};

    bitmap.width = width;
    bitmap.height = height;
    bitmap.data = malloc(width * height);

    if (!bitmap.data)
    {
    	fprintf(stderr, "%s: failed to allocate buffer for destination image\n",
				name);
	return EXIT_FAILURE;
    }

    uint8_t *writer = bitmap.data;

    for(int y = 0; y < height; y++)
    {
	for(int x = 0; x < width; x++)
	{
	    int r,g,b;

	    r = rows[y][x * 3 + 0];
	    g = rows[y][x * 3 + 1];
	    b = rows[y][x * 3 + 2];

	    GFX_Bitmap_Colour bmp_pix = eGFX_Black;

	    if (r == 0 && g == 0 && b > 0)
	    {
	    	bmp_pix = eGFX_Blue;
	    }
	    else if (r > 0 && g == 0 && b == 0)
	    {
	    	bmp_pix = eGFX_Red;
	    }
	    else if (r > 0 && g == 0 && b > 0)
	    {
	    	bmp_pix = eGFX_Magenta;
	    }
	    else if (r == 0 && g > 0 && b == 0)
	    {
	    	bmp_pix = eGFX_Green;
	    }
	    else if (r == 0 && g > 0 && b > 0)
	    {
	    	bmp_pix = eGFX_Cyan;
	    }
	    else if (r > 0 && g > 0 && b == 0)
	    {
	    	bmp_pix = eGFX_Yellow;
	    }
	    else if (r == g && g == b)
	    {
		if (r < 0x20)
		{
		    bmp_pix = eGFX_Black;
		}
		else if (r > 0xa0)
		{
		    bmp_pix = eGFX_White;
		}
		else
		{
		    bmp_pix = eGFX_Grey;
		}
	    }

	    *writer++ = bmp_pix;
	}
    }

    FILE *out;

    if (!(out = fopen(output, "wb")))
    {
    	perror(output);
	return EXIT_FAILURE;
    }

    uint8_t *encoded = NULL;
    size_t len = 0;

    GFX_Bitmap_Status status = GFX_Bitmap_Encode(&bitmap, &encoded, &len);

    if (status == eGFX_Ok)
    {
    	fwrite(encoded, sizeof(uint8_t), len, out);
	free(encoded);
    }

    fclose(out);

    free(bitmap.data);

    return status == eGFX_Ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int ConvertToPNG(const char *input, const char *output)
{
    FILE *in;
    unsigned char header[HEADER_SIZE];

    if (!(in = fopen(input, "rb")))
    {
    	perror(input);
	return EXIT_FAILURE;
    }

    fseek(in, 0, SEEK_END);
    long source_len = ftell(in);
    fseek(in, 0, SEEK_SET);

    uint8_t *source_data = malloc(source_len);

    if (!source_data)
    {
    	fprintf(stderr, "%s: failed to allocate buffer for source image\n",
				name);
	return EXIT_FAILURE;
    }

    fread(source_data, sizeof(uint8_t), source_len, in);
    fclose(in);

    GFX_Bitmap bitmap = {0};

    GFX_Bitmap_Status status =
    	GFX_Bitmap_Decode(source_data, source_len, &bitmap);

    free(source_data);

    switch(status)
    {
    	case eGFX_InvalidFile:
	    fprintf(stderr, "%s: invalid GFX file\n", name);
	    return EXIT_FAILURE;

    	case eGFX_AllocFailed:
	    fprintf(stderr, "%s: failed to allocate memory for GFX file\n",
	    				name);
	    return EXIT_FAILURE;

	default:
	    break;
    }

    png_bytepp row_pointers = malloc(bitmap.height * sizeof *row_pointers);

    if (!row_pointers)
    {
    	fprintf(stderr, "%s: failed to allocate buffer for source image\n",
				name);
	free(bitmap.data);
	return EXIT_FAILURE;
    }

    for(int f = 0; f < bitmap.height; f++)
    {
    	row_pointers[f] = bitmap.data + f * bitmap.width;
    }

    FILE *out;

    if (!(out = fopen(output, "wb")))
    {
    	perror(output);
	return EXIT_FAILURE;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					      NULL, NULL, NULL);

    if (!png)
    {
    	fprintf(stderr, "%s: unable to allocate PNG structures\n", name);
	fclose(out);
	return EXIT_FAILURE;
    }

    png_infop info = png_create_info_struct(png);

    if (!info)
    {
    	fprintf(stderr, "%s: unable to allocate PNG structures\n", name);
	fclose(out);
	return EXIT_FAILURE;
    }

    png_init_io(png, out);

    png_set_IHDR(png, info, bitmap.width, bitmap.height,
    		 8, PNG_COLOR_TYPE_PALETTE,
    		 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		 PNG_FILTER_TYPE_BASE);

    png_color *palette = png_malloc(png, eGFX_Num_Colours * sizeof(png_color));

    for(int f = 0; f < eGFX_Num_Colours; f++)
    {
    	palette[f].red = bmpix[f].r;
    	palette[f].green = bmpix[f].g;
    	palette[f].blue = bmpix[f].b;
    }

    png_set_PLTE(png, info, palette, eGFX_Num_Colours);

    png_write_info(png, info);
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    png_free(png, palette);

    png_destroy_write_struct(&png, &info);

    fclose(out);

    free(row_pointers);

    return EXIT_SUCCESS;
}

static void Usage(void)
{
    fprintf(stderr, "%s: usage %s input_png_file output_file\n", name, name);
    fprintf(stderr, "%s: usage %s -r input_file output_png_file\n",
    				name, name);
}

int main(int argc, char *argv[])
{
    name = Basename(argv[0]);

    if (argc > 1 && strcmp(argv[1], "-r") == 0)
    {
	if (argc != 4)
	{
	    Usage();
	    return EXIT_FAILURE;
	}

	return ConvertToPNG(argv[2], argv[3]);
    }
    else
    {
	if (argc != 3)
	{
	    Usage();
	    return EXIT_FAILURE;
	}

	return ConvertFromPNG(argv[1], argv[2]);
    }
}
