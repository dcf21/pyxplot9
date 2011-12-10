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
#include "userspace/pplObjUnits.h"

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>

#include "coreUtils/dict.h"

#ifndef _PPLOBJ_H
#define _PPLOBJ_H 1

#define PPLOBJ_NUM   0 /* numeric value */
#define PPLOBJ_STR   1 /* string value */
#define PPLOBJ_BOOL  2 /* boolean */
#define PPLOBJ_DATE  3 /* date/time */
#define PPLOBJ_COL   4 /* color */
#define PPLOBJ_DICT  5 /* dictionary */
#define PPLOBJ_MOD   6 /* module */
#define PPLOBJ_LIST  7 /* list */
#define PPLOBJ_VEC   8 /* vector (list of numerical values with common units) */
#define PPLOBJ_MAT   9 /* matrix */
#define PPLOBJ_FILE 10 /* file handle */
#define PPLOBJ_FUNC 11 /* function */
#define PPLOBJ_TYPE 12 /* data type */
#define PPLOBJ_NULL 13 /* null value */
#define PPLOBJ_EXC  14 /* exception type */
#define PPLOBJ_GLOB 15 /* global variable marker */
#define PPLOBJ_ZOM  16 /* zombie */
#define PPLOBJ_USER 17 /* user-defined datatype */

typedef struct pplObj
 {
  double         real, imag;
  unsigned char  dimensionless, flagComplex, tempType, immutable;
  unsigned char  amMalloced, auxilMalloced;
  int            objType, auxilLen;
  struct pplObj *objPrototype;
  void          *auxil;
  double         exponent[UNITS_MAX_BASEUNITS];
  struct pplObj *self_lval;
  double        *self_dval;
  struct pplObj *self_this;
 } pplObj;

#ifndef _PPLOBJ_C
extern const char *pplObjTypeNames[];
extern const int   pplObjTypeOrder[];
extern pplObj     *pplObjPrototypes;
#endif

// Structures for describing files and types
typedef struct pplFile      { int refCount; FILE *file;    int open; } pplFile;
typedef struct pplType      { int refCount; dict *methods; int id;   } pplType;
typedef struct pplVectorRaw { int refCount; gsl_vector *v; } pplVectorRaw;
typedef struct pplVector    { int refCount; gsl_vector *v; gsl_vector_view *view; pplVectorRaw *raw; } pplVector;
typedef struct pplMatrixRaw { int refCount; gsl_matrix *m; } pplMatrixRaw;
typedef struct pplMatrix    { int refCount; gsl_matrix *m; gsl_matrix_view *view; int sliceNext; pplMatrixRaw *raw; } pplMatrix;

#endif

