// pplObj.h
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

#include <stdio.h>
#include "expressions/pplObjUnits.h"

#ifndef _PPLOBJ_H
#define _PPLOBJ_H 1

#define PPLOBJ_NUM   0
#define PPLOBJ_STR   1
#define PPLOBJ_DATE  2
#define PPLOBJ_COL   3
#define PPLOBJ_DICT  4
#define PPLOBJ_LIST  5
#define PPLOBJ_FILE  6
#define PPLOBJ_FUNC  7
#define PPLOBJ_TYPE  8
#define PPLOBJ_NULL  9
#define PPLOBJ_EXC  10
#define PPLOBJ_USER 11

#ifndef _PPLOBJ_C
extern char *pplObjTypeNames[];
#endif

typedef struct pplObj
 {
  double         real, imag;
  unsigned char  dimensionless, FlagComplex, modified, TempType;
  unsigned char  amMalloced, auxilMalloced;
  int            objType, auxilLen;
  struct pplObj *objCustomType;
  void          *auxil;
  double         exponent[UNITS_MAX_BASEUNITS];
 } pplObj;

typedef struct pplFile { int iNodeCount; FILE   *file; int open; } pplFile;
typedef struct pplType { int iNodeCount; pplObj *type; int id; } pplType;
typedef struct pplFunc { int iNodeCount; } pplFunc;

void   *pplObjZero    (pplObj *in, unsigned char amMalloced);
void   *pplObjNullStr (pplObj *in, unsigned char amMalloced);
pplObj *pplObjCpy     (pplObj *in, unsigned char useMalloc);

#endif

