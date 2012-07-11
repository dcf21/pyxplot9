// bmp_gifread.h
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
//
//               2009-2010 Michael Rutter
//
// $Id$
//
// Pyxplot is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// You should have received a copy of the GNU General Public License along with
// Pyxplot; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA  02110-1301, USA

// ----------------------------------------------------------------------------

// This file is edited from code which was kindly contributed to Pyxplot by
// Michael Rutter. It reads in data from GIF files.

#ifndef _PPL_BMP_GIFREAD_H
#define _PPL_BMP_GIFREAD_H 1

#include <stdio.h>

#include "coreUtils/errorReport.h"
#include "epsMaker/bmp_image.h"

void          ppl_bmp_gifread        (pplerr_context *ec, FILE *in, bitmap_data *image);
int           ppl_bmp_de_gifinterlace(pplerr_context *ec, bitmap_data *image);
unsigned long ppl_bmp_de_lzw         (pplerr_context *ec, unsigned char *buff, unsigned char *out, unsigned long len, int cs);
unsigned int  ppl_bmp_de_lzw_bits    (pplerr_context *ec, unsigned char *c,int st, int len);

#endif

