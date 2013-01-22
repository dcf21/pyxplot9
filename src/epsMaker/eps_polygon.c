// eps_polygon.c
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

#define _PPL_EPS_ARROW 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "coreUtils/memAlloc.h"
#include "coreUtils/errorReport.h"
#include "settings/withWords_fns.h"
#include "settings/settingTypes.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_arrow.h"
#include "epsMaker/eps_plot_threedimbuff.h"
#include "epsMaker/eps_settings.h"

void eps_polygon_RenderEPS(EPSComm *x)
 {
  int           i;
  unsigned char filled=0, stroked=0;
  withWords     ww;
  double        lw, lw_scale;
  int           lt;

  // Print label at top of postscript description of polygon
  fprintf(x->epsbuffer, "%% Canvas item %d [polygon]\n", x->current->id);
  eps_core_clear(x);

  // Expand any numbered styles which may appear in the with words we are passed
  ppl_withWordsMerge(x->c, &ww, &x->current->with_data, NULL, NULL, NULL, NULL, 1);

  // Set fill color of polygon
  eps_core_SetFillColor(x, &ww);
  eps_core_SwitchTo_FillColor(x,1);

  // Fill polygon
  IF_NOT_INVISIBLE
   {
    fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n", x->current->polygonPoints[0]*M_TO_PS, x->current->polygonPoints[1]*M_TO_PS);
    for (i=0; i<x->current->NpolygonPoints; i++) fprintf(x->epsbuffer, "%.2f %.2f lineto\n", x->current->polygonPoints[2*i]*M_TO_PS, x->current->polygonPoints[2*i+1]*M_TO_PS);
    fprintf(x->epsbuffer, "closepath\nfill\n");
    filled=1;
   }

  // Set color of outline of polygon
  eps_core_SetColor(x, &ww, 1);

  // Set linewidth and linetype of outline
  if (ww.USElinewidth) lw_scale = ww.linewidth;
  else                 lw_scale = x->current->settings.LineWidth;
  lw = EPS_DEFAULT_LINEWIDTH * lw_scale;

  if (ww.USElinetype)  lt = ww.linetype;
  else                 lt = 1;

  IF_NOT_INVISIBLE eps_core_SetLinewidth(x, lw, lt, 0.0);

  // Stroke outline of polygon
  IF_NOT_INVISIBLE
   {
    fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n", x->current->polygonPoints[0]*M_TO_PS, x->current->polygonPoints[1]*M_TO_PS);
    for (i=0; i<x->current->NpolygonPoints; i++) fprintf(x->epsbuffer, "%.2f %.2f lineto\n", x->current->polygonPoints[2*i]*M_TO_PS, x->current->polygonPoints[2*i+1]*M_TO_PS);
    fprintf(x->epsbuffer, "closepath\nstroke\n");
    stroked=1;
   }

  // Factor four corners of box into EPS file's bounding box
  if (filled || stroked)
   for (i=0; i<x->current->NpolygonPoints; i++)
    eps_core_BoundingBox(x, x->current->polygonPoints[2*i]*M_TO_PS, x->current->polygonPoints[2*i+1]*M_TO_PS, lw);

  // Free with words
  ppl_withWordsDestroy(x->c, &ww);

  // Final newline at end of canvas item
  fprintf(x->epsbuffer, "\n");
  return;
 }

