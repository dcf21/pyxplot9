// pplObjCmp.h
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

#ifndef _PPLOBJCMP_H
#define _PPLOBJCMP_H 1

#include "userspace/context.h"
#include "userspace/pplObj.h"

void pplcol_RGBtoHSB(double ri, double gi, double bi, double *ho, double *so, double *bo);
void pplcol_CMYKtoRGB(double ci, double mi, double yi, double ki, double *ro, double *go, double *bo);
void pplcol_HSBtoRGB(double hi, double si, double bi, double *ro, double *go, double *bo);
void pplcol_CMYKtoHSB(double ci, double mi, double yi, double ki, double *ho, double *so, double *bo);
void pplcol_RGBtoCMYK(double ri, double gi, double bi, double *co, double *mo, double *yo, double *ko);
void pplcol_HSBtoCMYK(double hi, double si, double bi, double *co, double *mo, double *yo, double *ko);

int pplObjCmpQuiet(const void *a, const void *b);
int pplObjCmp(ppl_context *c, const pplObj *a, const pplObj *b, int *status, int *errType, char *errText, int iterDepth);

#endif

