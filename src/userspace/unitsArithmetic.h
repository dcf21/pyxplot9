// unitsArithmetic.h
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

#ifndef _PPL_UNITSARITH_H
#define _PPL_UNITSARITH_H 1

#include "userspace/context.h"
#include "userspace/pplObj.h"

void ppl_unitsDimCpy(pplObj *o, const pplObj *i);
void ppl_unitsDimInverse(pplObj *o, const pplObj *i);
int ppl_unitsDimEqual(const pplObj *a, const pplObj *b);
int ppl_unitsDimEqual2(const pplObj *a, const unit *b);
int ppl_unitsDimEqual3(const unit *a, const unit *b);
unsigned char ppl_tempTypeMatch(unsigned char a, unsigned char b);
void ppl_uaPow(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText);
void ppl_uaMul(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText);
void ppl_uaDiv(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText);
void ppl_uaAdd(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText);
void ppl_uaSub(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText);
void ppl_uaMod(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText);

#endif

