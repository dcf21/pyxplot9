// pplObj_fns.h
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

#ifndef _PPLOBJ_FNS_H
#define _PPLOBJ_FNS_H 1

#include <stdio.h>
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/context.h"

// Functions for acting on pplObjs
void    pplObjInit     (ppl_context *c);
pplObj *pplObjNum      (pplObj *in, unsigned char amMalloced, double real, double imag);
pplObj *pplObjStr      (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, char *str);
pplObj *pplObjBool     (pplObj *in, unsigned char amMalloced, int stat);
pplObj *pplObjDate     (pplObj *in, unsigned char amMalloced, double unixTime);
pplObj *pplObjColor    (pplObj *in, unsigned char amMalloced, int scheme, double c1, double c2, double c3, double c4);
pplObj *pplObjDict     (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, dict *d);
pplObj *pplObjModule   (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, unsigned char frozen);
pplObj *pplObjList     (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, list *l);
pplObj *pplObjVector   (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, int size);
pplObj *pplObjMatrix   (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, int size1, int size2);
pplObj *pplObjFile     (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, FILE *f, int pipe);
pplObj *pplObjFunc     (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, pplFunc *f);
pplObj *pplObjType     (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, pplType *t);
pplObj *pplObjNull     (pplObj *in, unsigned char amMalloced);
pplObj *pplObjException(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, char *str, int errCode);
pplObj *pplObjGlobal   (pplObj *in, unsigned char amMalloced);
pplObj *pplObjExpression(pplObj *in, unsigned char amMalloced, void *bytecode);
pplObj *pplObjBytecode (pplObj *in, unsigned char amMalloced, void *parserline);
pplObj *pplObjZom      (pplObj *in, unsigned char amMalloced);
pplObj *pplObjUser     (pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, pplObj *prototype);
pplObj *pplObjCpy      (pplObj *out, pplObj *in, unsigned char lval, unsigned char outMalloced, unsigned char useMalloc);

pplObj *pplObjDeepCpy(pplObj *out, pplObj *in, int deep, unsigned char outMalloced, unsigned char useMalloc);
#endif

