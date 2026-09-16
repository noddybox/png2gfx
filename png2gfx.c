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

#define NO_BMPIX 9

static struct
{
    int         r,g,b;
} bmpix[NO_BMPIX]=
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

#define BLACK	0
#define BLUE	1
#define RED	2
#define MAGENTA	3
#define GREEN	4
#define CYAN	5
#define YELLOW	6
#define WHITE	7
#define GREY	8

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

    unsigned char *dest = malloc(width * height);

    if (!dest)
    {
    	fprintf(stderr, "%s: failed to allocate buffer for destination image\n",
				name);
	return EXIT_FAILURE;
    }

    unsigned char *writer = dest;

    for(int y = 0; y < height; y++)
    {
	for(int x = 0; x < width; x++)
	{
	    int r,g,b;

	    r = rows[y][x * 3 + 0];
	    g = rows[y][x * 3 + 1];
	    b = rows[y][x * 3 + 2];

	    unsigned char bmp_pix = 0;

	    if (r == 0 && g == 0 && b > 0)
	    {
	    	bmp_pix = BLUE;
	    }
	    else if (r > 0 && g == 0 && b == 0)
	    {
	    	bmp_pix = RED;
	    }
	    else if (r > 0 && g == 0 && b > 0)
	    {
	    	bmp_pix = MAGENTA;
	    }
	    else if (r == 0 && g > 0 && b == 0)
	    {
	    	bmp_pix = GREEN;
	    }
	    else if (r == 0 && g > 0 && b > 0)
	    {
	    	bmp_pix = CYAN;
	    }
	    else if (r > 0 && g > 0 && b == 0)
	    {
	    	bmp_pix = YELLOW;
	    }
	    else if (r == g && g == b)
	    {
		if (r < 0x20)
		{
		    bmp_pix = BLACK;
		}
		else if (r > 0xa0)
		{
		    bmp_pix = WHITE;
		}
		else
		{
		    bmp_pix = GREY;
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

    unsigned char *reader = dest;
    int len = width * height;
    int count = 0;
    unsigned char last = UCHAR_MAX;

    while(len--)
    {
    	if (last == *reader)
	{
	    if (count < 0x7f)
	    {
	    	count++;
	    }
	    else
	    {
	    	fputc(0x80 + count, out);
	    	fputc(last, out);
		count = 0;
	    }
	}
	else
	{
	    if (count > 0)
	    {
	    	fputc(0x80 + count, out);
		count = 0;
	    }

	    fputc(*reader, out);
	}

	last = *reader++;
    }

    if (count > 0)
    {
	fputc(0x80 + count, out);
    }

    fclose(out);

    free(dest);

    return EXIT_SUCCESS;
}

static int ConvertToPNG(const char *input, const char *output,
			int width, int height)
{
    FILE *in;
    unsigned char header[HEADER_SIZE];

    if (!(in = fopen(input, "rb")))
    {
    	perror(input);
	return EXIT_FAILURE;
    }

    int len = width * height;
    unsigned char *source = malloc(len);
    png_bytepp row_pointers = malloc(height * sizeof *row_pointers);

    if (!source || !row_pointers)
    {
    	fprintf(stderr, "%s: failed to allocate buffer for source image\n",
				name);
	return EXIT_FAILURE;
    }

    for(int f = 0; f < height; f++)
    {
    	row_pointers[f] = source + f * width;
    }

    unsigned char *writer = source;
    unsigned char last = 0;
    unsigned char pix = 0;

    while(len && !feof(in))
    {
    	unsigned char b = fgetc(in);

	if (b < 0x80)
	{
	    pix = b;
	    *writer++ = pix;
	    len--;
	}
	else
	{
	    for(int f = 0; len && f < b - 0x80; f++)
	    {
	    	*writer++ = pix;
		len--;
	    }
	}
    }

    if (len)
    {
    	fprintf(stderr, "%s: source bitmap file too short\n", name);
	fclose(in);
	free(source);
	return EXIT_FAILURE;
    }

    fclose(in);

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

    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_PALETTE,
    		 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		 PNG_FILTER_TYPE_BASE);

    png_color *palette = png_malloc(png, NO_BMPIX * sizeof(png_color));

    for(int f = 0; f < NO_BMPIX; f++)
    {
    	palette[f].red = bmpix[f].r;
    	palette[f].green = bmpix[f].g;
    	palette[f].blue = bmpix[f].b;
    }

    png_set_PLTE(png, info, palette, NO_BMPIX);

    png_write_info(png, info);
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    png_free(png, palette);

    png_destroy_write_struct(&png, &info);

    fclose(out);

    free(source);
    free(row_pointers);

    return EXIT_SUCCESS;
}

static void Usage(void)
{
    fprintf(stderr, "%s: usage %s input_png_file output_file\n", name, name);
    fprintf(stderr, "%s: usage %s -c width height input_file output_png_file\n",
    				name, name);
}

int main(int argc, char *argv[])
{
    name = Basename(argv[0]);

    if (argc > 1 && strcmp(argv[1], "-c") == 0)
    {
	if (argc != 6)
	{
	    Usage();
	    return EXIT_FAILURE;
	}

	int width = atoi(argv[2]);
	int height = atoi(argv[3]);

	if (width < 1 || height < 1)
	{
	    fprintf(stderr, "%s: invalid size %dx%d\n", name, width, height);
	    return EXIT_FAILURE;
	}

	return ConvertToPNG(argv[4], argv[5], width, height);

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
