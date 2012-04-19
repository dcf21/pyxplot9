// set.c
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

#include "expressions/expCompile_fns.h"
#include "expressions/traceback_fns.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "settings/arrows_fns.h"
#include "settings/axes_fns.h"
#include "settings/colors.h"
#include "settings/epsColors.h"
#include "settings/labels_fns.h"
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
#include "pplConstants.h"

#define TBADD(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

void directive_set(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
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

    if ((editNo < 1) || (editNo>MULTIPLOT_MAXINDEX) || (canvas_items == NULL)) {sprintf(c->errcontext.tempErrStr, "No multiplot item with index %d.", editNo); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return;}
    ptr = canvas_items->first;
    for (i=1; i<editNo; i++)
     {
      if (ptr==NULL) break;
      ptr=ptr->next;
     }
    if (ptr == NULL) { sprintf(c->errcontext.tempErrStr, "No multiplot item with index %d.", editNo); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return; }

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
         int i = (int)round(command[pos+PARSE_set_axis_axis_axes].real);
         int j = (int)round(command[pos+PARSE_set_axis_axis_axes].exponent[0]);
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
    int pos = PARSE_unset_axis_axes;
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
        if      (i==1) { pplaxis_destroy(c, &(ya[i]) ); pplaxis_copy(c, &(ya[i]), &(c->set->YAxesDefault[i])); }
        else if (i==2) { pplaxis_destroy(c, &(za[i]) ); pplaxis_copy(c, &(za[i]), &(c->set->ZAxesDefault[i])); }
        else           { pplaxis_destroy(c, &(xa[i]) ); pplaxis_copy(c, &(xa[i]), &(c->set->XAxesDefault[i])); }
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
    if (gotbarsize && !gsl_finite(barsize)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The value supplied to the 'set bar' command was not finite."); return; }
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
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The value supplied to the 'set binorigin' command was not finite."); return; }
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
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The value supplied to the 'set binwidth' command was not finite."); return; }
      if (tempdbl<=0) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "Width of histogram bins must be greater than zero."); return; }
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
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The value supplied to the 'set boxfrom' command was not finite."); return; }
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
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The value supplied to the 'set boxwidth' command was not finite."); return; }
      sg->BoxWidthAuto = 0;
      sg->BoxWidth = command[PARSE_set_boxwidth_box_width];
     }
   }
  else if (strcmp_unset && (strcmp(setoption,"boxwidth")==0)) /* unset boxwidth */
   {
    sg->BoxWidth     = c->set->graph_default.BoxWidth;
    sg->BoxWidthAuto = c->set->graph_default.BoxWidthAuto;
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
    char *tempstr3 = (char *)malloc(strlen(tempstr2));
    pplObj val;
    if (tempstr3==NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1, "Out of memory."); return; }
    strcpy(tempstr3, tempstr2);
    val.refCount=1;
    pplObjStr(&val,1,1,tempstr3);
    ppl_dictAppendCpy(c->set->filters,tempstr,&val,sizeof(pplObj));
   }
  else if (strcmp_unset && (strcmp(setoption,"filter")==0)) /* unset filter */
   {
    char *tempstr   = (char *)command[PARSE_set_filter_filename].auxil;
    pplObj *tempobj = (pplObj *)ppl_dictLookup(c->set->filters,tempstr);
    if (tempobj == NULL) { ppl_warning(&c->errcontext, ERR_GENERAL, "Attempt to unset a filter which did not exist."); return; }
    ppl_garbageObject(tempobj);
    ppl_dictRemoveKey(c->set->filters,tempstr);
   }
  else if (strcmp_set && (strcmp(setoption,"fontsize")==0)) /* set fontsize */
   {
    double tempdbl = command[PARSE_set_fontsize_fontsize].real;
    if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The value supplied to the 'set fontsize' command was not finite."); return; }
    if (tempdbl <= 0.0) { ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Font sizes are not allowed to be less than or equal to zero."); return; }
    sg->FontSize = tempdbl;
   }
  else if (strcmp_unset && (strcmp(setoption,"fontsize")==0)) /* unset fontsize */
   {
    sg->FontSize = c->set->graph_default.FontSize;
   }
  else if ((strcmp(setoption,"axescolour")==0) || (strcmp(setoption,"gridmajcolour")==0) || (strcmp(setoption,"gridmincolour")==0) || (strcmp(setoption,"textcolour")==0)) /* set axescolour | set gridmajcolour | set gridmincolour */
   {
    if (strcmp_unset)
     {
      if (strcmp(setoption,"axescolour"   )==0) { sg->AxesColour=c->set->graph_default.AxesColour; sg->AxesCol1234Space=c->set->graph_default.AxesCol1234Space; sg->AxesColour1=c->set->graph_default.AxesColour1; sg->AxesColour2=c->set->graph_default.AxesColour2; sg->AxesColour3=c->set->graph_default.AxesColour3; sg->AxesColour4=c->set->graph_default.AxesColour4; }
      if (strcmp(setoption,"gridmajcolour")==0) { sg->GridMajColour=c->set->graph_default.GridMajColour; sg->GridMajCol1234Space=c->set->graph_default.GridMajCol1234Space; sg->GridMajColour1=c->set->graph_default.GridMajColour1; sg->GridMajColour2=c->set->graph_default.GridMajColour2; sg->GridMajColour3=c->set->graph_default.GridMajColour3; sg->GridMajColour4=c->set->graph_default.GridMajColour4; }
      if (strcmp(setoption,"gridmincolour")==0) { sg->GridMinColour=c->set->graph_default.GridMinColour; sg->GridMinCol1234Space=c->set->graph_default.GridMinCol1234Space; sg->GridMinColour1=c->set->graph_default.GridMinColour1; sg->GridMinColour2=c->set->graph_default.GridMinColour2; sg->GridMinColour3=c->set->graph_default.GridMinColour3; sg->GridMinColour4=c->set->graph_default.GridMinColour4; }
      if (strcmp(setoption,"textcolour"   )==0) { sg->TextColour=c->set->graph_default.TextColour; sg->TextCol1234Space=c->set->graph_default.TextCol1234Space; sg->TextColour1=c->set->graph_default.TextColour1; sg->TextColour2=c->set->graph_default.TextColour2; sg->TextColour3=c->set->graph_default.TextColour3; sg->TextColour4=c->set->graph_default.TextColour4; }
     } else {
      unsigned char useCol, use1234;
      if (strcmp(setoption,"axescolour"   )==0) ppl_colorFromObj(c,&command[PARSE_set_axescolour_color   ],&sg->AxesColour   ,&sg->AxesCol1234Space   ,NULL,&sg->AxesColour1   ,&sg->AxesColour2   ,&sg->AxesColour3   ,&sg->AxesColour4   ,&useCol,&use1234);
      if (strcmp(setoption,"gridmajcolour")==0) ppl_colorFromObj(c,&command[PARSE_set_gridmajcolour_color],&sg->GridMajColour,&sg->GridMajCol1234Space,NULL,&sg->GridMajColour1,&sg->GridMajColour2,&sg->GridMajColour3,&sg->GridMajColour4,&useCol,&use1234);
      if (strcmp(setoption,"gridmincolour")==0) ppl_colorFromObj(c,&command[PARSE_set_gridmincolour_color],&sg->GridMinColour,&sg->GridMinCol1234Space,NULL,&sg->GridMinColour1,&sg->GridMinColour2,&sg->GridMinColour3,&sg->GridMinColour4,&useCol,&use1234);
      if (strcmp(setoption,"textcolour"   )==0) ppl_colorFromObj(c,&command[PARSE_set_textcolour_color   ],&sg->TextColour   ,&sg->TextCol1234Space   ,NULL,&sg->TextColour1   ,&sg->TextColour2   ,&sg->TextColour3   ,&sg->TextColour4   ,&useCol,&use1234);
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
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The horizontal offset supplied to the 'set key' command was not finite."); }
      else                        sg->KeyXOff.real = tempdbl;
     }
    if (command[PARSE_set_key_offset+1].objType==PPLOBJ_NUM) // Vertical offset
     {
      double tempdbl = command[PARSE_set_key_offset+1].real;
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The vertical offset supplied to the 'set key' command was not finite."); }
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
        if (strcmp(tempstr,"xcentre")==0)
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
        if (strcmp(tempstr,"ycentre")==0)
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
    double tempdbl = command[PARSE_set_keycolumns_key_columns].real;
    if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The value supplied to the 'set keycolumns' command was not finite."); return; }
    if (tempdbl <= 0.0) { ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Number of key columns is not allowed to be less than or equal to zero."); return; }
    sg->KeyColumns = (int)round(tempdbl);
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
    if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The value supplied to the 'set linewidth' command was not finite."); return; }
    if (tempdbl <= 0.0) { ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Line widths are not allowed to be less than or equal to zero."); return; }
    sg->LineWidth = tempdbl;
   }
  else if (strcmp_unset && (strcmp(setoption,"linewidth")==0)) /* unset linewidth */
   {
    sg->LineWidth = c->set->graph_default.LineWidth;
   }
  else if (strcmp_set && (strcmp(setoption,"multiplot")==0)) /* set multiplot */
   {
    c->set->term_current.multiplot = SW_ONOFF_ON;
   }
  else if (strcmp_unset && (strcmp(setoption,"multiplot")==0)) /* unset multiplot */
   {
    if ((c->set->term_default.multiplot == SW_ONOFF_OFF) && (c->set->term_current.multiplot == SW_ONOFF_ON)) directive_clear(c,pl,in,interactive);
    c->set->term_current.multiplot = c->set->term_default.multiplot;
   }
  else if (strcmp_set && (strcmp(setoption,"nobackup")==0)) /* set nobackup */
   {
    c->set->term_current.backup = SW_ONOFF_OFF;
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
  else if (strcmp_set && (strcmp(setoption,"nokey")==0)) /* set nokey */
   {
    sg->key = SW_ONOFF_OFF;
   }
  else if (strcmp_set && (strcmp(setoption,"nomultiplot")==0)) /* set nomultiplot */
   {
    if (c->set->term_current.multiplot != SW_ONOFF_OFF) directive_clear(c,pl,in,interactive);
    c->set->term_current.multiplot = SW_ONOFF_OFF;
   }
  else if (strcmp_set && (strcmp(setoption,"notitle")==0)) /* set notitle */
   {
    strcpy(sg->title, "");
   }
  else if (strcmp_set && (strcmp(setoption,"numerics")==0)) /* set numerics */
   {
    int   tempint;
    char *tempstr = (char *)command[PARSE_set_numerics_complex].auxil;
    int   got     =        (command[PARSE_set_numerics_complex].objType == PPLOBJ_STR);
    if (got) c->set->term_current.ComplexNumbers = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_numerics_errortype].auxil;
    got     =        (command[PARSE_set_numerics_errortype].objType == PPLOBJ_STR);
    if (got) c->set->term_current.ExplicitErrors = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempint = (int)round(command[PARSE_set_numerics_number_significant_figures].real);
    got     =        (command[PARSE_set_numerics_number_significant_figures].objType == PPLOBJ_NUM);
    if (got)
     {
      if (tempint <  1) { ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Numbers cannot be displayed to fewer than one significant figure."); return; }
      if (tempint > 30) { ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "It is not sensible to try to display numbers to more than 30 significant figures. Calculations in PyXPlot are only accurate to double precision."); return; }
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
  else if (strcmp_unset && (strcmp(setoption,"samples")==0)) /* unset samples */
   {
    sg->samples        = c->set->graph_default.samples;
    sg->SamplesX       = c->set->graph_default.SamplesX;
    sg->SamplesXAuto   = c->set->graph_default.SamplesXAuto;
    sg->SamplesY       = c->set->graph_default.SamplesY;
    sg->SamplesYAuto   = c->set->graph_default.SamplesYAuto;
    sg->Sample2DMethod = c->set->graph_default.Sample2DMethod;
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
      if (!gsl_finite(tempdbl)) { ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, "The DPI resolution supplied to the 'set terminal' command was not finite."); }
      else if (tempdbl <= 2.0)  { ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Output image resolutions below two dots per inch are not supported."); }
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
    if (command[PARSE_set_texthalign_centre].objType == PPLOBJ_STR) sg->TextHAlign = SW_HALIGN_CENT;
    if (command[PARSE_set_texthalign_right ].objType == PPLOBJ_STR) sg->TextHAlign = SW_HALIGN_RIGHT;
   }
  else if (strcmp_set && (strcmp(setoption,"textvalign")==0)) /* set textvalign */
   {
    if (command[PARSE_set_textvalign_top   ].objType == PPLOBJ_STR) sg->TextVAlign = SW_VALIGN_TOP;
    if (command[PARSE_set_textvalign_centre].objType == PPLOBJ_STR) sg->TextVAlign = SW_VALIGN_CENT;
    if (command[PARSE_set_textvalign_bottom].objType == PPLOBJ_STR) sg->TextVAlign = SW_VALIGN_BOT;
   }
  else if (strcmp_unset && (strcmp(setoption,"textvalign")==0)) /* unset textvalign */
   {
    sg->TextVAlign = c->set->graph_default.TextVAlign;
   }
  else if (strcmp_set && (strcmp(setoption,"unit")==0)) /* set unit */
   {
    int got, got2, got3;
    char *tempstr, *tempstr2, *tempstr3;

    tempstr = (char *)command[PARSE_set_unit_angle].auxil;
    got     =        (command[PARSE_set_unit_angle].objType == PPLOBJ_STR);
    if (got) c->set->term_current.UnitAngleDimless    = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_unit_prefix].auxil;
    got     =        (command[PARSE_set_unit_prefix].objType == PPLOBJ_STR);
    if (got) c->set->term_current.UnitDisplayPrefix   = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ONOFF_INT, SW_ONOFF_STR);

    tempstr = (char *)command[PARSE_set_unit_scheme].auxil;
    got     =        (command[PARSE_set_unit_scheme].objType == PPLOBJ_STR);
    if (got) c->set->term_current.UnitScheme          = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_UNITSCH_INT, SW_UNITSCH_STR);

    tempstr2= (char *)command[PARSE_set_unit_preferred_unit].auxil;
    got2    =        (command[PARSE_set_unit_preferred_unit].objType == PPLOBJ_STR);
    tempstr3= (char *)command[PARSE_set_unit_unpreferred_unit].auxil;
    got3    =        (command[PARSE_set_unit_unpreferred_unit].objType == PPLOBJ_STR);

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
         if (errpos>=0) { ppl_error(&c->errcontext, ERR_NUMERIC,-1,-1,buf); continue; }

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
         if      ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].nameAp,1))!=0) p=1;
         else if ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].nameAs,1))!=0) p=1;
         else if ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].nameFp,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].nameFs,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].nameFp,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].alt1  ,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].alt2  ,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].alt3  ,0))!=0) p=1;
         else if ((k = ppl_unitNameCmp(tempstr2, c->unit_database[j].alt4  ,0))!=0) p=1;
         else
          {
           for (l=c->unit_database[j].minPrefix/3+8; l<=c->unit_database[j].maxPrefix/3+8; l++)
            {
             if (l==8) continue;
             for (k=0; ((SIprefixes_full[l][k]!='\0') && (toupper(SIprefixes_full[l][k])==toupper(tempstr2[k]))); k++);
             if (SIprefixes_full[l][k]=='\0')
              {
               if      ((m = ppl_unitNameCmp(tempstr2+k, c->unit_database[j].nameFp,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(tempstr2+k, c->unit_database[j].nameFs,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(tempstr2+k, c->unit_database[j].alt1  ,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(tempstr2+k, c->unit_database[j].alt2  ,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(tempstr2+k, c->unit_database[j].alt3  ,0))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(tempstr2+k, c->unit_database[j].alt4  ,0))!=0) { p=1; k+=m; multiplier=l; break; }
              }
             for (k=0; ((SIprefixes_abbrev[l][k]!='\0') && (SIprefixes_abbrev[l][k]==tempstr2[k])); k++);
             if (SIprefixes_abbrev[l][k]=='\0')
              {
               if      ((m = ppl_unitNameCmp(tempstr2+k, c->unit_database[j].nameAp,1))!=0) { p=1; k+=m; multiplier=l; break; }
               else if ((m = ppl_unitNameCmp(tempstr2+k, c->unit_database[j].nameAs,1))!=0) { p=1; k+=m; multiplier=l; break; }
              }
            }
          }
         if (p==0) continue;
         if (i!=2)
          {
           if ((c->unit_database[j].quantity!=NULL) && (c->unit_database[j].quantity[0]!='\0'))
            { sprintf(c->errcontext.tempErrStr, "'%s' is not a unit of '%s', but of '%s'.", unit, quantity, c->unit_database[j].quantity); ppl_error(&c->errcontext,ERR_GENERAL,-1,-1,NULL); }
           else
            { sprintf(c->errcontext.tempErrStr, "'%s' is not a unit of '%s'.", unit, quantity); ppl_error(&c->errcontext,ERR_GENERAL,-1,-1,NULL); }
          }
         c->unit_database[j].userSel = 1;
         c->unit_database[j].userSelPrefix = multiplier;
         pp=1;
        }
       if (i==0) { sprintf(c->errcontext.tempErrStr, "No such quantity as a '%s'.", quantity); ppl_error(&c->errcontext,ERR_GENERAL,-1,-1,NULL); }
       if (p==0) { sprintf(c->errcontext.tempErrStr, "No such unit as a '%s'.", unit); ppl_error(&c->errcontext,ERR_GENERAL,-1,-1,NULL); }
      }
    }
   }
  else
   {
    ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, "PyXPlot's set command could not find handler for this set command.");
   }

  return;
 }

