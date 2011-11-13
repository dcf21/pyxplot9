// settingsInit.c
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

#define _SETTINGSINIT_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "coreUtils/errorReport.h"
#include "coreUtils/dict.h"
#include "coreUtils/getPasswd.h"
#include "coreUtils/memAlloc.h"

#include "mathsTools/dcfmath.h"

#include "settings/epsColours.h"

#include "pplConstants.h"
#include "settings/papersizes.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/withWords.h"

#include "expressions/pplObj.h"

pplset_terminal pplset_term_default;
pplset_terminal pplset_term_current;

pplset_graph    pplset_graph_default;
pplset_graph    pplset_graph_current;

withWords       pplset_plot_styles        [MAX_PLOTSTYLES];
withWords       pplset_plot_styles_default[MAX_PLOTSTYLES];

pplset_axis     pplset_axis_default;

pplset_axis     XAxes [MAX_AXES], XAxesDefault[MAX_AXES];
pplset_axis     YAxes [MAX_AXES], YAxesDefault[MAX_AXES];
pplset_axis     ZAxes [MAX_AXES], ZAxesDefault[MAX_AXES];

pplset_session  pplset_session_default;

int               pplset_palette_default [PALETTE_LENGTH] = {COLOUR_BLACK, COLOUR_RED, COLOUR_BLUE, COLOUR_MAGENTA, COLOUR_CYAN, COLOUR_BROWN, COLOUR_SALMON, COLOUR_GRAY, COLOUR_GREEN, COLOUR_NAVYBLUE, COLOUR_PERIWINKLE, COLOUR_PINEGREEN, COLOUR_SEAGREEN, COLOUR_GREENYELLOW, COLOUR_ORANGE, COLOUR_CARNATIONPINK, COLOUR_PLUM, -1};
int               pplset_paletteS_default[PALETTE_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double            pplset_palette1_default[PALETTE_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double            pplset_palette2_default[PALETTE_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double            pplset_palette3_default[PALETTE_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double            pplset_palette4_default[PALETTE_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int               pplset_palette_current [PALETTE_LENGTH];
int               pplset_paletteS_current[PALETTE_LENGTH];
double            pplset_palette1_current[PALETTE_LENGTH];
double            pplset_palette2_current[PALETTE_LENGTH];
double            pplset_palette3_current[PALETTE_LENGTH];
double            pplset_palette4_current[PALETTE_LENGTH];

dict             *pplset_filters;

pplarrow_object     *pplarrow_list, *pplarrow_list_default;
ppllabel_object     *ppllabel_list, *ppllabel_list_default;

void pplset_makedefault()
 {
  FILE  *LocalePipe;
  int    Nchars,i;
  double PaperWidth, PaperHeight;
  pplObj tempval;
  char   ConfigFname[FNAME_LENGTH];
  char  *PaperSizePtr;

  // Default Terminal Settings, used when these values are not changed by any configuration files
  pplset_term_default.backup              = SW_ONOFF_OFF;
  pplObjZero(&(pplset_term_default.BinOrigin),0);
  pplset_term_default.BinOriginAuto       = 1;
  pplObjZero(&(pplset_term_default.BinWidth),0);
  pplset_term_default.BinWidth.real       = 1.0;
  pplset_term_default.BinWidthAuto        = 1;
  pplset_term_default.CalendarIn          = SW_CALENDAR_BRITISH;
  pplset_term_default.CalendarOut         = SW_CALENDAR_BRITISH;
  pplset_term_default.colour              = SW_ONOFF_ON;
  pplset_term_default.ComplexNumbers      = SW_ONOFF_OFF;
  pplset_term_default.display             = SW_ONOFF_ON;
  pplset_term_default.dpi                 = 300.0;
  pplset_term_default.ExplicitErrors      = SW_ONOFF_ON;
  pplset_term_default.landscape           = SW_ONOFF_OFF;
  strcpy(pplset_term_default.LatexPreamble, "");
  pplset_term_default.multiplot           = SW_ONOFF_OFF;
  pplset_term_default.NumDisplay          = SW_DISPLAY_N;
  strcpy(pplset_term_default.output, "");
  pplObjZero(&(pplset_term_default.PaperHeight),0);
  pplset_term_default.PaperHeight.real    = 297.30178 / 1000;
  pplset_term_default.PaperHeight.dimensionless = 0; pplset_term_default.PaperHeight.exponent[UNIT_LENGTH] = 1;
  strcpy(pplset_term_default.PaperName, "a4");
  pplObjZero(&(pplset_term_default.PaperWidth),0);
  pplset_term_default.PaperWidth.real     = 210.2241 / 1000;
  pplset_term_default.PaperWidth.dimensionless = 0; pplset_term_default.PaperWidth.exponent[UNIT_LENGTH] = 1;
  pplset_term_default.RandomSeed          = 0;
  pplset_term_default.SignificantFigures  = 8;
  pplset_term_default.TermAntiAlias       = SW_ONOFF_ON;
  pplset_term_default.TermType            = SW_TERMTYPE_X11S;
  pplset_term_default.TermEnlarge         = SW_ONOFF_OFF;
  pplset_term_default.TermInvert          = SW_ONOFF_OFF;
  pplset_term_default.TermTransparent     = SW_ONOFF_OFF;
  pplset_term_default.UnitScheme          = SW_UNITSCH_SI;
  pplset_term_default.UnitDisplayPrefix   = SW_ONOFF_ON;
  pplset_term_default.UnitDisplayAbbrev   = SW_ONOFF_ON;
  pplset_term_default.UnitAngleDimless    = SW_ONOFF_ON;
  pplset_term_default.viewer              = SW_VIEWER_GV;
  strcpy(pplset_term_default.ViewerCmd, "");

  // Default Graph Settings, used when these values are not changed by any configuration files
  pplset_graph_default.aspect                = 1.0;
  pplset_graph_default.zaspect               = 1.0;
  pplset_graph_default.AutoAspect            = SW_ONOFF_ON;
  pplset_graph_default.AutoZAspect           = SW_ONOFF_ON;
  pplset_graph_default.AxesColour            = COLOUR_BLACK;
  pplset_graph_default.AxesColour1           = 0.0;
  pplset_graph_default.AxesColour2           = 0.0;
  pplset_graph_default.AxesColour3           = 0.0;
  pplset_graph_default.AxesColour4           = 0.0;
  pplset_graph_default.AxisUnitStyle         = SW_AXISUNITSTY_RATIO;
  pplset_graph_default.bar                   = 1.0;
  pplObjZero(&(pplset_graph_default.BoxFrom),0);
  pplset_graph_default.BoxFromAuto           = 1;
  pplObjZero(&(pplset_graph_default.BoxWidth),0);
  pplset_graph_default.BoxWidthAuto          = 1;
  strcpy(pplset_graph_default.c1format, "");
  pplset_graph_default.c1formatset           = 0;
  strcpy(pplset_graph_default.c1label, "");
  pplset_graph_default.c1LabelRotate         = 0.0;
  pplset_graph_default.c1TickLabelRotate     = 0.0;
  pplset_graph_default.c1TickLabelRotation   = SW_TICLABDIR_HORI;
  pplset_graph_default.clip                  = SW_ONOFF_OFF;
  for (i=0; i<4; i++)
   {
    pplset_graph_default.Clog[i]             = SW_BOOL_FALSE;
    pplObjZero(&pplset_graph_default.Cmax[i],0);
    pplset_graph_default.Cmaxauto[i]         = SW_BOOL_TRUE;
    pplObjZero(&pplset_graph_default.Cmin[i],0);
    pplset_graph_default.Cminauto[i]         = SW_BOOL_TRUE;
    pplset_graph_default.Crenorm[i]          = SW_BOOL_TRUE;
    pplset_graph_default.Creverse[i]         = SW_BOOL_FALSE;
   }
  pplset_graph_default.ColKey                = SW_ONOFF_ON;
  pplset_graph_default.ColKeyPos             = SW_COLKEYPOS_R;
  pplset_graph_default.ColMapColSpace        = SW_COLSPACE_RGB;
  strcpy(pplset_graph_default.ColMapExpr1, "(c1)");
  strcpy(pplset_graph_default.ColMapExpr2, "(c1)");
  strcpy(pplset_graph_default.ColMapExpr3, "(c1)");
  strcpy(pplset_graph_default.ColMapExpr4, "");
  strcpy(pplset_graph_default.MaskExpr   , "");
  pplset_graph_default.ContoursLabel         = SW_ONOFF_OFF;
  pplset_graph_default.ContoursListLen       = -1;
  for (i=0; i<MAX_CONTOURS; i++) pplset_graph_default.ContoursList[i] = 0.0;
  pplset_graph_default.ContoursN             = 12;
  pplObjZero(&pplset_graph_default.ContoursUnit,0);
  ppl_withWordsZero(&(pplset_graph_default.DataStyle),1);
  pplset_graph_default.DataStyle.linespoints = SW_STYLE_POINTS;
  pplset_graph_default.FontSize              = 1.0;
  ppl_withWordsZero(&(pplset_graph_default.FuncStyle),1);
  pplset_graph_default.FuncStyle.linespoints = SW_STYLE_LINES;
  pplset_graph_default.grid                  = SW_ONOFF_OFF;
  for (i=0; i<MAX_AXES; i++) pplset_graph_default.GridAxisX[i] = 0;
  for (i=0; i<MAX_AXES; i++) pplset_graph_default.GridAxisY[i] = 0;
  for (i=0; i<MAX_AXES; i++) pplset_graph_default.GridAxisZ[i] = 0;
  pplset_graph_default.GridAxisX[1]  = 1;
  pplset_graph_default.GridAxisY[1]  = 1;
  pplset_graph_default.GridAxisZ[1]  = 1;
  pplset_graph_default.GridMajColour = COLOUR_GREY60;
  pplset_graph_default.GridMajColour1= 0;
  pplset_graph_default.GridMajColour2= 0;
  pplset_graph_default.GridMajColour3= 0;
  pplset_graph_default.GridMajColour4= 0;
  pplset_graph_default.GridMinColour = COLOUR_GREY85;
  pplset_graph_default.GridMinColour1= 0;
  pplset_graph_default.GridMinColour2= 0;
  pplset_graph_default.GridMinColour3= 0;
  pplset_graph_default.GridMinColour4= 0;
  pplset_graph_default.key           = SW_ONOFF_ON;
  pplset_graph_default.KeyColumns    = 0;
  pplset_graph_default.KeyPos        = SW_KEYPOS_TR;
  pplObjZero(&(pplset_graph_default.KeyXOff),0);
  pplset_graph_default.KeyXOff.real  = 0.0;
  pplset_graph_default.KeyXOff.dimensionless = 0; pplset_graph_default.KeyXOff.exponent[UNIT_LENGTH] = 1;
  pplObjZero(&(pplset_graph_default.KeyYOff),0);
  pplset_graph_default.KeyYOff.real  = 0.0;
  pplset_graph_default.KeyYOff.dimensionless = 0; pplset_graph_default.KeyYOff.exponent[UNIT_LENGTH] = 1;
  pplset_graph_default.LineWidth     = 1.0;
  pplObjZero(&(pplset_graph_default.OriginX),0);
  pplset_graph_default.OriginX.real  = 0.0;
  pplset_graph_default.OriginX.dimensionless = 0; pplset_graph_default.OriginX.exponent[UNIT_LENGTH] = 1;
  pplObjZero(&(pplset_graph_default.OriginY),0);
  pplset_graph_default.OriginY.real  = 0.0;
  pplset_graph_default.OriginY.dimensionless = 0; pplset_graph_default.OriginY.exponent[UNIT_LENGTH] = 1;
  pplset_graph_default.PointSize     = 1.0;
  pplset_graph_default.PointLineWidth= 1.0;
  pplset_graph_default.projection    = SW_PROJ_FLAT;
  pplset_graph_default.samples       = 250;
  pplset_graph_default.SamplesX      = 40;
  pplset_graph_default.SamplesXAuto  = SW_BOOL_FALSE;
  pplset_graph_default.SamplesY      = 40;
  pplset_graph_default.SamplesYAuto  = SW_BOOL_FALSE;
  pplset_graph_default.Sample2DMethod= SW_SAMPLEMETHOD_NEAREST;
  pplset_graph_default.TextColour    = COLOUR_BLACK;
  pplset_graph_default.TextColour1   = 0;
  pplset_graph_default.TextColour2   = 0;
  pplset_graph_default.TextColour3   = 0;
  pplset_graph_default.TextColour4   = 0;
  pplset_graph_default.TextHAlign    = SW_HALIGN_LEFT;
  pplset_graph_default.TextVAlign = SW_VALIGN_BOT;
  strcpy(pplset_graph_default.title, "");
  pplObjZero(&(pplset_graph_default.TitleXOff),0);
  pplset_graph_default.TitleXOff.real= 0.0;
  pplset_graph_default.TitleXOff.dimensionless = 0; pplset_graph_default.TitleXOff.exponent[UNIT_LENGTH] = 1;
  pplObjZero(&(pplset_graph_default.TitleYOff),0);
  pplset_graph_default.TitleYOff.real= 0.0;
  pplset_graph_default.TitleYOff.dimensionless = 0; pplset_graph_default.TitleYOff.exponent[UNIT_LENGTH] = 1;
  pplset_graph_default.Tlog          = SW_BOOL_FALSE;
  pplObjZero(&(pplset_graph_default.Tmin),0);
  pplObjZero(&(pplset_graph_default.Tmax),0);
  pplset_graph_default.Tmin.real     = 0.0;
  pplset_graph_default.Tmax.real     = 1.0;
  pplset_graph_default.USE_T_or_uv   = 1;
  pplset_graph_default.Ulog          = SW_BOOL_FALSE;
  pplObjZero(&(pplset_graph_default.Umin),0);
  pplObjZero(&(pplset_graph_default.Umax),0);
  pplset_graph_default.Umin.real     = 0.0;
  pplset_graph_default.Umax.real     = 1.0;
  pplset_graph_default.Vlog          = SW_BOOL_FALSE;
  pplObjZero(&(pplset_graph_default.Vmin),0);
  pplObjZero(&(pplset_graph_default.Vmax),0);
  pplset_graph_default.Vmin.real     = 0.0;
  pplset_graph_default.Vmax.real     = 1.0;
  pplObjZero(&(pplset_graph_default.width),0);
  pplset_graph_default.width.real    = 0.08; // 8cm
  pplset_graph_default.width.dimensionless = 0; pplset_graph_default.width.exponent[UNIT_LENGTH] = 1;
  pplObjZero(&(pplset_graph_default.XYview),0);
  pplset_graph_default.XYview.real   = 60.0 * M_PI / 180; // 60 degrees
  pplset_graph_default.XYview.dimensionless = 0; pplset_graph_default.XYview.exponent[UNIT_ANGLE] = 1;
  pplObjZero(&(pplset_graph_default.YZview),0);
  pplset_graph_default.YZview.real   = 30.0 * M_PI / 180; // 30 degrees
  pplset_graph_default.YZview.dimensionless = 0; pplset_graph_default.YZview.exponent[UNIT_ANGLE] = 1;

  // Default Axis Settings, used whenever a new axis is created
  pplset_axis_default.atzero      = 0;
  pplset_axis_default.enabled     = 0;
  pplset_axis_default.invisible   = 0;
  pplset_axis_default.linked      = 0;
  pplset_axis_default.RangeReversed=0;
  pplset_axis_default.topbottom   = 0;
  pplset_axis_default.ArrowType   = SW_AXISDISP_NOARR;
  pplset_axis_default.LinkedAxisCanvasID = 0;
  pplset_axis_default.LinkedAxisToXYZ    = 0;
  pplset_axis_default.LinkedAxisToNum    = 0;
  pplset_axis_default.log         = SW_BOOL_FALSE;
  pplset_axis_default.MaxSet      = SW_BOOL_FALSE;
  pplset_axis_default.MinSet      = SW_BOOL_FALSE;
  pplset_axis_default.MirrorType  = SW_AXISMIRROR_AUTO;
  pplset_axis_default.MTickDir    = SW_TICDIR_IN;
  pplset_axis_default.MTickMaxSet = 0;
  pplset_axis_default.MTickMinSet = 0;
  pplset_axis_default.MTickStepSet= 0;
  pplset_axis_default.TickDir     = SW_TICDIR_IN;
  pplset_axis_default.TickLabelRotation  = SW_TICLABDIR_HORI;
  pplset_axis_default.TickMaxSet  = 0;
  pplset_axis_default.TickMinSet  = 0;
  pplset_axis_default.TickStepSet = 0;
  pplset_axis_default.LabelRotate =  0.0;
  pplset_axis_default.LogBase     = 10.0;
  pplset_axis_default.max         =  0.0;
  pplset_axis_default.min         =  0.0;
  pplset_axis_default.MTickMax    =  0.0;
  pplset_axis_default.MTickMin    =  0.0;
  pplset_axis_default.MTickStep   =  0.0;
  pplset_axis_default.TickLabelRotate = 0.0;
  pplset_axis_default.TickMax     =  0.0;
  pplset_axis_default.TickMin     =  0.0;
  pplset_axis_default.TickStep    =  0.0;
  pplset_axis_default.format      = NULL;
  pplset_axis_default.label       = NULL;
  pplset_axis_default.linkusing   = NULL;
  pplset_axis_default.MTickList   = NULL;
  pplset_axis_default.TickList    = NULL;
  pplset_axis_default.MTickStrs   = NULL;
  pplset_axis_default.TickStrs    = NULL;
  pplObjZero(&(pplset_axis_default.unit),0);

  // Set up list of input filters
  pplset_filters = ppl_dictInit(HASHSIZE_SMALL,0);
  pplObjNullStr(&tempval,0);
  #ifdef HAVE_FITSIO
  tempval.auxil    = (void *)FITSHELPER;
  tempval.auxilLen = strlen(FITSHELPER);
  ppl_dictAppend(pplset_filters, "*.fits", pplObjCpy(&tempval,0));
  #endif
  #ifdef TIMEHELPER
  tempval.auxil = (void *)TIMEHELPER;
  tempval.auxilLen = strlen(TIMEHELPER);
  ppl_dictAppend(pplset_filters, "*.log", pplObjCpy(&tempval,0));
  #endif
  #ifdef GUNZIP_COMMAND
  tempval.auxil = (void *)GUNZIP_COMMAND;
  tempval.auxilLen = strlen(GUNZIP_COMMAND);
  ppl_dictAppend(pplset_filters, "*.gz", pplObjCpy(&tempval,0));
  #endif
  #ifdef WGET_COMMAND
  tempval.auxil = (void *)WGET_COMMAND;
  tempval.auxilLen = strlen(WGET_COMMAND);
  ppl_dictAppend(pplset_filters, "http://*", pplObjCpy(&tempval,0));
  ppl_dictAppend(pplset_filters, "ftp://*", pplObjCpy(&tempval,0));
  #endif

  // Set up empty lists of arrows and labels
  pplarrow_list = pplarrow_list_default = NULL;
  ppllabel_list = ppllabel_list_default = NULL;

  // Set up array of plot styles
  for (i=0; i<MAX_PLOTSTYLES; i++) ppl_withWordsZero(&(pplset_plot_styles        [i]),1);
  for (i=0; i<MAX_PLOTSTYLES; i++) ppl_withWordsZero(&(pplset_plot_styles_default[i]),1);

  // Set up current axes. Because default axis contains no malloced strings, we don't need to use AxisCopy() here.
  for (i=0; i<MAX_AXES; i++) XAxes[i] = YAxes[i] = ZAxes[i] = pplset_axis_default;
  XAxes[1].enabled   = YAxes[1].enabled   = ZAxes[1].enabled   = 1; // By default, only axes 1 are enabled
  XAxes[2].topbottom = YAxes[2].topbottom = ZAxes[2].topbottom = 1; // By default, axes 2 are opposite axes 1
  for (i=0; i<MAX_AXES; i++) { XAxesDefault[i] = XAxes[i]; YAxesDefault[i] = YAxes[i]; ZAxesDefault[i] = ZAxes[i]; }

  // Setting which affect how we talk to the current interactive session
  pplset_session_default.splash    = SW_ONOFF_ON;
  pplset_session_default.colour    = SW_ONOFF_ON;
  pplset_session_default.colour_rep= SW_TERMCOL_GRN;
  pplset_session_default.colour_wrn= SW_TERMCOL_BRN;
  pplset_session_default.colour_err= SW_TERMCOL_RED;
  strcpy(pplset_session_default.homedir, ppl_unixGetHomeDir());

  // Estimate the machine precision of the floating point unit we are using
  ppl_makeMachineEpsilon();

  // Try and find out the default papersize from the locale command
  // Do this using the popen() command rather than direct calls to nl_langinfo(_NL_PAPER_WIDTH), because the latter is gnu-specific
  if (DEBUG) ppl_log("Querying papersize from the locale command.");
  if ((LocalePipe = popen("locale -c LC_PAPER 2> /dev/null","r"))==NULL)
   {
    if (DEBUG) ppl_log("Failed to open a pipe to the locale command.");
   } else {
    ppl_file_readline(LocalePipe, ConfigFname, FNAME_LENGTH); // Should read LC_PAPER
    ppl_file_readline(LocalePipe, ConfigFname, FNAME_LENGTH); // Should quote the default paper width
    PaperHeight = ppl_getFloat(ConfigFname, &Nchars);
    if (Nchars != strlen(ConfigFname)) goto LC_PAPERSIZE_DONE;
    ppl_file_readline(LocalePipe, ConfigFname, FNAME_LENGTH); // Should quote the default paper height
    PaperWidth  = ppl_getFloat(ConfigFname, &Nchars);
    if (Nchars != strlen(ConfigFname)) goto LC_PAPERSIZE_DONE;
    if (DEBUG) { sprintf(ppl_tempErrStr, "Read papersize %f x %f", PaperWidth, PaperHeight); ppl_log(ppl_tempErrStr); }
    pplset_term_default.PaperHeight.real   = PaperHeight/1000;
    pplset_term_default.PaperWidth.real    = PaperWidth /1000;
    if (0) { LC_PAPERSIZE_DONE: if (DEBUG) ppl_log("Failed to read papersize from the locale command."); }
    pclose(LocalePipe);
    ppl_GetPaperName(pplset_term_default.PaperName, &PaperHeight, &PaperWidth);
   }

  // Try and find out the default papersize from /etc/papersize
  if (DEBUG) ppl_log("Querying papersize from /etc/papersize.");
  if ((LocalePipe = fopen("/etc/papersize","r"))==NULL)
   {
    if (DEBUG) ppl_log("Failed to open /etc/papersize.");
   } else {
    ppl_file_readline(LocalePipe, ConfigFname, FNAME_LENGTH); // Should a papersize name
    ppl_PaperSizeByName(ConfigFname, &PaperHeight, &PaperWidth);
    if (PaperHeight > 0)
     {
      if (DEBUG) { sprintf(ppl_tempErrStr, "Read papersize %s, with dimensions %f x %f", ConfigFname, PaperWidth, PaperHeight); ppl_log(ppl_tempErrStr); }
      pplset_term_default.PaperHeight.real   = PaperHeight/1000;
      pplset_term_default.PaperWidth.real    = PaperWidth /1000;
      ppl_GetPaperName(pplset_term_default.PaperName, &PaperHeight, &PaperWidth);
     } else {
      if (DEBUG) ppl_log("/etc/papersize returned an unrecognised papersize.");
     }
    fclose(LocalePipe);
   }

  // Try and find out the default papersize from PAPERSIZE environment variable
  if (DEBUG) ppl_log("Querying papersize from $PAPERSIZE");
  PaperSizePtr = getenv("PAPERSIZE");
  if (PaperSizePtr == NULL)
   {
    if (DEBUG) ppl_log("Environment variable $PAPERSIZE not set.");
   } else {
    ppl_PaperSizeByName(PaperSizePtr, &PaperHeight, &PaperWidth);
    if (PaperHeight > 0)
     {
      if (DEBUG) { sprintf(ppl_tempErrStr, "Read papersize %s, with dimensions %f x %f", PaperSizePtr, PaperWidth, PaperHeight); ppl_log(ppl_tempErrStr); }
      pplset_term_default.PaperHeight.real   = PaperHeight/1000;
      pplset_term_default.PaperWidth.real    = PaperWidth /1000;
      ppl_GetPaperName(pplset_term_default.PaperName, &PaperHeight, &PaperWidth);
     } else {
      if (DEBUG) ppl_log("$PAPERSIZE returned an unrecognised paper size.");
     }
   }

  // Copy Default Settings to Current Settings
  pplset_term_current  = pplset_term_default;
  pplset_graph_current = pplset_graph_default;
  for (i=0; i<PALETTE_LENGTH; i++)
   {
    pplset_palette_current [i] = pplset_palette_default [i];
    pplset_paletteS_current[i] = pplset_paletteS_default[i];
    pplset_palette1_current[i] = pplset_palette1_default[i];
    pplset_palette2_current[i] = pplset_palette2_default[i];
    pplset_palette3_current[i] = pplset_palette3_default[i];
    pplset_palette4_current[i] = pplset_palette4_default[i];
   }
  return;
 }

