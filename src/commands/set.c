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

#include "expressions/traceback_fns.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "settings/arrows_fns.h"
#include "settings/axes_fns.h"
#include "settings/epsColors.h"
#include "settings/labels_fns.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/textConstants.h"
#include "settings/withWords_fns.h"

#include "stringTools/asciidouble.h"

#include "userspace/context.h"
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
   }
  else if (strcmp_unset && (strcmp(setoption,"arrow")==0)) /* unset arrow */
   {
   }
  else if (strcmp_set && (strcmp(setoption,"backup")==0)) /* set backup */
   {
    c->set->term_current.backup = SW_ONOFF_ON;
   }
  else if (strcmp_unset && (strcmp(setoption,"backup")==0)) /* unset backup */
   {
    c->set->term_current.backup = c->set->term_default.backup;
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
  else
   {
    ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, "PyXPlot's set command could not find handler for this set command.");
   }

  return;
 }

