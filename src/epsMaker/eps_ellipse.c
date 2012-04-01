// eps_ellipse.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2011 Dominic Ford <coders@pyxplot.org.uk>
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

#define _PPL_EPS_ELLIPSE 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "settings/withWords_fns.h"

#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_ellipse.h"
#include "epsMaker/eps_settings.h"

void eps_ellps_RenderEPS(EPSComm *x)
 {
  int    lt;
  unsigned char filled=0, stroked=0;
  double lw, lw_scale, xc, yc, a, b, r;
  double a_inv, b_inv;
  withWords ww;

  // Print label at top of postscript description of ellipse
  fprintf(x->epsbuffer, "%% Canvas item %d [ellipse]\n", x->current->id);
  eps_core_clear(x);

  // Calculate position of centre of elipse and its major/minor axes in TeX points
  xc = x->current->xpos  * M_TO_PS;
  yc = x->current->ypos  * M_TO_PS;
  a  = x->current->xpos2 * M_TO_PS;
  b  = x->current->ypos2 * M_TO_PS;
  r  = x->current->rotation;

  // Expand any numbered styles which may appear in the with words we are passed
  ppl_withWordsMerge(x->c, &ww, &x->current->with_data, NULL, NULL, NULL, NULL, 1);

  // Set colour of outline of ellipse
  eps_core_SetColour(x, &ww, 1);

  // Set linewidth and linetype of outline
  if (ww.USElinewidth) lw_scale = ww.linewidth;
  else                 lw_scale = x->current->settings.LineWidth;
  lw = EPS_DEFAULT_LINEWIDTH * lw_scale;

  if (ww.USElinetype)  lt = ww.linetype;
  else                 lt = 1;

  IF_NOT_INVISIBLE eps_core_SetLinewidth(x, lw, lt, 0.0);

  // Set up postscript axes so that a unit circle produces the desired ellipse
  fprintf(x->epsbuffer, "gsave\n");
  fprintf(x->epsbuffer, "%.2f %.2f translate\n", xc, yc);
  fprintf(x->epsbuffer, "%.2f rotate\n", r*180/M_PI);
  fprintf(x->epsbuffer, "%.2f %.2f scale\n", a, b);

  // Set fill colour of ellipse
  eps_core_SetFillColour(x, &ww);
  eps_core_SwitchTo_FillColour(x,1);

  // Fill ellipse
  IF_NOT_INVISIBLE
   {
    fprintf(x->epsbuffer, "0 0 1 0 360 arc\nclosepath\nfill\n");
    filled = 1;
   }

  // Set colour of outline of ellipse
  eps_core_SetColour(x, &ww, 1);

  // Make path representing the outline of ellipse
  IF_NOT_INVISIBLE
   {
    fprintf(x->epsbuffer, "0 0 1 0 360 arc\nclosepath\n"); // NB: Leave this path unstroked until we've done a grestore
    stroked = 1;
   }

  // Undo scaling of axes so that linewidths come out right
  if (a < 1e-8) { a_inv=1e8; } else { a_inv=1.0/a; }
  if (b < 1e-8) { b_inv=1e8; } else { b_inv=1.0/a; }
  fprintf(x->epsbuffer, "%f %f scale\n", a_inv, b_inv);

  // Now we can stroke the path of the ellipse
  IF_NOT_INVISIBLE fprintf(x->epsbuffer, "stroke\n"); // Stroke outline AFTER grestore, so that linewidth is not scaled.

  // Undo scaling of postscript axes
  fprintf(x->epsbuffer, "grestore\n");
  x->LastLinewidth = -1; x->LastLinetype = -1; x->LastPSColour[0]='\0';

  // Factor ellipse into EPS file's bounding box
  if (filled || stroked)
   {
    eps_core_BoundingBox(x , xc-a*cos(r)-b*sin(r) , yc-a*sin(-r)-b*cos(r) , lw);
    eps_core_BoundingBox(x , xc-a*cos(r)+b*sin(r) , yc-a*sin(-r)+b*cos(r) , lw);
    eps_core_BoundingBox(x , xc+a*cos(r)-b*sin(r) , yc+a*sin(-r)-b*cos(r) , lw);
    eps_core_BoundingBox(x , xc+a*cos(r)+b*sin(r) , yc+a*sin(-r)+b*cos(r) , lw);
   }

  // Final newline at end of canvas item
  fprintf(x->epsbuffer, "\n");
  return;
 }

