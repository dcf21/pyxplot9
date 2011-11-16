// settings.h
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

#ifndef _SETTINGS_H
#define _SETTINGS_H 1

#include "stringTools/strConstants.h"

#include "coreUtils/dict.h"

#include "pplConstants.h"
#include "userspace/pplObj.h"

#include "settings/arrows.h"
#include "settings/labels.h"
#include "settings/withWords.h"

// Setting structures
typedef struct pplset_terminal {
 int    backup, CalendarIn, CalendarOut, colour, ComplexNumbers, display, ExplicitErrors, landscape, multiplot, NumDisplay, SignificantFigures, TermAntiAlias, TermType, TermEnlarge, TermInvert, TermTransparent, UnitScheme, UnitDisplayPrefix, UnitDisplayAbbrev, UnitAngleDimless, viewer;
 long int RandomSeed;
 double dpi;
 unsigned char BinOriginAuto, BinWidthAuto;
 pplObj BinOrigin, BinWidth, PaperHeight, PaperWidth;
 char   output[FNAME_LENGTH];
 char   PaperName[FNAME_LENGTH];
 char   LatexPreamble[FNAME_LENGTH];
 char   ViewerCmd[FNAME_LENGTH];
 } pplset_terminal;

typedef struct pplset_graph {
 int           AutoAspect, AutoZAspect, AxesColour, AxesCol1234Space, AxisUnitStyle, clip, Clog[4], Cminauto[4], Cmaxauto[4], Crenorm[4], Creverse[4], ColKey, ColKeyPos, ColMapColSpace, ContoursLabel, ContoursListLen, ContoursN, grid, GridMajColour, GridMajCol1234Space, GridMinColour, GridMinCol1234Space, key, KeyColumns, KeyPos, samples, SamplesX, SamplesXAuto, SamplesY, SamplesYAuto, Sample2DMethod, TextColour, TextCol1234Space, TextHAlign, TextVAlign, Tlog, Ulog, Vlog;
 double        AxesColour1, AxesColour2, AxesColour3, AxesColour4, GridMajColour1, GridMajColour2, GridMajColour3, GridMajColour4, GridMinColour1, GridMinColour2, GridMinColour3, GridMinColour4, TextColour1, TextColour2, TextColour3, TextColour4;
 double        aspect, zaspect, bar, ContoursList[MAX_CONTOURS], FontSize, LineWidth, PointSize, PointLineWidth, projection;
 unsigned char GridAxisX[MAX_AXES], GridAxisY[MAX_AXES], GridAxisZ[MAX_AXES];
 unsigned char BoxFromAuto, BoxWidthAuto, USE_T_or_uv;
 pplObj        BoxFrom, BoxWidth, Cmin[4], Cmax[4], ContoursUnit, KeyXOff, KeyYOff, OriginX, OriginY, TitleXOff, TitleYOff, Tmin, Tmax, Umin, Umax, Vmin, Vmax, width, XYview, YZview;
 char          title[FNAME_LENGTH], ColMapExpr1[FNAME_LENGTH], ColMapExpr2[FNAME_LENGTH], ColMapExpr3[FNAME_LENGTH], ColMapExpr4[FNAME_LENGTH], MaskExpr[FNAME_LENGTH];
 char          c1label[FNAME_LENGTH], c1format[FNAME_LENGTH], c1formatset;
 double        c1LabelRotate, c1TickLabelRotate;
 int           c1TickLabelRotation;
 withWords     DataStyle, FuncStyle;
 } pplset_graph;

typedef struct pplset_session {
 int   splash, colour, colour_rep, colour_wrn, colour_err;
 char  cwd[FNAME_LENGTH];
 char  tempdir[FNAME_LENGTH];
 char  homedir[FNAME_LENGTH];
 } pplset_session;

typedef struct pplset_axis {
 unsigned char atzero, enabled, invisible, linked, RangeReversed, topbottom, MTickMaxSet, MTickMinSet, MTickStepSet, TickMaxSet, TickMinSet, TickStepSet;
 int     ArrowType, LinkedAxisCanvasID, LinkedAxisToXYZ, LinkedAxisToNum, log, MaxSet, MinSet, MirrorType, MTickDir, TickDir, TickLabelRotation;
 double  LabelRotate, LogBase, max, min, MTickMax, MTickMin, MTickStep, TickLabelRotate, TickMax, TickMin, TickStep;
 char   *format, *label, *linkusing;
 double *MTickList, *TickList;
 char  **MTickStrs,**TickStrs;
 pplObj  unit;

 // Temporary data fields which are used when rendering an axis to postscript
 int           AxisValueTurnings;
 double       *AxisLinearInterpolation;
 int          *AxisTurnings;
 unsigned char CrossedAtZero;
 unsigned char MinUsedSet, MaxUsedSet, DataUnitSet, RangeFinalised, FinalActive;
 double        PhysicalLengthMajor, PhysicalLengthMinor;
 int           xyz, axis_n, canvas_id, FirstTextID;
 double        MinUsed, MaxUsed, MinFinal, MaxFinal, *OrdinateRaster;
 double        HardMin, HardMax; // Contains ranges set via plot [foo:bar]
 unsigned char HardMinSet, HardMaxSet, HardAutoMinSet, HardAutoMaxSet, HardUnitSet, Mode0BackPropagated;
 int           OrdinateRasterLen, LogFinal;
 pplObj        HardUnit, DataUnit;
 char         *FinalAxisLabel;
 unsigned char TickListFinalised;
 double       *TickListPositions,  *MTickListPositions;
 char        **TickListStrings  , **MTickListStrings;
 } pplset_axis;

// Variable initialisation functions
void  pplset_makedefault(ppl_context *context);

// Variables defined in settingsInit.c
#ifndef _SETTINGSINIT_C
extern pplset_terminal  pplset_term_default;
extern pplset_terminal  pplset_term_current;
extern pplset_graph     pplset_graph_default;
extern pplset_graph     pplset_graph_current;
extern pplset_session   pplset_session_default;
extern int              pplset_palette_current[], pplset_paletteS_current[];
extern double           pplset_palette1_current[], pplset_palette2_current[], pplset_palette3_current[], pplset_palette4_current[];
extern int              pplset_palette_default[], pplset_paletteS_default[];
extern double           pplset_palette1_default[], pplset_palette2_default[], pplset_palette3_default[], pplset_palette4_default[];
extern withWords        pplset_plot_styles[], pplset_plot_styles_default[];
extern pplset_axis      pplset_axis_default;
extern pplset_axis      XAxes[], XAxesDefault[];
extern pplset_axis      YAxes[], YAxesDefault[];
extern pplset_axis      ZAxes[], ZAxesDefault[];
extern dict            *pplset_filters;
extern pplarrow_object *pplarrow_list, *pplarrow_list_default;
extern ppllabel_object *ppllabel_list, *ppllabel_list_default;
#endif

#endif

