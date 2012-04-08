// eps_plot_axespaint.c
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

#define _PPL_EPS_PLOT_AXESPAINT 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/memAlloc.h"

#include "epsMaker/canvasDraw.h"
#include "settings/settings.h"
#include "settings/withWords_fns.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/unitsDisp.h"

#include "epsMaker/eps_arrow.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_plot_axespaint.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_threedimbuff.h"
#include "epsMaker/eps_settings.h"

void eps_plot_LabelAlignment(double theta_pinpoint, int *HALIGN, int *VALIGN)
 {
  theta_pinpoint = fmod(theta_pinpoint, 2*M_PI);
  while (theta_pinpoint < 0.0) theta_pinpoint += 2*M_PI;
  if      (ppl_dblApprox(theta_pinpoint,   0.0   , 1e-6)) { *HALIGN = SW_HALIGN_CENT ; *VALIGN = SW_VALIGN_TOP ; }
  else if (ppl_dblApprox(theta_pinpoint,   M_PI/2, 1e-6)) { *HALIGN = SW_HALIGN_RIGHT; *VALIGN = SW_VALIGN_CENT; }
  else if (ppl_dblApprox(theta_pinpoint,   M_PI  , 1e-6)) { *HALIGN = SW_HALIGN_CENT ; *VALIGN = SW_VALIGN_BOT ; }
  else if (ppl_dblApprox(theta_pinpoint, 3*M_PI/2, 1e-6)) { *HALIGN = SW_HALIGN_LEFT ; *VALIGN = SW_VALIGN_CENT; }
  else if (theta_pinpoint <   M_PI/2)                     { *HALIGN = SW_HALIGN_RIGHT; *VALIGN = SW_VALIGN_TOP ; }
  else if (theta_pinpoint <   M_PI  )                     { *HALIGN = SW_HALIGN_RIGHT; *VALIGN = SW_VALIGN_BOT ; }
  else if (theta_pinpoint < 3*M_PI/2)                     { *HALIGN = SW_HALIGN_LEFT ; *VALIGN = SW_VALIGN_BOT ; }
  else                                                    { *HALIGN = SW_HALIGN_LEFT ; *VALIGN = SW_VALIGN_TOP ; }
  return;
 }

