// arrows.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gsl/gsl_math.h>

#include "coreUtils/dict.h"
#include "coreUtils/list.h"
#include "coreUtils/errorReport.h"
#include "expressions/traceback_fns.h"
#include "parser/cmdList.h"
#include "settings/arrows_fns.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"
#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"
#include "pplConstants.h"

// -------------------------------------------
// ROUTINES FOR MANIPULATING ARROWS STRUCTURES
// -------------------------------------------

#define TBADD(et,pos) ppl_tbAdd(context,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

#define CMPVAL(X,Y) (ppl_unitsDimEqual(&X,&Y) && ppl_dblEqual(X.real , Y.real))

void pplarrow_add(ppl_context *context, pplarrow_object **inlist, parserOutput *in, parserLine *pl, const int *ptab)
 {
  int              i;
  int              system_x0, system_y0, system_z0, system_x1, system_y1, system_z1;
  char            *tempstr;
  pplarrow_object *out;

  pplarrow_add_get_system(PARSE_INDEX_x0_system , system_x0);
  pplarrow_add_get_system(PARSE_INDEX_y0_system , system_y0);
  pplarrow_add_get_system(PARSE_INDEX_z0_system , system_z0);
  pplarrow_add_get_system(PARSE_INDEX_x1_system , system_x1);
  pplarrow_add_get_system(PARSE_INDEX_y1_system , system_y1);
  pplarrow_add_get_system(PARSE_INDEX_z1_system , system_z1);

  pplarrow_add_check_dimensions(PARSE_INDEX_x0 , system_x0);
  pplarrow_add_check_dimensions(PARSE_INDEX_y0 , system_y0);
  pplarrow_add_check_dimensions(PARSE_INDEX_z0 , system_z0);
  pplarrow_add_check_dimensions(PARSE_INDEX_x1 , system_x1);
  pplarrow_add_check_dimensions(PARSE_INDEX_y1 , system_y1);
  pplarrow_add_check_dimensions(PARSE_INDEX_z1 , system_z1);

  pplarrow_add_check_axis(PARSE_INDEX_x0_axis);
  pplarrow_add_check_axis(PARSE_INDEX_y0_axis);
  pplarrow_add_check_axis(PARSE_INDEX_z0_axis);
  pplarrow_add_check_axis(PARSE_INDEX_x1_axis);
  pplarrow_add_check_axis(PARSE_INDEX_y1_axis);
  pplarrow_add_check_axis(PARSE_INDEX_z1_axis);

  i = (int)round(in->stk[ptab[PARSE_INDEX_arrow_id]].real);
  while ((*inlist != NULL) && ((*inlist)->id < i)) inlist = &((*inlist)->next);
  if ((*inlist != NULL) && ((*inlist)->id == i))
   {
    out = *inlist;
    ppl_withWordsDestroy(context, &out->style);
   } else {
    out = (pplarrow_object *)malloc(sizeof(pplarrow_object));
    if (out == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1, "Out of memory."); return; }
    out->id     = i;
    out->next   = *inlist;
    *inlist     = out;
   }

  // Check whether arrow head type has been specified
  tempstr = (char *)in->stk[ptab[PARSE_INDEX_arrow_style]].auxil;
  if (in->stk[ptab[PARSE_INDEX_arrow_style]].objType!=PPLOBJ_STR) out->pplarrow_style = SW_ARROWTYPE_HEAD;
  else                                                            out->pplarrow_style = ppl_fetchSettingByName(&context->errcontext, tempstr, SW_ARROWTYPE_INT, SW_ARROWTYPE_STR);

  // Check what style keywords have been specified in the 'with' clause
  ppl_withWordsFromDict(context, in, pl, ptab, 0, &out->style);

  out->system_x0 = system_x0; out->system_y0 = system_y0; out->system_z0 = system_z0;
  out->system_x1 = system_x1; out->system_y1 = system_y1; out->system_z1 = system_z1;

  pplarrow_add_get_axis(PARSE_INDEX_x0_axis, out->axis_x0);
  pplarrow_add_get_axis(PARSE_INDEX_y0_axis, out->axis_y0);
  pplarrow_add_get_axis(PARSE_INDEX_z0_axis, out->axis_z0);
  pplarrow_add_get_axis(PARSE_INDEX_x1_axis, out->axis_x1);
  pplarrow_add_get_axis(PARSE_INDEX_y1_axis, out->axis_y1);
  pplarrow_add_get_axis(PARSE_INDEX_z1_axis, out->axis_z1);

  pplarrow_add_copy_coordinate(PARSE_INDEX_x0, system_x0,out->x0);
  pplarrow_add_copy_coordinate(PARSE_INDEX_y0, system_y0,out->y0);
  pplarrow_add_copy_coordinate(PARSE_INDEX_z0, system_z0,out->z0);
  pplarrow_add_copy_coordinate(PARSE_INDEX_x1, system_x1,out->x1);
  pplarrow_add_copy_coordinate(PARSE_INDEX_y1, system_y1,out->y1);
  pplarrow_add_copy_coordinate(PARSE_INDEX_z1, system_z1,out->z1);
  return;
 }

