// eps_plot_legend.c
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

// This file contains routines for adding legends to plots

#define _PPL_EPS_PLOT_LEGEND_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/memAlloc.h"

#include "settings/settings.h"
#include "settings/withWords_fns.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/unitsDisp.h"

#include "epsMaker/canvasDraw.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_legend.h"
#include "epsMaker/eps_plot_styles.h"

#include "canvasItems.h"
#include "texify.h"

// Private routines for sorting 3D positions by depth and azimuth when working out where to put a legend on a 3D plot

static int SortByDepth(const void *x, const void *y)
 {
  const double *xd = (const double *)x;
  const double *yd = (const double *)y;
  if (xd[2]>yd[2]) return  1;
  if (xd[2]<yd[2]) return -1;
  return 0;
 }

static double SortByAzimuthXCentre, SortByAzimuthYCentre, SortByAzimuthTarget;

static int SortByAzimuthProximity(const void *x, const void *y)
 {
  const double *xd = (const double *)x;
  const double *yd = (const double *)y;
  double ax,ay;
  ax = fabs(atan2(xd[0]-SortByAzimuthXCentre , xd[1]-SortByAzimuthYCentre) - SortByAzimuthTarget);
  ay = fabs(atan2(yd[0]-SortByAzimuthXCentre , yd[1]-SortByAzimuthYCentre) - SortByAzimuthTarget);
  while (ax>2*M_PI) ax-=2*M_PI; if (ax>M_PI) ax = 2*M_PI-ax;
  while (ay>2*M_PI) ay-=2*M_PI; if (ay>M_PI) ay = 2*M_PI-ay;
  if (ax>ay) return  1;
  if (ax<ay) return -1;
  return 0;
 }

static void TriSwap(double *a, double *b)
 {
  double t[3];
  memcpy(t,a,3*sizeof(double));
  memcpy(a,b,3*sizeof(double));
  memcpy(b,t,3*sizeof(double));
  return;
 }

// Graph legend API routines

