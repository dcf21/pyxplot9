// canvasItems.h
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

#ifndef _CANVASITEMS_H
#define _CANVASITEMS_H 1

#include "coreUtils/dict.h"

#include "datafile.h"
#include "expressions/expCompile.h"
#include "settings/arrows.h"
#include "settings/labels.h"
#include "settings/settings.h"
#include "settings/withWords.h"

#include "parser/parser.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

#define CANVAS_ARROW   22001
#define CANVAS_BOX     22002
#define CANVAS_CIRC    22003
#define CANVAS_ELLPS   22004
#define CANVAS_EPS     22005
#define CANVAS_IMAGE   22006
#define CANVAS_PIE     22007
#define CANVAS_PLOT    22008
#define CANVAS_POINT   22009
#define CANVAS_POLYGON 22010
#define CANVAS_TEXT    22011

typedef struct canvas_plotrange {
 double                   min, max;
 pplObj                   unit;
 unsigned char            MinSet, MaxSet;
 unsigned char            AutoMinSet, AutoMaxSet;
 struct canvas_plotrange *next;
} canvas_plotrange;

typedef struct canvas_plotdesc {
 unsigned char           function, parametric, TRangeSet, VRangeSet, axis1set, axis2set, axis3set, ContinuitySet, IndexSet, EverySet, TitleSet, NoTitleSet;
 int                     NFunctions, axis1xyz, axis2xyz, axis3xyz, axis1, axis2, axis3, EveryList[6], index, continuity, UsingRowCols, NUsing;
 withWords               ww;
 char                   *filename, *title;
 pplExpr               **functions, *label, *SelectCriterion, **UsingList;
 pplObj                  Tmin, Tmax, Vmin, Vmax;
 struct canvas_plotdesc *next;

 // used with plot '-' and plot '--' to indicate that data is read only once; otherwise NULL
 dataTable              *PersistentDataTable;

 // Structure members which are used at plot time
 dataTable              *data;
 withWords               ww_final;
 char                   *TitleFinal;
 int                     TitleFinal_col;
 double                  TitleFinal_xpos, TitleFinal_ypos, TitleFinal_width, TitleFinal_height;
 pplObj                  CRangeUnit;
 unsigned char           CRangeDisplay;
 double                  CMinFinal, CMaxFinal;
 pplset_axis             C1Axis;
 int                     GridXSize, GridYSize;
 double                  PieChart_total;
} canvas_plotdesc;

typedef struct canvas_item {
 int                 id, type, ArrowType, TransColR, TransColG, TransColB;
 double              xpos, ypos, xpos2, ypos2, rotation;
 char               *text;
 unsigned char       deleted, xpos2set, ypos2set, clip, calcbbox, smooth, NoTransparency, CustomTransparency;
 withWords           with_data;
 pplset_graph        settings;
 pplset_axis        *XAxes, *YAxes, *ZAxes;
 pplarrow_object    *arrow_list;
 ppllabel_object    *label_list;
 struct canvas_item *next, *prev;

 // Parameters which can be used to define ellipses
 double              x1,y1,x2,y2,xc,yc,xf,yf,a,b,ecc,slr,arcfrom,arcto; // Parameters which can be used to define ellipses
 unsigned char       x1set, xcset, xfset, aset, bset, eccset, slrset, arcset;

 // Parameters which can be used to define polygons
 double             *polygonPoints;
 int                 NpolygonPoints;

 // Parameters which can be used to define plots
 unsigned char       ThreeDim;
 canvas_plotrange   *plotranges;
 canvas_plotdesc    *plotitems;
 dataTable         **plotdata; // used at plot time
 double              PlotLeftMargin, PlotRightMargin, PlotTopMargin, PlotBottomMargin;
 int                 FirstTextID, TitleTextID, LegendTextID, SetLabelTextID, *DatasetTextID;
} canvas_item;

typedef struct canvas_itemlist {
 canvas_item  *first;
 canvas_item  *last;
} canvas_itemlist;


int ppl_directive_clear   (ppl_context *c, parserLine *pl, parserOutput *in, int interactive);
char *ppl_canvas_item_textify(ppl_context *c, canvas_item *ptr, char *output);
int ppl_directive_list    (ppl_context *c, parserLine *pl, parserOutput *in, int interactive);
int ppl_directive_delete  (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_undelete(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_move    (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_swap    (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_arrow   (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_box     (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_circle  (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_ellipse (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_eps     (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_piechart(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_point   (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_polygon (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_text    (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_image   (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
int ppl_directive_plot    (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);

#endif