void eps_plot_axispaint(EPSComm *x, withWords *ww, pplset_axis *a, const int xyz, const double CP2, const unsigned char Lr, const double x1, const double y1, const double *z1, const double x2, const double y2, const double *z2, const double theta_a, const double theta_b, double *OutputWidth, const unsigned char PrintLabels)
 {
  int    i, l, m, lt;
  double TickMaxHeight = 0.0, height, width, theta_axis;
  int    HALIGN, VALIGN, HALIGN_THIS, VALIGN_THIS;
  double theta, theta_pinpoint; // clockwise rotation
  double lw, lw_scale;
  char  *last_colstr=NULL;

  x->LaTeXpageno = a->FirstTextID;
  *OutputWidth = 0.0;

  // Set linewidth and linetype
  if (ww->USElinewidth) lw_scale = ww->linewidth;
  else                  lw_scale = x->current->settings.LineWidth;
  lw = EPS_DEFAULT_LINEWIDTH * lw_scale;

  if (ww->USElinetype)  lt = ww->linetype;
  else                  lt = 1;

  // Draw line of axis
  IF_NOT_INVISIBLE
   {
    if      (a->ArrowType == SW_AXISDISP_NOARR) eps_primitive_arrow(x, SW_ARROWTYPE_NOHEAD, x1, y1, z1, x2, y2, z2, ww);
    else if (a->ArrowType == SW_AXISDISP_ARROW) eps_primitive_arrow(x, SW_ARROWTYPE_HEAD  , x1, y1, z1, x2, y2, z2, ww);
    else if (a->ArrowType == SW_AXISDISP_TWOAR) eps_primitive_arrow(x, SW_ARROWTYPE_TWOWAY, x1, y1, z1, x2, y2, z2, ww);
    else if (a->ArrowType == SW_AXISDISP_BACKA) eps_primitive_arrow(x, SW_ARROWTYPE_HEAD  , x2, y2, z2, x1, y1, z1, ww);

    theta_axis = atan2(x2-x1,y2-y1);
    if (!gsl_finite(theta_axis)) theta_axis=0.0;

    // Paint axis ticks
    for (i=0; i<2; i++)
     {
      int      TR;
      double  *TLP, TRA, TLEN;
      char   **TLS;

      if (i==0) { TLP=a-> TickListPositions; TLS=a-> TickListStrings; TRA=a->TickLabelRotate; TR=a->TickLabelRotation; TLEN = EPS_AXES_MAJTICKLEN; } // Major ticks
      else      { TLP=a->MTickListPositions; TLS=a->MTickListStrings; TRA=a->TickLabelRotate; TR=a->TickLabelRotation; TLEN = EPS_AXES_MINTICKLEN; } // Minor ticks

      // Work out the rotation of the tick labels
      if      (TR == SW_TICLABDIR_HORI) theta =  0.0;
      else if (TR == SW_TICLABDIR_VERT) theta = -M_PI/2; // the clockwise rotation of the labels relative to upright
      else                              theta = -TRA;
      theta_pinpoint = theta + M_PI/2 + theta_axis + M_PI*(!Lr); // Angle around textboxes where it is anchored
      eps_plot_LabelAlignment(theta_pinpoint, &HALIGN, &VALIGN);

      if (TLP != NULL)
       {
        for (l=0; TLS[l]!=NULL; l++)
         {
          unsigned char state=0;
          double tic_lab_xoff=0.0;
          double tic_x1 = x1 + (x2-x1) * TLP[l];
          double tic_y1 = y1 + (y2-y1) * TLP[l];
          double tic_x2 , tic_y2, tic_x3, tic_y3;

          for (m=0; ((m<2)&&(!state)); m++)
           {
            double        theta_ab = m ? theta_a : theta_b;
            if (!gsl_finite(theta_ab)) continue;
            tic_x2 = tic_x1 + (a->TickDir==SW_TICDIR_IN  ? 0.0 :  1.0) * sin(theta_ab) * TLEN * M_TO_PS; // top of tick
            tic_y2 = tic_y1 + (a->TickDir==SW_TICDIR_IN  ? 0.0 :  1.0) * cos(theta_ab) * TLEN * M_TO_PS;
            tic_x3 = tic_x1 + (a->TickDir==SW_TICDIR_OUT ? 0.0 : -1.0) * sin(theta_ab) * TLEN * M_TO_PS; // bottom of tick
            tic_y3 = tic_y1 + (a->TickDir==SW_TICDIR_OUT ? 0.0 : -1.0) * cos(theta_ab) * TLEN * M_TO_PS;

            // Check for special case of ticks at zero on axes crossed by atzero axes
            HALIGN_THIS = HALIGN;
            VALIGN_THIS = VALIGN;
            if ((a->CrossedAtZero) && (a->atzero))
             {
              double CP1 = TLP[l];
              double xl = TLP[l] - 1e-3;
              double xr = TLP[l] + 1e-3;
              if (xl<0.0) xl=0.0; if (xl>1.0) xl=1.0;
              if (xr<0.0) xr=0.0; if (xr>1.0) xr=1.0;
              xl = eps_plot_axis_InvGetPosition(xl, a);
              xr = eps_plot_axis_InvGetPosition(xr, a);
              if ((gsl_finite(xl)) && (gsl_finite(xr)) && ((xl==0)||(xr==0)||((xl<0)&&(xr>0))||((xl>0)&&(xr<0))))
               {
                if (xyz != 0) // y/z-axis -- only put label at zero if axis is at edge of graph
                 {
                  if (!( ((CP2<1e-3)&&( Lr)) || ((CP2>0.999)&&(!Lr)) )) { if (PrintLabels && (TLS[l][0] != '\0')) x->LaTeXpageno++; state=1; continue; }
                 }
                else // x-axis -- if at edge of graph, put label at zero. If y-axis is labelled at zero, don't put label at zero...
                 {
                  if (!( ((CP2<1e-3)&&(!Lr)) || ((CP2>0.999)&&( Lr)) ))
                   {
                    int Lr1 = (xyz==0) ^ (CP1>0.999);
                    if ( ((CP1<1e-3)&&( Lr1)) || ((CP1>0.999)&&(!Lr1)) ) { if (PrintLabels && (TLS[l][0] != '\0')) x->LaTeXpageno++; state=1; continue; }
                    else // ... otherwise, displace label at zero to one side
                     {
                      double theta_pinpoint = theta + 1.5 * M_PI/2 + theta_axis + M_PI*(!Lr); // Angle around textboxes where it is anchored
                      eps_plot_LabelAlignment(theta_pinpoint, &HALIGN_THIS, &VALIGN_THIS);
                      tic_lab_xoff = -TLEN; // Move label to the left
                     }
                   }
                 }
               }
             }

            // Stroke the tick, unless it is at the end of an axis with an arrowhead
            if (!( ((TLP[l]<1e-3)&&((a->ArrowType == SW_AXISDISP_BACKA)||(a->ArrowType == SW_AXISDISP_TWOAR))) || ((TLP[l]>0.999)&&((a->ArrowType == SW_AXISDISP_ARROW)||(a->ArrowType == SW_AXISDISP_TWOAR))) ))
             {
              if (z1==NULL) fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n", tic_x2, tic_y2, tic_x3, tic_y3);
              else
               {
                if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColour)!=0)) last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColour)+1);
                if (last_colstr!=NULL)
                 {
                  strcpy(last_colstr, x->CurrentColour);
                  sprintf(x->c->errcontext.tempErrStr, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n", tic_x2, tic_y2, tic_x3, tic_y3);
                  ThreeDimBuffer_writeps(x, *z1 + (*z2-*z1)*TLP[l], lt, lw, 0.0, 1, last_colstr, x->c->errcontext.tempErrStr);
                 }
               }
              eps_core_PlotBoundingBox(x, tic_x2, tic_y2, EPS_AXES_LINEWIDTH * EPS_DEFAULT_LINEWIDTH, 1);
              eps_core_PlotBoundingBox(x, tic_x3, tic_y3, EPS_AXES_LINEWIDTH * EPS_DEFAULT_LINEWIDTH, 1);
             }
           }
          if (state) continue;

          // Paint the tick label
          if (PrintLabels && (TLS[l][0] != '\0'))
           {
            int pageno = x->LaTeXpageno++;
            double xlab, ylab;

            xlab = tic_x1/M_TO_PS + (Lr ? -1.0 : 1.0) * sin(theta_axis + M_PI/2) * EPS_AXES_TEXTGAP + tic_lab_xoff;
            ylab = tic_y1/M_TO_PS + (Lr ? -1.0 : 1.0) * cos(theta_axis + M_PI/2) * EPS_AXES_TEXTGAP;

            IF_NOT_INVISIBLE
             {
              char *text=NULL;
              canvas_EPSRenderTextItem(x, &text, pageno, xlab, ylab, HALIGN_THIS, VALIGN_THIS, x->CurrentColour, x->current->settings.FontSize, theta, &width, &height);
              if (text!=NULL)
               {
                if (z1==NULL) fprintf(x->epsbuffer, "%s", text);
                else
                 {
                  if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColour)!=0)) last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColour)+1);
                  if (last_colstr!=NULL)
                   {
                    strcpy(last_colstr, x->CurrentColour);
                    ThreeDimBuffer_writeps(x, *z1 + (*z2-*z1)*TLP[l], lt, lw, 0.0, 1, last_colstr, text);
                   }
                 }
               }
              height = height*fabs(cos(theta_pinpoint)) + width*fabs(sin(theta_pinpoint));
              if (height > TickMaxHeight) TickMaxHeight = height;
             }
           }
         }
       }
     }
    if (TickMaxHeight>0.0) *OutputWidth = EPS_AXES_TEXTGAP * M_TO_PS + TickMaxHeight; // Allow a gap after axis labels

    // Write axis label
    if (PrintLabels && (a->FinalAxisLabel != NULL) && (a->FinalAxisLabel[0]!='\0'))
     {
      int pageno = x->LaTeXpageno++;
      double xlab, ylab;
      double width, height;

      // Work out the rotation of the tick label
      theta = -a->LabelRotate;
      theta_pinpoint = theta + M_PI*Lr; // Angle around textbox where it is anchored
      eps_plot_LabelAlignment(theta_pinpoint, &HALIGN, &VALIGN);

      xlab = (x1+x2)/2/M_TO_PS + (Lr ? -1.0 : 1.0) * (2*EPS_AXES_TEXTGAP+TickMaxHeight/M_TO_PS) * sin(theta_axis+M_PI/2);
      ylab = (y1+y2)/2/M_TO_PS + (Lr ? -1.0 : 1.0) * (2*EPS_AXES_TEXTGAP+TickMaxHeight/M_TO_PS) * cos(theta_axis+M_PI/2);

      IF_NOT_INVISIBLE
       {
        char *text=NULL;
        double theta_text = theta + M_PI/2 - theta_axis;
        theta_text = fmod(theta_text , 2*M_PI);
        if (theta_text < -M_PI  ) theta_text += 2*M_PI;
        if (theta_text >  M_PI  ) theta_text -= 2*M_PI;
        if (theta_text >  M_PI/2) theta_text -=   M_PI;
        if (theta_text < -M_PI/2) theta_text +=   M_PI;
        canvas_EPSRenderTextItem(x, &text, pageno, xlab, ylab, HALIGN, VALIGN, x->CurrentColour, x->current->settings.FontSize, theta_text, &width, &height);
        *OutputWidth += (EPS_AXES_TEXTGAP * M_TO_PS + height*fabs(cos(theta_pinpoint)) + width*fabs(sin(theta_pinpoint)) ); // Allow gap after label
        if (text!=NULL)
         {
          if (z1==NULL) fprintf(x->epsbuffer, "%s", text);
          else
           {
            if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColour)!=0)) last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColour)+1);
            if (last_colstr!=NULL)
             {
              strcpy(last_colstr, x->CurrentColour);
              ThreeDimBuffer_writeps(x, (*z2+*z1)/2, lt, lw, 0.0, 1, last_colstr, text);
             }
           }
         }
       }
     }

    // Allow a gap before next axis
    *OutputWidth += EPS_AXES_SEPARATION * M_TO_PS;
   }
 }

