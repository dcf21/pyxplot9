// set.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

#include <gsl/gsl_math.h>

#include "commands/set.h"

#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/list.h"

#include "defaultObjs/moduleRandom.h"

#include "expressions/expCompile_fns.h"
#include "expressions/traceback_fns.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "settings/arrows_fns.h"
#include "settings/axes_fns.h"
#include "settings/colors.h"
#include "settings/epsColors.h"
#include "settings/labels_fns.h"
#include "settings/papersizes.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/textConstants.h"
#include "settings/withWords_fns.h"

#include "stringTools/asciidouble.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjPrint.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "canvasItems.h"
#include "children.h"
#include "pplConstants.h"

#define TBADD(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

#define GET_AXISLOG(OUT,AXIS,DEFAULT) \
 { \
  pplset_axis *ai = (AXIS); \
  int C=0, *out=&ai->log; \
  while ((ai->linked)&&(C<100)) \
   { \
    if (ai->LinkedAxisCanvasID > 0) { out=&(AXIS)->log; *out=0; } \
    if (!DEFAULT) \
     { \
      if      (ai->LinkedAxisToXYZ == 0) ai = xa + ai->LinkedAxisToNum; \
      else if (ai->LinkedAxisToXYZ == 1) ai = ya + ai->LinkedAxisToNum; \
      else                               ai = za + ai->LinkedAxisToNum; \
     } \
    else \
     { \
      if      (ai->LinkedAxisToXYZ == 0) ai = c->set->XAxesDefault + ai->LinkedAxisToNum; \
      else if (ai->LinkedAxisToXYZ == 1) ai = c->set->YAxesDefault + ai->LinkedAxisToNum; \
      else                               ai = c->set->ZAxesDefault + ai->LinkedAxisToNum; \
     } \
    C++; \
    if (!ai->enabled) { out=&(AXIS)->log; *out=0; break; } \
    out=&ai->log; \
   } \
  OUT=out; \
 }

static void tics_rm(pplset_tics *a)
 {
  a->tickMinSet = a->tickMaxSet = a->tickStepSet = 0;
  if (a->tickList!=NULL) { free(a->tickList); a->tickList=NULL; }
  if (a->tickStrs!=NULL) { int i; for (i=0; a->tickStrs[i]!=NULL; i++) { free(a->tickStrs[i]); } free(a->tickStrs); a->tickStrs=NULL; }
 }

static void tics_cp(pplset_tics *o, pplset_tics *i)
 {
  *o = *i;
  if (i->tickList!=NULL)
   {
    int j,l; for (l=0; i->tickStrs[l]!=NULL; l++);
    o->tickList = (double *)malloc((l+1)*sizeof(double));
    o->tickStrs = (char  **)malloc((l+1)*sizeof(char *));
    if ((o->tickList==NULL)||(o->tickStrs==NULL)) { o->tickList=NULL; o->tickStrs=NULL; return; }
    memcpy(o->tickList, i->tickList, (l+1)*sizeof(double));
    for (j=0; j<l; j++)
     {
      o->tickStrs[j] = (char *)malloc(strlen(i->tickStrs[j])+1);
      if (o->tickStrs[j]==NULL) { o->tickList=NULL; o->tickStrs=NULL; return; }
      strcpy(o->tickStrs[j], i->tickStrs[j]);
     }
   }
 }

#define SET_NOTICKS(T) \
 { \
  tics_rm(&(T)); \
  T.tickList=(double *)malloc(sizeof(double)); \
  T.tickStrs=(char  **)malloc(sizeof(char *)); \
  T.tickStrs[0]=NULL; \
 }

#define SET_TICKS(T,TO,U,US,amLog) \
 { \
  const int gotAuto  = ((ptab[PARSE_INDEX_autofreq  ]>0) && (command[ptab[PARSE_INDEX_autofreq  ]].objType==PPLOBJ_STR)); \
  const int gotRange = ((ptab[PARSE_INDEX_start     ]>0) && (command[ptab[PARSE_INDEX_start     ]].objType==PPLOBJ_NUM)); \
  const int gotList  = ((ptab[PARSE_INDEX_got_list  ]>0) && (command[ptab[PARSE_INDEX_got_list  ]].objType==PPLOBJ_STR)); \
  if ((ptab[PARSE_INDEX_dir]>0) && (command[ptab[PARSE_INDEX_dir]].objType==PPLOBJ_STR)) \
   { \
    (T).tickDir = ppl_fetchSettingByName(&c->errcontext, (char *)command[ptab[PARSE_INDEX_dir]].auxil, SW_TICDIR_INT, SW_TICDIR_STR); \
   } \
  if (gotAuto || gotRange || gotList) tics_rm(&(T)); \
  if (gotRange) \
   { \
    int     gotStart = ((ptab[PARSE_INDEX_start     ]>0) && (command[ptab[PARSE_INDEX_start     ]].objType==PPLOBJ_NUM)); \
    int     gotIncr  = ((ptab[PARSE_INDEX_increment ]>0) && (command[ptab[PARSE_INDEX_increment ]].objType==PPLOBJ_NUM)); \
    int     gotEnd   = ((ptab[PARSE_INDEX_end       ]>0) && (command[ptab[PARSE_INDEX_end       ]].objType==PPLOBJ_NUM)); \
    pplObj *objstart = &command[ptab[PARSE_INDEX_start     ]]; \
    pplObj *objincr  = &command[ptab[PARSE_INDEX_increment ]]; \
    pplObj *objend   = &command[ptab[PARSE_INDEX_end       ]]; \
    double     start = objstart->real; \
    double     incr  = objincr ->real; \
    double     end   = objend  ->real; \
    if (!gotIncr) { objincr=objstart; incr=start; gotIncr=1; gotStart=0; } /* if only one number given, it is increment */ \
 \
    if((gotStart && gotIncr && !amLog) && (!ppl_unitsDimEqual(objstart,objincr))) { sprintf(c->errcontext.tempErrStr, "Start value for axis ticks has conflicting units with step size. Units of the start value are <%s>; units of the step size are <%s>.", ppl_printUnit(c, objstart, NULL, NULL, 0, 1, 0), ppl_printUnit(c, objincr, NULL, NULL, 1, 1, 0)); ppl_error(&c->errcontext, ERR_UNIT, -1, -1, NULL); return; } \
    if((gotStart && gotEnd           ) && (!ppl_unitsDimEqual(objstart,objend ))) { sprintf(c->errcontext.tempErrStr, "Start value for axis ticks has conflicting units with end value. Units of the start value are <%s>; units of the end value are <%s>.", ppl_printUnit(c, objstart, NULL, NULL, 0, 1, 0), ppl_printUnit(c, objend, NULL, NULL, 1, 1, 0)); ppl_error(&c->errcontext, ERR_UNIT, -1, -1, NULL); return; } \
    if (amLog    && (!objincr->dimensionless)) { sprintf(c->errcontext.tempErrStr, "Invalid step size for axis ticks. On a log axis, step size should be a dimensionless multiplier; supplied step size has units of <%s>.", ppl_printUnit(c, objincr, NULL, NULL, 0, 1, 0)); ppl_error(&c->errcontext, ERR_UNIT, -1, -1, NULL); return; } \
    if (gotStart && (!gsl_finite(start))) { sprintf(c->errcontext.tempErrStr, "Invalid starting value for axis ticks. Value supplied is not finite."); ppl_error(&c->errcontext,ERR_GENERIC,-1,-1,NULL); return; } \
    if (gotIncr  && (!gsl_finite(incr ))) { sprintf(c->errcontext.tempErrStr, "Invalid step size for axis ticks. Value supplied is not finite."); ppl_error(&c->errcontext,ERR_GENERIC,-1,-1,NULL); return; } \
    if (gotEnd   && (!gsl_finite(end  ))) { sprintf(c->errcontext.tempErrStr, "Invalid end value for axis ticks. Value supplied is not finite."); ppl_error(&c->errcontext,ERR_GENERIC,-1,-1,NULL); return; } \
    if ((!amLog)||(gotStart)) \
     { \
      pplObj *o = amLog ? objstart : objincr; \
      if ((US)&&(!ppl_unitsDimEqual(o,&(U)))) { sprintf(c->errcontext.tempErrStr, "Tick positions supplied to the '%s' command have units of <%s>, which conflicts with the axis range which has units of <%s>.", cmd, ppl_printUnit(c, o, NULL, NULL, 0, 1, 0), ppl_printUnit(c, &(U), NULL, NULL, 1, 1, 0)); ppl_error(&c->errcontext, ERR_UNIT, -1, -1, NULL); return; } \
      if (!ppl_unitsDimEqual(o,&(U))) { tics_rm(&(TO)); U=*o; } \
     } \
 \
    if (((T).tickMinSet  = gotStart)) (T).tickMin  = start; \
    if (((T).tickStepSet = gotIncr )) (T).tickStep = incr; \
    if (((T).tickMaxSet  = gotEnd  )) (T).tickMax  = end; \
   } \
  else if (gotList) \
   { \
    int       pos = ptab[PARSE_INDEX_0tick_list]; \
    const int x   = ptab[PARSE_INDEX_x]; \
    const int lab = ptab[PARSE_INDEX_label]; \
    int       i=0; \
    pplObj   *first=NULL; \
 \
    while (command[pos].objType==PPLOBJ_NUM) \
     { \
      pplObj *ox, *ol; \
      pos = (int)round(command[pos].real); \
      if (pos<=0) break; \
      ox = &command[pos+x]; \
      ol = &command[pos+lab]; \
      if((ox->objType == PPLOBJ_STR) && ((ol->objType==PPLOBJ_NUM)||(ol->objType==PPLOBJ_BOOL)||(ol->objType==PPLOBJ_DATE)) ) { pplObj *tmp=ol; ol=ox; ox=tmp; } \
      if (ox->objType != PPLOBJ_NUM) { sprintf(c->errcontext.tempErrStr, "Ticks can only be set at numeric values; supplied value is of type <%s>.", pplObjTypeNames[ox->objType]); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; } \
      if ((ol->objType != PPLOBJ_ZOM) && (ol->objType != PPLOBJ_STR)) \
         { sprintf(c->errcontext.tempErrStr, "Ticks labels must be strings; supplied value is of type <%s>.", pplObjTypeNames[ol->objType]); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; } \
      if (!gsl_finite(ox->real)) { sprintf(c->errcontext.tempErrStr, "Ticks can only be set at finite numeric values; supplied value is not finite."); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; } \
      if (ox->flagComplex) { sprintf(c->errcontext.tempErrStr, "Ticks can only be set at real numeric values; supplied value is complex."); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; } \
      if (i==0) \
       { \
        first=ox; \
        if ((US)&&(!ppl_unitsDimEqual(first,&(U)))) { sprintf(c->errcontext.tempErrStr, "Tick positions supplied to the '%s' command have units of <%s>, which conflicts with the axis range which has units of <%s>.", cmd, ppl_printUnit(c, first, NULL, NULL, 0, 1, 0), ppl_printUnit(c, &(U), NULL, NULL, 1, 1, 0)); ppl_error(&c->errcontext, ERR_UNIT, -1, -1, NULL); return; } \
        if (!ppl_unitsDimEqual(first,&(U))) { tics_rm(&(TO)); U=*first; } \
       } \
      else if (!ppl_unitsDimEqual(first,ox)) { sprintf(c->errcontext.tempErrStr, "Tick positions supplied to the '%s' command must all have the same physical units; supplied list has multiple units, including <%s> and <%s>.", cmd, ppl_printUnit(c, first, NULL, NULL, 0, 1, 0), ppl_printUnit(c, ox, NULL, NULL, 1, 1, 0)); ppl_error(&c->errcontext, ERR_UNIT, -1, -1, NULL); return; } \
      i++; \
     } \
    (T).tickList = (double *)malloc((i+1)*sizeof(double)); \
    (T).tickStrs = (char  **)malloc((i+1)*sizeof(char *)); \
    pos = ptab[PARSE_INDEX_0tick_list]; \
    i=0; \
    while (command[pos].objType==PPLOBJ_NUM) \
     { \
      pplObj *ox, *ol; \
      char   *label; \
      pos = (int)round(command[pos].real); \
      if (pos<=0) break; \
      ox = &command[pos+x]; \
      ol = &command[pos+lab]; \
      if (ox->objType == PPLOBJ_STR) { pplObj *tmp=ol; ol=ox; ox=tmp; } \
      (T).tickList[i] = ox->real; \
      label = (ol->objType==PPLOBJ_STR) ? ((char *)ol->auxil) : ("\xFF"); \
      (T).tickStrs[i] = (char *)malloc(strlen(label)+1); \
      if ((T).tickStrs[i] == NULL) { sprintf(c->errcontext.tempErrStr, "Out of memory."); ppl_error(&c->errcontext, ERR_MEMORY, -1, -1, NULL); return; } \
      strcpy((T).tickStrs[i], label); \
      i++; \
     } \
    (T).tickStrs[i] = NULL; \
   } \
 }

