// settings.h
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

#ifndef _SETTINGS_H
#define _SETTINGS_H 1

#include "stringTools/strConstants.h"

#include "coreUtils/dict.h"

#include "pplConstants.h"
#include "userspace/pplObj.h"
#include "settings/withWords.h"

#ifndef _SETTINGSINIT_C
extern int cancellationFlag;
#endif

// Setting structures
typedef struct pplset_terminal {
 int    backup, CalendarIn, CalendarOut, color, ComplexNumbers, display, ExplicitErrors, landscape, multiplot, NumDisplay, SignificantFigures, TermAntiAlias, TermType, TermEnlarge, TermInvert, TermTransparent, UnitScheme, UnitDisplayPrefix, UnitDisplayAbbrev, UnitAngleDimless, viewer;
 long int RandomSeed;
 double dpi;
 unsigned char BinOriginAuto, BinWidthAuto;
 pplObj BinOrigin, BinWidth, PaperHeight, PaperWidth;
 char   output[FNAME_LENGTH];
 char   PaperName[FNAME_LENGTH];
 char   LatexPreamble[FNAME_LENGTH];
 char   ViewerCmd[FNAME_LENGTH];
 char   timezone[FNAME_LENGTH];
 } pplset_terminal;

typedef struct pplset_tics {
 int           logBase, tickDir;
 unsigned char tickMaxSet, tickMinSet, tickStepSet;
 double        tickMax, tickMin, tickStep;
 double       *tickList;
 char        **tickStrs;
 } pplset_tics;

typedef struct pplset_graph {
 int           AutoAspect, AutoZAspect, AxesColor, AxesCol1234Space, AxisUnitStyle, clip, Clog[4], Cminauto[4], Cmaxauto[4], Crenorm[4], Creverse[4], ColKey, ColKeyPos, ContoursLabel, ContoursListLen, ContoursN, grid, GridMajColor, GridMajCol1234Space, GridMinColor, GridMinCol1234Space, key, KeyColumns, KeyPos, samples, SamplesX, SamplesXAuto, SamplesY, SamplesYAuto, Sample2DMethod, TextColor, TextCol1234Space, TextHAlign, TextVAlign, Tlog, Ulog, Vlog;
 double        AxesColor1, AxesColor2, AxesColor3, AxesColor4, GridMajColor1, GridMajColor2, GridMajColor3, GridMajColor4, GridMinColor1, GridMinColor2, GridMinColor3, GridMinColor4, TextColor1, TextColor2, TextColor3, TextColor4;
 double        aspect, zaspect, bar, ContoursList[MAX_CONTOURS], FontSize, LineWidth, PointSize, PointLineWidth, projection;
 unsigned char GridAxisX[MAX_AXES], GridAxisY[MAX_AXES], GridAxisZ[MAX_AXES];
 unsigned char BoxFromAuto, BoxWidthAuto, USE_T_or_uv;
 pplObj        BoxFrom, BoxWidth, Cmin[4], Cmax[4], ContoursUnit, KeyXOff, KeyYOff, OriginX, OriginY, TitleXOff, TitleYOff, Tmin, Tmax, Umin, Umax, Vmin, Vmax, width, XYview, YZview;
 char          title[FNAME_LENGTH];
 void         *ColMapExpr, *MaskExpr;
 char          c1label[FNAME_LENGTH], c1formatset;
 void         *c1format;
 double        c1LabelRotate, c1TickLabelRotate;
 int           c1TickLabelRotation;
 withWords     dataStyle, funcStyle;
 pplset_tics   ticsCM, ticsC;
 pplObj        unitC;
 } pplset_graph;

typedef struct pplset_axis {
 unsigned char atzero, enabled, invisible, linked, RangeReversed, topbottom;
 int           ArrowType, LinkedAxisCanvasID, LinkedAxisToXYZ, LinkedAxisToNum, log, MaxSet, MinSet, MirrorType, TickLabelRotation;
 double        LabelRotate, max, min, TickLabelRotate;
 char         *label;
 void         *linkusing;
 void         *format;
 pplset_tics   ticsM, tics;
 pplObj        unit;

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

#include "settings/arrows.h"
#include "settings/labels.h"

// Complete set of session settings
typedef struct ppl_settings_struc
 {
  pplset_terminal  term_default;
  pplset_terminal  term_current;
  pplset_graph     graph_default;
  pplset_graph     graph_current;
  int              palette_current[PALETTE_LENGTH], paletteS_current[PALETTE_LENGTH];
  double           palette1_current[PALETTE_LENGTH], palette2_current[PALETTE_LENGTH], palette3_current[PALETTE_LENGTH], palette4_current[PALETTE_LENGTH];
  int              palette_default[PALETTE_LENGTH], paletteS_default[PALETTE_LENGTH];
  double           palette1_default[PALETTE_LENGTH], palette2_default[PALETTE_LENGTH], palette3_default[PALETTE_LENGTH], palette4_default[PALETTE_LENGTH];
  withWords        plot_styles[MAX_PLOTSTYLES], plot_styles_default[MAX_PLOTSTYLES];
  pplset_axis      axis_default;
  pplset_axis      XAxes[MAX_AXES], XAxesDefault[MAX_AXES];
  pplset_axis      YAxes[MAX_AXES], YAxesDefault[MAX_AXES];
  pplset_axis      ZAxes[MAX_AXES], ZAxesDefault[MAX_AXES];
  dict            *filters;
  pplarrow_object *pplarrow_list, *pplarrow_list_default;
  ppllabel_object *ppllabel_list, *ppllabel_list_default;
 }
ppl_settings;

#endif

