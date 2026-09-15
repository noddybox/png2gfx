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

static const char *Basename(const char *p)
{
    const char *base = strrchr(p, '/');

    return base ? base + 1 : p;
}

int main(int argc, char *argv[])
{
    const char *base = Basename(argv[0]);

    if (argc != 3)
    {
    	fprintf(stderr, "%s: usage %s input_file output_file\n", base, base);
	return EXIT_FAILURE;
    }

    FILE *in;
    unsigned char header[HEADER_SIZE];

    if (!(in = fopen(argv[1], "rb")))
    {
    	perror(argv[1]);
	return EXIT_FAILURE;
    }

    if (fread(header, 1, HEADER_SIZE, in) != HEADER_SIZE)
    {
    	fprintf(stderr, "%s: unable to read header from '%s'\n", base, argv[1]);
	fclose(in);
	return EXIT_FAILURE;
    }

    if (png_sig_cmp(header, 0, HEADER_SIZE))
    {
    	fprintf(stderr, "%s: '%s' not a PNG file\n", base, argv[1]);
	fclose(in);
	return EXIT_FAILURE;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					     NULL, NULL, NULL);

    if (!png)
    {
    	fprintf(stderr, "%s: unable to allocate PNG structures\n", base);
	fclose(in);
	return EXIT_FAILURE;
    }

    png_infop info = png_create_info_struct(png);

    if (!info)
    {
    	fprintf(stderr, "%s: unable to allocate PNG structures\n", base);
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

    int width;
    int height;
    int depth;
    int colour_type;

    png_get_IHDR(png, info, &width, &height, &depth, &colour_type,
    		 NULL, NULL, NULL);

    switch (colour_type)
    {
    	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	    fprintf(stderr, "%s: grey scale image not supported\n", base);
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
				base);
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

    if (!(out = fopen(argv[2], "wb")))
    {
    	perror(argv[2]);
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