void GraphLegend_YieldUpText(EPSComm *x)
 {
  canvas_plotdesc *pd;
  CanvasTextItem  *i;
  char *cptr, *buffer;
  int j, k, inlen;

  if (x->current->settings.key != SW_ONOFF_ON) return;

  pd = x->current->plotitems;
  while (pd != NULL) // loop over all datasets
   {
    if      (pd->NoTitleSet) { pd=pd->next; continue; } // notitle set
    else if (pd->TitleSet  ) { pd->TitleFinal=pd->title; YIELD_TEXTITEM(pd->title); } // title for dataset manually set
    else // generate automatic title for dataset
     {
      pd->TitleFinal = cptr = (char *)ppl_memAlloc(LSTR_LENGTH);
      buffer = (char *)ppl_memAlloc(LSTR_LENGTH);
      if (buffer==NULL) cptr=NULL;
      if (cptr!=NULL)
       {
        k=0;
        if (pd->parametric) { sprintf(cptr+k, "parametric"); k+=strlen(cptr+k); }
        if (pd->TRangeSet)  { sprintf(cptr+k, " [%s:%s]", ppl_unitsNumericDisplay(x->c,&pd->Tmin,0,SW_DISPLAY_L,0), ppl_unitsNumericDisplay(x->c,&pd->Tmax,1,SW_DISPLAY_L,0)); k+=strlen(cptr+k); }
        if (!pd->function) { cptr[k++]=' '; ppl_strEscapify(pd->filename, buffer); inlen=strlen(buffer); ppl_texify_generic(x->c, buffer, &inlen, cptr+k, LSTR_LENGTH-k); k+=strlen(cptr+k); } // Filename of datafile we are plotting
        else
         for (j=0; j<pd->NFunctions; j++) // Print out the list of functions which we are plotting
          {
           cptr[k++]=(j!=0)?':':' ';
           inlen=strlen(pd->functions[j]->ascii);
           ppl_texify_generic(x->c, pd->functions[j]->ascii, &inlen, cptr+k, LSTR_LENGTH-k);
           k+=strlen(cptr+k);
          }
        if (pd->ContinuitySet) // Print continuous / discontinuous flag
         {
          if (pd->continuity == DATAFILE_DISCONTINUOUS) { sprintf(cptr+k, " discontinuous"); k+=strlen(cptr+k); }
          else                                          { sprintf(cptr+k,    " continuous"); k+=strlen(cptr+k); }
         }
        if (pd->axis1set || pd->axis2set || pd->axis3set) // Print axes to use
         {
          strcpy(cptr+k, " axes "); k+=strlen(cptr+k);
          if (pd->axis1set) { sprintf(cptr+k, "$%c%d$", "xyzc"[pd->axis1xyz], pd->axis1); k+=strlen(cptr+k); }
          if (pd->axis2set) { sprintf(cptr+k, "$%c%d$", "xyzc"[pd->axis2xyz], pd->axis2); k+=strlen(cptr+k); }
          if (pd->axis3set) { sprintf(cptr+k, "$%c%d$", "xyzc"[pd->axis3xyz], pd->axis3); k+=strlen(cptr+k); }
         }
        if (pd->EverySet>0) { sprintf(cptr+k, " every $%d$", pd->EveryList[0]); k+=strlen(cptr+k); } // Print out 'every' clause of plot command
        if (pd->EverySet>1) { sprintf(cptr+k, ":$%d$", pd->EveryList[1]); k+=strlen(cptr+k); }
        if (pd->EverySet>2) { sprintf(cptr+k, ":$%d$", pd->EveryList[2]); k+=strlen(cptr+k); }
        if (pd->EverySet>3) { sprintf(cptr+k, ":$%d$", pd->EveryList[3]); k+=strlen(cptr+k); }
        if (pd->EverySet>4) { sprintf(cptr+k, ":$%d$", pd->EveryList[4]); k+=strlen(cptr+k); }
        if (pd->EverySet>5) { sprintf(cptr+k, ":$%d$", pd->EveryList[5]); k+=strlen(cptr+k); }
        if (pd->IndexSet) { sprintf(cptr+k, " index $%d$", pd->index); k+=strlen(cptr+k); } // Print index to use
        if (pd->label!=NULL) { sprintf(cptr+k, " label "); k+=strlen(cptr+k); inlen=strlen(pd->label->ascii); ppl_texify_generic(x->c, pd->label->ascii, &inlen, cptr+k, LSTR_LENGTH-k); k+=strlen(cptr+k); } // Print label string
        if (pd->SelectCriterion!=NULL) { strcpy(cptr+k, " select "); k+=strlen(cptr+k); inlen=strlen(pd->SelectCriterion->ascii); ppl_texify_generic(x->c, pd->SelectCriterion->ascii, &inlen, cptr+k, LSTR_LENGTH-k); k+=strlen(cptr+k); } // Print select criterion
        if ((pd->NUsing>0)||(pd->UsingRowCols!=DATAFILE_COL))
         {
          sprintf(cptr+k, " using %s", (pd->UsingRowCols==DATAFILE_COL)?"":"rows"); k+=strlen(cptr+k); // Print using list
          for (j=0; j<pd->NUsing; j++)
           {
            cptr[k++]=(j!=0)?':':' ';
            inlen=strlen(pd->UsingList[j]->ascii);
            ppl_texify_generic(x->c, pd->UsingList[j]->ascii, &inlen, cptr+k, LSTR_LENGTH-k);
            k+=strlen(cptr+k);
           }
         }
        cptr[k]='\0';
        YIELD_TEXTITEM(cptr);
       }
     }
    pd=pd->next;
   }
  return;
 }

#define LOOP_OVER_DATASETS \
{\
  int iDataSet=-1; /* iDataSet counts over all data sets, even ones that we skip because they have no data */\
  pd = x->current->plotitems; \
  while (pd != NULL) \
   { \
    iDataSet++; \
    if ((pd->NoTitleSet) || (pd->TitleFinal==NULL) || (pd->TitleFinal[0]=='\0')) { pd=pd->next; continue; } /* no title set */

#define END_LOOP_OVER_DATASETS \
    pd = pd->next; \
   } \
}

