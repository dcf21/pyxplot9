// readConf.c
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

#define _PPL_READCONF_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <gsl/gsl_math.h>

#include "commands/set.h"
#include "coreUtils/errorReport.h"
#include "epsMaker/eps_settings.h"
#include "expressions/expCompile.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "mathsTools/dcfmath.h"
#include "parser/parser.h"
#include "settings/arrows_fns.h"
#include "settings/axes_fns.h"
#include "settings/epsColors.h"
#include "settings/colors.h"
#include "settings/labels_fns.h"
#include "settings/papersizes.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"

static void _ReadConfig_FetchKey(char *line, char *out)
 {
  char *scan = out;
  while ((*line != '\0') && ((*(scan) = *(line++)) != '=')) scan++;
  *scan = '\0';
  ppl_strStrip(out, out);
  return;
 }

static void _ReadConfig_FetchValue(char *line, char *out)
 {
  char *scan = out;
  while ((*line != '\0') && (*(line++) != '='));
  while  (*line != '\0') *(scan++) = *(line++);
  *scan = '\0';
  ppl_strStrip(out, out);
  return;
 }

static void ppl_readConfigFile(ppl_context *c, char *ConfigFname)
 {
  char   linebuffer[LSTR_LENGTH], setkey[LSTR_LENGTH], setvalue[LSTR_LENGTH], ColorName[SSTR_LENGTH], *StringScan;
  char   errtext[LSTR_LENGTH];
  FILE  *infile;
  int    state=-1;
  int    linecounter=0;
  int    i, j, k, PalettePos, ColorNumber;
  double fl, PaperHeight, PaperWidth;

  if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Scanning configuration file %s.", ConfigFname); ppl_log(&c->errcontext, NULL); }

  if ((infile=fopen(ConfigFname,"r")) == NULL)
   {
    if (DEBUG) { ppl_log(&c->errcontext, "File does not exist."); }
    return;
   }

  while ((!feof(infile)) && (!ferror(infile)))
   {
    linecounter++;
    ppl_file_readline(infile, linebuffer, LSTR_LENGTH);
    ppl_strStrip(linebuffer, linebuffer);
    if                       (linebuffer[0] == '\0')                    continue;
    else if (ppl_strCmpNoCase(linebuffer, "[settings]" )==0) {state= 1; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[terminal]" )==0) {state= 2; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[colours]"  )==0) {state= 3; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[colors]"   )==0) {state= 3; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[latex]"    )==0) {state= 4; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[variables]")==0) {state= 5; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[functions]")==0) {state= 6; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[units]"    )==0) {state= 7; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[filters]"  )==0) {state= 8; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[script]"   )==0) {state= 9; continue;}
    else if (ppl_strCmpNoCase(linebuffer, "[styling]"  )==0) {state=10; continue;}

    _ReadConfig_FetchKey  (linebuffer, setkey  );
    _ReadConfig_FetchValue(linebuffer, setvalue);

    if (state == 1) // [settings] section
     {
      ppl_strUpper(setkey, setkey);
      if      (strcmp(setkey, "ASPECT"       )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))&&(fl>=1e-6)&&(fl<=1e4)))
                                                                                               { c->set->graph_default.aspect        = fl;
                                                                                                 c->set->graph_default.AutoAspect    = SW_ONOFF_OFF;
                                                                                               }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Aspect."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "AUTOASPECT"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->graph_default.AutoAspect    = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting AutoAspect."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "ZASPECT"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))&&(fl>=1e-6)&&(fl<=1e4)))
                                                                                               { c->set->graph_default.zaspect       = fl;
                                                                                                 c->set->graph_default.AutoZAspect   = SW_ONOFF_OFF;
                                                                                               }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting ZAspect."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "AUTOZASPECT"  )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->graph_default.AutoZAspect    = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting AutoZAspect."  , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "AXESCOLOUR"   )==0) || (strcmp(setkey, "AXESCOLOR")==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_COLOR_INT,SW_COLOR_STR))>0)                      c->set->graph_default.AxesColor    = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Color."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "AXISUNITSTYLE")==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_AXISUNITSTY_INT,SW_AXISUNITSTY_STR))>0)            c->set->graph_default.AxisUnitStyle = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting AxisUnitStyle.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "BACKUP"       )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .backup        = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Backup."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "BAR"          )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.bar           = fl;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Bar."          , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "BINORIGIN"    )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->term_default.BinOrigin.real = fl;
                                                                                                 c->set->term_default.BinOriginAuto  = 0;  }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting BinOrigin."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "BINWIDTH"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->term_default.BinWidth.real  = fl;
                                                                                                 c->set->term_default.BinWidthAuto   = (fl>0.0);  }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting BinWidth."     , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "BOXFROM"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->graph_default.BoxFrom.real  = fl;
                                                                                                 c->set->graph_default.BoxWidthAuto  = 0;  }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting BoxFrom."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "BOXWIDTH"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->graph_default.BoxWidth.real = fl;
                                                                                                 c->set->graph_default.BoxWidthAuto  = (fl>0.0);  }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting BoxWidth."     , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "CALENDARIN"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_CALENDAR_INT, SW_CALENDAR_STR ))>0)                c->set->term_default.CalendarIn = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting CalendarIn."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "CALENDAROUT"  )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_CALENDAR_INT, SW_CALENDAR_STR ))>0)                c->set->term_default.CalendarOut= i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting CalendarOut."  , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "CLIP"         )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->graph_default.clip          = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Clip."         , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "COLKEY"       )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->graph_default.ColKey        = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting ColKey."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "COLKEYPOS"    )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_COLKEYPOS_INT,SW_COLKEYPOS_STR))>0)                c->set->graph_default.ColKeyPos     = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting ColKeyPos."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "COLOUR"       )==0) || (strcmp(setkey, "COLOR")==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .color         = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Color."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "CONTOURS"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.ContoursN     = ppl_max((int)fl, 2);
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Contours."     , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "CONTOURS_LABEL")==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->graph_default.ContoursLabel = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Contours_Label.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }


#define DO_CRANGE(C,X) \
      else if (strcmp(setkey, "C" X "RANGE_LOG"   )==0) \
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                          c->set->graph_default.Clog[C]            = i; \
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting C" X "Range_Log."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
      else if (strcmp(setkey, "C" X "RANGE_MIN"   )==0) \
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               { c->set->graph_default.Cmin[C].real = fl; c->set->graph_default.Cminauto[C] = SW_BOOL_FALSE; } \
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting C" X "Range_Min."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
      else if (strcmp(setkey, "C" X "RANGE_MIN_AUTO")==0) \
        if  ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                         c->set->graph_default.Cminauto[C]        = i; \
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting C" X "Range_Min_Auto.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
      else if (strcmp(setkey, "C" X "RANGE_MAX"   )==0) \
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               { c->set->graph_default.Cmax[C].real = fl; c->set->graph_default.Cmaxauto[C] = SW_BOOL_FALSE; } \
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting C" X "Range_Max."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
      else if (strcmp(setkey, "C" X "RANGE_MAX_AUTO")==0) \
        if  ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                         c->set->graph_default.Cmaxauto[C]        = i; \
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting C" X "Range_Max_Auto.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
      else if (strcmp(setkey, "C" X "RANGE_RENORM")==0) \
        if  ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                         c->set->graph_default.Crenorm[C]         = i; \
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting C" X "Range_Renorm.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
      else if (strcmp(setkey, "C" X "RANGE_REVERSE")==0) \
        if  ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                         c->set->graph_default.Creverse[C]        = i; \
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting C" X "Range_Reverse.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }


      DO_CRANGE(0,"1")  DO_CRANGE(1,"2")  DO_CRANGE(2,"3")  DO_CRANGE(3,"4")

      else if (strcmp(setkey, "DATASTYLE"    )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_STYLE_INT, SW_STYLE_STR ))>0)                      c->set->graph_default.dataStyle.style = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting DataStyle."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "DISPLAY"      )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .display       = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Display."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "DPI"          )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))&&(fl>2)))       c->set->term_default .dpi           = fl;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting DPI."          , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "FONTSIZE"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.FontSize      = fl;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting FontSize."     , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "FUNCSTYLE"    )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_STYLE_INT, SW_STYLE_STR ))>0)                      c->set->graph_default.funcStyle.style = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting FuncStyle."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "GRID"         )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->graph_default.grid          = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Grid."         , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "GRIDAXISX"    )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl)) && (i==strlen(setvalue)) && (fl>=0) && (fl<=MAX_AXES)))
         {
          c->set->graph_default.GridAxisX[1]       = 0;
          c->set->graph_default.GridAxisX[(int)fl] = 1;
         }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting GridAxisX."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "GRIDAXISY"    )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl)) && (i==strlen(setvalue)) && (fl>=0) && (fl<=MAX_AXES)))
         {
          c->set->graph_default.GridAxisY[1]       = 0;
          c->set->graph_default.GridAxisY[(int)fl] = 1;
         }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting GridAxisY."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "GRIDAXISZ"    )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl)) && (i==strlen(setvalue)) && (fl>=0) && (fl<=MAX_AXES)))
         {
          c->set->graph_default.GridAxisZ[1]       = 0;
          c->set->graph_default.GridAxisZ[(int)fl] = 1;
         }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting GridAxisZ."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "GRIDMAJCOLOUR")==0) || (strcmp(setkey, "GRIDMAJCOLOR")==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_COLOR_INT,SW_COLOR_STR))>0)                      c->set->graph_default.GridMajColor = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting GridMajColor.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "GRIDMINCOLOUR")==0) || (strcmp(setkey, "GRIDMINCOLOR")==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_COLOR_INT,SW_COLOR_STR))>0)                      c->set->graph_default.GridMinColor = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting GridMinColor.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "KEY"          )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->graph_default.key           = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Key."          , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "KEYCOLUMNS"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.KeyColumns    = ppl_max((int)fl, 0);
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting KeyColumns."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "KEYPOS"       )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_KEYPOS_INT,SW_KEYPOS_STR))>0)                      c->set->graph_default.KeyPos        = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting KeyPos."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "KEY_XOFF"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.KeyXOff.real  = fl/100;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Key_XOff."     , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "KEY_YOFF"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.KeyYOff.real  = fl/100;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Key_YOff."     , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "LANDSCAPE"    )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0) c->set->term_default .landscape     = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Landscape."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "LINEWIDTH"    )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.LineWidth     = fabs(fl);
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting LineWidth."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "MULTIPLOT"    )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .multiplot     = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting MultiPlot."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "NUMCOMPLEX"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .ComplexNumbers= i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting NumComplex."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "NUMDISPLAY"  )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_DISPLAY_INT, SW_DISPLAY_STR ))>0)                  c->set->term_default.NumDisplay = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting NumDisplay."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "NUMERR"       )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .ExplicitErrors= i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting NumErr."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "NUMSF"        )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->term_default .SignificantFigures = ppl_min(ppl_max((int)fl, 1), 30);
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting NumSF."        , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "ORIGINX"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.OriginX.real  = fl/100;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting OriginX."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "ORIGINY"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.OriginY.real  = fl/100;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting OriginY."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "OUTPUT"       )==0)
        strcpy(c->set->term_default.output , setvalue);
      else if (strcmp(setkey, "PAPERHEIGHT" )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->term_default .PaperHeight.real  = fl/1000;
                                                                              PaperHeight = c->set->term_default.PaperHeight.real * 1000;
                                                                              PaperWidth  = c->set->term_default.PaperWidth .real * 1000;
                                                                              ppl_GetPaperName(c->set->term_default.PaperName, &PaperHeight, &PaperWidth);
                                                                            }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting PaperHeight." , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "PAPERNAME"   )==0)
        {
         ppl_PaperSizeByName(setvalue, &PaperHeight, &PaperWidth);
         if (PaperHeight <= 0) {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Unrecognised papersize specified for setting PaperName."  , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
         c->set->term_default.PaperHeight.real = PaperHeight/1000;
         c->set->term_default.PaperWidth.real  = PaperWidth/1000;
         ppl_GetPaperName(c->set->term_default.PaperName, &PaperHeight, &PaperWidth);
        }
      else if (strcmp(setkey, "PAPERWIDTH"  )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->term_default .PaperWidth.real  = fl/1000;
                                                                              PaperHeight = c->set->term_default.PaperHeight.real * 1000;
                                                                              PaperWidth  = c->set->term_default.PaperWidth .real * 1000;
                                                                              ppl_GetPaperName(c->set->term_default.PaperName, &PaperHeight, &PaperWidth);
                                                                            }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting PaperWidth."  , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "POINTLINEWIDTH")==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.PointLineWidth= fl;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting PointLineWidth.",linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "POINTSIZE"    )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.PointSize     = fl;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting PointSize."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
