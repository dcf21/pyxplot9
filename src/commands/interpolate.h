// interpolate.h
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

#ifndef _INTERPOLATE_H
#define _INTERPOLATE_H 1

#include "parser/parser.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"

#define INTERP_AKIMA    23001
#define INTERP_LINEAR   23002
#define INTERP_LOGLIN   23003
#define INTERP_POLYN    23004
#define INTERP_SPLINE   23005
#define INTERP_STEPWISE 23006
#define INTERP_2D       23007
#define INTERP_BMPR     23008
#define INTERP_BMPG     23009
#define INTERP_BMPB     23010

void ppl_directive_interpolate(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth, int mode);
void ppl_spline_evaluate      (ppl_context *c, char *FuncName, splineDescriptor *desc, pplObj *in, pplObj *out, int *status, char *errout);
void ppl_interp2d_evaluate    (ppl_context *c, const char *FuncName, splineDescriptor *desc, const pplObj *in1, const pplObj *in2, const unsigned char bmp, pplObj *out, int *status, char *errout);

#endif

