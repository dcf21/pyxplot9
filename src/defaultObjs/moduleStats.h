// moduleStats.h
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2013 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2013 Ross Church
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

#ifndef _PPL_MODULE_STATS_H
#define _PPL_MODULE_STATS_H 1

#include "userspace/context.h"
#include "userspace/pplObj.h"

void pplfunc_binomialPDF  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_binomialCDF  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_chisqPDF     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_chisqCDF     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_chisqCDFi    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_gaussianPDF  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_gaussianCDF  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_gaussianCDFi (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lognormalPDF (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lognormalCDF (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lognormalCDFi(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_poissonPDF   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_poissonCDF   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tdistPDF     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tdistCDF     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tdistCDFi    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);

#endif

