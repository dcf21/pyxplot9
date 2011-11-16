// labels.c
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

#include "settings/arrows.h"
#include "settings/labels.h"
#include "settings/settings.h"

#include "userspace/context.h"

// -------------------------------------------
// ROUTINES FOR MANIPULATING LABELS STRUCTURES
// -------------------------------------------

#define ASSERT_LENGTH(VAR) \
  if (!(VAR->dimensionless)) \
   { \
    for (i=0; i<UNITS_MAX_BASEUNITS; i++) \
     if (VAR->exponent[i] != (i==UNIT_LENGTH)) \
      { \
       sprintf(context->errcontext.tempErrStr,"The gap size supplied to the 'set label' command must have dimensions of length. Supplied gap size input has units of <%s>.",ppl_units_GetUnitStr(VAR,NULL,NULL,1,1,0)); \
       ppl_error(&context->errcontext,ERR_NUMERIC, -1, -1, context->errcontext.tempErrStr); \
       return; \
      } \
   } \
  else { VAR->real /= 100; } // By default, dimensionless positions are in centimetres

#define ASSERT_ANGLE(VAR) \
  if (!(VAR->dimensionless)) \
   { \
    for (i=0; i<UNITS_MAX_BASEUNITS; i++) \
     if (VAR->exponent[i] != (i==UNIT_ANGLE)) \
      { \
       sprintf(context->errcontext.tempErrStr,"The rotation angle supplied to the 'set label' command must have dimensions of angle. Supplied input has units of <%s>.",ppl_units_GetUnitStr(VAR,NULL,NULL,1,1,0)); \
       ppl_error(&context->errcontext,ERR_NUMERIC, -1, -1, context->errcontext.tempErrStr); \
       return; \
      } \
   } \
  else { VAR->real *= M_PI/180.0; } // By default, dimensionless angles are in degrees

#define CMPVAL(X,Y) (ppl_units_DimEqual(&X,&Y) && ppl_units_DblEqual(X.real , Y.real))

void ppllabel_add(ppl_context *context, ppllabel_object **inlist, dict *in)
 {
  int             *tempint, i, system_x, system_y, system_z;
  char            *tempstr, *label;
  pplObj          *tempval, tempvalobj, *gap, *ang;
  withWords        ww_temp1;
  ppllabel_object *out;

  pplarrow_add_get_system("x",system_x); pplarrow_add_get_system("y",system_y); pplarrow_add_get_system("z",system_z);

  pplarrow_add_check_dimensions("x",system_x); pplarrow_add_check_dimensions("y",system_y); pplarrow_add_check_dimensions("z",system_z);

  pplarrow_add_check_axis("x"); pplarrow_add_check_axis("y"); pplarrow_add_check_axis("z");

  tempstr = (char *)ppl_dictLookup(in,"ppllabel_text");
  label = (char *)malloc(strlen(tempstr)+1);
  if (label == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1, "Out of memory"); return; }
  strcpy(label, tempstr);

  // Check for rotation modifier
  ang = (pplObj *)ppl_dictLookup(in, "rotation");
  if (ang != NULL) { ASSERT_ANGLE(ang); }

  // Check for gap modifier
  gap = (pplObj *)ppl_dictLookup(in, "gap");
  if (gap != NULL) { ASSERT_LENGTH(gap); }

  // Look up ID number of the label we are adding and find appropriate place for it in label list
  tempint = (int *)ppl_dictLookup(in,"ppllabel_id");
  while ((*inlist != NULL) && ((*inlist)->id < *tempint)) inlist = &((*inlist)->next);
  if ((*inlist != NULL) && ((*inlist)->id == *tempint))
   {
    out = *inlist;
    ppl_withWordsDestroy(&out->style);
   } else {
    out = (ppllabel_object *)malloc(sizeof(ppllabel_object));
    if (out == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1, "Out of memory"); return; }
    out->id   = *tempint;
    out->next = *inlist;
    *inlist   = out;
   }

  // Check for halign or valign modifiers
  tempstr = (char *)ppl_dictLookup(in,"halign");
  if (tempstr != NULL) out->HAlign = FetchSettingByName(tempstr, SW_HALIGN_INT, SW_HALIGN_STR);
  else                 out->HAlign = pplset_graph_current.TextHAlign;
  tempstr = (char *)ppl_dictLookup(in,"valign");
  if (tempstr != NULL) out->VAlign = FetchSettingByName(tempstr, SW_VALIGN_INT, SW_VALIGN_STR);
  else                 out->VAlign = pplset_graph_current.TextVAlign;

  if (ang != NULL) out->rotation = ang->real;
  else             out->rotation = 0.0;
  if (gap != NULL) out->gap      = gap->real;
  else             out->gap      = 0.0;

  ppl_withWordsFromDict(in, &ww_temp1, 1);
  ppl_withWordsCpy(&out->style, &ww_temp1);
  out->text  = label;
  out->system_x = system_x; out->system_y = system_y; out->system_z = system_z;
  pplarrow_add_get_axis("x", out->axis_x); pplarrow_add_get_axis("y", out->axis_y); pplarrow_add_get_axis("z", out->axis_z);
  pplarrow_add_copy_coordinate("x",system_x,out->x); pplarrow_add_copy_coordinate("y",system_y,out->y); pplarrow_add_copy_coordinate("z",system_z,out->z);
  return;
 }

