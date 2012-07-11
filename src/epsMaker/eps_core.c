// eps_core.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

// This file contains various numerical constants which are used by the eps
// generation routines

#define _PPL_EPS_CORE_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "settings/epsColors.h"
#include "settings/settingTypes.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_style.h"
#include "userspace/context.h"

// Clear EPS state information to ensure that we restate the linewidth, color, etc we are using for the next item
void eps_core_clear(EPSComm *x)
 {
  x->LastLinewidth = -1;
  strcpy(x->LastPSColor, "");
  strcpy(x->CurrentColor, "");
  strcpy(x->CurrentFillColor, "");
  x->LastLinetype = 0;
  return;
 }

// Write CurrentColor to postscript
void eps_core_WritePSColor(EPSComm *x)
 {
  if ((strcmp(x->CurrentColor, x->LastPSColor) != 0) && (x->CurrentColor[0]!='\0'))
   { strcpy(x->LastPSColor, x->CurrentColor); fprintf(x->epsbuffer, "%s\n", x->LastPSColor); }
  return;
 }

// Set the color of the EPS we are painting
void eps_core_SetColor(EPSComm *x, withWords *ww, unsigned char WritePS)
 {
  // Color may be specified as a named color, or as RGB components, or may not be specified at all, in which case we use black
  if      (ww->USEcolor1234)
   {
    if      (ww->Col1234Space==SW_COLSPACE_RGB ) sprintf(x->CurrentColor, "%.3f %.3f %.3f setrgbcolor", ww->color1, ww->color2, ww->color3);
    else if (ww->Col1234Space==SW_COLSPACE_HSB ) sprintf(x->CurrentColor, "%.3f %.3f %.3f sethsbcolor", ww->color1, ww->color2, ww->color3);
    else if (ww->Col1234Space==SW_COLSPACE_CMYK) sprintf(x->CurrentColor, "%.3f %.3f %.3f %.3f setcmykcolor", ww->color1, ww->color2, ww->color3, ww->color4);
    else    ppl_error(&x->c->errcontext,ERR_INTERNAL,-1,-1,"Illegal setting for color space switch.");
   }
  else if((ww->USEcolor   ) && ((ww->color==COLOR_NULL)||(ww->color==COLOR_INVISIBLE)||(ww->color==COLOR_TRANSPARENT)))
                              strcpy (x->CurrentColor, ""); // This is code to tell us we're writing in invisible ink
  else if (ww->USEcolor   )  sprintf(x->CurrentColor, "%.3f %.3f %.3f %.3f setcmykcolor",
                                          *(double *)ppl_fetchSettingName(&x->c->errcontext, ww->color, SW_COLOR_INT, (void *)SW_COLOR_CMYK_C, sizeof(double)),
                                          *(double *)ppl_fetchSettingName(&x->c->errcontext, ww->color, SW_COLOR_INT, (void *)SW_COLOR_CMYK_M, sizeof(double)),
                                          *(double *)ppl_fetchSettingName(&x->c->errcontext, ww->color, SW_COLOR_INT, (void *)SW_COLOR_CMYK_Y, sizeof(double)),
                                          *(double *)ppl_fetchSettingName(&x->c->errcontext, ww->color, SW_COLOR_INT, (void *)SW_COLOR_CMYK_K, sizeof(double)) );
  else                        sprintf(x->CurrentColor, "0 0 0 setrgbcolor");

  // Only change postscript color if the color we want isn't the one we are already using
  if ((strcmp(x->CurrentColor, x->LastPSColor) != 0) && WritePS && (x->CurrentColor[0]!='\0')) eps_core_WritePSColor(x);
  return;
 }

