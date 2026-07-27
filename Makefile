# png2gfx - convert PNG to the simple bitmap format used by espec and ezx81
# Copyright (C) 2026 Ian Cowburn <ianc@noddybox.co.uk>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
LIBS:=$(shell pkg-config --libs libpng)
CFLAGS:=$(shell pkg-config --cflags libpng)
TARGET=png2gfx

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LIBS)

clean:
	-rm -f $(TARGET) core
