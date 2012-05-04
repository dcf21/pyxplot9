// axes.c
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

#define _AXES_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gsl/gsl_math.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "coreUtils/dict.h"
#include "coreUtils/list.h"
#include "coreUtils/errorReport.h"

#include "expressions/expCompile_fns.h"

#include "pplConstants.h"
#include "settings/settingTypes.h"

#include "settings/axes_fns.h"
#include "settings/settings.h"
#include "settings/withWords_fns.h"

#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"

// ------------------------------------------------------
// Functions for creating and destroying axis descriptors
// ------------------------------------------------------

void pplaxis_destroy(ppl_context *context, pplset_axis *in)
 {
  int i;
  if (in->format    != NULL) { free        (           in->format   ); in->format    = NULL; }
  if (in->label     != NULL) { free        (           in->label    ); in->label     = NULL; }
  if (in->linkusing != NULL) { pplExpr_free((pplExpr *)in->linkusing); in->linkusing = NULL; }
  if (in->ticsM.tickList != NULL) { free(in->ticsM.tickList); in->ticsM.tickList = NULL; }
  if (in->ticsM.tickStrs != NULL)
   {
    for (i=0; in->ticsM.tickStrs[i]!=NULL; i++) free(in->ticsM.tickStrs[i]);
    free(in->ticsM.tickStrs);
    in->ticsM.tickStrs = NULL;
   }
  if (in->tics.tickList  != NULL) { free(in->tics.tickList ); in->tics.tickList  = NULL; }
  if (in->tics.tickStrs  != NULL)
   {
    for (i=0; in->tics.tickStrs[i]!=NULL; i++) free(in->tics.tickStrs[i]);
    free(in->tics.tickStrs );
    in->tics.tickStrs  = NULL;
   }
  return;
 }

// c->set->axis_default is a safe fallback axis because it contains no malloced strings
#define XMALLOC(X) (tmp = malloc(X)); if (tmp==NULL) { ppl_error(&context->errcontext, ERR_MEMORY, -1, -1,"Out of memory"); *out = context->set->axis_default; return; }

void pplaxis_copy(ppl_context *context, pplset_axis *out, const pplset_axis *in)
 {
  void *tmp;
  *out = *in;
  if (in->format    != NULL) { out->format   = (char *)XMALLOC(strlen(in->format    )+1); strcpy(out->format   , in->format    ); }
  if (in->label     != NULL) { out->label    = (char *)XMALLOC(strlen(in->label     )+1); strcpy(out->label    , in->label     ); }
  if (in->linkusing != NULL) { out->linkusing= (void *)pplExpr_cpy((pplExpr *)in->linkusing); }
  pplaxis_copyTics (context,out,in);
  pplaxis_copyMTics(context,out,in);
  return;
 }

void pplaxis_copyTics(ppl_context *context, pplset_axis *out, const pplset_axis *in)
 {
  int   i=0,j;
  void *tmp;
  if (in->tics.tickStrs != NULL)
   {
    for (i=0; in->tics.tickStrs[i]!=NULL; i++);
    out->tics.tickStrs= XMALLOC((i+1)*sizeof(char *));
    for (j=0; j<i; j++) { out->tics.tickStrs[j] = XMALLOC(strlen(in->tics.tickStrs[j])+1); strcpy(out->tics.tickStrs[j], in->tics.tickStrs[j]); }
    out->tics.tickStrs[i] = NULL;
   }
  if (in->tics.tickList != NULL)
   {
    out->tics.tickList= (double *)XMALLOC((i+1)*sizeof(double));
    memcpy(out->tics.tickList, in->tics.tickList, (i+1)*sizeof(double)); // NB: For this to be safe, tics.tickLists MUST have double to correspond to NULL in tics.tickStrs
   }
  return;
 }

void pplaxis_copyMTics(ppl_context *context, pplset_axis *out, const pplset_axis *in)
 {
  int   i=0,j;
  void *tmp;
  if (in->ticsM.tickStrs != NULL)
   {
    for (i=0; in->ticsM.tickStrs[i]!=NULL; i++);
    out->ticsM.tickStrs= XMALLOC((i+1)*sizeof(char *));
    for (j=0; j<i; j++) { out->ticsM.tickStrs[j] = XMALLOC(strlen(in->ticsM.tickStrs[j])+1); strcpy(out->ticsM.tickStrs[j], in->ticsM.tickStrs[j]); }
    out->ticsM.tickStrs[i] = NULL;
   }
  if (in->ticsM.tickList != NULL)
   {
    out->ticsM.tickList= (double *)XMALLOC((i+1)*sizeof(double));
    memcpy(out->ticsM.tickList, in->ticsM.tickList, (i+1)*sizeof(double)); // NB: For this to be safe, tics.tickLists MUST have double to correspond to NULL in tics.tickStrs
   }
  return;
 }

