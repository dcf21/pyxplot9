// unitsDisp.h
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

#ifndef _PPL_UNITSDISP_H
#define _PPL_UNITSDISP_H 1

#include "userspace/context.h"
#include "userspace/pplObj.h"

#ifndef _PPL_UNITSDISP_C
extern const char *SIprefixes_full[];
extern const char *SIprefixes_abbrev[];
extern const char *SIprefixes_latex[];
#endif

char *ppl_unitsNumericDisplay(ppl_context *c, pplObj *in, int N, int typeable, int NSigFigs);
void ppl_udFindOptimalNextUnit(ppl_context *c, pplObj *in, unsigned char first, unit **best, double *pow);
void ppl_udPrefixFix(ppl_context *c, pplObj *in, unit **UnitList, double *UnitPow, int *UnitPref, int Nunits);
char *ppl_printUnit(ppl_context *c, const pplObj *in, double *numberOutReal, double *numberOutImag, int N, int DivAllowed, int typeable);
void ppl_unitsStringEvaluate(ppl_context *c, char *in, pplObj *out, int *end, int *errpos, char *errText);
void ppl_newPreferredUnit(ppl_context *c, PreferredUnit **output, char *instr, int OutputContext, int *errpos, char *errText);

#endif

