// labels.c
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
#include "parser/parser.h"
#include "settings/settingTypes.h"
#include "settings/arrows_fns.h"
#include "settings/labels_fns.h"
#include "settings/settings.h"
#include "settings/withWords_fns.h"
#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"
#include "pplConstants.h"

// -------------------------------------------
// ROUTINES FOR MANIPULATING LABELS STRUCTURES
// -------------------------------------------

#define TBADD(et,pos) ppl_tbAdd(context,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

#define CMPVAL(X,Y) (ppl_unitsDimEqual(&X,&Y) && ppl_dblEqual(X.real , Y.real))

void ppllabel_add(ppl_context *context, ppllabel_object **inlist, parserOutput *in, parserLine *pl, const int *ptab)
 {
  int              i, system_x, system_y, system_z, gotTempstr, gotGap, gotAng;
  char            *tempstr, *label;
  double           gap, ang;
  ppllabel_object *out;

  pplarrow_add_get_system(PARSE_INDEX_x_system , system_x);
  pplarrow_add_get_system(PARSE_INDEX_y_system , system_y);
  pplarrow_add_get_system(PARSE_INDEX_z_system , system_z);

  pplarrow_add_check_dimensions(PARSE_INDEX_x , system_x);
  pplarrow_add_check_dimensions(PARSE_INDEX_y , system_y);
  pplarrow_add_check_dimensions(PARSE_INDEX_z , system_z);

  pplarrow_add_check_axis(PARSE_INDEX_x_axis);
  pplarrow_add_check_axis(PARSE_INDEX_y_axis);
  pplarrow_add_check_axis(PARSE_INDEX_z_axis);

  tempstr = (char *)in->stk[ptab[PARSE_INDEX_label_text]].auxil; // Read label text
  label = (char *)malloc(strlen(tempstr)+1);
  if (label == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1, "Out of memory"); return; }
  strcpy(label, tempstr);

  ang    =  in->stk[ptab[PARSE_INDEX_rotation]].real; // Check for rotation modifier
  gotAng = (in->stk[ptab[PARSE_INDEX_rotation]].objType == PPLOBJ_NUM);
  gap    =  in->stk[ptab[PARSE_INDEX_gap]].real;      // Check for gap modifier
  gotGap = (in->stk[ptab[PARSE_INDEX_gap]].objType == PPLOBJ_NUM);

  // Look up ID number of the label we are adding and find appropriate place for it in label list
  i = (int)round(in->stk[ptab[PARSE_INDEX_label_id]].real);
  while ((*inlist != NULL) && ((*inlist)->id < i)) inlist = &((*inlist)->next);
  if ((*inlist != NULL) && ((*inlist)->id == i))
   {
    out = *inlist;
    ppl_withWordsDestroy(context, &out->style);
   } else {
    out = (ppllabel_object *)malloc(sizeof(ppllabel_object));
    if (out == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1, "Out of memory"); return; }
    out->id     = i;
    out->next   = *inlist;
    *inlist     = out;
   }

  // Check for halign or valign modifiers
  tempstr    = (char *)in->stk[ptab[PARSE_INDEX_halign]].auxil;
  gotTempstr =        (in->stk[ptab[PARSE_INDEX_halign]].objType == PPLOBJ_STR);
  if (gotTempstr) out->HAlign = ppl_fetchSettingByName(&context->errcontext, tempstr, SW_HALIGN_INT, SW_HALIGN_STR);
  else            out->HAlign = context->set->graph_current.TextHAlign;
  tempstr    = (char *)in->stk[ptab[PARSE_INDEX_valign]].auxil;
  gotTempstr =        (in->stk[ptab[PARSE_INDEX_valign]].objType == PPLOBJ_STR);
  if (gotTempstr) out->VAlign = ppl_fetchSettingByName(&context->errcontext, tempstr, SW_VALIGN_INT, SW_VALIGN_STR);
  else            out->VAlign = context->set->graph_current.TextVAlign;

  if (gotAng) out->rotation = ang;
  else        out->rotation = 0.0;
  if (gotGap) out->gap      = gap;
  else        out->gap      = 0.0;

  // See if fontsize is specified
  out->fontsizeSet = (in->stk[ptab[PARSE_INDEX_fontsize]].objType==PPLOBJ_NUM);
  if (out->fontsizeSet)
   {
    double f = in->stk[ptab[PARSE_INDEX_fontsize]].real;
    if (f < 0) { f=1; ppl_error(&context->errcontext,ERR_MEMORY, -1, -1, "Fontsize specified in 'set fontsize' command was less than zero."); return; }
    if (!gsl_finite(f)) { f=1; ppl_error(&context->errcontext,ERR_MEMORY, -1, -1, "Fontsize specified in 'set fontsize' command was not finite."); return; }
    out->fontsize = f;
   }

  ppl_withWordsFromDict(context, in, pl, ptab, 0, &out->style);
  out->text  = label;
  out->system_x = system_x; out->system_y = system_y; out->system_z = system_z;
  pplarrow_add_get_axis(PARSE_INDEX_x_axis, out->axis_x);
  pplarrow_add_get_axis(PARSE_INDEX_y_axis, out->axis_y);
  pplarrow_add_get_axis(PARSE_INDEX_z_axis, out->axis_z);
  pplarrow_add_copy_coordinate(PARSE_INDEX_x, system_x, out->x);
  pplarrow_add_copy_coordinate(PARSE_INDEX_y, system_y, out->y);
  pplarrow_add_copy_coordinate(PARSE_INDEX_z, system_z, out->z);
  return;
 }