void pplarrow_remove(ppl_context *context, pplarrow_object **inlist, parserOutput *in, parserLine *pl, const int *ptab, int quiet)
 {
  int               pos;
  pplarrow_object **first;

  first = inlist;
  pos   = ptab[PARSE_INDEX_0arrow_list];

  if ((in->stk[pos].objType!=PPLOBJ_NUM)||(in->stk[pos].real<=0))
    pplarrow_list_destroy(context, inlist); // set noarrow with no number specified means all arrows deleted
  else
   {
    while (in->stk[pos].objType == PPLOBJ_NUM)
     {
      int i;
      pos = (int)round(in->stk[pos].real);
      if (pos<=0) break;
      i = (int)round(in->stk[pos+ptab[PARSE_INDEX_arrow_id]].real);
      inlist = first;
      while ((*inlist != NULL) && ((*inlist)->id < i)) inlist = &((*inlist)->next);
      if ((*inlist != NULL) && ((*inlist)->id == i))
       {
        pplarrow_object *obj = *inlist;
        *inlist = (*inlist)->next;
        ppl_withWordsDestroy(context, &obj->style);
        free(obj);
       } else if (!quiet) {
        sprintf(context->errcontext.tempErrStr,"Arrow number %d is not defined", i);
        ppl_warning(&context->errcontext, ERR_GENERIC, NULL);
       }
     }
   }
  return;
 }

void pplarrow_unset(ppl_context *context, pplarrow_object **inlist, parserOutput *in, parserLine *pl, const int *ptab)
 {
  int               pos;
  pplarrow_object **first;

  pplarrow_remove(context, inlist, in, pl, ptab, 1); // First of all, remove any arrows which we are unsetting
  first = inlist;
  pos   = ptab[PARSE_INDEX_0arrow_list];

  if ((in->stk[pos].objType!=PPLOBJ_NUM)||(in->stk[pos].real<=0))
   {
    pplarrow_list_destroy(context, inlist);
    pplarrow_list_copy(context, inlist, &context->set->pplarrow_list_default); // Check to see whether we are unsetting ALL arrows, and if so, use the copy command
   }
  else
   {
    while (in->stk[pos].objType == PPLOBJ_NUM)
     {
      int i;
      pplarrow_object *obj;
      pos = (int)round(in->stk[pos].real);
      if (pos<=0) break;
      i = (int)round(in->stk[pos+ptab[PARSE_INDEX_arrow_id]].real);

      obj  = context->set->pplarrow_list_default;
      while ((obj != NULL) && (obj->id < i)) obj = (obj->next);
      if ((obj != NULL) && (obj->id == i))
       {
        pplarrow_object *new;
        inlist = first;
        while ((*inlist != NULL) && ((*inlist)->id < i)) inlist = &((*inlist)->next);
        new = (pplarrow_object *)malloc(sizeof(pplarrow_object));
        if (new == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); return; }
        *new = *obj;
        new->next = *inlist;
        ppl_withWordsCpy(context, &new->style, &obj->style);
        *inlist = new;
       }
     }
   }
  return;
 }