void eps_core_SetFillColor(EPSComm *x, withWords *ww)
 {
  // Color may be specified as a named color, or as RGB components, or may not be specified at all, in which case we use black
  if      (ww->USEfillcolor1234)
   {
    if      (ww->FillCol1234Space==SW_COLSPACE_RGB ) sprintf(x->CurrentFillColor, "%.3f %.3f %.3f setrgbcolor", ww->fillcolor1, ww->fillcolor2, ww->fillcolor3);
    else if (ww->FillCol1234Space==SW_COLSPACE_HSB ) sprintf(x->CurrentFillColor, "%.3f %.3f %.3f sethsbcolor", ww->fillcolor1, ww->fillcolor2, ww->fillcolor3);
    else if (ww->FillCol1234Space==SW_COLSPACE_CMYK) sprintf(x->CurrentFillColor, "%.3f %.3f %.3f %.3f setcmykcolor", ww->fillcolor1, ww->fillcolor2, ww->fillcolor3, ww->fillcolor4);
    else    ppl_error(&x->c->errcontext,ERR_INTERNAL,-1,-1,"Illegal setting for color space switch.");
   }
  else if((ww->USEfillcolor   ) && ((ww->fillcolor==COLOR_NULL)||(ww->fillcolor==COLOR_INVISIBLE)||(ww->fillcolor==COLOR_TRANSPARENT)))
                                  strcpy (x->CurrentFillColor, ""); // This is code to tell us we're writing in invisible ink
  else if (ww->USEfillcolor   )  sprintf(x->CurrentFillColor, "%.3f %.3f %.3f %.3f setcmykcolor",
                                              *(double *)ppl_fetchSettingName(&x->c->errcontext, ww->fillcolor, SW_COLOR_INT, (void *)SW_COLOR_CMYK_C, sizeof(double)),
                                              *(double *)ppl_fetchSettingName(&x->c->errcontext, ww->fillcolor, SW_COLOR_INT, (void *)SW_COLOR_CMYK_M, sizeof(double)),
                                              *(double *)ppl_fetchSettingName(&x->c->errcontext, ww->fillcolor, SW_COLOR_INT, (void *)SW_COLOR_CMYK_Y, sizeof(double)),
                                              *(double *)ppl_fetchSettingName(&x->c->errcontext, ww->fillcolor, SW_COLOR_INT, (void *)SW_COLOR_CMYK_K, sizeof(double)) );
  else                            strcpy (x->CurrentFillColor, ""); // If no fill color is specified, we don't fill

  return;
 }

static char TempColor[256];

void eps_core_SwitchTo_FillColor(EPSComm *x, unsigned char WritePS)
 {
  strcpy(TempColor , x->CurrentColor); // Buffer the color we're stroking with so we can restore it in eps_core_SwitchFrom_FillColor
  if ((strcmp(x->CurrentFillColor, x->LastPSColor) != 0) && WritePS && (x->CurrentFillColor[0]!='\0'))
   { strcpy(x->LastPSColor, x->CurrentFillColor); fprintf(x->epsbuffer, "%s\n", x->LastPSColor); }
  strcpy(x->CurrentColor , x->CurrentFillColor); // This make the supression of invisible ink work...
  return;
 }

void eps_core_SwitchFrom_FillColor(EPSComm *x, unsigned char WritePS)
 {
  if ((strcmp(TempColor, x->LastPSColor) != 0) && WritePS && (TempColor[0]!='\0'))
   { strcpy(x->LastPSColor, TempColor); fprintf(x->epsbuffer, "%s\n", x->LastPSColor); }
  strcpy(x->CurrentColor, TempColor); // Restore the color we're stroking with
  return;
 }

// Set the linewidth of the EPS we are painting
void eps_core_SetLinewidth(EPSComm *x, double lw, int lt, double offset)
 {
  if ((lw == x->LastLinewidth) && (lt == x->LastLinetype) && (offset==0.0)) return;
  if (lw != x->LastLinewidth) fprintf(x->epsbuffer, "%f setlinewidth\n", lw);
  fprintf(x->epsbuffer, "%s\n", eps_LineType(lt, lw, offset));
  x->LastLinewidth = lw;
  x->LastLinetype  = lt;
  return;
 }

// Update the EPS bounding box
void eps_core_BoundingBox(EPSComm *x, double xpos, double ypos, double lw)
 {
  if ((!x->bb_set) || (x->bb_left   > (xpos-lw/2))) x->bb_left   = (xpos-lw/2);
  if ((!x->bb_set) || (x->bb_right  < (xpos+lw/2))) x->bb_right  = (xpos+lw/2);
  if ((!x->bb_set) || (x->bb_bottom > (ypos-lw/2))) x->bb_bottom = (ypos-lw/2);
  if ((!x->bb_set) || (x->bb_top    < (ypos+lw/2))) x->bb_top    = (ypos+lw/2);
  x->bb_set = 1;
  return;
 }

// Update plot item bounding box
void eps_core_PlotBoundingBox(EPSComm *x, double xpos, double ypos, double lw, unsigned char UpdatePsBB)
 {
  if (x->current->PlotLeftMargin   > (xpos-lw/2)) x->current->PlotLeftMargin   = (xpos-lw/2);
  if (x->current->PlotRightMargin  < (xpos+lw/2)) x->current->PlotRightMargin  = (xpos+lw/2);
  if (x->current->PlotBottomMargin > (ypos-lw/2)) x->current->PlotBottomMargin = (ypos-lw/2);
  if (x->current->PlotTopMargin    < (ypos+lw/2)) x->current->PlotTopMargin    = (ypos+lw/2);
  if (UpdatePsBB) eps_core_BoundingBox(x, xpos, ypos, lw);
  return;
 }