//      else if (strcmp(setkey, "PROJECTION"   )==0)
//        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_PROJ_INT, SW_PROJ_STR ))>0)   c->set->graph_default.projection    = i;
//        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Projection."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "SAMPLES"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.samples       = ppl_max((int)fl, 2);
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Samples."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "SAMPLES_METHOD")==0)
        if  ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_SAMPLEMETHOD_INT,SW_SAMPLEMETHOD_STR))>0)         c->set->graph_default.Sample2DMethod = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Samples_Method." , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "SAMPLES_X"    )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               { c->set->graph_default.SamplesX = ppl_max((int)fl, 2); c->set->graph_default.SamplesXAuto = SW_BOOL_FALSE; }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Samples_X."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "SAMPLES_X_AUTO")==0)
        if  ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                         c->set->graph_default.SamplesXAuto  = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Samples_X_Auto." , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "SAMPLES_Y"    )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               { c->set->graph_default.SamplesY = ppl_max((int)fl, 2); c->set->graph_default.SamplesYAuto = SW_BOOL_FALSE; }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Samples_Y."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "SAMPLES_Y_AUTO")==0)
        if  ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                         c->set->graph_default.SamplesYAuto  = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Samples_Y_Auto." , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TERMANTIALIAS")==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .TermAntiAlias = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TermAntiAlias.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TERMENLARGE"  )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .TermEnlarge   = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TermEnlarge."  , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "ENLARGE"      )==0) // ENLARGE, as opposed to TERMENLARGE is supported for back-compatibility with PyXPlot 0.7
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .TermEnlarge   = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Enlarge."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TERMINVERT"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .TermInvert = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TermInvert."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TERMTRANSPARENT")==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .TermTransparent= i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TermTransparent.",linecounter,ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TERMTYPE"     )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_TERMTYPE_INT,SW_TERMTYPE_STR))>0)                  c->set->term_default.TermType  = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TermType."     , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "TEXTCOLOUR"   )==0) || (strcmp(setkey, "TEXTCOLOR")==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_COLOR_INT,SW_COLOR_STR))>0)                      c->set->graph_default.TextColor    = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TextColor."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TEXTHALIGN"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_HALIGN_INT,SW_HALIGN_STR))>0)                      c->set->graph_default.TextHAlign    = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TextHAlign."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TEXTVALIGN"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_VALIGN_INT,SW_VALIGN_STR))>0)                      c->set->graph_default.TextVAlign    = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TextVAlign."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TITLE"        )==0)
        strcpy(c->set->graph_default.title  , setvalue);
      else if (strcmp(setkey, "TITLE_XOFF"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.TitleXOff.real  = fl/100;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Title_XOff."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TITLE_YOFF"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.TitleYOff.real  = fl/100;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Title_YOff."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TRANGE_LOG"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                          c->set->graph_default.Tlog            = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TRange_Log."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TRANGE_MIN"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->graph_default.Tmin.real       = fl; c->set->graph_default.USE_T_or_uv = 1; }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TRange_Min."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "TRANGE_MAX"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->graph_default.Tmax.real       = fl; c->set->graph_default.USE_T_or_uv = 1; }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting TRange_Max."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "UNITABBREV"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .UnitDisplayAbbrev= i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting UnitAbbrev."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "UNITANGLEDIMLESS")==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .UnitAngleDimless= i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting UnitAngleDimless."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "UNITPREFIX"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_ONOFF_INT, SW_ONOFF_STR ))>0)                      c->set->term_default .UnitDisplayPrefix= i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting UnitPrefix."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "UNITSCHEME"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_UNITSCH_INT,SW_UNITSCH_STR))>0)                    c->set->term_default.UnitScheme = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting UnitScheme."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "URANGE_LOG"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                          c->set->graph_default.Ulog            = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting URange_Log."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "URANGE_MIN"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->graph_default.Umin.real       = fl; c->set->graph_default.USE_T_or_uv = 0; }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting URange_Min."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "URANGE_MAX"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->graph_default.Umax.real       = fl; c->set->graph_default.USE_T_or_uv = 0; }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting URange_Max."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "VRANGE_LOG"   )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue,SW_BOOL_INT,SW_BOOL_STR))>0)                          c->set->graph_default.Vlog            = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting VRange_Log."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "VRANGE_MIN"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->graph_default.Vmin.real       = fl; c->set->graph_default.USE_T_or_uv = 0; }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting VRange_Min."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "VRANGE_MAX"   )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))             { c->set->graph_default.Vmax.real       = fl; c->set->graph_default.USE_T_or_uv = 0; }
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting VRange_Max."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "WIDTH"        )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.width.real      = fl/100;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Width."        , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "VIEW_XY"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.XYview.real     = fl/180*M_PI;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting View_XY."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "VIEW_YZ"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue))))               c->set->graph_default.YZview.real     = fl/180*M_PI;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting View_YZ."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else
       { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Unrecognised setting name '%s'.", linecounter, ConfigFname, setkey); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
     }
    else if (state == 2) // [terminal] section
     {
      ppl_strUpper(setkey, setkey);
      if     ((strcmp(setkey, "COLOUR"       )==0) || (strcmp(setkey, "COLOR"       )==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue, SW_ONOFF_INT,   SW_ONOFF_STR  ))>0)                      c->errcontext.session_default.color      = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Color."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "COLOUR_ERR"   )==0) || (strcmp(setkey, "COLOR_ERR"   )==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue, SW_TERMCOL_INT, SW_TERMCOL_STR))>0)                      c->errcontext.session_default.color_err  = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Color_Err."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "COLOUR_REP"   )==0) || (strcmp(setkey, "COLOR_REP"   )==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue, SW_TERMCOL_INT, SW_TERMCOL_STR))>0)                      c->errcontext.session_default.color_rep  = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Color_Rep."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "COLOUR_WRN"   )==0) || (strcmp(setkey, "COLOR_WRN"   )==0))
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue, SW_TERMCOL_INT, SW_TERMCOL_STR))>0)                      c->errcontext.session_default.color_wrn  = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Color_Wrn."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "SPLASH"       )==0)
        if ((i=ppl_fetchSettingByName(&c->errcontext,setvalue, SW_ONOFF_INT,   SW_ONOFF_STR  ))>0)                      c->errcontext.session_default.splash     = i;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Splash."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else
       { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Unrecognised setting name '%s'.", linecounter, ConfigFname, setkey); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
     }
    else if (state == 3) // [colors] section
     {
      ppl_strUpper(setkey, setkey);
      if      (strcmp(setkey, "PALETTE"      )==0)
       {
        PalettePos = 0;
        StringScan = setvalue;
        while (strlen(ppl_strCommaSeparatedListScan(&StringScan, ColorName)) != 0)
         {
          if (PalettePos == PALETTE_LENGTH-1)
           {
            sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Specified palette is too long.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr);
            c->set->palette_default[PalettePos] = -1;
            continue;
           } else {
            ppl_strUpper(ColorName,ColorName);
            ColorNumber = ppl_fetchSettingByName(&c->errcontext,ColorName, SW_COLOR_INT, SW_COLOR_STR);
            if (ColorNumber<=0)
             {
              sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Color '%s' not recognised.", linecounter, ConfigFname, ColorName); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr);
             } else {
              c->set->palette_default [PalettePos  ] = ColorNumber;
              c->set->paletteS_default[PalettePos  ] = 0;
              c->set->palette1_default[PalettePos  ] = 0.0;
              c->set->palette2_default[PalettePos  ] = 0.0;
              c->set->palette3_default[PalettePos  ] = 0.0;
              c->set->palette4_default[PalettePos++] = 0.0;
             }
           }
         }
        if (PalettePos > 0)
         {
          c->set->palette_default[PalettePos] = -1;
         } else {
          sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: The specified palette does not contain any colors.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr);
         }
       }
      else
       { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Unrecognised setting name '%s'.", linecounter, ConfigFname, setkey); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
     }
    else if (state == 4) // [latex] section
     {
      ppl_strUpper(setkey, setkey);
      if      (strcmp(setkey, "PREAMBLE"     )==0)
        strcpy(c->set->term_default.LatexPreamble, setvalue);
      else
       { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Unrecognised setting name '%s'.", linecounter, ConfigFname, setkey); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
     }
    else if (state == 5) // [variables] section
     {
      int stat=0;
      parserLine   *pl = NULL;
      parserStatus *ps = NULL;
      ppl_error_setstreaminfo(&c->errcontext, linecounter, ConfigFname);
      ppl_parserStatInit(&ps,&pl);
      if ( (ps==NULL) || (c->inputLineBuffer == NULL) ) { ppl_error(&c->errcontext,ERR_MEMORY,-1,-1,"Out of memory."); continue; }
      ppl_tbClear(c);

      // If line is blank, ignore it
      { int i=0,j=0; for (i=0; linebuffer[i]!='\0'; i++) if (linebuffer[i]>' ') { j=1; break; } if (j==0) continue; }

      stat = ppl_parserCompile(c, ps, c->errcontext.error_input_linenumber,c->errcontext.error_input_sourceId,c->errcontext.error_input_filename, linebuffer, 0, 0);
      if ( (!stat) && (!c->errStat.status) && (ps->blockDepth==0) ) ppl_parserExecute(c, *ps->rootpl, "var_set", 0, 1);
      if (stat || c->errStat.status) ppl_tbWrite(c);
      ppl_parserStatFree(&ps);
     }
    else if (state == 6) // [functions] section
     {
      int stat=0;
      parserLine   *pl = NULL;
      parserStatus *ps = NULL;
      ppl_error_setstreaminfo(&c->errcontext, linecounter, ConfigFname);
      ppl_parserStatInit(&ps,&pl);
      if ( (ps==NULL) || (c->inputLineBuffer == NULL) ) { ppl_error(&c->errcontext,ERR_MEMORY,-1,-1,"Out of memory."); continue; }
      ppl_tbClear(c);

      // If line is blank, ignore it
      { int i=0,j=0; for (i=0; linebuffer[i]!='\0'; i++) if (linebuffer[i]>' ') { j=1; break; } if (j==0) continue; }

      stat = ppl_parserCompile(c, ps, c->errcontext.error_input_linenumber,c->errcontext.error_input_sourceId,c->errcontext.error_input_filename, linebuffer, 0, 0);
      if ( (!stat) && (!c->errStat.status) && (ps->blockDepth==0) ) ppl_parserExecute(c, *ps->rootpl, "func_set", 0, 1);
      if (stat || c->errStat.status) ppl_tbWrite(c);
      ppl_parserStatFree(&ps);
     }
    else if (state == 7) // [units] section
     {

#define GET_UNITNAME(output, last, type, sep, LaTeX) \
      if (!LaTeX) { if (isalpha(setkey[i])) do { i++; } while ((isalnum(setkey[i])) || (setkey[i]=='_')); } \
      else        { while ((setkey[i]!=sep)&&(setkey[i]!='\0')) i++; } \
      if (i==j) \
       { \
        if (&last==&output) \
         { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal %s name.", linecounter, ConfigFname, type); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
        else \
         { \
          output = (char *)malloc(strlen(last)+1); \
          if (output==NULL) { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Out of memory error whilst generating new unit.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
          strcpy(output, last); \
         } \
       } else { \
        output = (char *)malloc(i-j+1); \
        if (output==NULL) { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Out of memory error whilst generating new unit.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; } \
        strncpy(output, setkey+j, i-j); output[i-j]='\0'; \
       } \
      while ((setkey[i]<=' ')&&(setkey[i]!='\0')) i++; \
      if (setkey[i]==sep) i++; \
      while ((setkey[i]<=' ')&&(setkey[i]!='\0')) i++; \
      j=i;

      i=j=0;
      if (c->unit_pos == UNITS_MAX) { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Unit definition list full.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }

      GET_UNITNAME( c->unit_database[c->unit_pos].nameFs  , c->unit_database[c->unit_pos].nameFs  , "unit"    , '/' , 0);
      GET_UNITNAME( c->unit_database[c->unit_pos].nameAs  , c->unit_database[c->unit_pos].nameFs  , "unit"    , '/' , 0);
      GET_UNITNAME( c->unit_database[c->unit_pos].nameLs  , c->unit_database[c->unit_pos].nameAs  , "unit"    , '/' , 1);
      GET_UNITNAME( c->unit_database[c->unit_pos].nameFp  , c->unit_database[c->unit_pos].nameFs  , "unit"    , '/' , 0);
      GET_UNITNAME( c->unit_database[c->unit_pos].nameAp  , c->unit_database[c->unit_pos].nameAs  , "unit"    , '/' , 0);
      GET_UNITNAME( c->unit_database[c->unit_pos].nameLp  , c->unit_database[c->unit_pos].nameAp  , "unit"    , ':' , 1);
      GET_UNITNAME( c->unit_database[c->unit_pos].quantity, c->unit_database[c->unit_pos].quantity, "quantity", ' ' , 0);

      if (setvalue[0]=='\0')
       {
        if (c->baseunit_pos == UNITS_MAX_BASEUNITS) { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s:\nBase unit definition list full.", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
        c->unit_database[c->unit_pos++].exponent[c->baseunit_pos++] = 1;
       }
      else
       {
        pplObj setnumeric;
        setnumeric.refCount=1;
        j = k = -1;
        ppl_unitsStringEvaluate(c, setvalue, &setnumeric, &k, &j, errtext);
        if (j >= 0) { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: %s", linecounter, ConfigFname, errtext); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
        if (setvalue[k]!='\0') { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Unexpected trailing matter in definition", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
        if (setnumeric.flagComplex) { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Multiplier in units definition cannot be complex", linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
        for (j=0; j<UNITS_MAX_BASEUNITS; j++) c->unit_database[c->unit_pos].exponent[j] = setnumeric.exponent[j];
        c->unit_database[c->unit_pos].multiplier = setnumeric.real;
        c->unit_pos++;
       }
     }
    else if (state == 8) // [filters] section
     {
      pplObj setnumeric;
      char *tmp = (char *)malloc(strlen(setvalue)+1);
      if (tmp!=NULL)
       {
        strcpy(tmp, setvalue);
        pplObjStr(&setnumeric,1,1,tmp);
        setnumeric.refCount=1;
        ppl_dictAppendCpy(c->set->filters,setkey,(void *)&setnumeric,sizeof(pplObj));
       }
     }
    else if (state == 9) // [script] section
     {
      c->set->term_current  = c->set->term_default; // Copy settings for directive_set()
      c->set->graph_current = c->set->graph_default;
      for (i=0; i<PALETTE_LENGTH; i++)
       {
        c->set->palette_current [i] = c->set->palette_default [i];
        c->set->paletteS_current[i] = c->set->paletteS_default[i];
        c->set->palette1_current[i] = c->set->palette1_default[i];
        c->set->palette2_current[i] = c->set->palette2_default[i];
        c->set->palette3_current[i] = c->set->palette3_default[i];
        c->set->palette4_current[i] = c->set->palette4_default[i];
       }
      for (i=0; i<MAX_AXES; i++) { pplaxis_destroy(c, &(c->set->XAxes[i]) ); pplaxis_copy(c,&(c->set->XAxes[i]), &(c->set->XAxesDefault[i]));
                                   pplaxis_destroy(c, &(c->set->YAxes[i]) ); pplaxis_copy(c,&(c->set->YAxes[i]), &(c->set->YAxesDefault[i]));
                                   pplaxis_destroy(c, &(c->set->ZAxes[i]) ); pplaxis_copy(c,&(c->set->ZAxes[i]), &(c->set->ZAxesDefault[i]));
                                 }
      for (i=0; i<MAX_PLOTSTYLES; i++) { ppl_withWordsDestroy(c, &(c->set->plot_styles[i])); ppl_withWordsCpy(c, &(c->set->plot_styles[i]) , &(c->set->plot_styles_default[i])); }
      pplarrow_list_destroy(c, &c->set->pplarrow_list);
      pplarrow_list_copy(c, &c->set->pplarrow_list, &c->set->pplarrow_list_default);
      ppllabel_list_destroy(c, &c->set->ppllabel_list);
      ppllabel_list_copy(c, &c->set->ppllabel_list, &c->set->ppllabel_list_default);

      {
      int stat=0;
      parserLine   *pl = NULL;
      parserStatus *ps = NULL;
      ppl_parserStatInit(&ps,&pl);
      ppl_error_setstreaminfo(&c->errcontext, linecounter, ConfigFname);
      if ( (ps==NULL) || (c->inputLineBuffer == NULL) ) { ppl_error(&c->errcontext,ERR_MEMORY,-1,-1,"Out of memory."); continue; }
      ppl_tbClear(c);

      // If line is blank, ignore it
      { int i=0,j=0; for (i=0; linebuffer[i]!='\0'; i++) if (linebuffer[i]>' ') { j=1; break; } if (j==0) continue; }

      stat = ppl_parserCompile(c, ps, c->errcontext.error_input_linenumber,c->errcontext.error_input_sourceId,c->errcontext.error_input_filename, linebuffer, 0, 0);
      if ( (!stat) && (!c->errStat.status) && (ps->blockDepth==0) ) ppl_parserExecute(c, *ps->rootpl, "set", 0, 1);
      if (stat || c->errStat.status) ppl_tbWrite(c);
      ppl_parserStatFree(&ps);
      }

      c->set->term_default  = c->set->term_current; // Copy changed settings into defaults
      c->set->graph_default = c->set->graph_current;
      for (i=0; i<PALETTE_LENGTH; i++)
       {
        c->set->palette_default [i] = c->set->palette_current [i];
        c->set->paletteS_default[i] = c->set->paletteS_current[i];
        c->set->palette1_default[i] = c->set->palette1_current[i];
        c->set->palette2_default[i] = c->set->palette2_current[i];
        c->set->palette3_default[i] = c->set->palette3_current[i];
        c->set->palette4_default[i] = c->set->palette4_current[i];
       }
      for (i=0; i<MAX_AXES; i++) { pplaxis_destroy(c, &(c->set->XAxesDefault[i]) ); pplaxis_copy(c,&(c->set->XAxesDefault[i]), &(c->set->XAxes[i]));
                                   pplaxis_destroy(c, &(c->set->YAxesDefault[i]) ); pplaxis_copy(c,&(c->set->YAxesDefault[i]), &(c->set->YAxes[i]));
                                   pplaxis_destroy(c, &(c->set->ZAxesDefault[i]) ); pplaxis_copy(c,&(c->set->ZAxesDefault[i]), &(c->set->ZAxes[i]));
                                 }
      for (i=0; i<MAX_PLOTSTYLES; i++) { ppl_withWordsDestroy(c, &(c->set->plot_styles_default[i])); ppl_withWordsCpy(c, &(c->set->plot_styles_default[i]) , &(c->set->plot_styles[i])); }
      pplarrow_list_destroy(c, &c->set->pplarrow_list_default);
      pplarrow_list_copy(c, &c->set->pplarrow_list_default, &c->set->pplarrow_list);
      ppllabel_list_destroy(c, &c->set->ppllabel_list_default);
      ppllabel_list_copy(c, &c->set->ppllabel_list_default, &c->set->ppllabel_list);
     }
    else if (state == 10) // [styling] section
     {
      ppl_strUpper(setkey, setkey);
      if      (strcmp(setkey, "BASELINE_LINEWIDTH"  )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_DEFAULT_LINEWIDTH = fl * EPS_BASE_DEFAULT_LINEWIDTH;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Baseline_LineWidth."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "BASELINE_POINTSIZE"  )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_DEFAULT_PS        = fl * EPS_BASE_DEFAULT_PS;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Baseline_PointSize."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "ARROW_HEADANGLE"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_ARROW_ANGLE       = fl * M_PI / 180;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Arrow_HeadAngle."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "ARROW_HEADSIZE"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_ARROW_HEADSIZE    = fl * EPS_BASE_ARROW_HEADSIZE;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Arrow_HeadSize."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "ARROW_HEADBACKINDENT")==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_ARROW_CONSTRICT   = fl;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Arrow_HeadBackIndent." , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "AXES_SEPARATION"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_AXES_SEPARATION   = fl * EPS_BASE_AXES_SEPARATION;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Axes_Separation."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "AXES_TEXTGAP"        )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_AXES_TEXTGAP      = fl * EPS_BASE_AXES_TEXTGAP;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Axes_TextGap."         , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "AXES_LINEWIDTH"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_AXES_LINEWIDTH    = fl * EPS_BASE_AXES_LINEWIDTH;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Axes_LineWidth."       , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "AXES_MAJTICKLEN"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_AXES_MAJTICKLEN   = fl * EPS_BASE_AXES_MAJTICKLEN;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Axes_MajTickLen."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "AXES_MINTICKLEN"     )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_AXES_MINTICKLEN   = fl * EPS_BASE_AXES_MINTICKLEN;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Axes_MinTickLen."      , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "COLOURSCALE_MARGIN"  )==0) || (strcmp(setkey, "COLORSCALE_MARGIN"  )==0))
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_COLORSCALE_MARGIN= fl * EPS_BASE_COLORSCALE_MARG;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting ColorScale_Margin."   , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if((strcmp(setkey, "COLOURSCALE_WIDTH"   )==0) || (strcmp(setkey, "COLORSCALE_WIDTH"   )==0))
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_COLORSCALE_WIDTH = fl * EPS_BASE_COLORSCALE_WIDTH;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting ColorScale_Width."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "GRID_MAJLINEWIDTH"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_GRID_MAJLINEWIDTH    = fl * EPS_BASE_GRID_MAJLINEWIDTH;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Grid_MajLineWidth."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else if (strcmp(setkey, "GRID_MINLINEWIDTH"      )==0)
        if  (fl=ppl_getFloat(setvalue, &i), ((gsl_finite(fl))&&(i==strlen(setvalue)))) EPS_GRID_MINLINEWIDTH    = fl * EPS_BASE_GRID_MINLINEWIDTH;
        else {sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Illegal value for setting Grid_MinLineWidth."    , linecounter, ConfigFname); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
      else
       { sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Unrecognised setting name '%s'.", linecounter, ConfigFname, setkey); ppl_warning(&c->errcontext, ERR_PREFORMED, c->errcontext.tempErrStr); continue; }
     }
    else
     {
      sprintf(c->errcontext.tempErrStr, "Error in line %d of configuration file %s: Settings need to be preceded by a block name such as [settings].", linecounter, ConfigFname);
      ppl_warning(&c->errcontext, ERR_PREFORMED, NULL);
      break;
     }
   }
  fclose(infile);
  return;
 }

void ppl_readconfig(ppl_context *c)
 {
  int    i;
  char   ConfigFname[FNAME_LENGTH];

  sprintf(ConfigFname, "%s%s%s", c->errcontext.session_default.homedir, PATHLINK, ".pyxplotrc");
  ppl_readConfigFile(c, ConfigFname);
  sprintf(ConfigFname, "%s", ".pyxplotrc");
  ppl_readConfigFile(c, ConfigFname);

  // Copy Default Settings to Current Settings
  c->set->term_current  = c->set->term_default;
  c->set->graph_current = c->set->graph_default;

  for (i=0; i<PALETTE_LENGTH; i++)
   {
    c->set->palette_current [i] = c->set->palette_default [i];
    c->set->paletteS_current[i] = c->set->paletteS_default[i];
    c->set->palette1_current[i] = c->set->palette1_default[i];
    c->set->palette2_current[i] = c->set->palette2_default[i];
    c->set->palette3_current[i] = c->set->palette3_default[i];
    c->set->palette4_current[i] = c->set->palette4_default[i];
   }
  for (i=0; i<MAX_AXES; i++) { pplaxis_destroy(c, &(c->set->XAxes[i]) ); pplaxis_copy(c,&(c->set->XAxes[i]), &(c->set->XAxesDefault[i]));
                               pplaxis_destroy(c, &(c->set->YAxes[i]) ); pplaxis_copy(c,&(c->set->YAxes[i]), &(c->set->YAxesDefault[i]));
                               pplaxis_destroy(c, &(c->set->ZAxes[i]) ); pplaxis_copy(c,&(c->set->ZAxes[i]), &(c->set->ZAxesDefault[i]));
                             }
  for (i=0; i<MAX_PLOTSTYLES; i++) { ppl_withWordsDestroy(c, &(c->set->plot_styles[i])); ppl_withWordsCpy(c, &(c->set->plot_styles[i]) , &(c->set->plot_styles_default[i])); }
  pplarrow_list_destroy(c, &c->set->pplarrow_list);
  pplarrow_list_copy(c, &c->set->pplarrow_list, &c->set->pplarrow_list_default);
  ppllabel_list_destroy(c, &c->set->ppllabel_list);
  ppllabel_list_copy(c, &c->set->ppllabel_list, &c->set->ppllabel_list_default);

  // Copy List of Preferred Units
  ppl_listFree(c->unit_PreferredUnits_default);
  c->unit_PreferredUnits_default = ppl_listCpy(c->unit_PreferredUnits, 1, sizeof(PreferredUnit));
  return;
 }

