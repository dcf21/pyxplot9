// colors.h
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

#ifndef _COLORS_H
#define _COLORS_H 1

#include "parser/parser.h"
#include "userspace/context.h"

int ppl_colorFromDict  (ppl_context *c, parserOutput *in, parserLine *pl, const int *ptab,
                        int fillColor, int *outcol, int *outcolspace, pplExpr **EXPoutcol,
                        double *outcol1, double *outcol2, double *outcol3, double *outcol4,
                        unsigned char *USEcol, unsigned char *USEcol1234);

int ppl_colorFromObj  (ppl_context *c, const pplObj *col, int *outcol, int *outcolspace, pplExpr **EXPoutcol,
                        double *outcol1, double *outcol2, double *outcol3, double *outcol4,
                        unsigned char *USEcol, unsigned char *USEcol1234);

#endif