unsigned char pplaxis_cmpTics(ppl_context *context, const pplset_tics *ta, const pplset_tics *tb, const pplObj *ua, const pplObj *ub, const int la, const int lb)
 {
  int i,j;
  if (la != lb) return 0;
  if (!ppl_unitsDimEqual(ua,ub)) return 0;
  if (ta->tickMinSet!=tb->tickMinSet) return 0;
  if (ta->tickMaxSet!=tb->tickMaxSet) return 0;
  if (ta->tickStepSet!=tb->tickStepSet) return 0;
  if (ta->tickMinSet && (ta->tickMin!=tb->tickMin)) return 0;
  if (ta->tickMaxSet && (ta->tickMax!=tb->tickMax)) return 0;
  if (ta->tickStepSet && (ta->tickStep!=tb->tickStep)) return 0;
  if ((ta->tickList==NULL)&&(tb->tickList==NULL)) return 1;
  if ((ta->tickList==NULL)||(tb->tickList==NULL)) return 0;
  for (i=0; ta->tickStrs[i]!=NULL; i++);
  for (j=0; tb->tickStrs[j]!=NULL; j++);
  if (i!=j) return 0; // tick lists have different lengths
  for (j=0; j<i; j++)
   {
    if (ta->tickList[j] != tb->tickList[j]) return 0;
    if (strcmp(ta->tickStrs[j], tb->tickStrs[j])!=0) return 0;
   }
  return 1;
 }

// Where along this axis, in the range 0 (left) to 1 (right) should the value
// xin go? xrn = Region Number for interpolated axes which do not have
// monotonically increasing ordinate values.
double pplaxis_GetPosition(double xin, pplset_axis *xa, int xrn, unsigned char AllowOffBounds)
 {
  int imin, imax, i;
  if (xa==NULL) return xin;
  if (xa->AxisLinearInterpolation != NULL) // Axis is linearly interpolated
   {
    imin = xa->AxisTurnings[xrn  ];
    imax = xa->AxisTurnings[xrn+1];
    for (i=imin; i<imax; i++)
     {
      if (   ((xa->AxisLinearInterpolation[i] < xin) && (xa->AxisLinearInterpolation[i+1] >= xin))
          || ((xa->AxisLinearInterpolation[i] > xin) && (xa->AxisLinearInterpolation[i+1] <= xin)) )
       return (i + (xin-xa->AxisLinearInterpolation[i])/(xa->AxisLinearInterpolation[i+1]-xa->AxisLinearInterpolation[i])) / (AXISLINEARINTERPOLATION_NPOINTS-1);
     }
    return GSL_NAN;
   }
  if (!AllowOffBounds)
   {
    if (xa->MaxFinal > xa->MinFinal)
     { if ((xin<xa->MinFinal) || (xin>xa->MaxFinal)) return GSL_NAN; }
    else
     { if ((xin>xa->MinFinal) || (xin<xa->MaxFinal)) return GSL_NAN; }
   }
  if ((xa->LogFinal==SW_BOOL_TRUE) && (xin <= 0)) return GSL_NAN;
  if (xa->LogFinal!=SW_BOOL_TRUE) return (xin - xa->MinFinal) / (xa->MaxFinal - xa->MinFinal); // Either linear...
  else                            return (log(xin)-log(xa->MinFinal)) / (log(xa->MaxFinal)-log(xa->MinFinal)); // ... or logarithmic
 }

// What is the value of this axis at point xin, in the range 0 (left) to 1 (right)?
double pplaxis_InvGetPosition(double xin, pplset_axis *xa)
 {
  if (xa->AxisLinearInterpolation != NULL) // Axis is linearly interpolated
   {
    int    i = floor(xin * (AXISLINEARINTERPOLATION_NPOINTS-1));
    double x = xin * (AXISLINEARINTERPOLATION_NPOINTS-1) - i;
    if (i>=AXISLINEARINTERPOLATION_NPOINTS-1) return xa->AxisLinearInterpolation[AXISLINEARINTERPOLATION_NPOINTS-1];
    if (i<                                 0) return xa->AxisLinearInterpolation[0];
    return xa->AxisLinearInterpolation[i]*(1-x) + xa->AxisLinearInterpolation[i+1]*x;
   }
  if (xa->LogFinal!=SW_BOOL_TRUE) return xa->MinFinal + xin * (xa->MaxFinal - xa->MinFinal); // Either linear...
  else                            return xa->MinFinal * pow(xa->MaxFinal / xa->MinFinal , xin); // ... or logarithmic
 }

