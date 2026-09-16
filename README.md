# png2gfx
A converter for PNG to the simple bitmap format used by espec and ezx81 and
from the bitmap format to PNG.

The format is best explained with this comment.

```
/* Draws a simply compressed bitmap.  The data is in the form (where b is a
   byte from the stream):

    	b < 0x80	Colour (0 Black, 1 Blue, 2 Red, 3 Magenta, 4 Green,
				5 Cyan, 6 Yellow, 7 White, 8 Grey)

	b >= 0x80	Repeat the last colour b-0x80 times.
*/
```

Usage: `png2gfx input.png outfile`
Usage: `png2gfx -c width height infile input.png`
