// arrows_fns.h
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

#ifndef _PPLSET_ARROWS_FNS_H
#define _PPLSET_ARROWS_FNS_H 1

#include "coreUtils/dict.h"
#include "parser/parser.h"
#include "settings/arrows.h"
#include "userspace/context.h"
#include "userspace/unitsDisp.h"

void          pplarrow_add         (ppl_context *context, pplarrow_object **inlist, parserOutput *in, parserLine *pl, const int *ptab);
void          pplarrow_remove      (ppl_context *context, pplarrow_object **inlist, parserOutput *in, parserLine *pl, const int *ptab, int quiet);
void          pplarrow_unset       (ppl_context *context, pplarrow_object **inlist, parserOutput *in, parserLine *pl, const int *ptab);
unsigned char pplarrow_compare     (ppl_context *context, pplarrow_object *a, pplarrow_object *b);
void          pplarrow_list_copy   (ppl_context *context, pplarrow_object **out, pplarrow_object **in);
void          pplarrow_list_destroy(ppl_context *context, pplarrow_object **inlist);
void          pplarrow_print       (ppl_context *context, pplarrow_object  *in, char *out);

#define pplarrow_add_get_system(X,Y) \
 { \
  pplObj *o = &in->stk[ptab[X]]; /* e.g. x0_system */ \
  if (o->objType != PPLOBJ_STR) Y = SW_SYSTEM_FIRST; \
  else                          Y = ppl_fetchSettingByName(&context->errcontext, (char *)o->auxil, SW_SYSTEM_INT, SW_SYSTEM_STR); \
 }

#define pplarrow_add_get_axis(X,Y) \
 { \
  pplObj *o = &in->stk[ptab[X]]; /* e.g. x0_axis */ \
  if   (o->objType != PPLOBJ_NUM) Y = 0; \
  else                            Y = (int)round(o->real); \
 }

#define pplarrow_add_check_axis(X) \
 { \
  pplObj *o = &in->stk[ptab[X]]; /* e.g. x0_axis */ \
  if (o->objType == PPLOBJ_NUM) \
   { \
    int i = (int)round(o->real); \
    if ((i<0)||(i>=MAX_AXES)) \
     { \
      sprintf(context->errStat.errBuff, "Axis number %d is out of range; axis numbers must be in the range 0 - %d", i, MAX_AXES-1); \
      TBADD(ERR_RANGE, in->stkCharPos[ptab[X]]); \
      return; \
     } \
   } \
 }

#define pplarrow_add_check_dimensions(X,Y) \
 { \
  pplObj *o = &in->stk[ptab[X]]; /* e.g. x0 */ \
  if (o->objType == PPLOBJ_NUM) \
   { \
    int i; \
    if ((Y == SW_SYSTEM_GRAPH) || (Y == SW_SYSTEM_PAGE)) \
     if (!o->dimensionless) \
      for (i=0; i<UNITS_MAX_BASEUNITS; i++) if (o->exponent[i] != (i==UNIT_LENGTH)) \
       { \
        sprintf(context->errStat.errBuff, "Coordinates specified in the graph and page systems must have dimensions of length. Received coordinate with dimensions of <%s>.", ppl_printUnit(context, o, NULL, NULL, 0, 1, 0)); \
        TBADD(ERR_UNIT, in->stkCharPos[ptab[X]]); \
       } \
    if (!gsl_finite(o->real)) \
     { \
      sprintf(context->errStat.errBuff, "Coordinates specified are not finite."); \
      TBADD(ERR_NUMERICAL, in->stkCharPos[ptab[X]]); \
     } \
   } \
 }

#define pplarrow_add_copy_coordinate(X,Y,Z) \
 { \
  pplObj *o = &in->stk[ptab[X]] , tempobj; /* e.g. x0 */ \
  if (o->objType != PPLOBJ_NUM) { o = &tempobj; pplObjNum(&tempobj,0,0,0); } \
  if ((Y == SW_SYSTEM_GRAPH) || (Y == SW_SYSTEM_PAGE)) \
    if (o->dimensionless) { o->dimensionless=0; o->exponent[UNIT_LENGTH]=1; o->real /= 100; } \
  Z = *o; \
 }

#endif