void ppl_directive_set(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplset_graph     *sg;
  pplarrow_object **al;
  ppllabel_object **ll;
  pplset_axis      *xa, *ya, *za;
  pplObj           *command   = in->stk;
  int               i, strcmp_set, strcmp_unset;
  int               gotEditNo = (command[PARSE_arc_editno].objType == PPLOBJ_NUM);
  char             *directive, *setoption;

  if (!gotEditNo)
   {
    sg = &c->set->graph_current;
    al = &c->set->pplarrow_list;
    ll = &c->set->ppllabel_list;
    xa = c->set->XAxes;
    ya = c->set->YAxes;
    za = c->set->ZAxes;
   }
  else
   {
    canvas_itemlist *canvas_items = (canvas_itemlist *)c->canvas_items;
    int              editNo = (int)round(command[PARSE_arc_editno].real);
    canvas_item     *ptr;

    if ((editNo < 1) || (editNo>MULTIPLOT_MAXINDEX) || (canvas_items == NULL)) {sprintf(c->errcontext.tempErrStr, "No multiplot item with index %d.", editNo); ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, NULL); return;}
    ptr = canvas_items->first;
    while ((ptr!=NULL)&&(ptr->id!=editNo)) ptr=ptr->next;
    if (ptr == NULL) { sprintf(c->errcontext.tempErrStr, "No multiplot item with index %d.", editNo); ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, NULL); return; }

    sg = &(ptr->settings);
    al = &(ptr->arrow_list);
    ll = &(ptr->label_list);
    xa = ptr->XAxes; ya = ptr->YAxes; za = ptr->ZAxes;
    if ((xa==NULL)||(ya==NULL)||(za==NULL)) { al=NULL; ll=NULL; } // Objects which do not store axes also do not store any text labels or arrows
   }

  directive    = (char *)command[PARSE_arc_directive].auxil;
  setoption    = (char *)command[PARSE_arc_set_option].auxil;
  strcmp_set   = (strcmp(directive,"set")==0);
  strcmp_unset = (strcmp(directive,"unset")==0);

  if      (strcmp_set && (strcmp(setoption,"arrow")==0)) /* set arrow */
   {
    if (al!=NULL) pplarrow_add(c, al, in, pl, PARSE_TABLE_set_arrow_);
   }
  else if (strcmp_unset && (strcmp(setoption,"arrow")==0)) /* unset arrow */
   {
    if (al!=NULL) pplarrow_unset(c, al, in, pl, PARSE_TABLE_unset_arrow_);
   }
  else if ((strcmp(setoption,"autoscale")==0)) /* set autoscale | unset autoscale */
   {

#define SET_AUTOSCALE_AXIS \
 { \
  pplset_axis *a, *ad; \
  if      (j==1) { a = &ya[i]; ad = &c->set->YAxesDefault[i]; } \
  else if (j==2) { a = &za[i]; ad = &c->set->ZAxesDefault[i]; } \
  else           { a = &xa[i]; ad = &c->set->XAxesDefault[i]; } \
  if (set && ((!SetAll) || (a->enabled))) \
   { \
    a->enabled = 1; \
    a->MinSet  = SW_BOOL_FALSE; \
    a->MaxSet  = SW_BOOL_FALSE; \
   } else { \
    a->MinSet  = ad->MinSet; \
    a->MaxSet  = ad->MaxSet; \
   } \
  a->min       = ad->min; \
  a->max       = ad->max; \
 }

    if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
     {
      unsigned char set = strcmp_set;
      int pos = PARSE_set_autoscale_0axes;
      unsigned char SetAll = (command[pos].objType!=PPLOBJ_NUM);
      if (!SetAll)
       while (command[pos].objType==PPLOBJ_NUM)
        {
         int i,j;
         pos = (int)round(command[pos].real);
         if (pos<=0) break;
         i = (int)round(command[pos+PARSE_set_autoscale_axis_0axes].real);
         j = (int)round(command[pos+PARSE_set_autoscale_axis_0axes].exponent[0]);
         SET_AUTOSCALE_AXIS;
        }
      else
       {
        int i,j;
        for (j=0; j<2; j++)
         for (i=0; i<MAX_AXES; i++)
          SET_AUTOSCALE_AXIS;
       }
     }
   }
  else if (strcmp_set && (strcmp(setoption,"axis")==0)) /* set axis */
   {
    int pos = PARSE_unset_axis_axes;
    int i, anum;
    while (command[pos].objType==PPLOBJ_NUM)
     {
      pplset_axis *a, *ad;
      pos = (int)round(command[pos].real);
      if (pos<=0) break;
      anum = (int)round(command[pos+PARSE_set_axis_axis_axes].real);
      i    = (int)round(command[pos+PARSE_set_axis_axis_axes].exponent[0]);
      if      (i==1) { a=&ya[anum]; ad = &c->set->YAxesDefault[anum]; }
      else if (i==2) { a=&za[anum]; ad = &c->set->ZAxesDefault[anum]; }
      else           { a=&xa[anum]; ad = &c->set->XAxesDefault[anum]; }
      a->enabled=1;
      if (command[PARSE_set_axis_invisible].objType==PPLOBJ_STR) a->invisible=1;
      if (command[PARSE_set_axis_visible  ].objType==PPLOBJ_STR) a->invisible=0;
      if (command[PARSE_set_axis_atzero   ].objType==PPLOBJ_STR) a->atzero   =1;
      if (command[PARSE_set_axis_notatzero].objType==PPLOBJ_STR) a->atzero   =0;
      if (command[PARSE_set_axis_linked   ].objType==PPLOBJ_STR) a->linked   =1;
      if (command[PARSE_set_axis_notlinked].objType==PPLOBJ_STR) a->linked   =0;
      if (command[PARSE_set_axis_xorient  ].objType==PPLOBJ_STR)
       {
        if (i!=0) ppl_warning(&c->errcontext, ERR_SYNTAX, "Can only specify the positions 'top' or 'bottom' for x axes.");
        else      a->topbottom=(strcmp((char *)command[PARSE_set_axis_xorient].auxil,"on")==0);
       }
      if (command[PARSE_set_axis_yorient  ].objType==PPLOBJ_STR)
       {
        if (i!=1) ppl_warning(&c->errcontext, ERR_SYNTAX, "Can only specify the positions 'left' and 'right' for y axes.");
        else      a->topbottom=(strcmp((char *)command[PARSE_set_axis_yorient].auxil,"on")==0);
       }
      if (command[PARSE_set_axis_zorient  ].objType==PPLOBJ_STR)
       {
        if (i!=2) ppl_warning(&c->errcontext, ERR_SYNTAX, "Can only specify the positions 'front' and 'back' for z axes.");
        else      a->topbottom=(strcmp((char *)command[PARSE_set_axis_zorient].auxil,"on")==0);
       }
      if (command[PARSE_set_axis_mirror   ].objType==PPLOBJ_STR) a->MirrorType = ppl_fetchSettingByName(&c->errcontext, (char *)command[PARSE_set_axis_mirror  ].auxil, SW_AXISMIRROR_INT, SW_AXISMIRROR_STR);
      if (command[PARSE_set_axis_axisdisp ].objType==PPLOBJ_STR) a->ArrowType  = ppl_fetchSettingByName(&c->errcontext, (char *)command[PARSE_set_axis_axisdisp].auxil, SW_AXISDISP_INT, SW_AXISDISP_STR);
      if (command[PARSE_set_axis_linkaxis ].objType==PPLOBJ_NUM)
       {
        int j = (int)round(command[PARSE_set_axis_linkaxis].real);
        if (a->linkusing!=NULL) { pplExpr_free((pplExpr *)a->linkusing); a->linkusing=ad->linkusing; if (a->linkusing!=NULL) ((pplExpr *)a->linkusing)->refCount++; }
        a->LinkedAxisCanvasID = -1;
        a->LinkedAxisToNum    = j;
        a->LinkedAxisToXYZ    = (int)round(command[PARSE_set_axis_linkaxis].exponent[0]);
       }
      if (command[PARSE_set_axis_linktoid].objType==PPLOBJ_NUM) { a->LinkedAxisCanvasID = (int)round(command[PARSE_set_axis_linktoid].real); }
      if (command[PARSE_set_axis_usingexp].objType==PPLOBJ_EXP)
       {
        if (a->linkusing!=NULL) pplExpr_free((pplExpr *)a->linkusing);
        a->linkusing = (void *)pplExpr_cpy((pplExpr *)command[PARSE_set_axis_usingexp].auxil);
       }
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"axis")==0)) /* unset axis */
   {
    int pos = PARSE_unset_axis_0axes;
    if (command[pos].objType!=PPLOBJ_NUM)
     {
      int i;
      for (i=0; i<MAX_AXES; i++) { pplaxis_destroy(c, &(xa[i]) ); pplaxis_copy(c, &(xa[i]), &(c->set->XAxesDefault[i])); }
      for (i=0; i<MAX_AXES; i++) { pplaxis_destroy(c, &(ya[i]) ); pplaxis_copy(c, &(ya[i]), &(c->set->YAxesDefault[i])); }
      for (i=0; i<MAX_AXES; i++) { pplaxis_destroy(c, &(za[i]) ); pplaxis_copy(c, &(za[i]), &(c->set->ZAxesDefault[i])); }
     }
    else
     {
      int i, anum;
      while (command[pos].objType==PPLOBJ_NUM)
       {
        pos = (int)round(command[pos].real);
        if (pos<=0) break;
        anum = (int)round(command[pos+PARSE_unset_axis_axis_axes].real);
        i    = (int)round(command[pos+PARSE_unset_axis_axis_axes].exponent[0]);
        if      (i==1) { pplaxis_destroy(c, &(ya[anum]) ); pplaxis_copy(c, &(ya[anum]), &(c->set->YAxesDefault[anum])); }
        else if (i==2) { pplaxis_destroy(c, &(za[anum]) ); pplaxis_copy(c, &(za[anum]), &(c->set->ZAxesDefault[anum])); }
        else           { pplaxis_destroy(c, &(xa[anum]) ); pplaxis_copy(c, &(xa[anum]), &(c->set->XAxesDefault[anum])); }
       }
     }
   }
  else if (strcmp_set && (strcmp(setoption,"axisunitstyle")==0)) /* set axisunitstyle */
   {
    sg->AxisUnitStyle = ppl_fetchSettingByName(&c->errcontext, (char *)command[PARSE_set_axisunitstyle_unitstyle].auxil, SW_AXISUNITSTY_INT, SW_AXISUNITSTY_STR);
   }
  else if (strcmp_unset && (strcmp(setoption,"axisunitstyle")==0)) /* unset axisunitstyle */
   {
    sg->AxisUnitStyle = c->set->graph_default.AxisUnitStyle;
   }
  else if (strcmp_set && (strcmp(setoption,"backup")==0)) /* set backup */
   {
    c->set->term_current.backup = SW_ONOFF_ON;
   }
  else if (strcmp_unset && (strcmp(setoption,"backup")==0)) /* unset backup */
   {
    c->set->term_current.backup = c->set->term_default.backup;
   }
  else if (strcmp_set && (strcmp(setoption,"bar")==0)) /* set bar */
   {
    double    barsize =  command[PARSE_set_bar_bar_size      ].real;
    int    gotbarsize = (command[PARSE_set_bar_bar_size      ].objType == PPLOBJ_NUM);
    int    gotlarge   = (command[PARSE_set_bar_bar_size_large].objType == PPLOBJ_STR);
    int    gotsmall   = (command[PARSE_set_bar_bar_size_small].objType == PPLOBJ_STR);
    if (gotbarsize && !gsl_finite(barsize)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set bar' command was not finite."); return; }
    if      (gotsmall  ) sg->bar = 0.0;
    else if (gotlarge  ) sg->bar = 1.0;
    else if (gotbarsize) sg->bar = barsize;
    else                 sg->bar = 1.0;
   }
  else if (strcmp_unset && (strcmp(setoption,"bar")==0)) /* unset bar */
   {
    sg->bar = c->set->graph_default.bar;
   }
  else if (strcmp_set && (strcmp(setoption,"binorigin")==0)) /* set binorigin */
   {
    if (command[PARSE_set_binwidth_auto].objType==PPLOBJ_STR)
     {
      c->set->term_current.BinOriginAuto = 1;
     }
    else
     {
      double tempdbl = command[PARSE_set_binorigin_bin_origin].real;
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set binorigin' command was not finite."); return; }
      c->set->term_current.BinOriginAuto = 0;
      c->set->term_current.BinOrigin = command[PARSE_set_binorigin_bin_origin];
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"binorigin")==0)) /* unset binorigin */
   {
    c->set->term_current.BinOrigin     = c->set->term_default.BinOrigin;
    c->set->term_current.BinOriginAuto = c->set->term_default.BinOriginAuto;
   }
  else if (strcmp_set && (strcmp(setoption,"binwidth")==0)) /* set binwidth */
   {
    if (command[PARSE_set_binwidth_auto].objType==PPLOBJ_STR)
     {
      c->set->term_current.BinWidthAuto = 1;
     }
    else
     {
      double tempdbl = command[PARSE_set_binwidth_bin_width].real;
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set binwidth' command was not finite."); return; }
      if (tempdbl<=0) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "Width of histogram bins must be greater than zero."); return; }
      c->set->term_current.BinWidthAuto = 0;
      c->set->term_current.BinWidth = command[PARSE_set_binwidth_bin_width];
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"binwidth")==0)) /* unset binwidth */
   {
    c->set->term_current.BinWidth     = c->set->term_default.BinWidth;
    c->set->term_current.BinWidthAuto = c->set->term_default.BinWidthAuto;
   }
  else if (strcmp_set && (strcmp(setoption,"boxfrom")==0)) /* set boxfrom */
   {
    if (command[PARSE_set_boxfrom_auto].objType==PPLOBJ_STR)
     {
      sg->BoxFromAuto = 1;
     }
    else
     {
      double tempdbl = command[PARSE_set_boxfrom_box_from].real;
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set boxfrom' command was not finite."); return; }
      sg->BoxFromAuto = 0;
      sg->BoxFrom = command[PARSE_set_boxfrom_box_from];
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"boxfrom")==0)) /* unset boxfrom */
   {
    sg->BoxFrom     = c->set->graph_default.BoxFrom;
    sg->BoxFromAuto = c->set->graph_default.BoxFromAuto;
   }
  else if (strcmp_set && (strcmp(setoption,"boxwidth")==0)) /* set boxwidth */
   {
    if (command[PARSE_set_boxwidth_auto].objType==PPLOBJ_STR)
     {
      sg->BoxWidthAuto = 1;
     }
    else
     {
      double tempdbl = command[PARSE_set_boxwidth_box_width].real;
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set boxwidth' command was not finite."); return; }
      sg->BoxWidthAuto = 0;
      sg->BoxWidth = command[PARSE_set_boxwidth_box_width];
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"boxwidth")==0)) /* unset boxwidth */
   {
    sg->BoxWidth     = c->set->graph_default.BoxWidth;
    sg->BoxWidthAuto = c->set->graph_default.BoxWidthAuto;
   }
  else if (strcmp_set && (strcmp(setoption,"c1format")==0)) /* set c1format */
   {
    if (command[PARSE_set_c1format_format_string].objType == PPLOBJ_EXP)
     {
      if (sg->c1format != NULL) pplExpr_free((pplExpr *)sg->c1format);
      sg->c1format = (void *)pplExpr_cpy((pplExpr *)command[PARSE_set_c1format_format_string].auxil);
      sg->c1formatset = 1;
     }
    if (command[PARSE_set_c1format_orient].objType == PPLOBJ_STR)
     {
      sg->c1TickLabelRotation = ppl_fetchSettingByName(&c->errcontext, (char *)command[PARSE_set_c1format_orient].auxil, SW_TICLABDIR_INT, SW_TICLABDIR_STR);
      sg->c1TickLabelRotate   = c->set->graph_default.c1TickLabelRotate;
     }
    if (command[PARSE_set_c1format_rotation].objType == PPLOBJ_NUM)
     {
      double r = command[PARSE_set_c1format_rotation].real;
      if (!gsl_finite(r)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The rotation angle supplied to the 'set c1format' command was not finite."); return; }
      sg->c1TickLabelRotate = r; // TickLabelRotation will already have been set by "rotate" keyword mapping to "orient"
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"c1format")==0)) /* unset c1format */
   {
    if (sg->c1format != NULL) pplExpr_free((pplExpr *)sg->c1format);
    sg->c1format            = (void *)pplExpr_cpy((pplExpr *)c->set->graph_default.c1format);
    sg->c1formatset         = c->set->graph_default.c1formatset;
    sg->c1TickLabelRotate   = c->set->graph_default.c1TickLabelRotate;
    sg->c1TickLabelRotation = c->set->graph_default.c1TickLabelRotation;
   }
  else if (strcmp_set && (strcmp(setoption,"c1label")==0)) /* set c1label */
   {
    if (command[PARSE_set_c1label_label_text].objType == PPLOBJ_STR)
     {
      snprintf(sg->c1label, FNAME_LENGTH, "%s", (char *)command[PARSE_set_c1label_label_text].auxil);
      sg->c1label[FNAME_LENGTH-1]='\0';
     }
    if (command[PARSE_set_c1label_rotation].objType == PPLOBJ_NUM)
     {
      double r = command[PARSE_set_c1label_rotation].real;
      if (!gsl_finite(r)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The rotation angle supplied to the 'set c1label' command was not finite."); return; }
      sg->c1LabelRotate = r;
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"c1label")==0)) /* unset c1label */
   {
    strcpy(sg->c1label, c->set->graph_default.c1label);
    sg->c1LabelRotate = c->set->graph_default.c1LabelRotate;
   }
  else if (strcmp_set && (strcmp(setoption,"c1tics")==0)) /* set c1tics */
   {
    int        m    = (command[PARSE_set_c1tics_minor].objType==PPLOBJ_STR);
    const int *ptab = PARSE_TABLE_set_c1tics_;
    char      *cmd  = m ? "mc1tics" : "c1tics";
    if (!m) { SET_TICKS( sg->ticsC  , sg->ticsCM , sg->unitC , ((sg->Cminauto[0]!=SW_BOOL_TRUE)||(sg->Cmaxauto[0]!=SW_BOOL_TRUE)) , (sg->Clog[0]==SW_BOOL_TRUE) ); }
    else    { SET_TICKS( sg->ticsCM , sg->ticsC  , sg->unitC , ((sg->Cminauto[0]!=SW_BOOL_TRUE)||(sg->Cmaxauto[0]!=SW_BOOL_TRUE)) , (sg->Clog[0]==SW_BOOL_TRUE) ); }
   }
  else if (strcmp_unset && (strcmp(setoption,"c1tics")==0)) /* unset c1tics */
   {
    int m = (command[PARSE_unset_c1tics_minor].objType==PPLOBJ_STR);
    if (!m) { tics_rm(&sg->ticsC ); if (ppl_unitsDimEqual(&sg->unitC , &c->set->graph_default.unitC)) tics_cp(&sg->ticsC ,&c->set->graph_default.ticsC ); }
              tics_rm(&sg->ticsCM); if (ppl_unitsDimEqual(&sg->unitC , &c->set->graph_default.unitC)) tics_cp(&sg->ticsCM,&c->set->graph_default.ticsCM);
   }
  else if (strcmp_set && (strcmp(setoption,"calendar")==0)) /* set calendar */
   {
    char *tempstr = (char *)command[PARSE_set_calendar_calendar].auxil;
    int   got     =        (command[PARSE_set_calendar_calendar].objType == PPLOBJ_STR);
    if (got) c->set->term_current.CalendarIn  =
             c->set->term_current.CalendarOut = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_CALENDAR_INT, SW_CALENDAR_STR);

    tempstr = (char *)command[PARSE_set_calendar_calendarin].auxil;
    got     =        (command[PARSE_set_calendar_calendarin].objType == PPLOBJ_STR);
    if (got) c->set->term_current.CalendarIn  = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_CALENDAR_INT, SW_CALENDAR_STR);

    tempstr = (char *)command[PARSE_set_calendar_calendarout].auxil;
    got     =        (command[PARSE_set_calendar_calendarout].objType == PPLOBJ_STR);
    if (got) c->set->term_current.CalendarOut = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_CALENDAR_INT, SW_CALENDAR_STR);
   }
  else if (strcmp_unset && (strcmp(setoption,"calendar")==0)) /* unset calendar */
   {
    c->set->term_current.CalendarIn  = c->set->term_default.CalendarIn;
    c->set->term_current.CalendarOut = c->set->term_default.CalendarOut;
   }
  else if (strcmp_set && (strcmp(setoption,"clip")==0)) /* set clip */
   {
    sg->clip = SW_ONOFF_ON;
   }
  else if (strcmp_unset && (strcmp(setoption,"clip")==0)) /* unset clip */
   {
    sg->clip = c->set->graph_default.clip;
   }
  else if (strcmp_set && (strcmp(setoption,"colkey")==0)) /* set colkey */
   {
    char *tempstr = (char *)command[PARSE_set_colkey_pos].auxil;
    int   got     =        (command[PARSE_set_colkey_pos].objType == PPLOBJ_STR);

    sg->ColKey = SW_ONOFF_ON;
    if (got) sg->ColKeyPos = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_COLKEYPOS_INT, SW_COLKEYPOS_STR);
   }
  else if (strcmp_unset && (strcmp(setoption,"colkey")==0)) /* unset colkey */
   {
    sg->ColKey    = c->set->graph_default.ColKey;
    sg->ColKeyPos = c->set->graph_default.ColKeyPos;
   }
  else if (strcmp_set && (strcmp(setoption,"colmap")==0)) /* set colmap */
   {
    if (command[PARSE_set_colmap_color].objType == PPLOBJ_EXP)
     {
      if (sg->ColMapExpr != NULL) pplExpr_free((pplExpr *)sg->ColMapExpr);
      sg->ColMapExpr = (void *)pplExpr_cpy((pplExpr *)command[PARSE_set_colmap_color].auxil);
     }
    if (command[PARSE_set_colmap_mask].objType == PPLOBJ_EXP)
     {
      if (sg->MaskExpr != NULL) pplExpr_free((pplExpr *)sg->MaskExpr);
      sg->MaskExpr = (void *)pplExpr_cpy((pplExpr *)command[PARSE_set_colmap_mask].auxil);
     }
    if (command[PARSE_set_colmap_nomask].objType == PPLOBJ_STR)
     {
      if (sg->MaskExpr != NULL) pplExpr_free((pplExpr *)sg->MaskExpr);
      sg->MaskExpr = NULL;
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"colmap")==0)) /* unset colmap */
   {
    if (sg->ColMapExpr != NULL) pplExpr_free((pplExpr *)sg->ColMapExpr);
    sg->ColMapExpr = (void *)pplExpr_cpy((pplExpr *)c->set->graph_default.ColMapExpr);
    if (sg->MaskExpr != NULL) pplExpr_free((pplExpr *)sg->MaskExpr);
    sg->MaskExpr = (void *)pplExpr_cpy((pplExpr *)c->set->graph_default.MaskExpr);
   }
  else if (strcmp_set && (strcmp(setoption,"contours")==0)) /* set contours */
   {
    if (command[PARSE_set_contours_label   ].objType == PPLOBJ_STR) sg->ContoursLabel = SW_ONOFF_ON;
    if (command[PARSE_set_contours_nolabel ].objType == PPLOBJ_STR) sg->ContoursLabel = SW_ONOFF_OFF;
    if (command[PARSE_set_contours_contours].objType == PPLOBJ_NUM)
     {
      double n = command[PARSE_set_contours_contours].real;
      if (n<2) { sprintf(c->errcontext.tempErrStr, "Contour plots must have at least two contours."); ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, NULL); return; }
      if (n>MAX_CONTOURS) { sprintf(c->errcontext.tempErrStr, "Contour maps cannot be constucted with more than %d contours.", MAX_CONTOURS); ppl_error(&c->errcontext, ERR_GENERIC,-1,-1,NULL); return; }
      sg->ContoursN       = (int)round(n);
      sg->ContoursListLen = -1;
     }
    else if (command[PARSE_set_contours_contour_list].objType == PPLOBJ_NUM)
     {
      int pos = PARSE_set_contours_contour_list, i=0;
      pplObj *first=NULL;
      while (command[pos].objType==PPLOBJ_NUM)
       {
        pplObj *o;
        pos = (int)round(command[pos].real);
        if (pos<=0) break;
        o = &command[pos+PARSE_set_contours_contour_contour_list];
        if (o->objType != PPLOBJ_NUM) { sprintf(c->errcontext.tempErrStr, "Contours can only be set at numeric values; supplied value is of type <%s>.", pplObjTypeNames[o->objType]); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; }
        if (!gsl_finite(o->real)) { sprintf(c->errcontext.tempErrStr, "Contours can only be set at finite numeric values; supplied value is not finite."); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; }
        if (o->flagComplex) { sprintf(c->errcontext.tempErrStr, "Contours can only be set at real numeric values; supplied value is complex."); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; }
        if (i==0) first=o;
        else if (!ppl_unitsDimEqual(first,o)) { sprintf(c->errcontext.tempErrStr, "Contour positions must all have the same physical units; supplied list has multiple units, including <%s> and <%s>.", ppl_printUnit(c, first, NULL, NULL, 0, 1, 0), ppl_printUnit(c, o, NULL, NULL, 1, 1, 0)); ppl_error(&c->errcontext, ERR_UNIT, -1, -1, NULL); return; }
        i++;
       }
      if (first==NULL) { ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, "Empty list of contours."); return; }
      sg->ContoursUnit = *first;
      sg->ContoursListLen = i;
      pos = PARSE_set_contours_contour_list;
      i=0;
      while (command[pos].objType==PPLOBJ_NUM)
       {
        pplObj *o;
        pos = (int)round(command[pos].real);
        if (pos<=0) break;
        o = &command[pos+PARSE_set_contours_contour_contour_list];
        sg->ContoursList[i] = o->real;
        i++;
       }
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"contours")==0)) /* unset contours */
   {
    sg->ContoursN       = c->set->graph_default.ContoursN;
    memcpy((void*)sg->ContoursList, (void*)c->set->graph_default.ContoursList, MAX_CONTOURS*sizeof(double));
    sg->ContoursListLen = c->set->graph_default.ContoursListLen;
    sg->ContoursUnit    = c->set->graph_default.ContoursUnit;
    sg->ContoursLabel   = c->set->graph_default.ContoursLabel;
   }
  else if (strcmp_set && (strcmp(setoption,"crange")==0)) /* set crange */
   {
    char   *cstr = (char *)command[PARSE_set_crange_c_number].auxil;
    int     C    = (int)(cstr[0]-'1');
    pplObj *min  = &command[PARSE_set_crange_min];
    pplObj *max  = &command[PARSE_set_crange_max];
    int     mina = (command[PARSE_set_crange_minauto].objType == PPLOBJ_STR);
    int     maxa = (command[PARSE_set_crange_maxauto].objType == PPLOBJ_STR);
    if (command[PARSE_set_crange_reverse      ].objType == PPLOBJ_STR) sg->Creverse[C] = SW_BOOL_TRUE;
    if (command[PARSE_set_crange_noreverse    ].objType == PPLOBJ_STR) sg->Creverse[C] = SW_BOOL_FALSE;
    if (command[PARSE_set_crange_renormalise  ].objType == PPLOBJ_STR) sg->Crenorm [C] = SW_BOOL_TRUE;
    if (command[PARSE_set_crange_norenormalise].objType == PPLOBJ_STR) sg->Crenorm [C] = SW_BOOL_FALSE;
    if ((!mina)&&(min->objType!=PPLOBJ_NUM)&&(sg->Cminauto[C]==SW_BOOL_FALSE)) min = &sg->Cmin[C];
    if ((!maxa)&&(max->objType!=PPLOBJ_NUM)&&(sg->Cmaxauto[C]==SW_BOOL_FALSE)) max = &sg->Cmax[C];
    if ((min->objType==PPLOBJ_NUM)&&(!gsl_finite(min->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set crange' command had non-finite limits."); return; }
    if ((max->objType==PPLOBJ_NUM)&&(!gsl_finite(max->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set crange' command had non-finite limits."); return; }
    if ((min->objType==PPLOBJ_NUM)&&(max->objType==PPLOBJ_NUM)&&(!ppl_unitsDimEqual(min,max))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "Attempt to set crange with dimensionally incompatible minimum and maximum."); return; }
    if (min->objType==PPLOBJ_NUM) { sg->Cmin[C] = *min; sg->Cminauto[C] = SW_BOOL_FALSE; }
    if (max->objType==PPLOBJ_NUM) { sg->Cmax[C] = *max; sg->Cmaxauto[C] = SW_BOOL_FALSE; }
    if (mina) sg->Cminauto[C] = SW_BOOL_TRUE;
    if (maxa) sg->Cmaxauto[C] = SW_BOOL_TRUE;
    if (C==0)
     {
      pplObj *newunit = (sg->Cminauto[0]!=SW_BOOL_TRUE) ? &sg->Cmin[0] : &sg->Cmax[0];
      if ((sg->Cminauto[0]!=SW_BOOL_TRUE)||(sg->Cmaxauto[0]!=SW_BOOL_TRUE))
       {
        if (!ppl_unitsDimEqual(&sg->unitC,newunit)) { sg->unitC = *newunit; }
        tics_rm(&sg->ticsC); tics_rm(&sg->ticsCM);
       }
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"crange")==0)) /* unset crange */
   {
    char *cstr = (char *)command[PARSE_set_crange_c_number].auxil;
    int   C    = (int)(cstr[0]-'1');
    sg->Cmin[C]     = c->set->graph_default.Cmin[C];
    sg->Cminauto[C] = c->set->graph_default.Cminauto[C];
    sg->Cmax[C]     = c->set->graph_default.Cmax[C];
    sg->Cmaxauto[C] = c->set->graph_default.Cmaxauto[C];
    sg->Crenorm[C]  = c->set->graph_default.Crenorm[C];
    sg->Creverse[C] = c->set->graph_default.Creverse[C];
    if (C==0)
     {
      pplObj *newunit = (sg->Cminauto[0]!=SW_BOOL_TRUE) ? &sg->Cmin[0] : &sg->Cmax[0];
      if ((sg->Cminauto[0]!=SW_BOOL_TRUE)||(sg->Cmaxauto[0]!=SW_BOOL_TRUE))
       {
        if (!ppl_unitsDimEqual(&sg->unitC,newunit)) { sg->unitC = *newunit; }
        tics_rm(&sg->ticsC); tics_rm(&sg->ticsCM);
       }
     }
   }
  else if (strcmp_set && (strcmp(setoption,"display")==0)) /* set display */
   {
    c->set->term_current.display = SW_ONOFF_ON;
   }
  else if (strcmp_unset && (strcmp(setoption,"display")==0)) /* unset display */
   {
    c->set->term_current.display = c->set->term_default.display;
   }
  else if (strcmp_set && (strcmp(setoption,"filter")==0)) /* set filter */
   {
    char *tempstr  = (char *)command[PARSE_set_filter_filename].auxil;
    char *tempstr2 = (char *)command[PARSE_set_filter_filter  ].auxil;
    char *tempstr3 = (char *)malloc(strlen(tempstr2)+1);
    pplObj val;
    if (tempstr3==NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1, "Out of memory."); return; }
    strcpy(tempstr3, tempstr2);
    val.refCount=1;
    pplObjStr(&val,0,1,tempstr3); // declare object not to be malloced, since ppl_dictRemoveKey will free it
    ppl_dictAppendCpy(c->set->filters,tempstr,&val,sizeof(pplObj));
   }
  else if (strcmp_unset && (strcmp(setoption,"filter")==0)) /* unset filter */
   {
    char *tempstr   = (char *)command[PARSE_set_filter_filename].auxil;
    pplObj *tempobj = (pplObj *)ppl_dictLookup(c->set->filters,tempstr);
    if (tempobj == NULL) { ppl_warning(&c->errcontext, ERR_GENERIC, "Attempt to unset a filter which did not exist."); return; }
    ppl_garbageObject(tempobj);
    ppl_dictRemoveKey(c->set->filters,tempstr);
   }
  else if (strcmp_set && (strcmp(setoption,"fontsize")==0)) /* set fontsize */
   {
    double tempdbl = command[PARSE_set_fontsize_fontsize].real;
    if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set fontsize' command was not finite."); return; }
    if (tempdbl <= 0.0) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Font sizes are not allowed to be less than or equal to zero."); return; }
    sg->FontSize = tempdbl;
   }
  else if (strcmp_unset && (strcmp(setoption,"fontsize")==0)) /* unset fontsize */
   {
    sg->FontSize = c->set->graph_default.FontSize;
   }
  else if ((strcmp(setoption,"axescolor")==0) || (strcmp(setoption,"gridmajcolor")==0) || (strcmp(setoption,"gridmincolor")==0) || (strcmp(setoption,"textcolor")==0)) /* set axescolor | set gridmajcolor | set gridmincolor */
   {
    if (strcmp_unset)
     {
      if (strcmp(setoption,"axescolor"   )==0) { sg->AxesColor=c->set->graph_default.AxesColor; sg->AxesCol1234Space=c->set->graph_default.AxesCol1234Space; sg->AxesColor1=c->set->graph_default.AxesColor1; sg->AxesColor2=c->set->graph_default.AxesColor2; sg->AxesColor3=c->set->graph_default.AxesColor3; sg->AxesColor4=c->set->graph_default.AxesColor4; }
      if (strcmp(setoption,"gridmajcolor")==0) { sg->GridMajColor=c->set->graph_default.GridMajColor; sg->GridMajCol1234Space=c->set->graph_default.GridMajCol1234Space; sg->GridMajColor1=c->set->graph_default.GridMajColor1; sg->GridMajColor2=c->set->graph_default.GridMajColor2; sg->GridMajColor3=c->set->graph_default.GridMajColor3; sg->GridMajColor4=c->set->graph_default.GridMajColor4; }
      if (strcmp(setoption,"gridmincolor")==0) { sg->GridMinColor=c->set->graph_default.GridMinColor; sg->GridMinCol1234Space=c->set->graph_default.GridMinCol1234Space; sg->GridMinColor1=c->set->graph_default.GridMinColor1; sg->GridMinColor2=c->set->graph_default.GridMinColor2; sg->GridMinColor3=c->set->graph_default.GridMinColor3; sg->GridMinColor4=c->set->graph_default.GridMinColor4; }
      if (strcmp(setoption,"textcolor"   )==0) { sg->TextColor=c->set->graph_default.TextColor; sg->TextCol1234Space=c->set->graph_default.TextCol1234Space; sg->TextColor1=c->set->graph_default.TextColor1; sg->TextColor2=c->set->graph_default.TextColor2; sg->TextColor3=c->set->graph_default.TextColor3; sg->TextColor4=c->set->graph_default.TextColor4; }
     } else {
      unsigned char useCol, use1234;
      if (strcmp(setoption,"axescolor"   )==0) ppl_colorFromObj(c,&command[PARSE_set_axescolor_color   ],&sg->AxesColor   ,&sg->AxesCol1234Space   ,NULL,&sg->AxesColor1   ,&sg->AxesColor2   ,&sg->AxesColor3   ,&sg->AxesColor4   ,&useCol,&use1234);
      if (strcmp(setoption,"gridmajcolor")==0) ppl_colorFromObj(c,&command[PARSE_set_gridmajcolor_color],&sg->GridMajColor,&sg->GridMajCol1234Space,NULL,&sg->GridMajColor1,&sg->GridMajColor2,&sg->GridMajColor3,&sg->GridMajColor4,&useCol,&use1234);
      if (strcmp(setoption,"gridmincolor")==0) ppl_colorFromObj(c,&command[PARSE_set_gridmincolor_color],&sg->GridMinColor,&sg->GridMinCol1234Space,NULL,&sg->GridMinColor1,&sg->GridMinColor2,&sg->GridMinColor3,&sg->GridMinColor4,&useCol,&use1234);
      if (strcmp(setoption,"textcolor"   )==0) ppl_colorFromObj(c,&command[PARSE_set_textcolor_color   ],&sg->TextColor   ,&sg->TextCol1234Space   ,NULL,&sg->TextColor1   ,&sg->TextColor2   ,&sg->TextColor3   ,&sg->TextColor4   ,&useCol,&use1234);
     }
   }
  else if (strcmp_set && (strcmp(setoption,"grid")==0)) /* set grid */
   {
    int pos = PARSE_set_grid_0axes;
    if (command[pos].objType!=PPLOBJ_NUM)
     {
      int i;
      if (sg->grid != SW_ONOFF_ON)
       {
        for (i=0; i<MAX_AXES; i++) sg->GridAxisX[i] = c->set->graph_default.GridAxisX[i];
        for (i=0; i<MAX_AXES; i++) sg->GridAxisY[i] = c->set->graph_default.GridAxisY[i];
        for (i=0; i<MAX_AXES; i++) sg->GridAxisZ[i] = c->set->graph_default.GridAxisZ[i];
       }
      sg->grid = SW_ONOFF_ON;
     }
    else
     {
      int i, anum;
      if (sg->grid != SW_ONOFF_ON)
       {
        for (i=0; i<MAX_AXES; i++) sg->GridAxisX[i] = 0;
        for (i=0; i<MAX_AXES; i++) sg->GridAxisY[i] = 0;
        for (i=0; i<MAX_AXES; i++) sg->GridAxisZ[i] = 0;
       }
      sg->grid = SW_ONOFF_ON;
      while (command[pos].objType==PPLOBJ_NUM)
       {
        pos = (int)round(command[pos].real);
        if (pos<=0) break;
        anum = (int)round(command[pos+PARSE_set_grid_axis_0axes].real);
        i    = (int)round(command[pos+PARSE_set_grid_axis_0axes].exponent[0]);
        if      (i==1) { sg->GridAxisY[anum] = 1; }
        else if (i==2) { sg->GridAxisZ[anum] = 1; }
        else           { sg->GridAxisX[anum] = 1; }
       }
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"grid")==0)) /* unset grid */
   {
    int i;
    sg->grid = c->set->graph_default.grid;
    for (i=0; i<MAX_AXES; i++) sg->GridAxisX[i] = c->set->graph_default.GridAxisX[i];
    for (i=0; i<MAX_AXES; i++) sg->GridAxisY[i] = c->set->graph_default.GridAxisY[i];
    for (i=0; i<MAX_AXES; i++) sg->GridAxisZ[i] = c->set->graph_default.GridAxisZ[i];
   }
  else if (strcmp_set && (strcmp(setoption,"key")==0)) /* set key */
   {
    sg->key = SW_ONOFF_ON; // Turn key on
    if (command[PARSE_set_key_offset].objType==PPLOBJ_NUM) // Horizontal offset
     {
      double tempdbl = command[PARSE_set_key_offset].real;
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The horizontal offset supplied to the 'set key' command was not finite."); }
      else                        sg->KeyXOff.real = tempdbl;
     }
    if (command[PARSE_set_key_offset+1].objType==PPLOBJ_NUM) // Vertical offset
     {
      double tempdbl = command[PARSE_set_key_offset+1].real;
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The vertical offset supplied to the 'set key' command was not finite."); }
      else                        sg->KeyYOff.real = tempdbl;
     }

    // Now work out position of key
    if (command[PARSE_set_key_pos].objType==PPLOBJ_STR)
     {
      sg->KeyPos = ppl_fetchSettingByName(&c->errcontext, (char *)command[PARSE_set_key_pos].auxil, SW_KEYPOS_INT, SW_KEYPOS_STR);
     }
    if (command[PARSE_set_key_xpos].objType==PPLOBJ_STR)
     {
      char *tempstr = command[PARSE_set_key_xpos].auxil;
      if (tempstr != NULL)
       {
        if (strcmp(tempstr,"left")==0)
         {
          if      ((sg->KeyPos==SW_KEYPOS_TR)||(sg->KeyPos==SW_KEYPOS_TM)||(sg->KeyPos==SW_KEYPOS_TL)) sg->KeyPos=SW_KEYPOS_TL;
          else if ((sg->KeyPos==SW_KEYPOS_BR)||(sg->KeyPos==SW_KEYPOS_BM)||(sg->KeyPos==SW_KEYPOS_BL)) sg->KeyPos=SW_KEYPOS_BL;
          else                                                                                         sg->KeyPos=SW_KEYPOS_ML;
         }
        if (strcmp(tempstr,"xcenter")==0)
         {
          if      ((sg->KeyPos==SW_KEYPOS_TR)||(sg->KeyPos==SW_KEYPOS_TM)||(sg->KeyPos==SW_KEYPOS_TL)) sg->KeyPos=SW_KEYPOS_TM;
          else if ((sg->KeyPos==SW_KEYPOS_BR)||(sg->KeyPos==SW_KEYPOS_BM)||(sg->KeyPos==SW_KEYPOS_BL)) sg->KeyPos=SW_KEYPOS_BM;
          else                                                                                         sg->KeyPos=SW_KEYPOS_MM;
         }
        if (strcmp(tempstr,"right")==0)
         {
          if      ((sg->KeyPos==SW_KEYPOS_TR)||(sg->KeyPos==SW_KEYPOS_TM)||(sg->KeyPos==SW_KEYPOS_TL)) sg->KeyPos=SW_KEYPOS_TR;
          else if ((sg->KeyPos==SW_KEYPOS_BR)||(sg->KeyPos==SW_KEYPOS_BM)||(sg->KeyPos==SW_KEYPOS_BL)) sg->KeyPos=SW_KEYPOS_BR;
          else                                                                                         sg->KeyPos=SW_KEYPOS_MR;
         }
       }
      }
    if (command[PARSE_set_key_ypos].objType==PPLOBJ_STR)
     {
      char *tempstr = command[PARSE_set_key_ypos].auxil;
      if (tempstr != NULL)
       {
        if (strcmp(tempstr,"top")==0)
         {
          if      ((sg->KeyPos==SW_KEYPOS_TL)||(sg->KeyPos==SW_KEYPOS_ML)||(sg->KeyPos==SW_KEYPOS_BL)) sg->KeyPos=SW_KEYPOS_TL;
          else if ((sg->KeyPos==SW_KEYPOS_TR)||(sg->KeyPos==SW_KEYPOS_MR)||(sg->KeyPos==SW_KEYPOS_BR)) sg->KeyPos=SW_KEYPOS_TR;
          else                                                                                         sg->KeyPos=SW_KEYPOS_TM;
         }
        if (strcmp(tempstr,"ycenter")==0)
         {
          if      ((sg->KeyPos==SW_KEYPOS_TL)||(sg->KeyPos==SW_KEYPOS_ML)||(sg->KeyPos==SW_KEYPOS_BL)) sg->KeyPos=SW_KEYPOS_ML;
          else if ((sg->KeyPos==SW_KEYPOS_TR)||(sg->KeyPos==SW_KEYPOS_MR)||(sg->KeyPos==SW_KEYPOS_BR)) sg->KeyPos=SW_KEYPOS_MR;
          else                                                                                         sg->KeyPos=SW_KEYPOS_MM;
         }
        if (strcmp(tempstr,"bottom")==0)
         {
          if      ((sg->KeyPos==SW_KEYPOS_TL)||(sg->KeyPos==SW_KEYPOS_ML)||(sg->KeyPos==SW_KEYPOS_BL)) sg->KeyPos=SW_KEYPOS_BL;
          else if ((sg->KeyPos==SW_KEYPOS_TR)||(sg->KeyPos==SW_KEYPOS_MR)||(sg->KeyPos==SW_KEYPOS_BR)) sg->KeyPos=SW_KEYPOS_BR;
          else                                                                                         sg->KeyPos=SW_KEYPOS_BM;
         }
       }
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"key")==0)) /* unset key */
   {
    sg->key     = c->set->graph_default.key;
    sg->KeyPos  = c->set->graph_default.KeyPos;
    sg->KeyXOff = c->set->graph_default.KeyXOff;
    sg->KeyYOff = c->set->graph_default.KeyYOff;
   }
  else if (strcmp_set && (strcmp(setoption,"keycolumns")==0)) /* set keycolumns */
   {
    if (command[PARSE_set_keycolumns_key_columns].objType==PPLOBJ_NUM)
     {
      double tempdbl = command[PARSE_set_keycolumns_key_columns].real;
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set keycolumns' command was not finite."); return; }
      if (tempdbl <= 0.0) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Number of key columns is not allowed to be less than or equal to zero."); return; }
      sg->KeyColumns = (int)round(tempdbl);
     }
    else
     {
      sg->KeyColumns = c->set->graph_default.KeyColumns; // set keycolumns auto -- equivalent to unset
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"keycolumns")==0)) /* unset keycolumns */
   {
    sg->KeyColumns = c->set->graph_default.KeyColumns;
   }
  else if (strcmp_set && (strcmp(setoption,"label")==0)) /* set label */
   {
    if (ll!=NULL) ppllabel_add(c, ll, in, pl, PARSE_TABLE_set_label_);
   }
  else if (strcmp_unset && (strcmp(setoption,"label")==0)) /* unset label */
   {
    if (ll!=NULL) ppllabel_unset(c, ll, in, pl, PARSE_TABLE_unset_label_);
   }
  else if (strcmp_set && (strcmp(setoption,"linewidth")==0)) /* set linewidth */
   {
    double tempdbl = command[PARSE_set_linewidth_linewidth].real;
    if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set linewidth' command was not finite."); return; }
    if (tempdbl <= 0.0) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Line widths are not allowed to be less than or equal to zero."); return; }
    sg->LineWidth = tempdbl;
   }
  else if (strcmp_unset && (strcmp(setoption,"linewidth")==0)) /* unset linewidth */
   {
    sg->LineWidth = c->set->graph_default.LineWidth;
   }
  else if ( (strcmp_set   && (strcmp(setoption,"logscale")==0)) || /* set logscale */
            (strcmp_unset && (strcmp(setoption,"logscale")==0)) || /* unset logscale */
            (strcmp_set   && (strcmp(setoption,"nologscale")==0)) )
   {
    const int sl = (strcmp_set   && (strcmp(setoption,"logscale")==0));
    const int ul = (strcmp_unset && (strcmp(setoption,"logscale")==0));
    const int snl= (strcmp_set   && (strcmp(setoption,"nologscale")==0));
    int       b  = 10;
    int       pos= sl ? PARSE_set_logscale_0axes       : (ul ? PARSE_unset_logscale_0axes       : PARSE_set_nologscale_0axes);
    const int ap = sl ? PARSE_set_logscale_axis_0axes  : (ul ? PARSE_unset_logscale_axis_0axes  : PARSE_set_nologscale_axis_0axes);
    const int c1p= sl ? PARSE_set_logscale_c1log_0axes : (ul ? PARSE_unset_logscale_c1log_0axes : PARSE_set_nologscale_c1log_0axes);
    const int c2p= sl ? PARSE_set_logscale_c2log_0axes : (ul ? PARSE_unset_logscale_c2log_0axes : PARSE_set_nologscale_c2log_0axes);
    const int c3p= sl ? PARSE_set_logscale_c3log_0axes : (ul ? PARSE_unset_logscale_c3log_0axes : PARSE_set_nologscale_c3log_0axes);
    const int c4p= sl ? PARSE_set_logscale_c4log_0axes : (ul ? PARSE_unset_logscale_c4log_0axes : PARSE_set_nologscale_c4log_0axes);
    const int tp = sl ? PARSE_set_logscale_tlog_0axes  : (ul ? PARSE_unset_logscale_tlog_0axes  : PARSE_set_nologscale_tlog_0axes );
    const int up = sl ? PARSE_set_logscale_ulog_0axes  : (ul ? PARSE_unset_logscale_ulog_0axes  : PARSE_set_nologscale_ulog_0axes );
    const int vp = sl ? PARSE_set_logscale_vlog_0axes  : (ul ? PARSE_unset_logscale_vlog_0axes  : PARSE_set_nologscale_vlog_0axes );
    const int setAll = (command[pos].objType!=PPLOBJ_NUM);

    if (sl && (command[PARSE_set_logscale_base].objType==PPLOBJ_NUM))
     {
      b = (int)round(command[PARSE_set_logscale_base].real);
      if ((b<2)||(b>1024)) { sprintf(c->errcontext.tempErrStr, "Attempt to use log axis with base %d. Pyxplot only supports bases in the range 2 - 1024. Defaulting to base 10.", b); ppl_warning(&c->errcontext, ERR_GENERIC, NULL); b=10; }
     }

    if (!setAll)
     {
      while (command[pos].objType==PPLOBJ_NUM)
       {
        pos = (int)round(command[pos].real);
        if (pos<=0) break;
        if (command[pos+ap].objType==PPLOBJ_NUM)
         {
          int *alog, *adlog;
          int i = (int)round(command[pos+ap].real);
          int j = (int)round(command[pos+ap].exponent[0]);
          int newstate;
          pplset_axis *a, *ad;
          if      (j==1) { a = &ya[i]; ad = &c->set->YAxesDefault[i]; }
          else if (j==2) { a = &za[i]; ad = &c->set->ZAxesDefault[i]; }
          else           { a = &xa[i]; ad = &c->set->XAxesDefault[i]; }
          GET_AXISLOG(alog , a , 0);
          GET_AXISLOG(adlog, ad, 1);
          if      (sl ) { a->enabled = 1; newstate = SW_BOOL_TRUE; }
          else if (snl) { a->enabled = 1; newstate = SW_BOOL_FALSE; }
          else          {                 newstate = *adlog; }

          if (*alog != newstate) { tics_rm(&a->tics); tics_rm(&a->ticsM); }
          *alog = newstate;
          a->tics .logBase = b;
          a->ticsM.logBase = b;
         }
        else
         {
          if (command[pos+c1p].objType==PPLOBJ_STR)
           {
            int newstate = sl ? SW_BOOL_TRUE : (ul ? c->set->graph_default.Clog[0] : SW_BOOL_FALSE);
            if (sg->Clog[0] != newstate) { tics_rm(&sg->ticsC); tics_rm(&sg->ticsCM); }
            sg->Clog[0]  = newstate;
            sg->ticsC .logBase = b;
            sg->ticsCM.logBase = b;
           }
          if (command[pos+c2p].objType==PPLOBJ_STR) { sg->Clog[1] = sl ? SW_BOOL_TRUE : (ul ? c->set->graph_default.Clog[1] : SW_BOOL_FALSE); }
          if (command[pos+c3p].objType==PPLOBJ_STR) { sg->Clog[2] = sl ? SW_BOOL_TRUE : (ul ? c->set->graph_default.Clog[2] : SW_BOOL_FALSE); }
          if (command[pos+c4p].objType==PPLOBJ_STR) { sg->Clog[3] = sl ? SW_BOOL_TRUE : (ul ? c->set->graph_default.Clog[3] : SW_BOOL_FALSE); }
          if (command[pos+tp ].objType==PPLOBJ_STR) { sg->Tlog    = sl ? SW_BOOL_TRUE : (ul ? c->set->graph_default.Tlog    : SW_BOOL_FALSE); }
          if (command[pos+up ].objType==PPLOBJ_STR) { sg->Ulog    = sl ? SW_BOOL_TRUE : (ul ? c->set->graph_default.Ulog    : SW_BOOL_FALSE); }
          if (command[pos+vp ].objType==PPLOBJ_STR) { sg->Vlog    = sl ? SW_BOOL_TRUE : (ul ? c->set->graph_default.Vlog    : SW_BOOL_FALSE); }
         }
       }
     }
    else if (!ul) // set logscale or nologscale on all axes
     {
      int i, *alog;
      pplset_axis *a;
      int newstate = sl ? SW_BOOL_TRUE : SW_BOOL_FALSE;
      if (sg->Clog[0] != newstate) { tics_rm(&sg->ticsC); tics_rm(&sg->ticsCM); }
      sg->Clog[0] = sg->Clog[1] = sg->Clog[2] = sg->Clog[3] = sg->Tlog = sg->Ulog = sg->Vlog = newstate;
      for (i=0;i<MAX_AXES;i++) { a=&xa[i]; if (a->enabled) { GET_AXISLOG(alog,a,0); if (*alog != newstate) { tics_rm(&a->tics); tics_rm(&a->ticsM); } *alog=newstate; a->tics.logBase=a->ticsM.logBase=b; } }
      for (i=0;i<MAX_AXES;i++) { a=&ya[i]; if (a->enabled) { GET_AXISLOG(alog,a,0); if (*alog != newstate) { tics_rm(&a->tics); tics_rm(&a->ticsM); } *alog=newstate; a->tics.logBase=a->ticsM.logBase=b; } }
      for (i=0;i<MAX_AXES;i++) { a=&za[i]; if (a->enabled) { GET_AXISLOG(alog,a,0); if (*alog != newstate) { tics_rm(&a->tics); tics_rm(&a->ticsM); } *alog=newstate; a->tics.logBase=a->ticsM.logBase=b; } }
     }
    else // unset logscale on all axes
     {
      int i, *alog, *adlog;
      pplset_axis *a, *ad;
      if (sg->Clog[0] != c->set->graph_default.Clog[0]) { tics_rm(&sg->ticsC); tics_rm(&sg->ticsCM); }
      sg->Clog[0] = c->set->graph_default.Clog[0];
      sg->Clog[1] = c->set->graph_default.Clog[1];
      sg->Clog[2] = c->set->graph_default.Clog[2];
      sg->Clog[3] = c->set->graph_default.Clog[3];
      sg->Tlog    = c->set->graph_default.Tlog;
      sg->Ulog    = c->set->graph_default.Ulog;
      sg->Vlog    = c->set->graph_default.Vlog;
      for (i=0;i<MAX_AXES;i++) { a=&xa[i]; ad=&c->set->XAxesDefault[i]; if (a->enabled) { GET_AXISLOG(alog,a,0); GET_AXISLOG(adlog,ad,1); if (*alog != *adlog) { tics_rm(&a->tics); tics_rm(&a->ticsM); } *alog=*adlog; a->tics.logBase=a->ticsM.logBase=b; } }
      for (i=0;i<MAX_AXES;i++) { a=&ya[i]; ad=&c->set->YAxesDefault[i]; if (a->enabled) { GET_AXISLOG(alog,a,0); GET_AXISLOG(adlog,ad,1); if (*alog != *adlog) { tics_rm(&a->tics); tics_rm(&a->ticsM); } *alog=*adlog; a->tics.logBase=a->ticsM.logBase=b; } }
      for (i=0;i<MAX_AXES;i++) { a=&za[i]; ad=&c->set->ZAxesDefault[i]; if (a->enabled) { GET_AXISLOG(alog,a,0); GET_AXISLOG(adlog,ad,1); if (*alog != *adlog) { tics_rm(&a->tics); tics_rm(&a->ticsM); } *alog=*adlog; a->tics.logBase=a->ticsM.logBase=b; } }
     }
   }
  else if (strcmp_set && (strcmp(setoption,"multiplot")==0)) /* set multiplot */
   {
    c->set->term_current.multiplot = SW_ONOFF_ON;
   }
  else if (strcmp_unset && (strcmp(setoption,"multiplot")==0)) /* unset multiplot */
   {
    if ((c->set->term_default.multiplot == SW_ONOFF_OFF) && (c->set->term_current.multiplot == SW_ONOFF_ON)) ppl_directive_clear(c,pl,in,interactive);
    c->set->term_current.multiplot = c->set->term_default.multiplot;
   }
  else if (strcmp_set && (strcmp(setoption,"noarrow")==0)) /* set noarrow */
   {
    if (al!=NULL) pplarrow_remove(c, al, in, pl, PARSE_TABLE_set_noarrow_, 0);
   }
  else if (strcmp_set && (strcmp(setoption,"nobackup")==0)) /* set nobackup */
   {
    c->set->term_current.backup = SW_ONOFF_OFF;
   }
  else if (strcmp_set && (strcmp(setoption,"noc1format")==0)) /* set noc1format */
   {
    sg->c1format    = NULL;
    sg->c1formatset = 0;
   }
  else if (strcmp_set && (strcmp(setoption,"noc1label")==0)) /* set noc1label */
   {
    sg->c1label[0] = '\0';
   }
  else if (strcmp_set && (strcmp(setoption,"noc1tics")==0)) /* set noc1tics */
   {
    int m = (command[PARSE_set_notics_minor].objType==PPLOBJ_STR);
    if (!m) { SET_NOTICKS(sg->ticsC); }
    SET_NOTICKS(sg->ticsCM);
   }
  else if (strcmp_set && (strcmp(setoption,"noclip")==0)) /* set noclip */
   {
    sg->clip = SW_ONOFF_OFF;
   }
  else if (strcmp_set && (strcmp(setoption,"nocolkey")==0)) /* set nocolkey */
   {
    sg->ColKey = SW_ONOFF_OFF;
   }
  else if (strcmp_set && (strcmp(setoption,"nodisplay")==0)) /* set nodisplay */
   {
    c->set->term_current.display = SW_ONOFF_OFF;
   }
  else if (strcmp_set && (strcmp(setoption,"nogrid")==0)) /* set nogrid */
   {
    int pos = PARSE_set_nogrid_0axes;
    if (command[pos].objType!=PPLOBJ_NUM)
     {
      sg->grid = SW_ONOFF_OFF;
     }
    else
     {
      int i, anum;
      while (command[pos].objType==PPLOBJ_NUM)
       {
        pos = (int)round(command[pos].real);
        if (pos<=0) break;
        anum = (int)round(command[pos+PARSE_set_nogrid_axis_0axes].real);
        i    = (int)round(command[pos+PARSE_set_nogrid_axis_0axes].exponent[0]);
        if      (i==1) { sg->GridAxisY[anum] = 0; }
        else if (i==2) { sg->GridAxisZ[anum] = 0; }
        else           { sg->GridAxisX[anum] = 0; }
       }
     }
   }
  else if (strcmp_set && (strcmp(setoption,"nokey")==0)) /* set nokey */
   {
    sg->key = SW_ONOFF_OFF;
   }
  else if (strcmp_set && (strcmp(setoption,"nolabel")==0)) /* set nolabel */
   {
    if (ll!=NULL) ppllabel_remove(c, ll, in, pl, PARSE_TABLE_set_nolabel_, 0);
   }
  else if (strcmp_set && (strcmp(setoption,"nomultiplot")==0)) /* set nomultiplot */
   {
    if (c->set->term_current.multiplot != SW_ONOFF_OFF) ppl_directive_clear(c,pl,in,interactive);
    c->set->term_current.multiplot = SW_ONOFF_OFF;
   }
  else if (strcmp_set && (strcmp(setoption,"notics")==0)) /* set notics */
   {
    int m = (command[PARSE_set_notics_minor].objType==PPLOBJ_STR);
    if (command[PARSE_set_notics_axis].objType==PPLOBJ_NUM)
     {
      int i = (int)round(command[PARSE_set_notics_axis].real);
      int j = (int)round(command[PARSE_set_notics_axis].exponent[0]);
      if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
       {
        if      (j==1) { pplset_axis *a = &ya[i]; if (!m) { SET_NOTICKS(a->tics); } SET_NOTICKS(a->ticsM); }
        else if (j==2) { pplset_axis *a = &za[i]; if (!m) { SET_NOTICKS(a->tics); } SET_NOTICKS(a->ticsM); }
        else           { pplset_axis *a = &xa[i]; if (!m) { SET_NOTICKS(a->tics); } SET_NOTICKS(a->ticsM); }
       }
     }
    else
     {
      int i;
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a = &xa[i]; if (a->enabled) { if (!m) { SET_NOTICKS(a->tics); } SET_NOTICKS(a->ticsM); } }
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a = &ya[i]; if (a->enabled) { if (!m) { SET_NOTICKS(a->tics); } SET_NOTICKS(a->ticsM); } }
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a = &za[i]; if (a->enabled) { if (!m) { SET_NOTICKS(a->tics); } SET_NOTICKS(a->ticsM); } }
     }
   }
  else if (strcmp_set && (strcmp(setoption,"notitle")==0)) /* set notitle */
   {
    strcpy(sg->title, "");
   }
  else if (strcmp_set && (strcmp(setoption,"noxformat")==0)) /* set noxformat */
   {
    int i = (int)round(command[PARSE_set_noxformat_axis].real);
    int j = (int)round(command[PARSE_set_noxformat_axis].exponent[0]);
    if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
     {
      pplset_axis *a, *ad;
      if      (j==1) { a = &ya[i]; ad = &c->set->YAxesDefault[i]; }
      else if (j==2) { a = &za[i]; ad = &c->set->ZAxesDefault[i]; }
      else           { a = &xa[i]; ad = &c->set->XAxesDefault[i]; }

      a->enabled = 1;
      if (a->format != NULL) { pplExpr_free((pplExpr *)a->format); a->format=NULL; }
      a->TickLabelRotation = ad->TickLabelRotation;
      a->TickLabelRotate   = ad->TickLabelRotate;
     }
   }
  else if (strcmp_set && (strcmp(setoption,"noxlabel")==0)) /* set noxlabel */
   {
    int i = (int)round(command[PARSE_set_noxlabel_axis].real);
    int j = (int)round(command[PARSE_set_noxlabel_axis].exponent[0]);
    if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
     {
      pplset_axis *a;
      if      (j==1) a = &ya[i];
      else if (j==2) a = &za[i];
      else           a = &xa[i];
      a->enabled=1;
      if (a->label!=NULL) free(a->label);
      a->label=NULL;
     }
   }
  else if (strcmp_set && (strcmp(setoption,"numerics")==0)) /* set numerics */
   {
    char *tempstr = (char *)command[PARSE_set_numerics_complex].auxil;
    int   got     =        (command[PARSE_set_numerics_complex].objType == PPLOBJ_STR);
    if (got) c->set->term_current.ComplexNumbers = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_numerics_errortype].auxil;
    got     =        (command[PARSE_set_numerics_errortype].objType == PPLOBJ_STR);
    if (got) c->set->term_current.ExplicitErrors = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    got     =        (command[PARSE_set_numerics_number_significant_figures].objType == PPLOBJ_NUM);
    if (got)
     {
      int tempint = (int)round(command[PARSE_set_numerics_number_significant_figures].real);
      if (tempint <  1) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Numbers cannot be displayed to fewer than one significant figure."); return; }
      if (tempint > 30) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "It is not sensible to try to display numbers to more than 30 significant figures. Calculations in Pyxplot are only accurate to double precision."); return; }
      c->set->term_current.SignificantFigures = tempint;
     }

    tempstr = (char *)command[PARSE_set_numerics_display].auxil;
    got     =        (command[PARSE_set_numerics_display].objType == PPLOBJ_STR);
    if (got) c->set->term_current.NumDisplay = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_DISPLAY_INT, SW_DISPLAY_STR);
   }
  else if (strcmp_unset && (strcmp(setoption,"numerics")==0)) /* unset numerics */
   {
    c->set->term_current.ComplexNumbers     = c->set->term_default.ComplexNumbers;
    c->set->term_current.ExplicitErrors     = c->set->term_default.ExplicitErrors;
    c->set->term_current.NumDisplay         = c->set->term_default.NumDisplay;
    c->set->term_current.SignificantFigures = c->set->term_default.SignificantFigures;
   }
  else if (strcmp_unset && (strcmp(setoption,"numerics_sigfig")==0)) /* unset numerics sigfig */
   {
    c->set->term_current.SignificantFigures = c->set->term_default.SignificantFigures;
   }
  else if (strcmp_unset && (strcmp(setoption,"numerics_errors")==0)) /* unset numerics errors */
   {
    c->set->term_current.ExplicitErrors     = c->set->term_default.ExplicitErrors;
   }
  else if (strcmp_unset && (strcmp(setoption,"numerics_sigfig")==0)) /* unset numerics complex */
   {
    c->set->term_current.ComplexNumbers     = c->set->term_default.ComplexNumbers;
   }
  else if (strcmp_unset && (strcmp(setoption,"numerics_display")==0)) /* unset numerics display */
   {
    c->set->term_current.NumDisplay         = c->set->term_default.NumDisplay;
   }
  else if (strcmp_set && (strcmp(setoption,"origin")==0)) /* set origin */
   {
    double x = command[PARSE_set_origin_origin  ].real;
    double y = command[PARSE_set_origin_origin+1].real;
    sg->OriginX.real = x;
    sg->OriginY.real = y;
   }
  else if (strcmp_unset && (strcmp(setoption,"origin")==0)) /* unset origin */
   {
    sg->OriginX = c->set->graph_default.OriginX;
    sg->OriginY = c->set->graph_default.OriginY;
   }
  else if (strcmp_set && (strcmp(setoption,"output")==0)) /* set output */
   {
    char *tempstr = (char *)command[PARSE_set_output_filename].auxil;
    strncpy(c->set->term_current.output, tempstr, FNAME_LENGTH-1);
    c->set->term_current.output[FNAME_LENGTH-1]='\0';
   }
  else if (strcmp_unset && (strcmp(setoption,"output")==0)) /* unset output */
   {
    strncpy(c->set->term_current.output, c->set->term_default.output, FNAME_LENGTH-1);
    c->set->term_current.output[FNAME_LENGTH-1]='\0';
   }
  else if (strcmp_set && (strcmp(setoption,"palette")==0)) /* set palette */
   {
    if (command[PARSE_set_palette_palette].objType==PPLOBJ_NUM)
     {
      int pos = PARSE_set_palette_palette, count=0;
      while (command[pos].objType==PPLOBJ_NUM)
       {
        unsigned char d1,d2;
        pplObj *co;
        pos = (int)round(command[pos].real);
        if (pos<=0) break;
        if (count>=PALETTE_LENGTH-1) { ppl_warning(&c->errcontext, ERR_GENERIC, "The 'set palette' command has been passed a palette which is too long; truncating it."); break; }
        co = &command[pos+PARSE_set_palette_color_palette];
        c->set->palette_current[count] = -1;
        ppl_colorFromObj(c,co,&c->set->palette_current[count],&c->set->paletteS_current[count],NULL,&c->set->palette1_current[count],&c->set->palette2_current[count],&c->set->palette3_current[count],&c->set->palette4_current[count],&d1,&d2);
        count++;
       }
      c->set->palette_current[count] = -1;
     }
    else
     {
      pplObj *lo = &command[PARSE_set_palette_list];
      list   *l  = (list *)lo->auxil;
      int     ll,i;
      if (lo->objType!=PPLOBJ_LIST) { sprintf(c->errcontext.tempErrStr, "The 'set palette' command can only generate palettes from objects of type list; supplied object has type <%s>.", pplObjTypeNames[lo->objType]); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; }
      ll = ppl_listLen(l);
      if (ll<1) { sprintf(c->errcontext.tempErrStr, "The 'set palette' command was passed a palette of zero length."); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; }
      for (i=0; i<ll; i++)
       {
        pplObj *o = ppl_listGetItem(l,i);
        if ((o->objType!=PPLOBJ_NUM)&&(o->objType!=PPLOBJ_COL)) { sprintf(c->errcontext.tempErrStr, "Object of type <%s> in list supplied to the 'set palette' command could not be converted to a color.", pplObjTypeNames[o->objType]); ppl_error(&c->errcontext, ERR_TYPE, -1, -1, NULL); return; }
       }
      for (i=0; i<ll; i++)
       {
        unsigned char d1,d2;
        pplObj *o = ppl_listGetItem(l,i);
        if (i>=PALETTE_LENGTH-1) { ppl_warning(&c->errcontext, ERR_GENERIC, "The 'set palette' command has been passed a palette which is too long; truncating it."); break; }
        c->set->palette_current[i] = -1;
        ppl_colorFromObj(c,o,&c->set->palette_current[i],&c->set->paletteS_current[i],NULL,&c->set->palette1_current[i],&c->set->palette2_current[i],&c->set->palette3_current[i],&c->set->palette4_current[i],&d1,&d2);
       }
      c->set->palette_current[i] = -1;
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"palette")==0)) /* unset palette */
   {
    for (i=0; i<PALETTE_LENGTH; i++) c->set->palette_current [i] = c->set->palette_default [i];
    for (i=0; i<PALETTE_LENGTH; i++) c->set->paletteS_current[i] = c->set->paletteS_default[i];
    for (i=0; i<PALETTE_LENGTH; i++) c->set->palette1_current[i] = c->set->palette1_default[i];
    for (i=0; i<PALETTE_LENGTH; i++) c->set->palette2_current[i] = c->set->palette2_default[i];
    for (i=0; i<PALETTE_LENGTH; i++) c->set->palette3_current[i] = c->set->palette3_default[i];
    for (i=0; i<PALETTE_LENGTH; i++) c->set->palette4_current[i] = c->set->palette4_default[i];
   }
  else if (strcmp_set && (strcmp(setoption,"papersize")==0)) /* set papersize */
   {
    if (command[PARSE_set_papersize_paper_name].objType==PPLOBJ_STR)
     {
      double d1,d2;
      char *paperName = (char *)command[PARSE_set_papersize_paper_name].auxil;
      ppl_PaperSizeByName(paperName, &d1, &d2);
      if (d1>0)
       {
        c->set->term_current.PaperHeight.real = d1/1000;
        c->set->term_current.PaperWidth.real  = d2/1000;
        ppl_GetPaperName(c->set->term_current.PaperName, &d1, &d2);
       }
      else
       {
        sprintf(c->errcontext.tempErrStr, "Unrecognised paper size '%s'.", paperName); ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, NULL); return;
       }
     }
    else
     {
      double d1 = command[PARSE_set_papersize_size  ].real;
      double d2 = command[PARSE_set_papersize_size+1].real;
      if ((!gsl_finite(d1))||(!gsl_finite(d2))) { sprintf(c->errcontext.tempErrStr, "The size coordinates supplied to the 'set papersize' command was not finite."); ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, NULL); return; }
      c->set->term_current.PaperWidth .real = d1;
      c->set->term_current.PaperHeight.real = d2;
      d1 *= 1000; // Function below takes size input in mm
      d2 *= 1000;
      ppl_GetPaperName(c->set->term_current.PaperName, &d1, &d2);
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"papersize")==0)) /* unset papersize */
   {
    c->set->term_current.PaperHeight.real = c->set->term_default.PaperHeight.real;
    c->set->term_current.PaperWidth .real = c->set->term_default.PaperWidth .real;
    strcpy(c->set->term_current.PaperName, c->set->term_default.PaperName);
   }
  else if (strcmp_set && (strcmp(setoption,"pointlinewidth")==0)) /* set pointlinewidth */
   {
    double tempdbl = command[PARSE_set_pointlinewidth_pointlinewidth].real;
    if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set pointlinewidth' command was not finite."); return; }
    if (tempdbl <= 0.0) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Line widths are not allowed to be less than or equal to zero."); return; }
    sg->PointLineWidth = tempdbl;
   }
  else if (strcmp_unset && (strcmp(setoption,"pointlinewidth")==0)) /* unset pointlinewidth */
   {
    sg->PointLineWidth = c->set->graph_default.PointLineWidth;
   }
  else if (strcmp_set && (strcmp(setoption,"pointsize")==0)) /* set pointsize */
   {
    double tempdbl = command[PARSE_set_pointsize_pointsize].real;
    if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set pointsize' command was not finite."); return; }
    if (tempdbl <= 0.0) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Point sizes are not allowed to be less than or equal to zero."); return; }
    sg->PointSize = tempdbl;
   }
  else if (strcmp_unset && (strcmp(setoption,"pointsize")==0)) /* unset pointsize */
   {
    sg->PointSize = c->set->graph_default.PointSize;
   }
  else if (strcmp_set && (strcmp(setoption,"preamble")==0)) /* set preamble */
   {
    char *tempstr = (char *)command[PARSE_set_preamble_preamble].auxil;
    strncpy(c->set->term_current.LatexPreamble, tempstr, FNAME_LENGTH-1);
    c->set->term_current.LatexPreamble[FNAME_LENGTH-1]='\0';
   }
  else if (strcmp_unset && (strcmp(setoption,"preamble")==0)) /* unset preamble */
   {
    strncpy(c->set->term_current.LatexPreamble, c->set->term_default.LatexPreamble, FNAME_LENGTH-1);
    c->set->term_current.LatexPreamble[FNAME_LENGTH-1]='\0';
   }
  else if (strcmp_set && (strcmp(setoption,"samples")==0)) /* set samples */
   {
    if (command[PARSE_set_samples_samples].objType==PPLOBJ_NUM)
     {
      int i = (int)round(command[PARSE_set_samples_samples].real);
      if (i<2) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Graphs cannot be constucted based on fewer than two samples."); i=2; }
      sg->samples = i;
     }
    if (command[PARSE_set_samples_samplesX].objType==PPLOBJ_NUM)
     {
      int i = (int)round(command[PARSE_set_samples_samplesX].real);
      if (i<2) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Graphs cannot be constucted based on fewer than two samples."); i=2; }
      sg->SamplesX = i;
      sg->SamplesXAuto = SW_BOOL_FALSE;
     }
    if (command[PARSE_set_samples_samplesY].objType==PPLOBJ_NUM)
     {
      int i = (int)round(command[PARSE_set_samples_samplesY].real);
      if (i<2) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Graphs cannot be constucted based on fewer than two samples."); i=2; }
      sg->SamplesY = i;
      sg->SamplesYAuto = SW_BOOL_FALSE;
     }
    if (command[PARSE_set_samples_samplesXauto].objType==PPLOBJ_STR) sg->SamplesXAuto = SW_BOOL_TRUE;
    if (command[PARSE_set_samples_samplesYauto].objType==PPLOBJ_STR) sg->SamplesYAuto = SW_BOOL_TRUE;
    if (command[PARSE_set_samples_method].objType==PPLOBJ_STR)
     {
      sg->Sample2DMethod = ppl_fetchSettingByName(&c->errcontext, (char *)command[PARSE_set_samples_method].auxil, SW_SAMPLEMETHOD_INT, SW_SAMPLEMETHOD_STR);
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"samples")==0)) /* unset samples */
   {
    sg->samples        = c->set->graph_default.samples;
    sg->SamplesX       = c->set->graph_default.SamplesX;
    sg->SamplesXAuto   = c->set->graph_default.SamplesXAuto;
    sg->SamplesY       = c->set->graph_default.SamplesY;
    sg->SamplesYAuto   = c->set->graph_default.SamplesYAuto;
    sg->Sample2DMethod = c->set->graph_default.Sample2DMethod;
   }
  else if (strcmp_set && (strcmp(setoption,"seed")==0)) /* set seed */
   {
    long li;
    double d = command[PARSE_set_seed_seed].real;
    if (!gsl_finite(d)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The value supplied to the 'set seed' command was not finite."); return; }
    if      (d < LONG_MIN) li = LONG_MIN;
    else if (d > LONG_MAX) li = LONG_MAX;
    else                   li = (long)d;
    c->set->term_current.RandomSeed = li;
    pplfunc_setRandomSeed(li);
   }
  else if (strcmp_unset && (strcmp(setoption,"seed")==0)) /* unset seed */
   {
    c->set->term_current.RandomSeed = c->set->term_default.RandomSeed;
    pplfunc_setRandomSeed(c->set->term_current.RandomSeed);
   }
  else if (strcmp_set && (strcmp(setoption,"size")==0)) /* set size */
   {
    if (command[PARSE_set_size_width].objType==PPLOBJ_NUM)
     {
      if (!gsl_finite(command[PARSE_set_size_width].real)) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "The width supplied to the 'set size' command was not finite."); return; }
      sg->width.real=command[PARSE_set_size_width].real;
     }
    if (command[PARSE_set_size_height].objType==PPLOBJ_NUM)
     {
      double r = command[PARSE_set_size_height].real / sg->width.real;
      if ((!gsl_finite(r)) || (fabs(r) < 1e-6) || (fabs(r) > 1e4)) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "The requested y/x aspect ratios for graphs must be in the range 1e-6 to 10000."); return; }
      sg->aspect = r;
      sg->AutoAspect = SW_ONOFF_OFF;
     }
    if (command[PARSE_set_size_depth].objType==PPLOBJ_NUM)
     {
      double r = command[PARSE_set_size_depth].real / sg->width.real;
      if ((!gsl_finite(r)) || (fabs(r) < 1e-6) || (fabs(r) > 1e4)) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "The requested z/x aspect ratios for graphs must be in the range 1e-6 to 10000."); return; }
      sg->zaspect = r;
      sg->AutoZAspect = SW_ONOFF_OFF;
     }
    if (command[PARSE_set_size_ratio].objType==PPLOBJ_NUM)
     {
      double r = command[PARSE_set_size_ratio].real;
      if ((!gsl_finite(r)) || (fabs(r) < 1e-6) || (fabs(r) > 1e4)) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "The requested y/x aspect ratios for graphs must be in the range 1e-6 to 10000."); return; }
      sg->aspect = r;
      sg->AutoAspect = SW_ONOFF_OFF;
     }
    if (command[PARSE_set_size_zratio].objType==PPLOBJ_NUM)
     {
      double r = command[PARSE_set_size_zratio].real;
      if ((!gsl_finite(r)) || (fabs(r) < 1e-6) || (fabs(r) > 1e4)) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "The requested z/x aspect ratios for graphs must be in the range 1e-6 to 10000."); return; }
      sg->zaspect = r;
      sg->AutoZAspect = SW_ONOFF_OFF;
     }
    if (command[PARSE_set_size_square  ].objType==PPLOBJ_STR) { sg->aspect = 1; sg->AutoAspect = SW_ONOFF_OFF; sg->zaspect = 1; sg->AutoZAspect = SW_ONOFF_OFF; }
    if (command[PARSE_set_size_noratio ].objType==PPLOBJ_STR) { sg->aspect = c->set->graph_default.aspect; sg->AutoAspect = 1; }
    if (command[PARSE_set_size_nozratio].objType==PPLOBJ_STR) { sg->zaspect = c->set->graph_default.zaspect; sg->AutoZAspect = 1; }
   }
  else if (strcmp_unset && (strcmp(setoption,"size")==0)) /* unset size */
   {
    sg->width.real   = c->set->graph_default.width.real;
    sg->aspect       = c->set->graph_default.aspect;
    sg->AutoAspect   = c->set->graph_default.AutoAspect;
    sg->zaspect      = c->set->graph_default.zaspect;
    sg->AutoZAspect  = c->set->graph_default.AutoZAspect;
   }
  else if (strcmp_set && (strcmp(setoption,"style")==0)) /* set style data / function */
   {
    char      *type     = (char *)command[PARSE_set_style_numbered_dataset_type].auxil;
    withWords *outstyle = (type[0]=='d') ? &sg->dataStyle : &sg->funcStyle;
    withWords  ww_tmp, ww_tmp2;
    ppl_withWordsFromDict(c, in, pl, PARSE_TABLE_set_style_numbered_, 0, &ww_tmp); // this is correct!
    ppl_withWordsMerge   (c, &ww_tmp2, &ww_tmp, outstyle, NULL, NULL, NULL, 0);
    ppl_withWordsDestroy (c, &ww_tmp);
    *outstyle = ww_tmp2;
    ppl_withWordsDestroy (c, &ww_tmp2);
   }
  else if (strcmp_set && (strcmp(setoption,"style_numbered")==0)) /* set style */
   {
    double     nd = command[PARSE_set_style_numbered_style_set_number].real;
    int        n;
    withWords *outstyle, ww_tmp, ww_tmp2;
    if ((nd<0)||(nd>=MAX_PLOTSTYLES)) { sprintf(c->errcontext.tempErrStr, "plot style numbers must be in the range 0-%d", MAX_PLOTSTYLES-1); ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, NULL); return; }
    n = (int)floor(nd);
    outstyle = &(c->set->plot_styles[n]);
    ppl_withWordsFromDict(c, in, pl, PARSE_TABLE_set_style_numbered_, 0, &ww_tmp);
    ppl_withWordsMerge   (c, &ww_tmp2, &ww_tmp, outstyle, NULL, NULL, NULL, 0);
    ppl_withWordsDestroy (c, &ww_tmp);
    ppl_withWordsDestroy (c, outstyle);
    *outstyle = ww_tmp2;
   }
  else if ( (strcmp_unset && (strcmp(setoption,"style")==0)) || /* unset style */
            (strcmp_set && (strcmp(setoption,"nostyle")==0)) )
   {
    const int dataset_type = strcmp_set ? PARSE_set_nostyle_dataset_type : PARSE_unset_style_dataset_type;
    const int style_ids    = strcmp_set ? PARSE_set_nostyle_0style_ids : PARSE_unset_style_0style_ids;
    const int id_style_ids = strcmp_set ? PARSE_set_nostyle_id_0style_ids : PARSE_unset_style_id_0style_ids;

    if (command[dataset_type].objType==PPLOBJ_STR)
     {
      char      *type     = (char *)command[dataset_type].auxil;
      withWords *instyle  = (type[0]=='d') ? &c->set->graph_default.dataStyle : &c->set->graph_default.funcStyle;
      withWords *outstyle = (type[0]=='d') ?                   &sg->dataStyle :                   &sg->funcStyle;
      ppl_withWordsDestroy (c, outstyle);
      ppl_withWordsCpy     (c, outstyle, instyle);
     }
    else
     {
      int pos = style_ids;
      while (command[pos].objType == PPLOBJ_NUM)
       {
        double     nd;
        int        n;
        withWords *outstyle, *instyle;
        pos = (int)round(command[pos].real);
        if (pos<=0) break;
        nd = command[pos+id_style_ids].real;
        if ((nd<0)||(nd>=MAX_PLOTSTYLES)) { sprintf(c->errcontext.tempErrStr, "plot style numbers must be in the range 0-%d", MAX_PLOTSTYLES-1); ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, NULL); return; }
        n = (int)floor(nd);
        outstyle = &(c->set->plot_styles[n]);
        instyle  = &(c->set->plot_styles_default[n]);
        ppl_withWordsDestroy (c, outstyle);
        ppl_withWordsCpy     (c, outstyle, instyle);
       }
     }
   }
  else if (strcmp_set && (strcmp(setoption,"terminal")==0)) /* set terminal */
   {
    double tempdbl;
    char  *tempstr = (char *)command[PARSE_set_terminal_term].auxil;
    int    got     =        (command[PARSE_set_terminal_term].objType == PPLOBJ_STR);
    if (got) c->set->term_current.TermType        = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_TERMTYPE_INT, SW_TERMTYPE_STR);

    tempstr = (char *)command[PARSE_set_terminal_antiali].auxil;
    got     =        (command[PARSE_set_terminal_antiali].objType == PPLOBJ_STR);
    if (got) c->set->term_current.TermAntiAlias   = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_terminal_col].auxil;
    got     =        (command[PARSE_set_terminal_col].objType == PPLOBJ_STR);
    if (got) c->set->term_current.color           = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempdbl =  command[PARSE_set_terminal_dpi].real;
    got     = (command[PARSE_set_terminal_dpi].objType == PPLOBJ_NUM);
    if (got)
     {
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The DPI resolution supplied to the 'set terminal' command was not finite."); }
      else if (tempdbl <= 2.0)  { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "Output image resolutions below two dots per inch are not supported."); }
      else                      { c->set->term_current.dpi = tempdbl; }
     }

    tempstr = (char *)command[PARSE_set_terminal_enlarge].auxil;
    got     =        (command[PARSE_set_terminal_enlarge].objType == PPLOBJ_STR);
    if (got) c->set->term_current.TermEnlarge     = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_terminal_land].auxil;
    got     =        (command[PARSE_set_terminal_land].objType == PPLOBJ_STR);
    if (got) c->set->term_current.landscape       = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_terminal_trans].auxil;
    got     =        (command[PARSE_set_terminal_trans].objType == PPLOBJ_STR);
    if (got) c->set->term_current.TermTransparent = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_terminal_invert].auxil;
    got     =        (command[PARSE_set_terminal_invert].objType == PPLOBJ_STR);
    if (got) c->set->term_current.TermInvert      = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);
   }
  else if (strcmp_unset && (strcmp(setoption,"terminal")==0)) /* unset terminal */
   {
    c->set->term_current.color          = c->set->term_default.color;
    c->set->term_current.dpi            = c->set->term_default.dpi;
    c->set->term_current.landscape      = c->set->term_default.landscape;
    c->set->term_current.TermAntiAlias  = c->set->term_default.TermAntiAlias;
    c->set->term_current.TermType       = c->set->term_default.TermType;
    c->set->term_current.TermEnlarge    = c->set->term_default.TermEnlarge;
    c->set->term_current.TermInvert     = c->set->term_default.TermInvert;
    c->set->term_current.TermTransparent= c->set->term_default.TermTransparent;
   }
  else if (strcmp_set && (strcmp(setoption,"texthalign")==0)) /* set texthalign */
   {
    if (command[PARSE_set_texthalign_left  ].objType == PPLOBJ_STR) sg->TextHAlign = SW_HALIGN_LEFT;
    if (command[PARSE_set_texthalign_center].objType == PPLOBJ_STR) sg->TextHAlign = SW_HALIGN_CENT;
    if (command[PARSE_set_texthalign_right ].objType == PPLOBJ_STR) sg->TextHAlign = SW_HALIGN_RIGHT;
   }
  else if (strcmp_unset && (strcmp(setoption,"texthalign")==0)) /* unset texthalign */
   {
    sg->TextHAlign = c->set->graph_default.TextHAlign;
   }
  else if (strcmp_set && (strcmp(setoption,"textvalign")==0)) /* set textvalign */
   {
    if (command[PARSE_set_textvalign_top   ].objType == PPLOBJ_STR) sg->TextVAlign = SW_VALIGN_TOP;
    if (command[PARSE_set_textvalign_center].objType == PPLOBJ_STR) sg->TextVAlign = SW_VALIGN_CENT;
    if (command[PARSE_set_textvalign_bottom].objType == PPLOBJ_STR) sg->TextVAlign = SW_VALIGN_BOT;
   }
  else if (strcmp_unset && (strcmp(setoption,"textvalign")==0)) /* unset textvalign */
   {
    sg->TextVAlign = c->set->graph_default.TextVAlign;
   }
  else if (strcmp_set && (strcmp(setoption,"tics")==0)) /* set tics */
   {
    int        m    = (command[PARSE_set_tics_minor].objType==PPLOBJ_STR);
    const int *ptab = PARSE_TABLE_set_tics_;

    if (command[PARSE_unset_tics_axis].objType==PPLOBJ_NUM)
     {
      int  i   = (int)round(command[PARSE_unset_tics_axis].real);
      int  j   = (int)round(command[PARSE_unset_tics_axis].exponent[0]);
      char cmd[16];
      sprintf(cmd, "%s%c%d", m?"m":"", 'x'+j, i);
      if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
       {
        int *alog;
        pplset_axis *a;
        if      (j==1) a=&ya[i];
        else if (j==2) a=&za[i];
        else           a=&xa[i];
        GET_AXISLOG(alog,a,0);
        if (!m) { tics_rm(&a->tics ); SET_TICKS(a->tics ,a->ticsM,a->unit,((a->MinSet==SW_BOOL_TRUE)||(a->MaxSet==SW_BOOL_TRUE)),(*alog==SW_BOOL_TRUE)); }
        else    { tics_rm(&a->ticsM); SET_TICKS(a->ticsM,a->tics ,a->unit,((a->MinSet==SW_BOOL_TRUE)||(a->MaxSet==SW_BOOL_TRUE)),(*alog==SW_BOOL_TRUE)); }
       }
     }
    else
     {
      int i;
      char *cmd = m ? "mtics" : "tics";
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a = &xa[i]; if (a->enabled)
       {
        int *alog; GET_AXISLOG(alog,a,0);
        if (!m) { tics_rm(&a->tics ); SET_TICKS(a->tics ,a->ticsM,a->unit,((a->MinSet==SW_BOOL_TRUE)||(a->MaxSet==SW_BOOL_TRUE)),(*alog==SW_BOOL_TRUE)); }
        else    { tics_rm(&a->ticsM); SET_TICKS(a->ticsM,a->tics ,a->unit,((a->MinSet==SW_BOOL_TRUE)||(a->MaxSet==SW_BOOL_TRUE)),(*alog==SW_BOOL_TRUE)); }
       }}
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a = &ya[i]; if (a->enabled)
       {
        int *alog; GET_AXISLOG(alog,a,0);
        if (!m) { tics_rm(&a->tics ); SET_TICKS(a->tics ,a->ticsM,a->unit,((a->MinSet==SW_BOOL_TRUE)||(a->MaxSet==SW_BOOL_TRUE)),(*alog==SW_BOOL_TRUE)); }
        else    { tics_rm(&a->ticsM); SET_TICKS(a->ticsM,a->tics ,a->unit,((a->MinSet==SW_BOOL_TRUE)||(a->MaxSet==SW_BOOL_TRUE)),(*alog==SW_BOOL_TRUE)); }
       }}
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a = &za[i]; if (a->enabled)
       {
        int *alog; GET_AXISLOG(alog,a,0);
        if (!m) { tics_rm(&a->tics ); SET_TICKS(a->tics ,a->ticsM,a->unit,((a->MinSet==SW_BOOL_TRUE)||(a->MaxSet==SW_BOOL_TRUE)),(*alog==SW_BOOL_TRUE)); }
        else    { tics_rm(&a->ticsM); SET_TICKS(a->ticsM,a->tics ,a->unit,((a->MinSet==SW_BOOL_TRUE)||(a->MaxSet==SW_BOOL_TRUE)),(*alog==SW_BOOL_TRUE)); }
       }}
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"tics")==0)) /* unset tics */
   {
    int m = (command[PARSE_unset_tics_minor].objType==PPLOBJ_STR);

#define IDE if (ppl_unitsDimEqual(&a->unit , &ad->unit))

    if (command[PARSE_unset_tics_axis].objType==PPLOBJ_NUM)
     {
      int i = (int)round(command[PARSE_unset_tics_axis].real);
      int j = (int)round(command[PARSE_unset_tics_axis].exponent[0]);
      if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
       {
        if      (j==1) { pplset_axis *a=&ya[i], *ad=&c->set->YAxesDefault[i]; if (!m) { tics_rm(&a->tics); IDE tics_cp(&a->tics,&ad->tics); } tics_rm(&a->ticsM); IDE tics_cp(&a->ticsM,&ad->ticsM); }
        else if (j==2) { pplset_axis *a=&za[i], *ad=&c->set->ZAxesDefault[i]; if (!m) { tics_rm(&a->tics); IDE tics_cp(&a->tics,&ad->tics); } tics_rm(&a->ticsM); IDE tics_cp(&a->ticsM,&ad->ticsM); }
        else           { pplset_axis *a=&xa[i], *ad=&c->set->XAxesDefault[i]; if (!m) { tics_rm(&a->tics); IDE tics_cp(&a->tics,&ad->tics); } tics_rm(&a->ticsM); IDE tics_cp(&a->ticsM,&ad->ticsM); }
       }
     }
    else
     {
      int i;
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a=&xa[i], *ad=&c->set->XAxesDefault[i]; if (a->enabled)
         { if (!m) { tics_rm(&a->tics); IDE tics_cp(&a->tics,&ad->tics); } tics_rm(&a->ticsM); IDE tics_cp(&a->ticsM,&ad->ticsM); } }
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a=&ya[i], *ad=&c->set->YAxesDefault[i]; if (a->enabled)
         { if (!m) { tics_rm(&a->tics); IDE tics_cp(&a->tics,&ad->tics); } tics_rm(&a->ticsM); IDE tics_cp(&a->ticsM,&ad->ticsM); } }
      for (i=0; i<MAX_AXES; i++) { pplset_axis *a=&za[i], *ad=&c->set->ZAxesDefault[i]; if (a->enabled)
         { if (!m) { tics_rm(&a->tics); IDE tics_cp(&a->tics,&ad->tics); } tics_rm(&a->ticsM); IDE tics_cp(&a->ticsM,&ad->ticsM); } }
     }
   }
  else if (strcmp_set && (strcmp(setoption,"title")==0)) /* set title */
   {
    if (command[PARSE_set_title_title].objType==PPLOBJ_STR)
     {
      char *tempstr = (char *)command[PARSE_set_title_title].auxil;
      strncpy(sg->title, tempstr, FNAME_LENGTH-1);
      sg->title[FNAME_LENGTH-1]='\0';
     }
    if (command[PARSE_set_title_offset  ].objType==PPLOBJ_NUM) sg->TitleXOff.real = command[PARSE_set_title_offset  ].real;
    if (command[PARSE_set_title_offset+1].objType==PPLOBJ_NUM) sg->TitleYOff.real = command[PARSE_set_title_offset+1].real;
   }
  else if (strcmp_unset && (strcmp(setoption,"title")==0)) /* unset title */
   {
    strncpy(sg->title, c->set->graph_default.title, FNAME_LENGTH-1);
    sg->title[FNAME_LENGTH-1]='\0';
    sg->TitleXOff = c->set->graph_default.TitleXOff;
    sg->TitleYOff = c->set->graph_default.TitleYOff;
   }
  else if (strcmp_set && (strcmp(setoption,"trange")==0)) /* set trange */
   {
    if (command[PARSE_set_trange_range].objType==PPLOBJ_STR)
     {
      pplObj *min  = &command[PARSE_set_trange_min];
      pplObj *max  = &command[PARSE_set_trange_max];
      if ((min->objType!=PPLOBJ_NUM)) min = &sg->Tmin;
      if ((max->objType!=PPLOBJ_NUM)) max = &sg->Tmax;
      if ((!gsl_finite(min->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set trange' command had non-finite limits."); return; }
      if ((!gsl_finite(max->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set trange' command had non-finite limits."); return; }
      if ((!ppl_unitsDimEqual(min,max))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "Attempt to set trange with dimensionally incompatible minimum and maximum."); return; }
      sg->Tmin = *min;
      sg->Tmax = *max;
     }
    if (command[PARSE_set_trange_reverse].objType==PPLOBJ_STR)
     {
      pplObj tmp = sg->Tmin;
      sg->Tmin   = sg->Tmax;
      sg->Tmax   = tmp;
     }
    sg->USE_T_or_uv = 1;
   }
  else if (strcmp_unset && (strcmp(setoption,"trange")==0)) /* unset trange */
   {
    sg->USE_T_or_uv = c->set->graph_default.USE_T_or_uv;
    sg->Tmin        = c->set->graph_default.Tmin;
    sg->Tmax        = c->set->graph_default.Tmax;
   }
  else if (strcmp_set && (strcmp(setoption,"unit")==0)) /* set unit */
   {
    int got, got2, got3;
    char *tempstr, *tempstr2=NULL, *tempstr3=NULL;

    tempstr = (char *)command[PARSE_set_unit_abbrev].auxil;
    got     =        (command[PARSE_set_unit_abbrev].objType == PPLOBJ_STR);
    if (got) c->set->term_current.UnitDisplayAbbrev   = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_unit_angle].auxil;
    got     =        (command[PARSE_set_unit_angle].objType == PPLOBJ_STR);
    if (got) c->set->term_current.UnitAngleDimless    = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_unit_prefix].auxil;
    got     =        (command[PARSE_set_unit_prefix].objType == PPLOBJ_STR);
    if (got) c->set->term_current.UnitDisplayPrefix   = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_unit_scheme].auxil;
    got     =        (command[PARSE_set_unit_scheme].objType == PPLOBJ_STR);
    if (got) c->set->term_current.UnitScheme          = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_UNITSCH_INT, SW_UNITSCH_STR);

    got2               =            (command[PARSE_set_unit_preferred_unit].objType == PPLOBJ_EXP);
    if (got2) tempstr2 = ((pplExpr *)command[PARSE_set_unit_preferred_unit].auxil)->ascii;
    got3               =            (command[PARSE_set_unit_unpreferred_unit].objType == PPLOBJ_EXP);
    if (got3) tempstr3 = ((pplExpr *)command[PARSE_set_unit_unpreferred_unit].auxil)->ascii;

    if (got2 || got3)
     {
      int i;
      char *buf = (char *)ppl_memAlloc(LSTR_LENGTH);
      PreferredUnit *pu, *pui;
      if (buf==NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1, "Out of memory."); }
      else
       for (i=0; i<2; i++)
        {
         int errpos=-1;
         listIterator *listiter;
         if (i==0) { if (!got2) continue; tempstr=tempstr2; }
         else      { if (!got3) continue; tempstr=tempstr3; }
         ppl_newPreferredUnit(c, &pu, tempstr, 0, &errpos, buf);
         if (errpos>=0) { ppl_error(&c->errcontext, ERR_NUMERICAL,-1,-1,buf); continue; }

         // Remove any preferred unit which is dimensionally equal to new preferred unit
         listiter = ppl_listIterateInit(c->unit_PreferredUnits);
         while (listiter != NULL)
          {
           pui = (PreferredUnit *)listiter->data;
           ppl_listIterate(&listiter);
           if (ppl_unitsDimEqual(&pui->value , &pu->value) && (pui->value.tempType == pu->value.tempType))
             ppl_listRemove(c->unit_PreferredUnits, (void *)pui );
          }

         // Add new preferred unit
         if (i==0) ppl_listAppendCpy(c->unit_PreferredUnits, (void *)pu, sizeof(PreferredUnit));
        }
     }

   // set unit of length meter
    {
     int pos = PARSE_set_unit_preferred_units;
     while (command[pos].objType == PPLOBJ_NUM)
      {
       int i=0, j=0, k=0, l=0, m=0, multiplier;
       int p=0;
       int pp=0;
       char *quantity, *unit;
       pos = (int)round(command[pos].real);
       if (pos<=0) break;
       quantity = (char *)command[pos+PARSE_set_unit_quantity_preferred_units].auxil;
       unit     = (char *)command[pos+PARSE_set_unit_unit_preferred_units].auxil;
       for (j=0; j<c->unit_pos; j++)
        {
         if (i>1) i=1;
         if ((c->unit_database[j].quantity != NULL) && (ppl_strCmpNoCase(c->unit_database[j].quantity, quantity) == 0))
          {
           i=2;
           c->unit_database[j].userSel = 0;
          }

         if (pp!=0) continue;
         multiplier = 8;
         if      ((k = ppl_unitNameCmp(unit, c->unit_database[j].nameAp,1))!=0) p=1;
         else if ((k = ppl_unitNameCmp(unit, c->unit_database[j].nameAs,1))!=0) p=1;
         else if ((k = ppl_unitNameCmp(unit, c->unit_database[j].nameFp,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(unit, c->unit_database[j].nameFs,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(unit, c->unit_database[j].nameFp,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(unit, c->unit_database[j].alt1  ,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(unit, c->unit_database[j].alt2  ,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(unit, c->unit_database[j].alt3  ,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(unit, c->unit_database[j].alt4  ,0))!=0) p=1;
         else
          {
           for (l=c->unit_database[j].minPrefix/3+8; l<=c->unit_database[j].maxPrefix/3+8; l++)
            {
             if (l==8) continue;
             for (k=0; ((SIprefixes_full[l][k]!='\0') && (toupper(SIprefixes_full[l][k])==toupper(unit[k]))); k++);
             if (SIprefixes_full[l][k]=='\0')
              {
               if      ((m = ppl_unitNameCmp(unit+k, c->unit_database[j].nameFp,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(unit+k, c->unit_database[j].nameFs,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(unit+k, c->unit_database[j].alt1  ,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(unit+k, c->unit_database[j].alt2  ,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(unit+k, c->unit_database[j].alt3  ,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(unit+k, c->unit_database[j].alt4  ,0))!=0) { p=1; k+=m; multiplier=l; break; }
              }
             for (k=0; ((SIprefixes_abbrev[l][k]!='\0') && (SIprefixes_abbrev[l][k]==unit[k])); k++);
             if (SIprefixes_abbrev[l][k]=='\0')
              {
               if      ((m = ppl_unitNameCmp(unit+k, c->unit_database[j].nameAp,1))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(unit+k, c->unit_database[j].nameAs,1))!=0) { p=1; k+=m; multiplier=l; break; }
              }
            }
          }
         if (p==0) continue;
         if (i!=2)
          {
           if ((c->unit_database[j].quantity!=NULL) && (c->unit_database[j].quantity[0]!='\0'))
            { sprintf(c->errcontext.tempErrStr, "'%s' is not a unit of '%s', but of '%s'.", unit, quantity, c->unit_database[j].quantity); ppl_error(&c->errcontext,ERR_GENERIC,-1,-1,NULL); }
           else
            { sprintf(c->errcontext.tempErrStr, "'%s' is not a unit of '%s'.", unit, quantity); ppl_error(&c->errcontext,ERR_GENERIC,-1,-1,NULL); }
          }
         c->unit_database[j].userSel = 1;
         c->unit_database[j].userSelPrefix = multiplier;
         pp=1;
        }
       if (i==0) { sprintf(c->errcontext.tempErrStr, "No such quantity as a '%s'.", quantity); ppl_error(&c->errcontext,ERR_GENERIC,-1,-1,NULL); }
       if (p==0) { sprintf(c->errcontext.tempErrStr, "No such unit as a '%s'.", unit); ppl_error(&c->errcontext,ERR_GENERIC,-1,-1,NULL); }
      }
    }
   }
  else if (strcmp_unset && (strcmp(setoption,"unit")==0)) /* unset unit */
   {
    c->set->term_current.UnitAngleDimless    = c->set->term_default.UnitAngleDimless;
    c->set->term_current.UnitDisplayAbbrev   = c->set->term_default.UnitDisplayAbbrev;
    c->set->term_current.UnitDisplayPrefix   = c->set->term_default.UnitDisplayPrefix;
    c->set->term_current.UnitScheme          = c->set->term_default.UnitScheme;
    for (i=0; i<c->unit_pos; i++) c->unit_database[i].userSel = 0;
    ppl_listFree(c->unit_PreferredUnits);
    c->unit_PreferredUnits = ppl_listCpy(c->unit_PreferredUnits_default, 1, sizeof(PreferredUnit));
   }
  else if (strcmp_unset && (strcmp(setoption,"unit_angle")==0)) /* unset unit angle */
   {
    c->set->term_current.UnitAngleDimless    = c->set->term_default.UnitAngleDimless;
   }
  else if (strcmp_unset && (strcmp(setoption,"unit_display")==0)) /* unset unit display */
   {
    c->set->term_current.UnitDisplayAbbrev   = c->set->term_default.UnitDisplayAbbrev;
    c->set->term_current.UnitDisplayPrefix   = c->set->term_default.UnitDisplayPrefix;
   }
  else if (strcmp_unset && (strcmp(setoption,"unit_scheme")==0)) /* unset unit scheme */
   {
    c->set->term_current.UnitScheme          = c->set->term_default.UnitScheme;
   }
  else if (strcmp_set && (strcmp(setoption,"urange")==0)) /* set urange */
   {
    if (command[PARSE_set_urange_range].objType==PPLOBJ_STR)
     {
      pplObj *min  = &command[PARSE_set_urange_min];
      pplObj *max  = &command[PARSE_set_urange_max];
      if ((min->objType!=PPLOBJ_NUM)) min = &sg->Umin;
      if ((max->objType!=PPLOBJ_NUM)) max = &sg->Umax;
      if ((!gsl_finite(min->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set urange' command had non-finite limits."); return; }
      if ((!gsl_finite(max->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set urange' command had non-finite limits."); return; }
      if ((!ppl_unitsDimEqual(min,max))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "Attempt to set urange with dimensionally incompatible minimum and maximum."); return; }
      sg->Umin = *min;
      sg->Umax = *max;
     }
    if (command[PARSE_set_urange_reverse].objType==PPLOBJ_STR)
     {
      pplObj tmp = sg->Umin;
      sg->Umin   = sg->Umax;
      sg->Umax   = tmp;
     }
    sg->USE_T_or_uv = 0;
   }
  else if (strcmp_unset && (strcmp(setoption,"urange")==0)) /* unset urange */
   {
    sg->USE_T_or_uv = c->set->graph_default.USE_T_or_uv;
    sg->Umin        = c->set->graph_default.Umin;
    sg->Umax        = c->set->graph_default.Umax;
   }
  else if (strcmp_set && (strcmp(setoption,"vrange")==0)) /* set vrange */
   {
    if (command[PARSE_set_vrange_range].objType==PPLOBJ_STR)
     {
      pplObj *min  = &command[PARSE_set_vrange_min];
      pplObj *max  = &command[PARSE_set_vrange_max];
      if ((min->objType!=PPLOBJ_NUM)) min = &sg->Tmin;
      if ((max->objType!=PPLOBJ_NUM)) max = &sg->Tmax;
      if ((!gsl_finite(min->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set vrange' command had non-finite limits."); return; }
      if ((!gsl_finite(max->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set vrange' command had non-finite limits."); return; }
      if ((!ppl_unitsDimEqual(min,max))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "Attempt to set vrange with dimensionally incompatible minimum and maximum."); return; }
      sg->Vmin = *min;
      sg->Vmax = *max;
     }
    if (command[PARSE_set_vrange_reverse].objType==PPLOBJ_STR)
     {
      pplObj tmp = sg->Vmin;
      sg->Vmin   = sg->Vmax;
      sg->Vmax   = tmp;
     }
    sg->USE_T_or_uv = 0;
   }
  else if (strcmp_unset && (strcmp(setoption,"vrange")==0)) /* unset vrange */
   {
    sg->USE_T_or_uv = c->set->graph_default.USE_T_or_uv;
    sg->Vmin        = c->set->graph_default.Vmin;
    sg->Vmax        = c->set->graph_default.Vmax;
   }
  else if (strcmp_set && (strcmp(setoption,"view")==0)) /* set view */
   {
    if (command[PARSE_set_view_xy_angle].objType==PPLOBJ_NUM)
     {
      double d=command[PARSE_set_view_xy_angle].real;
      if (!gsl_finite(d)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The viewing angles supplied to the 'set view' command were not finite."); return; }
      sg->XYview.real = d;
     }
    if (command[PARSE_set_view_yz_angle].objType==PPLOBJ_NUM)
     {
      double d=command[PARSE_set_view_yz_angle].real;
      if (!gsl_finite(d)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The viewing angles supplied to the 'set view' command were not finite."); return; }
      sg->YZview.real = d;
     }
    sg->XYview.real = fmod(sg->XYview.real , 2*M_PI);
    sg->YZview.real = fmod(sg->YZview.real , 2*M_PI);
    while (sg->XYview.real < 0.0) sg->XYview.real += 2*M_PI;
    while (sg->YZview.real < 0.0) sg->YZview.real += 2*M_PI;
   }
  else if (strcmp_unset && (strcmp(setoption,"view")==0)) /* unset view */
   {
    sg->XYview.real = c->set->graph_default.XYview.real;
    sg->YZview.real = c->set->graph_default.YZview.real;
   }
  else if (strcmp_set && (strcmp(setoption,"width")==0)) /* set width */
   {
    if (command[PARSE_set_width_width].objType==PPLOBJ_NUM)
     {
      if (!gsl_finite(command[PARSE_set_width_width].real)) { ppl_error(&c->errcontext, ERR_GENERIC, -1, -1, "The width supplied to the 'set width' command was not finite."); return; }
      sg->width.real=command[PARSE_set_width_width].real;
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"width")==0)) /* unset width */
   {
    sg->width.real = c->set->graph_default.width.real;
   }
  else if (strcmp_set && (strcmp(setoption,"viewer")==0)) /* set viewer */
   {
    int changedViewer = 0;
    if (command[PARSE_set_viewer_auto_viewer].objType == PPLOBJ_STR)
     {
      int viewerOld = c->set->term_current.viewer;
      if      (strcmp(GHOSTVIEW_COMMAND, "/bin/false")!=0) c->set->term_current.viewer = SW_VIEWER_GV;
      else if (strcmp(GGV_COMMAND      , "/bin/false")!=0) c->set->term_current.viewer = SW_VIEWER_GGV;
      else                                                 c->set->term_current.viewer = SW_VIEWER_NULL;
      changedViewer = (viewerOld != c->set->term_current.viewer);
     }
    else
     {
      char *nv = (char *)command[PARSE_set_viewer_viewer].auxil;
      if (c->set->term_current.viewer != SW_VIEWER_CUSTOM) { changedViewer=1; c->set->term_current.viewer = SW_VIEWER_CUSTOM; }
      if (strcmp(c->set->term_current.ViewerCmd, nv)!=0)   { changedViewer=1; snprintf(c->set->term_current.ViewerCmd, FNAME_LENGTH, "%s", nv); }
      c->set->term_current.ViewerCmd[FNAME_LENGTH-1]='\0';
     }
    if (changedViewer) pplcsp_sendCommand(c,"A\n"); // Clear away SingleWindow viewer
   }
  else if (strcmp_unset && (strcmp(setoption,"viewer")==0)) /* unset viewer */
   {
    int changedViewer = ( (c->set->term_current.viewer != c->set->term_default.viewer) || (strcmp(c->set->term_current.ViewerCmd,c->set->term_default.ViewerCmd)!=0) );
    c->set->term_current.viewer = c->set->term_default.viewer;
    strcpy(c->set->term_current.ViewerCmd, c->set->term_default.ViewerCmd);
    if (changedViewer) pplcsp_sendCommand(c,"A\n"); // Clear away SingleWindow viewer
   }
  else if (strcmp(setoption,"xformat")==0) /* set xformat | unset xformat */
   {
    int i,j;
    if (strcmp_set) { i = (int)round(command[PARSE_set_xformat_axis  ].real); j = (int)round(command[PARSE_set_xformat_axis  ].exponent[0]); }
    else            { i = (int)round(command[PARSE_unset_xformat_axis].real); j = (int)round(command[PARSE_unset_xformat_axis].exponent[0]); }
    if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
     {
      pplset_axis *a, *ad;
      if      (j==1) { a = &ya[i]; ad = &c->set->YAxesDefault[i]; }
      else if (j==2) { a = &za[i]; ad = &c->set->ZAxesDefault[i]; }
      else           { a = &xa[i]; ad = &c->set->XAxesDefault[i]; }

      if (strcmp_set)
       {
        a->enabled = 1;
        if (command[PARSE_set_xformat_format_string].objType == PPLOBJ_EXP)
         {
          if (a->format != NULL) pplExpr_free((pplExpr *)a->format);
          a->format = (void *)pplExpr_cpy((pplExpr *)command[PARSE_set_xformat_format_string].auxil);
         }
        if (command[PARSE_set_xformat_orient].objType == PPLOBJ_STR)
         {
          a->TickLabelRotation = ppl_fetchSettingByName(&c->errcontext, (char *)command[PARSE_set_xformat_orient].auxil, SW_TICLABDIR_INT, SW_TICLABDIR_STR);
          a->TickLabelRotate   = ad->TickLabelRotate;
         }
        if (command[PARSE_set_xformat_rotation].objType == PPLOBJ_NUM)
         {
          double r = command[PARSE_set_xformat_rotation].real;
          if (!gsl_finite(r)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The rotation angle supplied to the 'set format' command was not finite."); return; }
          a->TickLabelRotate = r; // TickLabelRotation will already have been set by "rotate" keyword mapping to "orient"
         }
       }
      else
       {
        if (a->format != NULL) { pplExpr_free((pplExpr *)a->format); a->format=NULL; }
        a->format = (void *)pplExpr_cpy((pplExpr *)ad->format);
        a->TickLabelRotation = ad->TickLabelRotation;
        a->TickLabelRotate   = ad->TickLabelRotate;
       }
     }
   }
  else if (strcmp(setoption,"xlabel")==0) /* set xlabel */
   {
    int i = (int)round(command[strcmp_set ? PARSE_set_xlabel_axis : PARSE_unset_xlabel_axis].real);
    int j = (int)round(command[strcmp_set ? PARSE_set_xlabel_axis : PARSE_unset_xlabel_axis].exponent[0]);
    if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
     {
      pplset_axis *a;
      if      (j==1) a = &ya[i];
      else if (j==2) a = &za[i];
      else           a = &xa[i];

      if (strcmp_set)
       {
        a->enabled = 1;
        if (command[PARSE_set_xlabel_label_text].objType == PPLOBJ_STR)
         {
          char *in = (char *)command[PARSE_set_xlabel_label_text].auxil;
          char *l  = (char *)malloc(strlen(in)+1);
          if (l!=NULL) strcpy(l,in);
          if (a->label!=NULL) free(a->label);
          a->label=l;
         }
        if (command[PARSE_set_xlabel_rotation].objType == PPLOBJ_NUM)
         {
          double r = command[PARSE_set_xlabel_rotation].real;
          if (!gsl_finite(r)) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The rotation angle supplied to the set axis label command was not finite."); return; }
          a->LabelRotate = r;
         }
       }
      else
       {
        char *in = c->set->axis_default.label;
        char *l  = (in==NULL) ? NULL : (char *)malloc(strlen(in)+1);
        if (l!=NULL) strcpy(l,in);
        if (a->label!=NULL) free(a->label);
        a->label=l;
        a->LabelRotate = c->set->axis_default.LabelRotate;
       }
     }
   }
  else if (strcmp(setoption,"range")==0) /* set xrange | unset xrange */
   {
    int i,j;
    if (strcmp_set) { i = (int)round(command[PARSE_set_range_axis  ].real); j = (int)round(command[PARSE_set_range_axis  ].exponent[0]); }
    else            { i = (int)round(command[PARSE_unset_range_axis].real); j = (int)round(command[PARSE_unset_range_axis].exponent[0]); }
    if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
     {
      pplset_axis *a, *ad;
      pplObj      *unitNew, unitOld;
      if      (j==1) { a = &ya[i]; ad = &c->set->YAxesDefault[i]; }
      else if (j==2) { a = &za[i]; ad = &c->set->ZAxesDefault[i]; }
      else           { a = &xa[i]; ad = &c->set->XAxesDefault[i]; }
      unitOld = a->unit;

      if (strcmp_set)
       {
        pplObj *min  = &command[PARSE_set_range_min], d1, d2;
        pplObj *max  = &command[PARSE_set_range_max];
        int     mina = (command[PARSE_set_range_minauto].objType == PPLOBJ_STR);
        int     maxa = (command[PARSE_set_range_maxauto].objType == PPLOBJ_STR);
        if (command[PARSE_set_range_reverse      ].objType == PPLOBJ_STR) a->RangeReversed = 1;
        if (command[PARSE_set_range_noreverse    ].objType == PPLOBJ_STR) a->RangeReversed = 0;
        if ((!mina)&&(min->objType!=PPLOBJ_NUM)&&(a->MinSet==SW_BOOL_TRUE)) { min=&d1; d1=a->unit; d1.real=a->min; }
        if ((!maxa)&&(max->objType!=PPLOBJ_NUM)&&(a->MaxSet==SW_BOOL_TRUE)) { max=&d2; d2=a->unit; d2.real=a->max; }
        if ((min->objType==PPLOBJ_NUM)&&(!gsl_finite(min->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set range' command had non-finite limits."); return; }
        if ((max->objType==PPLOBJ_NUM)&&(!gsl_finite(max->real))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "The range supplied to the 'set range' command had non-finite limits."); return; }
        if ((min->objType==PPLOBJ_NUM)&&(max->objType==PPLOBJ_NUM)&&(!ppl_unitsDimEqual(min,max))) { ppl_error(&c->errcontext, ERR_NUMERICAL, -1, -1, "Attempt to set range with dimensionally incompatible minimum and maximum."); return; }
        if (min->objType==PPLOBJ_NUM) { a->unit = *min; a->min = min->real; a->MinSet = SW_BOOL_TRUE; }
        if (max->objType==PPLOBJ_NUM) { a->unit = *max; a->max = max->real; a->MaxSet = SW_BOOL_TRUE; }
        if (mina) { a->MinSet = SW_BOOL_FALSE; a->min = ad->min; }
        if (maxa) { a->MaxSet = SW_BOOL_FALSE; a->max = ad->max; }
        a->enabled = 1;
       }
      else
       {
        a->unit          = ad->unit;
        a->max           = ad->max;
        a->MaxSet        = ad->MaxSet;
        a->min           = ad->min;
        a->MinSet        = ad->MinSet;
        a->RangeReversed = ad->RangeReversed;
       }

      // Check whether ticks need removing, if units of axis have changed
      unitNew = &a->unit;
      if (!ppl_unitsDimEqual(unitNew,&unitOld)) { tics_rm(&a->tics); tics_rm(&a->ticsM); }
     }
   }
  else
   {
    sprintf(c->errcontext.tempErrStr, "Pyxplot's set command could not find handler for the set option <%s>.", setoption);
    ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, NULL);
   }

  return;
 }