unsigned char pplarrow_compare(ppl_context *context, pplarrow_object *a, pplarrow_object *b)
 {
  if (a->id!=b->id) return 0;
  if ( (!CMPVAL(a->x0 , b->x0)) || (!CMPVAL(a->y0 , b->y0)) || (!CMPVAL(a->z0 , b->z0)) ) return 0;
  if ( (!CMPVAL(a->x1 , b->x1)) || (!CMPVAL(a->y1 , b->y1)) || (!CMPVAL(a->z1 , b->z1)) ) return 0;
  if ( (a->system_x0!=b->system_x0) || (a->system_y0!=b->system_y0) || (a->system_z0!=b->system_z0) ) return 0;
  if ( (a->system_x1!=b->system_x1) || (a->system_y1!=b->system_y1) || (a->system_z1!=b->system_z1) ) return 0;
  if ( (a->axis_x0!=b->axis_x0) || (a->axis_y0!=b->axis_y0) || (a->axis_z0!=b->axis_z0) ) return 0;
  if ( (a->axis_x1!=b->axis_x1) || (a->axis_y1!=b->axis_y1) || (a->axis_z1!=b->axis_z1) ) return 0;
  if (!ppl_withWordsCmp(context, &a->style , &b->style)) return 0;
  return 1;
 }

void pplarrow_list_copy(ppl_context *context, pplarrow_object **out, pplarrow_object **in)
 {
  *out = NULL;
  while (*in != NULL)
   {
    *out = (pplarrow_object *)malloc(sizeof(pplarrow_object));
    if (*out == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); return; }
    **out = **in;
    (*out)->next = NULL;
    ppl_withWordsCpy(context, &(*out)->style, &(*in)->style);
    in  = &((*in )->next);
    out = &((*out)->next);
   }
  return;
 }

void pplarrow_list_destroy(ppl_context *context, pplarrow_object **inlist)
 {
  pplarrow_object *obj, **first;

  first = inlist;
  while (*inlist != NULL)
   {
    obj = *inlist;
    *inlist = (*inlist)->next;
    ppl_withWordsDestroy(context, &obj->style);
    free(obj);
   }
  *first = NULL;
  return;
 }

void pplarrow_print(ppl_context *context, pplarrow_object *in, char *out)
 {
  int i;
  sprintf(out, "from %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_x0, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *)));
  i = strlen(out);
  if (in->system_x0==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_x0); i+=strlen(out+i); }
  sprintf(out+i, " %s,", ppl_unitsNumericDisplay(context,&(in->x0),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_y0, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_y0==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_y0); i+=strlen(out+i); }
  sprintf(out+i, " %s,", ppl_unitsNumericDisplay(context,&(in->y0),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_z0, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_z0==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_z0); i+=strlen(out+i); }
  sprintf(out+i, " %s ", ppl_unitsNumericDisplay(context,&(in->z0),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, "to %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_x1, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_x1==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_x1); i+=strlen(out+i); }
  sprintf(out+i, " %s,", ppl_unitsNumericDisplay(context,&(in->x1),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_y1, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_y1==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_y1); i+=strlen(out+i); }
  sprintf(out+i, " %s,", ppl_unitsNumericDisplay(context,&(in->y1),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_z1, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_z1==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_z1); i+=strlen(out+i); }
  sprintf(out+i, " %s", ppl_unitsNumericDisplay(context,&(in->z1),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " with %s ", *(char **)ppl_fetchSettingName(&context->errcontext, in->pplarrow_style, SW_ARROWTYPE_INT, (void *)SW_ARROWTYPE_STR, sizeof(char *))); i+=strlen(out+i);
  ppl_withWordsPrint(context, &in->style, out+i);
  return;
 }

