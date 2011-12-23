// expEvalCalculus.h
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

#ifndef _EXPEVALCALCULUS_H
#define _EXPEVALCALCULUS_H 1

#include "userspace/context.h"
#include "userspace/pplObj.h"

void ppl_expIntegrate    (ppl_context *c, char *expr, char *dummy, pplObj *min  , pplObj *max , pplObj *out, int dollarAllowed, int *errPos, int *errType, char *errText, int recursionDepth);
void ppl_expDifferentiate(ppl_context *c, char *expr, char *dummy, pplObj *point, pplObj *step, pplObj *out, int dollarAllowed, int *errPos, int *errType, char *errText, int recursionDepth);

#endif