void ppllabel_remove(ppl_context *context, ppllabel_object **inlist, dict *in)
 {
  int          *tempint;
  ppllabel_object *obj, **first;
  list         *templist;
  dict         *tempdict;
  listIterator *listiter;

  first = inlist;
  templist = (list *)ppl_dictLookup(in,"ppllabel_list,");
  listiter = ppl_listIterateInit(templist);
  if (listiter == NULL) ppllabel_list_destroy(inlist); // set nolabel with no number specified means all labels deleted
  while (listiter != NULL)
   {
    tempdict = (dict *)listiter->data;
    tempint = (int *)ppl_dictLookup(tempdict,"ppllabel_id");
    inlist = first;
    while ((*inlist != NULL) && ((*inlist)->id < *tempint)) inlist = &((*inlist)->next);
    if ((*inlist != NULL) && ((*inlist)->id == *tempint))
     {
      obj   = *inlist;
      *inlist = (*inlist)->next;
      ppl_withWordsDestroy(&obj->style);
      free(obj->text);
      free(obj);
     } else {
      //sprintf(context->errcontext.tempErrStr,"Label number %d is not defined", *tempint);
      //ppl_error(&context->errcontext,ERR_GENERAL, -1, -1, context->errcontext.tempErrStr);
     }
    ppl_listIterate(&listiter);
   }
  return;
 }

void ppllabel_unset(ppl_context *context, ppllabel_object **inlist, dict *in)
 {
  int             *tempint;
  ppllabel_object *obj, *new, **first;
  list            *templist;
  dict            *tempdict;
  listIterator    *listiter;

  ppllabel_remove(inlist, in); // First of all, remove any labels which we are unsetting
  first = inlist;
  templist = (list *)ppl_dictLookup(in,"ppllabel_list,");
  listiter = ppl_listIterateInit(templist);

  if (listiter == NULL) ppllabel_list_copy(inlist, &ppllabel_list_default); // Check to see whether we are unsetting ALL labels, and if so, use the copy command
  while (listiter != NULL)
   {
    tempdict = (dict *)listiter->data;
    tempint = (int *)ppl_dictLookup(tempdict,"ppllabel_id"); // Go through each ppllabel_id in supplied list and copy items from default list into current settings
    obj  = ppllabel_list_default;
    while ((obj != NULL) && (obj->id < *tempint)) obj = (obj->next);
    if ((obj != NULL) && (obj->id == *tempint))
     {
      inlist = first;
      while ((*inlist != NULL) && ((*inlist)->id < *tempint)) inlist = &((*inlist)->next);
      new = (ppllabel_object *)malloc(sizeof(ppllabel_object));
      if (new == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); return; }
      *new = *obj;
      new->next = *inlist;
      new->text = (char *)malloc(strlen(obj->text)+1);
      if (new->text == NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); free(new); return; }
      strcpy(new->text, obj->text);
      ppl_withWordsCpy(&new->style, &obj->style);
      *inlist = new;
     }
    ppl_listIterate(&listiter);
   }
  return;
 }

unsigned char ppllabel_compare(ppl_context *context, ppllabel_object *a, ppllabel_object *b)
 {
  if (a->id!=b->id) return 0;
  if ( (!CMPVAL(a->x , b->x)) || (!CMPVAL(a->y , b->y)) || (!CMPVAL(a->z , b->z)) ) return 0;
  if ( (a->system_x!=b->system_x) || (a->system_y!=b->system_y) || (a->system_z!=b->system_z) ) return 0;
  if ( (a->axis_x!=b->axis_x) || (a->axis_y!=b->axis_y) || (a->axis_z!=b->axis_z) ) return 0;
  if (!ppl_withWordsCmp(&a->style , &b->style)) return 0;
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
    ppl_withWordsCpy(&(*out)->style, &(*in)->style);
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
    ppl_withWordsDestroy(&obj->style);
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
  sprintf(out+i, " at %s", *(char **)FetchSettingName(in->system_x, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_x==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_x); i+=strlen(out+i); }
  sprintf(out+i, " %s,", ppl_units_NumericDisplay(&(in->x),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " %s", *(char **)FetchSettingName(in->system_y, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_y==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_y); i+=strlen(out+i); }
  sprintf(out+i, " %s,", ppl_units_NumericDisplay(&(in->y),0,0,0)); i+=strlen(out+i);
  sprintf(out+i, " %s", *(char **)FetchSettingName(in->system_z, SW_SYSTEM_INT, (void *)SW_SYSTEM_STR, sizeof(char *))); i+=strlen(out+i);
  if (in->system_z==SW_SYSTEM_AXISN) { sprintf(out+i, " %d",in->axis_z); i+=strlen(out+i); }
  sprintf(out+i, " %s", ppl_units_NumericDisplay(&(in->z),0,0,0)); i+=strlen(out+i);
  if (in->rotation!=0.0) { sprintf(out+i, " rotate %s",
             NumericDisplay( in->rotation *180/M_PI , 0, pplset_term_current.SignificantFigures, (pplset_term_current.NumDisplay==SW_DISPLAY_L))
           ); i+=strlen(out+i); }
  if (in->HAlign>0) { sprintf(out+i, " halign %s", *(char **)FetchSettingName(in->HAlign, SW_HALIGN_INT, (void *)SW_HALIGN_STR, sizeof(char *))); i+=strlen(out+i); }
  if (in->VAlign>0) { sprintf(out+i, " valign %s", *(char **)FetchSettingName(in->VAlign, SW_VALIGN_INT, (void *)SW_VALIGN_STR, sizeof(char *))); i+=strlen(out+i); }
  if (in->gap!=0.0) { sprintf(out+i, " gap %s",
             NumericDisplay( in->gap * 100          , 0, pplset_term_current.SignificantFigures, (pplset_term_current.NumDisplay==SW_DISPLAY_L))
           ); i+=strlen(out+i); }
  ppl_withWordsPrint(&in->style, out+i+6);
  if (strlen(out+i+6)>0) { sprintf(out+i, " with"); out[i+5]=' '; }
  else                   { out[i]='\0'; }
  return;
 }

