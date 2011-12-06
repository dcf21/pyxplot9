// expEvalOps.h
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

#ifndef _EXPEVALOPS_H
#define _EXPEVALOPS_H 1

#include "userspace/context.h"
#include "userspace/pplObj.h"

void ppl_opAdd(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int *status, int *errType, char *errText);
void ppl_opSub(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int *status, int *errType, char *errText);
void ppl_opMul(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int *status, int *errType, char *errText);
void ppl_opDiv(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int *status, int *errType, char *errText);

#endif