void eps_plot_axespaint(EPSComm *x, double origin_x, double origin_y, double width, double height, double zdepth, int pass)
 {
  int            i, j, k;
  double         xc, yc, zc, theta[3]={M_PI/2,0,0}; // Coordinates of centre of graph
  pplset_axis   *axes;
  withWords      ww;

  if ((pass==0) && (!x->current->ThreeDim)) return; // 2D plots do not have back axes

  // Set colour for painting axes
  ppl_withWordsZero(x->c,&ww);
  if (x->current->settings.AxesColour > 0) { ww.color = x->current->settings.AxesColour; ww.USEcolor = 1; }
  else                                     { ww.Col1234Space = x->current->settings.AxesCol1234Space; ww.color1 = x->current->settings.AxesColour1; ww.color2 = x->current->settings.AxesColour2; ww.color3 = x->current->settings.AxesColour3; ww.color4 = x->current->settings.AxesColour4; ww.USEcolor1234 = 1; }
  ww.linewidth = EPS_AXES_LINEWIDTH; ww.USElinewidth = 1;
  ww.linetype  = 1;                  ww.USElinetype  = 1;
  eps_core_SetColour(x, &ww, 1);
  IF_NOT_INVISIBLE eps_core_SetLinewidth(x, EPS_AXES_LINEWIDTH * EPS_DEFAULT_LINEWIDTH, 1, 0.0);

  // Reset plot bounding box; we're about to make this sensible below by factoring in plot vertices
  if ((pass==0)||(!x->current->ThreeDim))
   {
    x->current->PlotLeftMargin = x->current->PlotRightMargin  = origin_x;
    x->current->PlotTopMargin  = x->current->PlotBottomMargin = origin_y;
   }

  // Fetch coordinates of centre of graph
  if (x->current->ThreeDim) { eps_plot_ThreeDimProject(0.5,0.5,0.5, &x->current->settings,origin_x,origin_y,width,height,zdepth, &xc,&yc,&zc); }
  else                      { xc = origin_x + 0.5*width; yc = origin_y + 0.5*height; zc = 0.0; }

  // Work out directions of each of the axes
  if (x->current->ThreeDim)
   for (j=0; j<3; j++)
    {
     double x1,y1,z1,x2,y2,z2;
     eps_plot_ThreeDimProject(    0 ,    0 ,    0 , &x->current->settings,origin_x,origin_y,width,height,zdepth, &x1,&y1,&z1);
     eps_plot_ThreeDimProject((j==0),(j==1),(j==2), &x->current->settings,origin_x,origin_y,width,height,zdepth, &x2,&y2,&z2);
     theta[j] = atan2(x2-x1,y2-y1);
     if (!gsl_finite(theta[j])) theta[j] = 0.0;
    }

  // Set CrossedAtZero flags
  for (j=0; j<3; j++)
   {
    if      (j==0) axes = x->current->XAxes;
    else if (j==1) axes = x->current->YAxes;
    else           axes = x->current->ZAxes;

    for (i=0; i<MAX_AXES; i++)
     if ((axes[i].FinalActive) && (!axes[i].invisible) && (axes[i].atzero))
      {
       if (j!=0) x->current->XAxes[1].CrossedAtZero = 1;
       if (j!=1) x->current->YAxes[1].CrossedAtZero = 1;
       if (j!=2) x->current->ZAxes[1].CrossedAtZero = 1;
       break;
      }
   }

  // Loop over all axes to draw. First loop over x, y and z directions.
  for (j=0; j<2+(x->current->ThreeDim!=0); j++)
   {
    int           xap, yap, zap, Naxes[4], FirstAutoMirror[4];
    unsigned char side_a[4]={0,1,0,1}, side_b[4]={0,0,1,1};
    double        xpos1[4], ypos1[4], zpos1[4], xpos2[4], ypos2[4], zpos2[4];
    int           NDIRS   = x->current->ThreeDim ? 4 : 2;
    double        theta_a = theta[(j==0)?1:0];
    double        theta_b = theta[(j==2)?1:2];

    // Set count of axes to zero
    Naxes          [0] = Naxes          [1] = Naxes          [2] = Naxes          [3] =  0;
    FirstAutoMirror[0] = FirstAutoMirror[1] = FirstAutoMirror[2] = FirstAutoMirror[3] = -1;
    if (!x->current->ThreeDim) theta_b=GSL_NAN;

    // Loop over vertices of graph factoring into bounding box and setting pos1 and pos2
    for (xap=0; xap<=1; xap++) for (yap=0; yap<=1; yap++) for (zap=0; zap<=1; zap++)
     {
      double tmp_x, tmp_y, tmp_z;
      int    k=0, l=0;
      if (x->current->ThreeDim) { eps_plot_ThreeDimProject(xap, yap, zap, &x->current->settings,origin_x,origin_y,width,height,zdepth,&tmp_x,&tmp_y,&tmp_z); }
      else                      { tmp_x = origin_x + xap*width; tmp_y = origin_y + yap*height; tmp_z = 0.0; }
      if (j!=2) k=2*k+zap; else l=zap;
      if (j!=1) k=2*k+yap; else l=yap;
      if (j!=0) k=2*k+xap; else l=xap;
      if (l==0) { xpos1[k] = tmp_x; ypos1[k] = tmp_y; zpos1[k] = tmp_z; }
      else      { xpos2[k] = tmp_x; ypos2[k] = tmp_y; zpos2[k] = tmp_z; }
      eps_core_PlotBoundingBox(x, tmp_x, tmp_y, 0.0, 0);
     }

    // Swap edges of cube around to put edges at (x,y)-extremes first
    if (x->current->ThreeDim)
     {
      #define TSM(A) hypot(xpos1[A]+xpos2[A]-2*origin_x , ypos1[A]+ypos2[A]-2*origin_y)
      #define TRY_SWAP(A,B) if (TSM(A)<0.9999999*TSM(B)) { double tmp[6]={xpos1[A],ypos1[A],zpos1[A],xpos2[A],ypos2[A],zpos2[A]}; unsigned char tsa=side_a[A], tsb=side_b[A]; xpos1[A]=xpos1[B]; ypos1[A]=ypos1[B]; zpos1[A]=zpos1[B]; xpos2[A]=xpos2[B]; ypos2[A]=ypos2[B]; zpos2[A]=zpos2[B]; xpos1[B]=tmp[0]; ypos1[B]=tmp[1]; zpos1[B]=tmp[2]; xpos2[B]=tmp[3]; ypos2[B]=tmp[4]; zpos2[B]=tmp[5]; side_a[A]=side_a[B]; side_b[A]=side_b[B]; side_a[B]=tsa; side_b[B]=tsb; }
      TRY_SWAP(2,3); TRY_SWAP(1,2); TRY_SWAP(0,1);
      TRY_SWAP(2,3); TRY_SWAP(1,2);
     }

    // Fetch relevant list of axes
    if      (j==0) axes = x->current->XAxes;
    else if (j==1) axes = x->current->YAxes;
    else           axes = x->current->ZAxes;

    // Loop over all axes in this direction and draw any which are active and visible
    for (i=0; i<MAX_AXES; i++)
     if ((axes[i].FinalActive) && (!axes[i].invisible))
      {
       // 2D Gnomonic axes
       if (x->current->settings.projection == SW_PROJ_GNOM)
        {
        }

       // Flat axes
       else
        {
         if (axes[i].atzero)
          {
           pplset_axis   *PerpAxisX = &(x->current->XAxes[1]); // Put axis at zero of perpendicular x1-, y1- and z1-axes
           pplset_axis   *PerpAxisY = &(x->current->YAxes[1]);
           pplset_axis   *PerpAxisZ = &(x->current->ZAxes[1]);
           unsigned char  LastX, LastY, LastZ, DoneX, DoneY, DoneZ;
           int            xrn, yrn, zrn;
           double         pos[3], xpos1, ypos1, zpos1, xpos2, ypos2, zpos2, dummy;

           for (DoneX=LastX=0,xrn=0; !LastX; xrn++)
            {
             pos[0] = 0.5;
             LastX  = ((j==0)||(!PerpAxisX->FinalActive)||(xrn>PerpAxisX->AxisValueTurnings));
             if ((j!=0)&&(PerpAxisX->FinalActive)) pos[0] = eps_plot_axis_GetPosition(0.0, PerpAxisX, xrn, 0);
             if ((!gsl_finite(pos[0])) && (!DoneX) && LastX) pos[0] = 0.5;
             if  (!gsl_finite(pos[0])) continue;

           for (DoneY=LastY=0,yrn=0; !LastY; yrn++)
            {
             pos[1] = 0.5;
             LastY  = ((j==1)||(!PerpAxisY->FinalActive)||(yrn>PerpAxisY->AxisValueTurnings));
             if ((j!=1)&&(PerpAxisY->FinalActive)) pos[1] = eps_plot_axis_GetPosition(0.0, PerpAxisY, yrn, 0);
             if ((!gsl_finite(pos[1])) && (!DoneY) && LastY) pos[1] = 0.5;
             if  (!gsl_finite(pos[1])) continue;

           for (DoneZ=LastZ=0,zrn=0; !LastZ; zrn++)
            {
             pos[2] = 0.5;
             LastZ  = ((j==2)||(!x->current->ThreeDim)||(!PerpAxisZ->FinalActive)||(zrn>PerpAxisZ->AxisValueTurnings));
             if ((j!=2)&&(PerpAxisZ->FinalActive)) pos[2] = eps_plot_axis_GetPosition(0.0, PerpAxisZ, zrn, 0);
             if ((!gsl_finite(pos[2])) && (!DoneZ) && LastZ) pos[2] = 0.5;
             if  (!gsl_finite(pos[2])) continue;

           pos[j] = 0.0;
           if (x->current->ThreeDim) { eps_plot_ThreeDimProject(pos[0],pos[1],pos[2],&x->current->settings,origin_x,origin_y,width,height,zdepth,&xpos1,&ypos1,&zpos1); }
           else                      { xpos1=origin_x+pos[0]*width; ypos1=origin_y+pos[1]*height; }
           pos[j] = 1.0;
           if (x->current->ThreeDim) { eps_plot_ThreeDimProject(pos[0],pos[1],pos[2],&x->current->settings,origin_x,origin_y,width,height,zdepth,&xpos2,&ypos2,&zpos2); }
           else                      { xpos2=origin_x+pos[0]*width; ypos2=origin_y+pos[1]*height; }

           if ((pass==0)&&(x->current->ThreeDim))
            {
             double        b  = atan2((xpos2+xpos1)/2-xc , (ypos2+ypos1)/2-yc) - theta[j];
             unsigned char Lr = sin(b)<0;
             eps_plot_axispaint(x, &ww, axes+i, j, GSL_NAN, Lr, xpos1, ypos1, &zpos1, xpos2, ypos2, &zpos2, theta_a+M_PI*((j==0)?(pos[1]<=0.5):(pos[0]<=0.5)), theta_b+M_PI*((j==2)?(pos[1]<=0.5):(pos[2]<=0.5)), &dummy, 1);
            } else if ((pass==1)&&(!x->current->ThreeDim)) {
             eps_plot_axispaint(x, &ww, axes+i, j, pos[j==0], (j!=0) ^ (pos[j==0]>0.999), xpos1, ypos1, NULL, xpos2, ypos2, NULL, theta_a, theta_b, &dummy, 1);
            }
           }}} // Loop over xrn, yrn and zrn
          }
         else // axis is notatzero... i.e. it is either at top or bottom of graph
          {
           if (axes[i].MirrorType == SW_AXISMIRROR_AUTO)
            for (k=0; k<4; k++)
             if ((k!=axes[i].topbottom) && (FirstAutoMirror[k]<0))
              FirstAutoMirror[k] = i;

           for (k=0; k<NDIRS; k++)
            if ((k==axes[i].topbottom) || (axes[i].MirrorType == SW_AXISMIRROR_MIRROR) || (axes[i].MirrorType == SW_AXISMIRROR_FULLMIRROR))
             if ((!x->current->ThreeDim) || (((zpos1[k]+zpos2[k])>0.0)==(pass==0)))
              {
               double        b  = atan2((xpos2[k]+xpos1[k])/2-xc , (ypos2[k]+ypos1[k])/2-yc) - theta[j];
               unsigned char Lr = sin(b)<0;
               unsigned char label = ((k==axes[i].topbottom) || (axes[i].MirrorType == SW_AXISMIRROR_FULLMIRROR));
               double        IncGap, ang;
               eps_plot_axispaint(x, &ww, axes+i, j, GSL_NAN, Lr, xpos1[k], ypos1[k], NULL, xpos2[k], ypos2[k], NULL, theta_a+(side_a[k]?0:M_PI), theta_b+(side_b[k]?0:M_PI), &IncGap, label);
               ang = theta[j] + M_PI/2 * ((Lr==0)*2-1);
               xpos1[k] += IncGap * sin(ang);
               ypos1[k] += IncGap * cos(ang);
               xpos2[k] += IncGap * sin(ang);
               ypos2[k] += IncGap * cos(ang);
               Naxes[k]++;
              }
          }
        }
      }

    // If there is no axis on any side, but was an auto-mirrored axis on another side, mirror it now
    for (k=0; k<NDIRS; k++)
     if ((Naxes[k]==0) && (FirstAutoMirror[k]>=0))
      if ((!x->current->ThreeDim) || (((zpos1[k]+zpos2[k])>0.0)==(pass==0)))
       {
        double        b  = atan2((xpos2[k]+xpos1[k])/2-xc , (ypos2[k]+ypos1[k])/2-yc) - theta[j];
        unsigned char Lr = sin(b)<0;
        double        IncGap, ang;
        eps_plot_axispaint(x, &ww, axes+FirstAutoMirror[k], j, GSL_NAN, Lr, xpos1[k], ypos1[k], NULL, xpos2[k], ypos2[k], NULL, theta_a+(side_a[k]?0:M_PI), theta_b+(side_b[k]?0:M_PI), &IncGap, 0);
        ang = theta[j] + M_PI/2 * ((Lr==0)*2-1);
        xpos1[k] += IncGap * sin(ang);
        ypos1[k] += IncGap * cos(ang);
        xpos2[k] += IncGap * sin(ang);
        ypos2[k] += IncGap * cos(ang);
        Naxes[k]++;
       }
   }
  return;
 }

