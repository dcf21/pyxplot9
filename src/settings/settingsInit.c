// settingsInit.c
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

#define _SETTINGSINIT_C 1

int cancellationFlag = 0;

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

#include "settings/epsColors.h"

#include "pplConstants.h"
#include "settings/papersizes.h"
#include "settings/settings_fns.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"

#include "userspace/pplObj_fns.h"

void pplset_makedefault(ppl_context *context)
 {
  FILE  *LocalePipe;
  int    Nchars,i;
  double PaperWidth, PaperHeight;
  pplObj tempval;
  char   ConfigFname[FNAME_LENGTH];
  char  *PaperSizePtr;
  ppl_settings *s = context->set;
  pplerr_context *se = &context->errcontext;

  const int default_cols[] = {COLOR_BLACK, COLOR_RED, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_BROWN, COLOR_SALMON, COLOR_GRAY, COLOR_GREEN, COLOR_NAVYBLUE, COLOR_PERIWINKLE, COLOR_PINEGREEN, COLOR_SEAGREEN, COLOR_GREENYELLOW, COLOR_ORANGE, COLOR_CARNATIONPINK, COLOR_PLUM, -1};

  // Clear settings
  memset((void *)s, 0, sizeof(s));

  // Default palette
  for (i=0; default_cols[i]>=0; i++)
   {
    s->palette_default [i] = default_cols[i];
    s->paletteS_default[i] = SW_COLSPACE_CMYK;
    s->palette1_default[i] = SW_COLOR_CMYK_C[default_cols[i]];
    s->palette2_default[i] = SW_COLOR_CMYK_M[default_cols[i]];
    s->palette3_default[i] = SW_COLOR_CMYK_Y[default_cols[i]];
    s->palette4_default[i] = SW_COLOR_CMYK_K[default_cols[i]];
   }
  s->palette_default[i] = -1;

  // Default Terminal Settings, used when these values are not changed by any configuration files
  s->term_default.backup              = SW_ONOFF_OFF;
  s->term_default.BinOrigin.refCount=1;
  pplObjNum(&(s->term_default.BinOrigin),0,0.0,0.0);
  s->term_default.BinOriginAuto       = 1;
  s->term_default.BinWidth.refCount=1;
  pplObjNum(&(s->term_default.BinWidth),0,0.0,0.0);
  s->term_default.BinWidth.real       = 1.0;
  s->term_default.BinWidthAuto        = 1;
  s->term_default.CalendarIn          = SW_CALENDAR_BRITISH;
  s->term_default.CalendarOut         = SW_CALENDAR_BRITISH;
  s->term_default.color              = SW_ONOFF_ON;
  s->term_default.ComplexNumbers      = SW_ONOFF_OFF;
  s->term_default.display             = SW_ONOFF_ON;
  s->term_default.dpi                 = 300.0;
  s->term_default.ExplicitErrors      = SW_ONOFF_ON;
  s->term_default.landscape           = SW_ONOFF_OFF;
  strcpy(s->term_default.LatexPreamble, "");
  s->term_default.multiplot           = SW_ONOFF_OFF;
  s->term_default.NumDisplay          = SW_DISPLAY_N;
  strcpy(s->term_default.output, "");
  s->term_default.PaperHeight.refCount=1;
  pplObjNum(&(s->term_default.PaperHeight),0,0.0,0.0);
  s->term_default.PaperHeight.real    = 297.30178 / 1000;
  s->term_default.PaperHeight.dimensionless = 0; s->term_default.PaperHeight.exponent[UNIT_LENGTH] = 1;
  strcpy(s->term_default.PaperName, "a4");
  s->term_default.PaperWidth.refCount=1;
  pplObjNum(&(s->term_default.PaperWidth),0,0.0,0.0);
  s->term_default.PaperWidth.real     = 210.2241 / 1000;
  s->term_default.PaperWidth.dimensionless = 0; s->term_default.PaperWidth.exponent[UNIT_LENGTH] = 1;
  s->term_default.RandomSeed          = 0;
  s->term_default.SignificantFigures  = 8;
  s->term_default.TermAntiAlias       = SW_ONOFF_ON;
  s->term_default.TermType            = SW_TERMTYPE_X11S;
  s->term_default.TermEnlarge         = SW_ONOFF_OFF;
  s->term_default.TermInvert          = SW_ONOFF_OFF;
  s->term_default.TermTransparent     = SW_ONOFF_OFF;
  s->term_default.UnitScheme          = SW_UNITSCH_SI;
  s->term_default.UnitDisplayPrefix   = SW_ONOFF_ON;
  s->term_default.UnitDisplayAbbrev   = SW_ONOFF_ON;
  s->term_default.UnitAngleDimless    = SW_ONOFF_ON;
  s->term_default.viewer              = SW_VIEWER_GV;
  strcpy(s->term_default.ViewerCmd, "");

  // Default Graph Settings, used when these values are not changed by any configuration files
  s->graph_default.aspect                = 1.0;
  s->graph_default.zaspect               = 1.0;
  s->graph_default.AutoAspect            = SW_ONOFF_ON;
  s->graph_default.AutoZAspect           = SW_ONOFF_ON;
  s->graph_default.AxesColour            = COLOR_BLACK;
  s->graph_default.AxesColour1           = 0.0;
  s->graph_default.AxesColour2           = 0.0;
  s->graph_default.AxesColour3           = 0.0;
  s->graph_default.AxesColour4           = 0.0;
  s->graph_default.AxisUnitStyle         = SW_AXISUNITSTY_RATIO;
  s->graph_default.bar                   = 1.0;
  s->graph_default.BoxFrom.refCount=1;
  pplObjNum(&(s->graph_default.BoxFrom),0,0.0,0.0);
  s->graph_default.BoxFromAuto           = 1;
  s->graph_default.BoxWidth.refCount=1;
  pplObjNum(&(s->graph_default.BoxWidth),0,0.0,0.0);
  s->graph_default.BoxWidthAuto          = 1;
  s->graph_default.c1format              = NULL;
  s->graph_default.c1formatset           = 0;
  strcpy(s->graph_default.c1label, "");
  s->graph_default.c1LabelRotate         = 0.0;
  s->graph_default.c1TickLabelRotate     = 0.0;
  s->graph_default.c1TickLabelRotation   = SW_TICLABDIR_HORI;
  s->graph_default.clip                  = SW_ONOFF_OFF;
  for (i=0; i<4; i++)
   {
    s->graph_default.Clog[i]             = SW_BOOL_FALSE;
    s->graph_default.Cmax[i].refCount=1;
    pplObjNum(&s->graph_default.Cmax[i],0,0.0,0.0);
    s->graph_default.Cmaxauto[i]         = SW_BOOL_TRUE;
    s->graph_default.Cmin[i].refCount=1;
    pplObjNum(&s->graph_default.Cmin[i],0,0.0,0.0);
    s->graph_default.Cminauto[i]         = SW_BOOL_TRUE;
    s->graph_default.Crenorm[i]          = SW_BOOL_TRUE;
    s->graph_default.Creverse[i]         = SW_BOOL_FALSE;
   }
  s->graph_default.ColKey                = SW_ONOFF_ON;
  s->graph_default.ColKeyPos             = SW_COLKEYPOS_R;
  s->graph_default.ColMapExpr            = NULL;
  s->graph_default.MaskExpr              = NULL;
  s->graph_default.ContoursLabel         = SW_ONOFF_OFF;
  s->graph_default.ContoursListLen       = -1;
  for (i=0; i<MAX_CONTOURS; i++) s->graph_default.ContoursList[i] = 0.0;
  s->graph_default.ContoursN             = 12;
  s->graph_default.ContoursUnit.refCount=1;
  pplObjNum(&s->graph_default.ContoursUnit,0,0.0,0.0);
  ppl_withWordsZero(context,&(s->graph_default.dataStyle));
  s->graph_default.dataStyle.linespoints = SW_STYLE_POINTS;
  s->graph_default.FontSize              = 1.0;
  ppl_withWordsZero(context,&(s->graph_default.funcStyle));
  s->graph_default.funcStyle.linespoints = SW_STYLE_LINES;
  s->graph_default.grid                  = SW_ONOFF_OFF;
  for (i=0; i<MAX_AXES; i++) s->graph_default.GridAxisX[i] = 0;
  for (i=0; i<MAX_AXES; i++) s->graph_default.GridAxisY[i] = 0;
  for (i=0; i<MAX_AXES; i++) s->graph_default.GridAxisZ[i] = 0;
  s->graph_default.GridAxisX[1]  = 1;
  s->graph_default.GridAxisY[1]  = 1;
  s->graph_default.GridAxisZ[1]  = 1;
  s->graph_default.GridMajColour = COLOR_GREY60;
  s->graph_default.GridMajColour1= 0;
  s->graph_default.GridMajColour2= 0;
  s->graph_default.GridMajColour3= 0;
  s->graph_default.GridMajColour4= 0;
  s->graph_default.GridMinColour = COLOR_GREY85;
  s->graph_default.GridMinColour1= 0;
  s->graph_default.GridMinColour2= 0;
  s->graph_default.GridMinColour3= 0;
  s->graph_default.GridMinColour4= 0;
  s->graph_default.key           = SW_ONOFF_ON;
  s->graph_default.KeyColumns    = 0;
  s->graph_default.KeyPos        = SW_KEYPOS_TR;
  s->graph_default.KeyXOff.refCount=1;
  pplObjNum(&(s->graph_default.KeyXOff),0,0.0,0.0);
  s->graph_default.KeyXOff.real  = 0.0;
  s->graph_default.KeyXOff.dimensionless = 0; s->graph_default.KeyXOff.exponent[UNIT_LENGTH] = 1;
  s->graph_default.KeyYOff.refCount=1;
  pplObjNum(&(s->graph_default.KeyYOff),0,0.0,0.0);
  s->graph_default.KeyYOff.real  = 0.0;
  s->graph_default.KeyYOff.dimensionless = 0; s->graph_default.KeyYOff.exponent[UNIT_LENGTH] = 1;
  s->graph_default.LineWidth     = 1.0;
  s->graph_default.OriginX.refCount=1;
  pplObjNum(&(s->graph_default.OriginX),0,0.0,0.0);
  s->graph_default.OriginX.real  = 0.0;
  s->graph_default.OriginX.dimensionless = 0; s->graph_default.OriginX.exponent[UNIT_LENGTH] = 1;
  s->graph_default.OriginY.refCount=1;
  pplObjNum(&(s->graph_default.OriginY),0,0.0,0.0);
  s->graph_default.OriginY.real  = 0.0;
  s->graph_default.OriginY.dimensionless = 0; s->graph_default.OriginY.exponent[UNIT_LENGTH] = 1;
  s->graph_default.PointSize     = 1.0;
  s->graph_default.PointLineWidth= 1.0;
  s->graph_default.projection    = SW_PROJ_FLAT;
  s->graph_default.samples       = 250;
  s->graph_default.SamplesX      = 40;
  s->graph_default.SamplesXAuto  = SW_BOOL_FALSE;
  s->graph_default.SamplesY      = 40;
  s->graph_default.SamplesYAuto  = SW_BOOL_FALSE;
  s->graph_default.Sample2DMethod= SW_SAMPLEMETHOD_NEAREST;
  s->graph_default.TextColour    = COLOR_BLACK;
  s->graph_default.TextColour1   = 0;
  s->graph_default.TextColour2   = 0;
  s->graph_default.TextColour3   = 0;
  s->graph_default.TextColour4   = 0;
  s->graph_default.TextHAlign    = SW_HALIGN_LEFT;
  s->graph_default.TextVAlign = SW_VALIGN_BOT;
  strcpy(s->graph_default.title, "");
  s->graph_default.TitleXOff.refCount=1;
  pplObjNum(&(s->graph_default.TitleXOff),0,0.0,0.0);
  s->graph_default.TitleXOff.real= 0.0;
  s->graph_default.TitleXOff.dimensionless = 0; s->graph_default.TitleXOff.exponent[UNIT_LENGTH] = 1;
  s->graph_default.TitleYOff.refCount=1;
  pplObjNum(&(s->graph_default.TitleYOff),0,0.0,0.0);
  s->graph_default.TitleYOff.real= 0.0;
  s->graph_default.TitleYOff.dimensionless = 0; s->graph_default.TitleYOff.exponent[UNIT_LENGTH] = 1;
  s->graph_default.Tlog          = SW_BOOL_FALSE;
  s->graph_default.Tmin.refCount=1;
  pplObjNum(&(s->graph_default.Tmin),0,0.0,0.0);
  s->graph_default.Tmax.refCount=1;
  pplObjNum(&(s->graph_default.Tmax),0,0.0,0.0);
  s->graph_default.Tmin.real     = 0.0;
  s->graph_default.Tmax.real     = 1.0;
  s->graph_default.USE_T_or_uv   = 1;
  s->graph_default.Ulog          = SW_BOOL_FALSE;
  s->graph_default.Umin.refCount=1;
  pplObjNum(&(s->graph_default.Umin),0,0.0,0.0);
  s->graph_default.Umax.refCount=1;
  pplObjNum(&(s->graph_default.Umax),0,0.0,0.0);
  s->graph_default.Umin.real     = 0.0;
  s->graph_default.Umax.real     = 1.0;
  s->graph_default.Vlog          = SW_BOOL_FALSE;
  s->graph_default.Vmin.refCount=1;
  pplObjNum(&(s->graph_default.Vmin),0,0.0,0.0);
  s->graph_default.Vmax.refCount=1;
  pplObjNum(&(s->graph_default.Vmax),0,0.0,0.0);
  s->graph_default.Vmin.real     = 0.0;
  s->graph_default.Vmax.real     = 1.0;
  s->graph_default.width.refCount=1;
  pplObjNum(&(s->graph_default.width),0,0.0,0.0);
  s->graph_default.width.real    = 0.08; // 8cm
  s->graph_default.width.dimensionless = 0; s->graph_default.width.exponent[UNIT_LENGTH] = 1;
  s->graph_default.XYview.refCount=1;
  pplObjNum(&(s->graph_default.XYview),0,0.0,0.0);
  s->graph_default.XYview.real   = 60.0 * M_PI / 180; // 60 degrees
  s->graph_default.XYview.dimensionless = 0; s->graph_default.XYview.exponent[UNIT_ANGLE] = 1;
  s->graph_default.YZview.refCount=1;
  pplObjNum(&(s->graph_default.YZview),0,0.0,0.0);
  s->graph_default.YZview.real   = 30.0 * M_PI / 180; // 30 degrees
  s->graph_default.YZview.dimensionless = 0; s->graph_default.YZview.exponent[UNIT_ANGLE] = 1;

  // Default Axis Settings, used whenever a new axis is created
  s->axis_default.atzero      = 0;
  s->axis_default.enabled     = 0;
  s->axis_default.invisible   = 0;
  s->axis_default.linked      = 0;
  s->axis_default.RangeReversed=0;
  s->axis_default.topbottom   = 0;
  s->axis_default.ArrowType   = SW_AXISDISP_NOARR;
  s->axis_default.LinkedAxisCanvasID = 0;
  s->axis_default.LinkedAxisToXYZ    = 0;
  s->axis_default.LinkedAxisToNum    = 0;
  s->axis_default.log         = SW_BOOL_FALSE;
  s->axis_default.MaxSet      = SW_BOOL_FALSE;
  s->axis_default.MinSet      = SW_BOOL_FALSE;
  s->axis_default.MirrorType  = SW_AXISMIRROR_AUTO;
  s->axis_default.MTickDir    = SW_TICDIR_IN;
  s->axis_default.MTickMaxSet = 0;
  s->axis_default.MTickMinSet = 0;
  s->axis_default.MTickStepSet= 0;
  s->axis_default.TickDir     = SW_TICDIR_IN;
  s->axis_default.TickLabelRotation  = SW_TICLABDIR_HORI;
  s->axis_default.TickMaxSet  = 0;
  s->axis_default.TickMinSet  = 0;
  s->axis_default.TickStepSet = 0;
  s->axis_default.LabelRotate =  0.0;
  s->axis_default.LogBase     = 10.0;
  s->axis_default.max         =  0.0;
  s->axis_default.min         =  0.0;
  s->axis_default.MTickMax    =  0.0;
  s->axis_default.MTickMin    =  0.0;
  s->axis_default.MTickStep   =  0.0;
  s->axis_default.TickLabelRotate = 0.0;
  s->axis_default.TickMax     =  0.0;
  s->axis_default.TickMin     =  0.0;
  s->axis_default.TickStep    =  0.0;
  s->axis_default.format      = NULL;
  s->axis_default.label       = NULL;
  s->axis_default.linkusing   = NULL;
  s->axis_default.MTickList   = NULL;
  s->axis_default.TickList    = NULL;
  s->axis_default.MTickStrs   = NULL;
  s->axis_default.TickStrs    = NULL;
  s->axis_default.unit.refCount=1;
  pplObjNum(&(s->axis_default.unit),0,0.0,0.0);

  // Set up list of input filters
  tempval.refCount=1;
  s->filters = ppl_dictInit(HASHSIZE_SMALL,1);
  #ifdef HAVE_FITSIO
  pplObjStr(&tempval,0,0,FITSHELPER);
  ppl_dictAppend(s->filters, "*.fits", pplObjCpy(NULL,&tempval,0,1,1));
  #endif
  #ifdef TIMEHELPER
  pplObjStr(&tempval,0,0,TIMEHELPER);
  ppl_dictAppend(s->filters, "*.log", pplObjCpy(NULL,&tempval,0,1,1));
  #endif
  #ifdef GUNZIP_COMMAND
  pplObjStr(&tempval,0,0,GUNZIP_COMMAND);
  ppl_dictAppend(s->filters, "*.gz", pplObjCpy(NULL,&tempval,0,1,1));
  #endif
  #ifdef WGET_COMMAND
  pplObjStr(&tempval,0,0,WGET_COMMAND);
  ppl_dictAppend(s->filters, "http://*", pplObjCpy(NULL,&tempval,0,1,1));
  ppl_dictAppend(s->filters, "ftp://*", pplObjCpy(NULL,&tempval,0,1,1));
  #endif

  // Set up empty lists of arrows and labels
  s->pplarrow_list = s->pplarrow_list_default = NULL;
  s->ppllabel_list = s->ppllabel_list_default = NULL;

  // Set up array of plot styles
  for (i=0; i<MAX_PLOTSTYLES; i++) ppl_withWordsZero(context,&(s->plot_styles        [i]));
  for (i=0; i<MAX_PLOTSTYLES; i++) ppl_withWordsZero(context,&(s->plot_styles_default[i]));

  // Set up current axes. Because default axis contains no malloced strings, we don't need to use AxisCopy() here.
  for (i=0; i<MAX_AXES; i++) s->XAxes[i] = s->YAxes[i] = s->ZAxes[i] = s->axis_default;
  s->XAxes[1].enabled   = s->YAxes[1].enabled   = s->ZAxes[1].enabled   = 1; // By default, only axes 1 are enabled
  s->XAxes[2].topbottom = s->YAxes[2].topbottom = s->ZAxes[2].topbottom = 1; // By default, axes 2 are opposite axes 1
  for (i=0; i<MAX_AXES; i++) { s->XAxesDefault[i] = s->XAxes[i]; s->YAxesDefault[i] = s->YAxes[i]; s->ZAxesDefault[i] = s->ZAxes[i]; }

  // Setting which affect how we talk to the current interactive session
  se->session_default.splash    = SW_ONOFF_ON;
  se->session_default.color    = SW_ONOFF_ON;
  se->session_default.color_rep= SW_TERMCOL_GRN;
  se->session_default.color_wrn= SW_TERMCOL_BRN;
  se->session_default.color_err= SW_TERMCOL_RED;
  strcpy(se->session_default.homedir, ppl_unixGetHomeDir(&context->errcontext));

  // Estimate the machine precision of the floating point unit we are using
  ppl_makeMachineEpsilon();

  // Try and find out the default papersize from the locale command
  // Do this using the popen() command rather than direct calls to nl_langinfo(_NL_PAPER_WIDTH), because the latter is gnu-specific
  if (DEBUG) ppl_log(&context->errcontext,"Querying papersize from the locale command.");
  if ((LocalePipe = popen("locale -c LC_PAPER 2> /dev/null","r"))==NULL)
   {
    if (DEBUG) ppl_log(&context->errcontext,"Failed to open a pipe to the locale command.");
   } else {
    ppl_file_readline(LocalePipe, ConfigFname, FNAME_LENGTH); // Should read LC_PAPER
    ppl_file_readline(LocalePipe, ConfigFname, FNAME_LENGTH); // Should quote the default paper width
    PaperHeight = ppl_getFloat(ConfigFname, &Nchars);
    if (Nchars != strlen(ConfigFname)) goto LC_PAPERSIZE_DONE;
    ppl_file_readline(LocalePipe, ConfigFname, FNAME_LENGTH); // Should quote the default paper height
    PaperWidth  = ppl_getFloat(ConfigFname, &Nchars);
    if (Nchars != strlen(ConfigFname)) goto LC_PAPERSIZE_DONE;
    if (DEBUG) { sprintf(context->errcontext.tempErrStr, "Read papersize %f x %f", PaperWidth, PaperHeight); ppl_log(&context->errcontext,NULL); }
    s->term_default.PaperHeight.real   = PaperHeight/1000;
    s->term_default.PaperWidth.real    = PaperWidth /1000;
    if (0) { LC_PAPERSIZE_DONE: if (DEBUG) ppl_log(&context->errcontext,"Failed to read papersize from the locale command."); }
    pclose(LocalePipe);
    ppl_GetPaperName(s->term_default.PaperName, &PaperHeight, &PaperWidth);
   }

  // Try and find out the default papersize from /etc/papersize
  if (DEBUG) ppl_log(&context->errcontext,"Querying papersize from /etc/papersize.");
  if ((LocalePipe = fopen("/etc/papersize","r"))==NULL)
   {
    if (DEBUG) ppl_log(&context->errcontext,"Failed to open /etc/papersize.");
   } else {
    ppl_file_readline(LocalePipe, ConfigFname, FNAME_LENGTH); // Should a papersize name
    ppl_PaperSizeByName(ConfigFname, &PaperHeight, &PaperWidth);
    if (PaperHeight > 0)
     {
      if (DEBUG) { sprintf(context->errcontext.tempErrStr, "Read papersize %s, with dimensions %f x %f", ConfigFname, PaperWidth, PaperHeight); ppl_log(&context->errcontext,NULL); }
      s->term_default.PaperHeight.real   = PaperHeight/1000;
      s->term_default.PaperWidth.real    = PaperWidth /1000;
      ppl_GetPaperName(s->term_default.PaperName, &PaperHeight, &PaperWidth);
     } else {
      if (DEBUG) ppl_log(&context->errcontext,"/etc/papersize returned an unrecognised papersize.");
     }
    fclose(LocalePipe);
   }

  // Try and find out the default papersize from PAPERSIZE environment variable
  if (DEBUG) ppl_log(&context->errcontext,"Querying papersize from $PAPERSIZE");
  PaperSizePtr = getenv("PAPERSIZE");
  if (PaperSizePtr == NULL)
   {
    if (DEBUG) ppl_log(&context->errcontext,"Environment variable $PAPERSIZE not set.");
   } else {
    ppl_PaperSizeByName(PaperSizePtr, &PaperHeight, &PaperWidth);
    if (PaperHeight > 0)
     {
      if (DEBUG) { sprintf(context->errcontext.tempErrStr, "Read papersize %s, with dimensions %f x %f", PaperSizePtr, PaperWidth, PaperHeight); ppl_log(&context->errcontext,NULL); }
      s->term_default.PaperHeight.real   = PaperHeight/1000;
      s->term_default.PaperWidth.real    = PaperWidth /1000;
      ppl_GetPaperName(s->term_default.PaperName, &PaperHeight, &PaperWidth);
     } else {
      if (DEBUG) ppl_log(&context->errcontext,"$PAPERSIZE returned an unrecognised paper size.");
     }
   }

  // Copy Default Settings to Current Settings
  s->term_current  = s->term_default;
  s->graph_current = s->graph_default;
  for (i=0; i<PALETTE_LENGTH; i++)
   {
    s->palette_current [i] = s->palette_default [i];
    s->paletteS_current[i] = s->paletteS_default[i];
    s->palette1_current[i] = s->palette1_default[i];
    s->palette2_current[i] = s->palette2_default[i];
    s->palette3_current[i] = s->palette3_default[i];
    s->palette4_current[i] = s->palette4_default[i];
   }
  return;
 }

