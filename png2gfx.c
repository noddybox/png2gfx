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

#include <png.h>

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

    if (!(in = fopen(argv[1], "rb")))
    {
    	perror(argv[1]);
	return EXIT_FAILURE;
    }

    FILE *out;

    if (!(out = fopen(argv[2], "wb")))
    {
    	perror(argv[2]);
	return EXIT_FAILURE;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					     NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, in);
    png_read_png(png, info, PNG_TRANSFORM_EXPAND, NULL);

    return EXIT_SUCCESS;
}
