// moduleStats.h
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

#ifndef _PPL_MODULE_STATS_H
#define _PPL_MODULE_STATS_H 1

#include "coreUtils/dict.h"
#include "settings/settings.h"
#include "userspace/pplObj.h"

void pplfunc_binomialPDF  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_binomialCDF  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_chisqPDF     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_chisqCDF     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_chisqCDFi    (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_gaussianPDF  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_gaussianCDF  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_gaussianCDFi (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lognormalPDF (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lognormalCDF (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lognormalCDFi(pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_poissonPDF   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_poissonCDF   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tdistPDF     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tdistCDF     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tdistCDFi    (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);

#endif