// Lay out all of the items in the current legend with a maximum allowed column height of TrialHeight.
void GraphLegend_ArrangeToHeight(EPSComm *x, double TrialHeight, double *AttainedHeight, int *Ncolumns, double *ColumnX, double *ColumnHeight, int *ColumnNItems)
 {
  canvas_plotdesc *pd;
  int i, ColumnNo = 0;
  double ColumnYPos = 0.0, ColumnWidth = 0.0;

  *AttainedHeight = 0.0;
  for (i=0; i<MAX_LEGEND_COLUMNS; i++) ColumnNItems[i]=0;
  ColumnX[0]=0.0;
  LOOP_OVER_DATASETS;
    if ((ColumnYPos>0) && (pd->TitleFinal_height>TrialHeight-ColumnYPos) && (ColumnNo<MAX_LEGEND_COLUMNS-1)) { ColumnHeight[ColumnNo]=ColumnYPos; ColumnX[ColumnNo+1]=ColumnX[ColumnNo] + ColumnWidth; ColumnYPos=ColumnWidth=0.0; ColumnNo++; }
    ColumnNItems[ColumnNo]++;
    pd->TitleFinal_col  = ColumnNo;
    pd->TitleFinal_xpos = ColumnX[ColumnNo];
    pd->TitleFinal_ypos = -ColumnYPos; // Minus sign since postscript measures height from bottom, and legend runs down page
    ColumnYPos+=pd->TitleFinal_height;
    if (ColumnWidth < pd->TitleFinal_width) ColumnWidth=pd->TitleFinal_width;
    if (ColumnYPos > *AttainedHeight) *AttainedHeight=ColumnYPos;
  END_LOOP_OVER_DATASETS;
  ColumnHeight[ColumnNo]=ColumnYPos;
  ColumnX[ColumnNo+1] = ColumnX[ColumnNo] + ColumnWidth;
  *Ncolumns = ColumnNo+1;
  return;
 }

