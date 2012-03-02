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

void pplaxis_destroyAxis(ppl_context *context, pplset_axis *in)
 {
  int i;
  if (in->format    != NULL) { free(in->format   ); in->format    = NULL; }
  if (in->label     != NULL) { free(in->label    ); in->label     = NULL; }
  if (in->linkusing != NULL) { free(in->linkusing); in->linkusing = NULL; }
  if (in->MTickList != NULL) { free(in->MTickList); in->MTickList = NULL; }
  if (in->MTickStrs != NULL)
   {
    for (i=0; in->MTickStrs[i]!=NULL; i++) free(in->MTickStrs[i]);
    free(in->MTickStrs);
    in->MTickStrs = NULL;
   }
  if (in->TickList  != NULL) { free(in->TickList ); in->TickList  = NULL; }
  if (in->TickStrs  != NULL)
   {
    for (i=0; in->TickStrs[i]!=NULL; i++) free(in->TickStrs[i]);
    free(in->TickStrs );
    in->TickStrs  = NULL;
   }
  return;
 }

// c->set->axis_default is a safe fallback axis because it contains no malloced strings
#define XMALLOC(X) (tmp = malloc(X)); if (tmp==NULL) { ppl_error(&context->errcontext, ERR_MEMORY, -1, -1,"Out of memory"); *out = context->set->axis_default; return; }

void pplaxis_copy(ppl_context *context, pplset_axis *out, const pplset_axis *in)
 {
  void *tmp;
  *out = *in;
  if (in->format    != NULL) { out->format   = (char   *)XMALLOC(strlen(in->format    )+1); strcpy(out->format   , in->format    ); }
  if (in->label     != NULL) { out->label    = (char   *)XMALLOC(strlen(in->label     )+1); strcpy(out->label    , in->label     ); }
  if (in->linkusing != NULL) { out->linkusing= (char   *)XMALLOC(strlen(in->linkusing )+1); strcpy(out->linkusing, in->linkusing ); }
  pplaxis_copyTics (context,out,in);
  pplaxis_copyMTics(context,out,in);
  return;
 }

void pplaxis_copyTics(ppl_context *context, pplset_axis *out, const pplset_axis *in)
 {
  int   i=0,j;
  void *tmp;
  if (in->TickStrs != NULL)
   {
    for (i=0; in->TickStrs[i]!=NULL; i++);
    out->TickStrs= XMALLOC((i+1)*sizeof(char *));
    for (j=0; j<i; j++) { out->TickStrs[j] = XMALLOC(strlen(in->TickStrs[j])+1); strcpy(out->TickStrs[j], in->TickStrs[j]); }
    out->TickStrs[i] = NULL;
   }
  if (in->TickList != NULL)
   {
    out->TickList= (double *)XMALLOC((i+1)*sizeof(double));
    memcpy(out->TickList, in->TickList, (i+1)*sizeof(double)); // NB: For this to be safe, TickLists MUST have double to correspond to NULL in TickStrs
   }
  return;
 }

void pplaxis_copyMTics(ppl_context *context, pplset_axis *out, const pplset_axis *in)
 {
  int   i=0,j;
  void *tmp;
  if (in->MTickStrs != NULL)
   {
    for (i=0; in->MTickStrs[i]!=NULL; i++);
    out->MTickStrs= XMALLOC((i+1)*sizeof(char *));
    for (j=0; j<i; j++) { out->MTickStrs[j] = XMALLOC(strlen(in->MTickStrs[j])+1); strcpy(out->MTickStrs[j], in->MTickStrs[j]); }
    out->MTickStrs[i] = NULL;
   }
  if (in->MTickList != NULL)
   {
    out->MTickList= (double *)XMALLOC((i+1)*sizeof(double));
    memcpy(out->MTickList, in->MTickList, (i+1)*sizeof(double)); // NB: For this to be safe, TickLists MUST have double to correspond to NULL in TickStrs
   }
  return;
 }

unsigned char pplaxis_cmpTics(ppl_context *context, const pplset_axis *a, const pplset_axis *b)
 {
  int i,j;
  if ((a->TickList==NULL)&&(b->TickList==NULL)) return 1;
  if ((a->TickList==NULL)||(b->TickList==NULL)) return 0;
  for (i=0; a->TickStrs[i]!=NULL; i++);
  for (j=0; b->TickStrs[j]!=NULL; j++);
  if (i!=j) return 0; // Tick lists have different lengths
  for (j=0; j<i; j++)
   {
    if (a->TickList[j] != b->TickList[j]) return 0;
    if (strcmp(a->TickStrs[j], b->TickStrs[j])!=0) return 0;
   }
  return 1;
 }

unsigned char pplaxis_cmpMTics(ppl_context *context, const pplset_axis *a, const pplset_axis *b)
 {
  int i,j;
  if ((a->MTickList==NULL)&&(b->MTickList==NULL)) return 1;
  if ((a->MTickList==NULL)||(b->MTickList==NULL)) return 0;
  for (i=0; a->MTickStrs[i]!=NULL; i++);
  for (j=0; b->MTickStrs[j]!=NULL; j++);
  if (i!=j) return 0; // Tick lists have different lengths
  for (j=0; j<i; j++)
   {
    if (a->MTickList[j] != b->MTickList[j]) return 0;
    if (strcmp(a->MTickStrs[j], b->MTickStrs[j])!=0) return 0;
   }
  return 1;
 }