void ppllabel_remove(ppl_context *context, ppllabel_object **inlist, parserOutput *in, parserLine *pl, const int *ptab, int quiet)
 {
  int               pos;
  ppllabel_object **first;

  first = inlist;
  pos   = ptab[PARSE_INDEX_0label_list];

  if ((in->stk[pos].objType!=PPLOBJ_NUM)||(in->stk[pos].real<=0))
    ppllabel_list_destroy(context, inlist); // set noarrow with no number specified means all arrows deleted
  else
   {
    while (in->stk[pos].objType == PPLOBJ_NUM)
     {
      int i;
      pos = (int)round(in->stk[pos].real);
      if (pos<=0) break;
      i = (int)round(in->stk[pos+ptab[PARSE_INDEX_label_id]].real);
      inlist = first;
      while ((*inlist != NULL) && ((*inlist)->id < i)) inlist = &((*inlist)->next);
      if ((*inlist != NULL) && ((*inlist)->id == i))
       {
        ppllabel_object *obj = *inlist;
        *inlist = (*inlist)->next;
        ppl_withWordsDestroy(context, &obj->style);
        free(obj->text);
        free(obj);
       } else if (!quiet) {
        sprintf(context->errcontext.tempErrStr,"Label number %d is not defined", i);
        ppl_warning(&context->errcontext, ERR_GENERIC, NULL);
       }
     }
   }
  return;
 }

void ppllabel_unset(ppl_context *context, ppllabel_object **inlist, parserOutput *in, parserLine *pl, const int *ptab)
 {
  int               pos;
  ppllabel_object **first;

  ppllabel_remove(context, inlist, in, pl, ptab, 1); // First of all, remove any arrows which we are unsetting
  first = inlist;
  pos   = ptab[PARSE_INDEX_0label_list];

  if ((in->stk[pos].objType!=PPLOBJ_NUM)||(in->stk[pos].real<=0))
   {
    ppllabel_list_destroy(context, inlist);
    ppllabel_list_copy(context, inlist, &context->set->ppllabel_list_default); // Check to see whether we are unsetting ALL arrows, and if so, use the copy command
   }
  else
   {
    while (in->stk[pos].objType == PPLOBJ_NUM)
     {
      int i;
      ppllabel_object *obj;
      pos = (int)round(in->stk[pos].real);
      if (pos<=0) break;
      i = (int)round(in->stk[pos+ptab[PARSE_INDEX_label_id]].real);

      obj  = context->set->ppllabel_list_default;
      while ((obj != NULL) && (obj->id < i)) obj = (obj->next);
      if ((obj != NULL) && (obj->id == i))
       {
        ppllabel_object *new;
        inlist = first;
        while ((*inlist != NULL) && ((*inlist)->id < i)) inlist = &((*inlist)->next);
        new = (ppllabel_object *)malloc(sizeof(ppllabel_object));
        if (new == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); return; }
        *new = *obj;
        new->next = *inlist;
        new->text = (char *)malloc(strlen(obj->text)+1);
        if (new->text == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); free(new); return; }
        strcpy(new->text, obj->text);
        ppl_withWordsCpy(context, &new->style, &obj->style);
        *inlist = new;
       }
     }
   }
  return;
 }