void GraphLegend_Render(EPSComm *x, double width, double height, double zdepth)
 {
  double fs=x->current->settings.FontSize, CombinedHeight=0.0, MinimumHeight=0.0;
  double xoff=0, yoff=0;
  double ColumnX[MAX_LEGEND_COLUMNS], ColumnHeight[MAX_LEGEND_COLUMNS];
  double BestHeight, AttainedHeight, TrialHeight;
  int    Ncolumns, ColumnNItems[MAX_LEGEND_COLUMNS];
  double height1,height2,bb_top,bb_bottom,ab_left,ab_right,ab_top,ab_bottom;
  unsigned char hfixed=0, vfixed=0;
  canvas_plotdesc *pd;
  int    pageno, j, kp=x->current->settings.KeyPos;
  postscriptPage *dviPage;
  withWords ww;

  if (x->current->settings.key != SW_ONOFF_ON) return;

  // Loop over all legend items to calculate their individual heights and widths, as well as the combined height of all of them
  pageno = x->LaTeXpageno = x->current->LegendTextID;
  LOOP_OVER_DATASETS;
    // Fetch dimensions of requested page of postscript
    if (x->dvi == NULL) { pd->TitleFinal_width=0; pd->TitleFinal_height=0; pd=pd->next; pageno++; continue; }
    dviPage = (postscriptPage *)ppl_listGetItem(x->dvi->output->pages, pageno+1);
    if (dviPage== NULL) { pd->TitleFinal_width=0; pd->TitleFinal_height=0; pd=pd->next; pageno++; continue; } // Such doom will trigger errors later
    //bb_left   = dviPage->boundingBox[0];
    bb_bottom = dviPage->boundingBox[1];
    //bb_right  = dviPage->boundingBox[2];
    bb_top    = dviPage->boundingBox[3];
    ab_left   = dviPage->textSizeBox[0];
    ab_bottom = dviPage->textSizeBox[1];
    ab_right  = dviPage->textSizeBox[2];
    ab_top    = dviPage->textSizeBox[3];
    height1 = fabs(ab_top - ab_bottom) * AB_ENLARGE_FACTOR;
    height2 = fabs(bb_top - bb_bottom) * BB_ENLARGE_FACTOR;
    pd->TitleFinal_height = ((height2<height1) ? height2 : height1) * fs;
    pd->TitleFinal_width  = ((ab_right - ab_left) + MARGIN_HSIZE  ) * fs;
    CombinedHeight += pd->TitleFinal_height;
    if (MinimumHeight < pd->TitleFinal_height) MinimumHeight = pd->TitleFinal_height;
    pageno++;
  END_LOOP_OVER_DATASETS;

  // If number of columns is manually specified, repeatedly reduce height of legend until the desired number of columns is exceeded.
  if      (x->current->settings.KeyColumns > 0)
   {
    BestHeight = TrialHeight = CombinedHeight+2;
    while (TrialHeight>MinimumHeight)
     {
      GraphLegend_ArrangeToHeight(x, TrialHeight, &AttainedHeight, &Ncolumns, ColumnX, ColumnHeight, ColumnNItems);
      if (Ncolumns > x->current->settings.KeyColumns) break;
      if (AttainedHeight>TrialHeight) break;
      BestHeight = TrialHeight;
      TrialHeight = AttainedHeight-1;
     }
   }

  // In ABOVE and BELOW alignment modes, repeatedly reduce height of legend until its width exceeds that of the plot
  else if ((kp == SW_KEYPOS_ABOVE) || (kp == SW_KEYPOS_BELOW))
   {
    hfixed = 1;
    BestHeight = TrialHeight = CombinedHeight+2;
    while (TrialHeight>MinimumHeight)
     {
      GraphLegend_ArrangeToHeight(x, TrialHeight, &AttainedHeight, &Ncolumns, ColumnX, ColumnHeight, ColumnNItems);
      if (ColumnX[Ncolumns] > width-2*LEGEND_MARGIN) break;
      if (AttainedHeight>TrialHeight) break;
      BestHeight = TrialHeight;
      TrialHeight = AttainedHeight-1;
     }
   }

  // In all other modes, make maximum height of legend equal height of plot.
  else
   {
    vfixed = 1;
    BestHeight = height-2*LEGEND_MARGIN;
   }

  // Adjust spacing between legend items to justify them as necessary
  GraphLegend_ArrangeToHeight(x, BestHeight, &AttainedHeight, &Ncolumns, ColumnX, ColumnHeight, ColumnNItems);
  TrialHeight = vfixed ? (height-2*LEGEND_MARGIN) : AttainedHeight; // Vertical justification
  for (j=0; j<Ncolumns; j++) if ((ColumnNItems[j]<2) || (ColumnHeight[j] < TrialHeight-LEGEND_VGAP_MAXIMUM*(ColumnNItems[j]-1))) { TrialHeight = AttainedHeight; break; }
  for (j=0; j<Ncolumns; j++)
   if (ColumnHeight[j] >= TrialHeight-LEGEND_VGAP_MAXIMUM*(ColumnNItems[j]-1))
    {
     double GapPerItem = (TrialHeight-ColumnHeight[j])/(ColumnNItems[j]-1), gap=0.0;
     if (gsl_finite(GapPerItem))
      {
       LOOP_OVER_DATASETS; if (pd->TitleFinal_col==j) { pd->TitleFinal_ypos-=gap; gap+=GapPerItem; } END_LOOP_OVER_DATASETS;
      }
    }
  AttainedHeight = TrialHeight;
  // Horizonal justification
  if (hfixed && (ColumnX[Ncolumns] < width-2*LEGEND_MARGIN) && (Ncolumns>1) && (ColumnX[Ncolumns] >= width-2*LEGEND_MARGIN-LEGEND_HGAP_MAXIMUM*(Ncolumns-1)))
   {
    double GapPerColumn = (width - 2*LEGEND_MARGIN - ColumnX[Ncolumns])/Ncolumns;
    if (gsl_finite(GapPerColumn))
     {
      LOOP_OVER_DATASETS; pd->TitleFinal_xpos+=pd->TitleFinal_col*GapPerColumn; END_LOOP_OVER_DATASETS;
      for (j=0; j<Ncolumns; j++) ColumnX[j] += j*GapPerColumn;
      ColumnX[Ncolumns] += (Ncolumns-1)*GapPerColumn;
     }
   }

  // Translate legend to desired place on canvas (2D case)
  if (!x->current->ThreeDim)
   {
    switch (kp)
     {
      case SW_KEYPOS_TR:      xoff = width   - ColumnX[Ncolumns]   - LEGEND_MARGIN; yoff = height                     - LEGEND_MARGIN; break;
      case SW_KEYPOS_TM:      xoff = width/2 - ColumnX[Ncolumns]/2                ; yoff = height                     - LEGEND_MARGIN; break;
      case SW_KEYPOS_TL:      xoff =                                 LEGEND_MARGIN; yoff = height                     - LEGEND_MARGIN; break;
      case SW_KEYPOS_MR:      xoff = width   - ColumnX[Ncolumns]   - LEGEND_MARGIN; yoff = height/2 + AttainedHeight/2               ; break;
      case SW_KEYPOS_MM:      xoff = width/2 - ColumnX[Ncolumns]/2                ; yoff = height/2 + AttainedHeight/2               ; break;
      case SW_KEYPOS_ML:      xoff =                                 LEGEND_MARGIN; yoff = height/2 + AttainedHeight/2               ; break;
      case SW_KEYPOS_BR:      xoff = width   - ColumnX[Ncolumns]   - LEGEND_MARGIN; yoff =            AttainedHeight  + LEGEND_MARGIN; break;
      case SW_KEYPOS_BM:      xoff = width/2 - ColumnX[Ncolumns]/2                ; yoff =            AttainedHeight  + LEGEND_MARGIN; break;
      case SW_KEYPOS_BL:      xoff =                                 LEGEND_MARGIN; yoff =            AttainedHeight  + LEGEND_MARGIN; break;
      case SW_KEYPOS_ABOVE:   xoff = width/2 - ColumnX[Ncolumns]/2                ; yoff =            AttainedHeight  + LEGEND_MARGIN + x->current->PlotTopMargin    - x->current->settings.OriginY.real*M_TO_PS; break;
      case SW_KEYPOS_BELOW:   xoff = width/2 - ColumnX[Ncolumns]/2                ; yoff =                            - LEGEND_MARGIN + x->current->PlotBottomMargin - x->current->settings.OriginY.real*M_TO_PS; break;
      case SW_KEYPOS_OUTSIDE: xoff =                                 LEGEND_MARGIN; yoff = height                     - LEGEND_MARGIN;
                              xoff+= x->current->PlotRightMargin - x->current->settings.OriginX.real*M_TO_PS; break;
     }
    xoff += x->current->settings.OriginX.real * M_TO_PS;
    yoff += x->current->settings.OriginY.real * M_TO_PS;
   }

  // Translate legend to desired place on canvas (3D case)
  else
   {
    SortByAzimuthTarget = -999;

    switch (kp)
     {
      case SW_KEYPOS_TR: SortByAzimuthTarget =  1*M_PI/4; xoff =                       LEGEND_MARGIN + x->current->PlotRightMargin; yoff = AttainedHeight  + LEGEND_MARGIN; break;
      case SW_KEYPOS_TM: SortByAzimuthTarget =  0*M_PI/4; xoff = -ColumnX[Ncolumns]/2                                             ; yoff = AttainedHeight + LEGEND_MARGIN + x->current->PlotTopMargin; break;
      case SW_KEYPOS_TL: SortByAzimuthTarget = -1*M_PI/4; xoff = -ColumnX[Ncolumns]  - LEGEND_MARGIN + x->current->PlotLeftMargin ; yoff = AttainedHeight  + LEGEND_MARGIN; break;
      case SW_KEYPOS_MR: SortByAzimuthTarget =  2*M_PI/4; xoff =                       LEGEND_MARGIN + x->current->PlotRightMargin; yoff = AttainedHeight/2               ; break;
      case SW_KEYPOS_MM:                                  xoff = -ColumnX[Ncolumns]/2 + x->current->settings.OriginX.real * M_TO_PS; yoff = AttainedHeight/2 + x->current->settings.OriginY.real * M_TO_PS; break;
      case SW_KEYPOS_ML: SortByAzimuthTarget = -2*M_PI/4; xoff = -ColumnX[Ncolumns]  - LEGEND_MARGIN + x->current->PlotLeftMargin ; yoff = AttainedHeight/2               ; break;
      case SW_KEYPOS_BR: SortByAzimuthTarget =  3*M_PI/4; xoff =                       LEGEND_MARGIN + x->current->PlotRightMargin; yoff =                 - LEGEND_MARGIN; break;
      case SW_KEYPOS_BM: SortByAzimuthTarget =  4*M_PI/4; xoff = -ColumnX[Ncolumns]/2                                             ; yoff = - LEGEND_MARGIN + x->current->PlotBottomMargin; break;
      case SW_KEYPOS_BL: SortByAzimuthTarget = -3*M_PI/4; xoff = -ColumnX[Ncolumns]  - LEGEND_MARGIN + x->current->PlotLeftMargin ; yoff =                 - LEGEND_MARGIN; break;

      case SW_KEYPOS_ABOVE:   xoff = -ColumnX[Ncolumns]/2 + x->current->settings.OriginX.real * M_TO_PS;
                              yoff = AttainedHeight + LEGEND_MARGIN + x->current->PlotTopMargin;
                              break;
      case SW_KEYPOS_BELOW:   xoff = -ColumnX[Ncolumns]/2 + x->current->settings.OriginX.real * M_TO_PS;
                              yoff =                - LEGEND_MARGIN + x->current->PlotBottomMargin;
                              break;
      case SW_KEYPOS_OUTSIDE: xoff =  LEGEND_MARGIN + x->current->PlotRightMargin;
                              yoff = -LEGEND_MARGIN + x->current->PlotTopMargin;
                              break;
     }

    // Find the vertex of the graph closest to the target azimuth
    if (SortByAzimuthTarget>-100)
     {
      int i;
      double xap, yap, zap, data[3*8];
      double origin_x = x->current->settings.OriginX.real*M_TO_PS;
      double origin_y = x->current->settings.OriginY.real*M_TO_PS;
      for (i=0;i<8;i++)
       {
        xap=((i&1)!=0);
        yap=((i&2)!=0);
        zap=((i&4)!=0);
        eps_plot_ThreeDimProject(xap,yap,zap,&x->current->settings,origin_x,origin_y,width,height,zdepth,data+3*i,data+3*i+1,data+3*i+2);
       }
      SortByAzimuthXCentre = origin_x;
      SortByAzimuthYCentre = origin_y;
      qsort((void *)(data  ),8,3*sizeof(double),SortByDepth);
      if ((data[3*0+2]==data[3*1+2])&&(hypot(data[3*0+0]-origin_x,data[3*0+1]-origin_y)>hypot(data[3*1+0]-origin_x,data[3*1+1]-origin_y))) TriSwap(data+3*0,data+3*1);
      if ((data[3*7+2]==data[3*6+2])&&(hypot(data[3*7+0]-origin_x,data[3*7+1]-origin_y)>hypot(data[3*6+0]-origin_x,data[3*6+1]-origin_y))) TriSwap(data+3*7,data+3*6);
      qsort((void *)(data+3),6,3*sizeof(double),SortByAzimuthProximity);
      if ((kp==SW_KEYPOS_TM)||(kp==SW_KEYPOS_BM))
        xoff += data[3*1+0];
      else
        yoff += data[3*1+1];
     }
   }
 
  xoff += x->current->settings.KeyXOff.real * M_TO_PS;
  yoff += x->current->settings.KeyYOff.real * M_TO_PS;   
  LOOP_OVER_DATASETS;
  pd->TitleFinal_xpos += xoff;
  pd->TitleFinal_ypos += yoff;
  END_LOOP_OVER_DATASETS;

  // Finally loop over all datasets to display legend items
  LOOP_OVER_DATASETS
    int xyzaxis[3];
    pplset_axis *a1, *a2, *a3, *axissets[3];
    axissets[0] = x->current->XAxes;
    axissets[1] = x->current->YAxes;
    axissets[2] = x->current->ZAxes;
    a1 = &axissets[pd->axis1xyz][pd->axis1];
    a2 = &axissets[pd->axis2xyz][pd->axis2];
    a3 = &axissets[pd->axis3xyz][pd->axis3];
    xyzaxis[pd->axis1xyz] = 0;
    xyzaxis[pd->axis2xyz] = 1;
    xyzaxis[pd->axis3xyz] = 2;
    eps_plot_LegendIcon(x, iDataSet, pd, pd->TitleFinal_xpos + MARGIN_HSIZE_LEFT/2, pd->TitleFinal_ypos - pd->TitleFinal_height/2, MARGIN_HSIZE_LEFT, a1, a2, a3, xyzaxis[0], xyzaxis[1], xyzaxis[2]);
    pageno = x->LaTeXpageno++;
    ppl_withWordsZero(x->c,&ww);
    if (x->current->settings.TextColour > 0) { ww.color = x->current->settings.TextColour; ww.USEcolor = 1; }
    else                                     { ww.Col1234Space = x->current->settings.TextCol1234Space; ww.color1 = x->current->settings.TextColour1; ww.color2 = x->current->settings.TextColour2; ww.color3 = x->current->settings.TextColour3; ww.color4 = x->current->settings.TextColour4; ww.USEcolor1234 = 1; }
    eps_core_SetColour(x, &ww, 1);
    IF_NOT_INVISIBLE canvas_EPSRenderTextItem(x, NULL, pageno, (pd->TitleFinal_xpos+MARGIN_HSIZE_LEFT)/M_TO_PS, (pd->TitleFinal_ypos - pd->TitleFinal_height/2)/ M_TO_PS, SW_HALIGN_LEFT, SW_VALIGN_CENT, x->CurrentColour, fs, 0.0, NULL, NULL);
  END_LOOP_OVER_DATASETS
  return;
 }

