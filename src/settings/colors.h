// colors.h
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
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

#ifndef _COLORS_H
#define _COLORS_H 1

#include "coreUtils/dict.h"
#include "userspace/context.h"

int ppl_colorFromDict  (ppl_context *c, dict *in, char *prefix, int *outcol, int *outcolspace,
                        double *outcol1, double *outcol2, double *outcol3, double *outcol4,
                        char **outcolS, char **outcol1S, char **outcol2S, char **outcol3S, char **outcol4S,
                        unsigned char *USEcol, unsigned char *USEcol1234, int *errpos, unsigned char malloced);

#endif

