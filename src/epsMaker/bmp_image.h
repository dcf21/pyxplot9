// bmp_image.h
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
//
// $Id$
//
// PyXPlot is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// You should have received a copy of the GNU General Public License along with
// PyXPlot; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA  02110-1301, USA

// ----------------------------------------------------------------------------

#ifndef _BMP_IMAGE_H
#define _BMP_IMAGE_H 1

// Colour channel configurations

#define BMP_COLOUR_BMP     1001
#define BMP_COLOUR_PALETTE 1002
#define BMP_COLOUR_GREY    1003
#define BMP_COLOUR_RGB     1004

// Image compression types

#define BMP_ENCODING_NULL  1100
#define BMP_ENCODING_LZW   1101
#define BMP_ENCODING_FLATE 1102
#define BMP_ENCODING_DCT   1103

typedef struct bitmap_data
 {
  unsigned char *data, *palette, *trans;
  unsigned long  data_len;
  int            pal_len, width, height, depth, type, colour, TargetCompression, flags;
  double         XDPI, YDPI;
 } bitmap_data;

#endif