unsigned char ppllabel_compare(ppl_context *context, ppllabel_object *a, ppllabel_object *b)
 {
  if (a->id!=b->id) return 0;
  if ( (!CMPVAL(a->x , b->x)) || (!CMPVAL(a->y , b->y)) || (!CMPVAL(a->z , b->z)) ) return 0;
  if ( (a->system_x!=b->system_x) || (a->system_y!=b->system_y) || (a->system_z!=b->system_z) ) return 0;
  if ( (a->axis_x!=b->axis_x) || (a->axis_y!=b->axis_y) || (a->axis_z!=b->axis_z) ) return 0;
  if ( (a->fontsizeSet!=b->fontsizeSet) || ((a->fontsizeSet)&&(a->fontsize!=b->fontsize)) ) return 0;
  if (!ppl_withWordsCmp(context, &a->style , &b->style)) return 0;
  if (strcmp(a->text , b->text)!=0) return 0;
  return 1;
 }

void ppllabel_list_copy(ppl_context *context, ppllabel_object **out, ppllabel_object **in)
 {
  *out = NULL;
  while (*in != NULL)
   {
    *out = (ppllabel_object *)malloc(sizeof(ppllabel_object));
    if (*out == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); return; }
    **out = **in;
    (*out)->next = NULL;
    (*out)->text = (char *)malloc(strlen((*in)->text)+1);
    if ((*out)->text == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); free(*out); *out=NULL; return; }
    strcpy((*out)->text, (*in)->text);
    ppl_withWordsCpy(context, &(*out)->style, &(*in)->style);
    in  = &((*in )->next);
    out = &((*out)->next);
   }
  return;
 }

void ppllabel_list_destroy(ppl_context *context, ppllabel_object **inlist)
 {
  ppllabel_object *obj, **first;

  first = inlist;
  while (*inlist != NULL)
   {
    obj = *inlist;
    *inlist = (*inlist)->next;
    ppl_withWordsDestroy(context, &obj->style);
    free(obj->text);
    free(obj);
   }
  *first = NULL;
  return;
 }

void ppllabel_print(ppl_context *context, ppllabel_object *in, char *out)
 {
  int i;
  ppl_strEscapify(in->text, out);
  i = strlen(out);
  sprintf(out+i, " at %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_x, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_x==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_x); i+=strlen(out+i); }
  sprintf(out+i, " %s,", ppl_unitsNumericDisplay(context, &(in->x),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_y, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_y==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_y); i+=strlen(out+i); }
  sprintf(out+i, " %s,", ppl_unitsNumericDisplay(context, &(in->y),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->system_z, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_z==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_z); i+=strlen(out+i); }
  sprintf(out+i, " %s", ppl_unitsNumericDisplay(context, &(in->z),0,0,0)); i+=strlen(out+i);
  if (in->rotation!=0.0) { sprintf(out+i, " rotate %s",
             ppl_numericDisplay( in->rotation *180/M_PI , context->numdispBuff[0], context->set->term_current.SignificantFigures, (context->set->term_current.NumDisplay==SW_DISPLAY_L))
           ); i+=strlen(out+i); }
  if (in->HAlign>0) { sprintf(out+i, " halign %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->HAlign, SW_HALIGN_INT, (void *)SW_HALIGN_STR, sizeof(char *))); i+=strlen(out+i); }
  if (in->VAlign>0) { sprintf(out+i, " valign %s", *(char **)ppl_fetchSettingName(&context->errcontext, in->VAlign, SW_VALIGN_INT, (void *)SW_VALIGN_STR, sizeof(char *))); i+=strlen(out+i); }
  if (in->gap!=0.0) { sprintf(out+i, " gap %s",
             ppl_numericDisplay( in->gap * 100          , context->numdispBuff[0], context->set->term_current.SignificantFigures, (context->set->term_current.NumDisplay==SW_DISPLAY_L))
           ); i+=strlen(out+i); }
  ppl_withWordsPrint(context, &in->style, out+i+6);
  if ((strlen(out+i+6)>0)||in->fontsizeSet) { sprintf(out+i, " with"); out[i+5]=' '; }
  else                                      { out[i]='\0'; }
  i+=strlen(out+i);
  if (in->fontsizeSet) { sprintf(out+i, " fontsize %s", ppl_numericDisplay(in->fontsize, context->numdispBuff[0], context->set->term_current.SignificantFigures, (context->set->term_current.NumDisplay==SW_DISPLAY_L))); i+=strlen(out+i); }
  return;
 }

