// eps_box.c
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

#define _PPL_EPS_BOX 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "settings/withWords_fns.h"

#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_box.h"
#include "epsMaker/eps_settings.h"

void eps_box_RenderEPS(EPSComm *x)
 {
  int    lt;
  unsigned char filled=0, stroked=0;
  double lw, lw_scale, x1, x2, y1, y2, xo, yo, r;
  withWords ww;

  // Print label at top of postscript description of box
  fprintf(x->epsbuffer, "%% Canvas item %d [box]\n", x->current->id);
  eps_core_clear(x);

  // Calculate positions of two opposite corners of box
  x1 =  x->current->xpos                      * M_TO_PS; // First corner
  y1 =  x->current->ypos                      * M_TO_PS;
  x2 = (x->current->xpos2 + x->current->xpos) * M_TO_PS; // Second corner
  y2 = (x->current->ypos2 + x->current->ypos) * M_TO_PS;
  r  = x->current->rotation;

  // Expand any numbered styles which may appear in the with words we are passed
  ppl_withWordsMerge(x->c, &ww, &x->current->with_data, NULL, NULL, NULL, NULL, 1);

  // Set fill color of box
  eps_core_SetFillColor(x, &ww);
  eps_core_SwitchTo_FillColor(x,1);

  // Work out the origin that we're rotating about
  if (!x->current->xpos2set) { xo=(x1+x2)/2.0; yo=(y1+y2)/2.0; } // Rotate about centre of box if specified as 'from .... to ....'
  else                       { xo=x1;          yo=y1;          } // Rotate about pinned corner of box is specified as 'at .... width .... height ....'

  // Apply rotation
  fprintf(x->epsbuffer, "gsave\n");
  fprintf(x->epsbuffer, "%.2f %.2f translate\n", xo, yo);
  fprintf(x->epsbuffer, "%.2f rotate\n", r*180/M_PI);

  // Fill box
  IF_NOT_INVISIBLE
   {
    fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\n%.2f %.2f lineto\n%.2f %.2f lineto\nclosepath\nfill\n", x1-xo,y1-yo,x1-xo,y2-yo,x2-xo,y2-yo,x2-xo,y1-yo);
    filled=1;
   }

  // Set color of outline of box
  eps_core_SetColor(x, &ww, 1);

  // Set linewidth and linetype of outline
  if (ww.USElinewidth) lw_scale = ww.linewidth;
  else                 lw_scale = x->current->settings.LineWidth;
  lw = EPS_DEFAULT_LINEWIDTH * lw_scale;

  if (ww.USElinetype)  lt = ww.linetype;
  else                 lt = 1;

  IF_NOT_INVISIBLE eps_core_SetLinewidth(x, lw, lt, 0.0);

  // Stroke outline of box
  IF_NOT_INVISIBLE
   {
    fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\n%.2f %.2f lineto\n%.2f %.2f lineto\nclosepath\nstroke\n", x1-xo,y1-yo,x1-xo,y2-yo,x2-xo,y2-yo,x2-xo,y1-yo);
    stroked=1;
   }

  // Undo scaling of postscript axes
  fprintf(x->epsbuffer, "grestore\n");
  x->LastLinewidth = -1; x->LastLinetype = -1; x->LastPSColor[0]='\0';

  // Factor four corners of box into EPS file's bounding box
  if (filled || stroked)
   {
    eps_core_BoundingBox(x, (x1-xo)*cos(r) - (y1-yo)*sin(r) + xo, (x1-xo)*sin(r) + (y1-yo)*cos(r) + yo, lw);
    eps_core_BoundingBox(x, (x1-xo)*cos(r) - (y2-yo)*sin(r) + xo, (x1-xo)*sin(r) + (y2-yo)*cos(r) + yo, lw);
    eps_core_BoundingBox(x, (x2-xo)*cos(r) - (y2-yo)*sin(r) + xo, (x2-xo)*sin(r) + (y2-yo)*cos(r) + yo, lw);
    eps_core_BoundingBox(x, (x2-xo)*cos(r) - (y1-yo)*sin(r) + xo, (x2-xo)*sin(r) + (y1-yo)*cos(r) + yo, lw);
   }

  // Free with words
  ppl_withWordsDestroy(x->c, &ww);

  // Final newline at end of canvas item
  fprintf(x->epsbuffer, "\n");
  return;
 }

