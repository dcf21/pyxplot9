// bmp_a85.h
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
//
//               2009-2010 Michael Rutter
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

// This file is edited from code which was kindly contributed to PyXPlot by
// Michael Rutter. It efficiently encodes a string of raw image data into
// postscript's ASCII 85 data format, making use of all of the printable ASCII
// characters.

#ifndef _PPL_BMP_A85_H
#define _PPL_BMP_A85_H 1

#include <stdio.h>
#include "coreUtils/errorReport.h"

unsigned int ppl_bmp_A85(pplerr_context *ec, FILE* fout, unsigned char* in, int len);

#endif

