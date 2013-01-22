// eps_circle.c
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

#define _PPL_EPS_CIRCLE 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "settings/withWords_fns.h"

#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_circle.h"
#include "epsMaker/eps_settings.h"

void eps_circ_RenderEPS(EPSComm *x)
 {
  int    lt;
  unsigned char filled=0, stroked=0;
  double lw, lw_scale, xpos, ypos, r, start=0, end=360, theta;
  withWords ww;

  // Print label at top of postscript description of circle
  fprintf(x->epsbuffer, "%% Canvas item %d [%s]\n", x->current->id, x->current->xfset ? "arc" : "circle");
  eps_core_clear(x);

  // Calculate position of centre of circle and its radius in TeX points
  xpos = x->current->xpos  * M_TO_PS;
  ypos = x->current->ypos  * M_TO_PS;
  r    = x->current->xpos2 * M_TO_PS;

  // If this is an arc, set start and end points
  if (x->current->xfset)
   {
    start = fmod(M_PI/2 - x->current->yf, 2*M_PI);
    end   = fmod(M_PI/2 - x->current->xf, 2*M_PI);
    while (start < 0.0) start += 2*M_PI;
    while (end   < 0.0) end   += 2*M_PI;
    if      ((x->current->yf < x->current->xf) && (start <= end)) start += 2*M_PI; // Make sure that arc is stroked clockwise from start to end
    else if ((x->current->yf > x->current->xf) && (start >= end)) end   += 2*M_PI;
   }

  // Expand any numbered styles which may appear in the with words we are passed
  ppl_withWordsMerge(x->c, &ww, &x->current->with_data, NULL, NULL, NULL, NULL, 1);

  // Set fill color of circle
  eps_core_SetFillColor(x, &ww);
  eps_core_SwitchTo_FillColor(x,1);

  // Fill circle
  IF_NOT_INVISIBLE
   {
    if (!x->current->xfset) fprintf(x->epsbuffer, "newpath\n%.2f %.2f %.2f 0 360 arc\nclosepath\nfill\n", xpos,ypos,r);
    else                    fprintf(x->epsbuffer, "newpath\n%.2f %.2f %.2f %.2f %.2f arc\n%.2f %.2f lineto\nclosepath\nfill\n", xpos,ypos,r,start*180/M_PI,end*180/M_PI,xpos,ypos);
    eps_core_BoundingBox(x, xpos, ypos, 0); // Filled arcs extend to the centre of the circle
    filled=1;
   }

  // Set color of outline of circle
  eps_core_SetColor(x, &ww, 1);

  // Set linewidth and linetype of outline
  if (ww.USElinewidth) lw_scale = ww.linewidth;
  else                 lw_scale = x->current->settings.LineWidth;
  lw = EPS_DEFAULT_LINEWIDTH * lw_scale;

  if (ww.USElinetype)  lt = ww.linetype;
  else                 lt = 1;

  IF_NOT_INVISIBLE eps_core_SetLinewidth(x, lw, lt, 0.0);

  // Stroke outline of circle
  IF_NOT_INVISIBLE
   {
    if (!x->current->xfset) fprintf(x->epsbuffer, "newpath\n%.2f %.2f %.2f 0 360 arc\nclosepath\nstroke\n", xpos,ypos,r);
    else                    fprintf(x->epsbuffer, "newpath\n%.2f %.2f %.2f %.2f %.2f arc\nstroke\n", xpos,ypos,r,start*180/M_PI,end*180/M_PI);
    stroked=1;
   }

  // Factor circle into EPS file's bounding box
  if (filled || stroked)
   {
    if (!x->current->xfset)
     {
      eps_core_BoundingBox(x, xpos-r, ypos  , lw);
      eps_core_BoundingBox(x, xpos+r, ypos  , lw);
      eps_core_BoundingBox(x, xpos  , ypos-r, lw);
      eps_core_BoundingBox(x, xpos  , ypos+r, lw);
     } else {
      if (end<start) end+=2*M_PI;
      for (theta=0.0; theta<=1.0; theta+=0.01) eps_core_BoundingBox(x, xpos+r*cos(start+(end-start)*theta), ypos+r*sin(start+(end-start)*theta), lw);
     }
   }

  // Free with words
  ppl_withWordsDestroy(x->c, &ww);

  // Final newline at end of canvas item
  fprintf(x->epsbuffer, "\n");
  return;
 }

