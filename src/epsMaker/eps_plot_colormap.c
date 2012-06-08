// eps_plot_colormap.c
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

#define _PPL_EPS_PLOT_COLORMAP_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <gsl/gsl_math.h>

#include "coreUtils/memAlloc.h"

#include "canvasItems.h"
#include "coreUtils/errorReport.h"
#include "expressions/traceback_fns.h"
#include "expressions/expEval.h"
#include "settings/colors.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"
#include "stringTools/asciidouble.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/unitsDisp.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/pplObj_fns.h"

#include "epsMaker/canvasDraw.h"
#include "epsMaker/bmp_a85.h"
#include "epsMaker/bmp_optimise.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "settings/epsColors.h"
#include "epsMaker/eps_image.h"
#include "epsMaker/eps_plot.h"
#include "epsMaker/eps_plot_axespaint.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_colormap.h"
#include "epsMaker/eps_plot_legend.h"
#include "epsMaker/eps_plot_ticking.h"
#include "epsMaker/eps_settings.h"
#include "epsMaker/eps_style.h"

// Random shade of purple to use as mask color
#define TRANS_R 35
#define TRANS_G 2
#define TRANS_B 20

#define CLIP_COMPS \
  comp[0] = (comp[0] < 0.0) ? 0.0 : ((comp[0]>1.0) ? 1.0 : comp[0] ); \
  comp[1] = (comp[1] < 0.0) ? 0.0 : ((comp[1]>1.0) ? 1.0 : comp[1] ); \
  comp[2] = (comp[2] < 0.0) ? 0.0 : ((comp[2]>1.0) ? 1.0 : comp[2] ); \
  comp[3] = (comp[3] < 0.0) ? 0.0 : ((comp[3]>1.0) ? 1.0 : comp[3] );

// Yield up text items which label color scale of a colormap
void eps_plot_colormap_YieldText(EPSComm *x, dataTable *data, pplset_graph *sg, canvas_plotdesc *pd)
 {
  dataBlock     *blk;
  int            XSize = pd->GridXSize;
  int            YSize = pd->GridYSize;
  int            i, j, k, l, Ncol;
  double         CMin, CMax, PhysicalLength, PhysicalLengthMajor, PhysicalLengthMinor;
  double         origin_x, origin_y, width, height, zdepth;
  double         xmin, xmax, ymin, ymax;
  unsigned char  CMinAuto, CMinSet, CMaxAuto, CMaxSet, CLog, horizontal;

  // Check that we have some data
  if ((data==NULL) || (data->Nrows<1)) return; // No data present
  Ncol = data->Ncolumns_real;
  blk  = data->first;

  // Calculate positions of the four corners of graph
  origin_x = x->current->settings.OriginX.real * M_TO_PS;
  origin_y = x->current->settings.OriginY.real * M_TO_PS;
  width    = x->current->settings.width  .real * M_TO_PS;
  if (x->current->settings.AutoAspect  == SW_ONOFF_ON) height = width * 2.0/(1.0+sqrt(5));
  else                                                 height = width * x->current->settings.aspect;
  if (x->current->settings.AutoZAspect == SW_ONOFF_ON) zdepth = width * 2.0/(1.0+sqrt(5));
  else                                                 zdepth = width * x->current->settings.zaspect;

  // Work out normalisation of variable c1
  CMinAuto = (sg->Cminauto[0]==SW_BOOL_TRUE);
  CMinSet  = !CMinAuto;
  CMin     = sg->Cmin[0].real;
  CMaxAuto = (sg->Cmaxauto[0]==SW_BOOL_TRUE);
  CMaxSet  = !CMaxAuto;
  CMax     = sg->Cmax[0].real;
  CLog     = (sg->Clog[0]==SW_BOOL_TRUE);

  // Find extremal values
  if (CMinAuto || CMaxAuto)
   for (j=0; j<YSize; j++)
    for (i=0; i<XSize; i++)
     {
      double val = blk->data_real[2 + Ncol*(i+XSize*j)];
      if (!gsl_finite(val)) continue;
      if ((CMinAuto) && ((!CMinSet) || (CMin>val)) && ((!CLog)||(val>0.0))) { CMin=val; CMinSet=1; }
      if ((CMaxAuto) && ((!CMaxSet) || (CMax<val)) && ((!CLog)||(val>0.0))) { CMax=val; CMaxSet=1; }
     }

  // If no data present, stop now
  if ((!CMinSet)||(!CMaxSet)) return;

  // If log spacing, make sure range is strictly positive
  if (CLog && (CMin<1e-200)) CMin=1e-200;
  if (CLog && (CMax<1e-200)) CMax=1e-200;

  // Estimate length of color key
  if (!x->current->ThreeDim)
   {
    xmin = origin_x;
    xmax = origin_x + width;
    ymin = origin_y;
    ymax = origin_y + height;
   }
  else
   {
    xmin = xmax = origin_x;
    ymin = ymax = origin_y;
    for (k=0; k<8; k++)
     {
      double x0,y0,z0;
      eps_plot_ThreeDimProject((k&1),(k&2)!=0,(k&4)!=0,sg,origin_x,origin_y,width,height,zdepth,&x0,&y0,&z0);
      if (x0<xmin) xmin=x0; if (x0>xmax) xmax=x0;
      if (y0<ymin) ymin=y0; if (y0>ymax) ymax=y0;
     }
   }

  horizontal     = ((sg->ColKeyPos==SW_COLKEYPOS_T)||(sg->ColKeyPos==SW_COLKEYPOS_B));
  PhysicalLength = fabs( horizontal ? (xmax-xmin) : (ymax-ymin) ) / M_TO_PS;
  PhysicalLengthMinor = PhysicalLength / 0.004;
  PhysicalLengthMajor = PhysicalLength / (horizontal ? 0.025 : 0.015);

  // Set up axis object
  pd->C1Axis = x->c->set->axis_default;
  pd->C1Axis.label = sg->c1label;
  pd->C1Axis.LabelRotate = sg->c1LabelRotate;
  pd->C1Axis.format = sg->c1formatset ? sg->c1format : NULL;
  pd->C1Axis.tics = sg->ticsC;
  pd->C1Axis.ticsM = sg->ticsCM;
  pd->C1Axis.TickLabelRotation = sg->c1TickLabelRotation;
  pd->C1Axis.TickLabelRotate = sg->c1TickLabelRotate;
  pd->C1Axis.enabled = pd->C1Axis.FinalActive = pd->C1Axis.RangeFinalised = 1;
  pd->C1Axis.TickListFinalised = 0;
  pd->C1Axis.topbottom = (sg->ColKeyPos==SW_COLKEYPOS_T)||(sg->ColKeyPos==SW_COLKEYPOS_R);
  pd->C1Axis.log = pd->C1Axis.LogFinal = CLog ? SW_BOOL_TRUE : SW_BOOL_FALSE;
  pd->C1Axis.MinSet = pd->C1Axis.MaxSet = SW_BOOL_TRUE;
  pd->C1Axis.HardMinSet = pd->C1Axis.HardMaxSet = 1;
  pd->C1Axis.min = pd->C1Axis.MinFinal = pd->C1Axis.HardMin = CMin;
  pd->C1Axis.max = pd->C1Axis.MaxFinal = pd->C1Axis.HardMax = CMax;
  pd->C1Axis.DataUnitSet = pd->C1Axis.HardUnitSet = 1;
  pd->C1Axis.DataUnit = pd->C1Axis.HardUnit = pd->C1Axis.unit = data->firstEntries[2];
  pd->C1Axis.AxisValueTurnings = 0;
  pd->C1Axis.AxisLinearInterpolation = NULL;
  pd->C1Axis.CrossedAtZero = 0;
  pd->C1Axis.MinUsedSet = pd->C1Axis.MaxUsedSet = 0;
  pd->C1Axis.MinUsed    = pd->C1Axis.MaxUsed = 0.0;
  pd->C1Axis.HardAutoMinSet = pd->C1Axis.HardAutoMaxSet = 0;
  pd->C1Axis.Mode0BackPropagated = 0;
  pd->C1Axis.OrdinateRasterLen = 0;
  pd->C1Axis.OrdinateRaster = NULL;
  pd->C1Axis.FinalAxisLabel = NULL;
  pd->C1Axis.PhysicalLengthMajor = PhysicalLengthMajor;
  pd->C1Axis.PhysicalLengthMinor = PhysicalLengthMinor;
  pd->C1Axis.xyz            = 3;
  pd->C1Axis.axis_n         = 1;
  pd->C1Axis.canvas_id      = x->current->id;

  // Make tick labels along this axis
  eps_plot_ticking(x, &pd->C1Axis, SW_AXISUNITSTY_RATIO, NULL);

  // Submit axis labels for color key
  pd->C1Axis.FirstTextID = x->NTextItems;
  if (pd->C1Axis.TickListStrings!=NULL)
   for (l=0; pd->C1Axis.TickListStrings[l]!=NULL; l++)
    {
     YIELD_TEXTITEM(pd->C1Axis.TickListStrings[l]);
    }
  YIELD_TEXTITEM(pd->C1Axis.FinalAxisLabel);

  // If we have three columns of data, consider drawing a color scale bar
  if ((Ncol==3)&&(sg->ColKey==SW_ONOFF_ON))
   {
    pd->CRangeDisplay = 1;
    pd->CMinFinal     = CMin;
    pd->CMaxFinal     = CMax;
    pd->CRangeUnit    = data->firstEntries[2];
   }

  return;
 }

// Render a colormap to postscript
int  eps_plot_colormap(EPSComm *x, dataTable *data, unsigned char ThreeDim, int xn, int yn, int zn, pplset_graph *sg, canvas_plotdesc *pd, int pdn, double origin_x, double origin_y, double width, double height, double zdepth)
 {
  double         scale_x, scale_y, scale_z;
  dataBlock     *blk;
  int            XSize = pd->GridXSize;
  int            YSize = pd->GridYSize;
  int            i, j, c, cmax, Ncol_real, Ncol_obj, NcolsData;
  long           p;
  double         xo, yo, Lx, Ly, ThetaX, ThetaY, CMin[4], CMax[4];
  pplObj        *CVar[4], *C1Var, CDummy[4];
  uLongf         zlen; // Length of buffer passed to zlib
  unsigned char *imagez, CMinAuto[4], CMinSet[4], CMaxAuto[4], CMaxSet[4], CLog[4];
  char          *errtext;
  unsigned char  component_r, component_g, component_b, transparent[3] = {TRANS_R, TRANS_G, TRANS_B};
  bitmap_data    img;

  if ((data==NULL) || (data->Nrows<1)) return 0; // No data present
  Ncol_real = data->Ncolumns_real;
  Ncol_obj  = data->Ncolumns_obj;
  if (eps_plot_WithWordsCheckUsingItemsDimLess(x->c, &pd->ww_final, data->firstEntries, Ncol_real, Ncol_obj, &NcolsData)) return 1;
  if (!ThreeDim) { scale_x=width; scale_y=height; scale_z=1.0;    }
  else           { scale_x=width; scale_y=height; scale_z=zdepth; }
  blk = data->first;

  errtext = ppl_memAlloc(LSTR_LENGTH);
  if (errtext==NULL) { ppl_error(&x->c->errcontext,ERR_MEMORY,-1,-1,"Out of memory (k)."); return 1; }

  // Work out orientation of colormap
  if (!ThreeDim)
   {
    xo     = origin_x;
    yo     = origin_y;
    Lx     = scale_x;
    Ly     = scale_y;
    ThetaX = M_PI/2;
    ThetaY = 0.0;
    if (xn!=0) { double t1=Lx, t2=ThetaX; Lx=Ly; ThetaX=ThetaY; Ly=t1; ThetaY=t2; }
   }
  else // 3D case: put color map on back face of cuboid
   {
    double ap[3]={0.5,0.5,0.0}, xtmp, ytmp, z0tmp, z1tmp;
    eps_plot_ThreeDimProject(ap[xn], ap[yn], ap[zn], sg, origin_x, origin_y, scale_x, scale_y, scale_z, &xtmp, &ytmp, &z0tmp);
    ap[2]=1.0;
    eps_plot_ThreeDimProject(ap[xn], ap[yn], ap[zn], sg, origin_x, origin_y, scale_x, scale_y, scale_z, &xtmp, &ytmp, &z1tmp);
    ap[2]=(z1tmp>z0tmp)?1.0:0.0; // Determine whether zap=0 or zap=1 represents back of cuboid

    ap[0]=ap[1]=0.0;
    eps_plot_ThreeDimProject(ap[xn], ap[yn], ap[zn], sg, origin_x, origin_y, scale_x, scale_y, scale_z, &xo, &yo, &z0tmp);

    ap[0]=1.0;
    eps_plot_ThreeDimProject(ap[xn], ap[yn], ap[zn], sg, origin_x, origin_y, scale_x, scale_y, scale_z, &xtmp, &ytmp, &z0tmp);
    Lx = hypot(xtmp-xo, ytmp-yo);
    ThetaX = atan2(xtmp-xo, ytmp-yo);
    if (!gsl_finite(ThetaX)) ThetaX = 0.0;

    ap[0]=0.0;
    ap[1]=1.0;
    eps_plot_ThreeDimProject(ap[xn], ap[yn], ap[zn], sg, origin_x, origin_y, scale_x, scale_y, scale_z, &xtmp, &ytmp, &z0tmp);
    Ly = hypot(xtmp-xo, ytmp-yo);
    ThetaY = atan2(xtmp-xo, ytmp-yo);
    if (!gsl_finite(ThetaY)) ThetaY = 0.0;
   }

  // Update bounding box
  eps_core_BoundingBox(x, xo                                   , yo                                   , 0);
  eps_core_BoundingBox(x, xo + Lx*sin(ThetaX)                  , yo + Lx*cos(ThetaX)                  , 0);
  eps_core_BoundingBox(x, xo +                  Ly*sin(ThetaY) , yo +                  Ly*cos(ThetaY) , 0);
  eps_core_BoundingBox(x, xo + Lx*sin(ThetaX) + Ly*sin(ThetaY) , yo + Lx*cos(ThetaX) + Ly*cos(ThetaY) , 0);

  // Populate bitmap image decriptor
  img.type     = BMP_COLOUR_BMP;
  img.colour   = BMP_COLOUR_RGB;
  img.pal_len  = 0;
  img.palette  = NULL;
  img.trans    = transparent;
  img.height   = YSize;
  img.width    = XSize;
  img.depth    = 24;
  img.data_len = 3*XSize*YSize;
  img.TargetCompression = BMP_ENCODING_FLATE;
  BMP_ALLOC(img.data , 3*XSize*YSize);
  if (img.data==NULL) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1,"Out of memory (l)."); return 1; }

  // Get pointer to variable c in the user's variable space
  for (i=0; i<4; i++)
   {
    char v[3]={'c','1'+i,'\0'};
    ppl_contextGetVarPointer(x->c, v, CVar+i, CDummy+i);
    if ((i<NcolsData-2)&&(sg->Crenorm[i]==SW_BOOL_FALSE)) { *CVar[i] = data->firstEntries[i+2]; CVar[i]->flagComplex=0; CVar[i]->imag=0.0; }
    else pplObjNum(CVar[i],0,0,0); // c1...c4 are dimensionless numbers in range 0-1, regardless of units of input data
   }

  // Work out normalisation of variables c1...c4
  for (c=0; ((c<4)&&(c<NcolsData-2)); c++)
   {
    CMinAuto[c] = (sg->Cminauto[c]==SW_BOOL_TRUE);
    CMinSet [c] = !CMinAuto[c];
    CMin    [c] = sg->Cmin[c].real;
    CMaxAuto[c] = (sg->Cmaxauto[c]==SW_BOOL_TRUE);
    CMaxSet [c] = !CMaxAuto[c];
    CMax    [c] = sg->Cmax[c].real;
    CLog    [c] = (sg->Clog[c]==SW_BOOL_TRUE);

    // Find extremal values
    if (CMinAuto[c] || CMaxAuto[c])
     for (j=0; j<YSize; j++)
      for (i=0; i<XSize; i++)
       {
        double val = blk->data_real[c+2 + Ncol_real*(i+XSize*j)];
        if (!gsl_finite(val)) continue;
        if ((CMinAuto[c]) && ((!CMinSet[c]) || (CMin[c]>val)) && ((!CLog[c])||(val>0.0))) { CMin[c]=val; CMinSet[c]=1; }
        if ((CMaxAuto[c]) && ((!CMaxSet[c]) || (CMax[c]<val)) && ((!CLog[c])||(val>0.0))) { CMax[c]=val; CMaxSet[c]=1; }
       }

    // If log spacing, make sure range is strictly positive
    if (CLog[c] && (CMin[c]<1e-200)) CMin[c]=1e-200;
    if (CLog[c] && (CMax[c]<1e-200)) CMax[c]=1e-200;

    // Reverse range of color scale if requested
    if (sg->Creverse[c]==SW_BOOL_TRUE)
     {
      double td=CMin[c]; unsigned char tc=CMinSet[c], tc2=CMinAuto[c];
      CMinAuto[c] = CMaxAuto[c];
      CMinSet [c] = CMaxSet [c];
      CMin    [c] = CMax    [c];
      CMaxAuto[c] = tc2;
      CMaxSet [c] = tc;
      CMax    [c] = td;
     }

    // If no data present, stop now
    if ((!CMinSet[c])||(!CMaxSet[c]))
     {
      sprintf(x->c->errcontext.tempErrStr, "No data supplied to determine range for variable c%d", c+1);
      ppl_log(&x->c->errcontext,NULL);
      for (i=0; i<4; i++)
       {
        char v[3]={'c','1'+i,'\0'};
        ppl_contextRestoreVarPointer(x->c, v, CDummy+i);
       }
      return 0;
     }

    // Output result to debugging output
    if (DEBUG)
     {
      int SF = x->c->set->term_current.SignificantFigures;
      sprintf(x->c->errcontext.tempErrStr, "Range for variable c%d is [%s:%s]", c+1, ppl_numericDisplay(CMin[c],x->c->numdispBuff[0],SF,0), ppl_numericDisplay(CMax[c],x->c->numdispBuff[1],SF,0));
      ppl_log(&x->c->errcontext,NULL);
     }
   }
  cmax = c-1;

  // Check that variables c1...c4 has appropriate units
  for (c=0; c<=cmax; c++)
   if ( ((!CMinAuto[c])||(!CMaxAuto[c])) && (!ppl_unitsDimEqual(CVar[c] , (sg->Cminauto[c]==SW_BOOL_TRUE)?(&sg->Cmax[c]):(&sg->Cmin[c]))) )
    {
     sprintf(x->c->errcontext.tempErrStr, "Column %d of data supplied to the colormap plot style has conflicting units with those set in the 'set crange' command. The former has units of <%s> whilst the latter has units of <%s>.", c+3, ppl_printUnit(x->c,CVar[c], NULL, NULL, 0, 1, 0), ppl_printUnit(x->c,(sg->Cminauto[c]==SW_BOOL_TRUE)?(&sg->Cmax[c]):(&sg->Cmin[c]), NULL, NULL, 1, 1, 0));
     ppl_error(&x->c->errcontext,ERR_NUMERICAL,-1,-1,NULL);
     return 1;
    }

  // Populate bitmap data array
  for (p=0, j=YSize-1; j>=0; j--) // Postscript images are top-first. Data block is bottom-first.
   for (i=0; i<XSize; i++)
    {
     // Set values of c1...c4
     for (c=0;c<4; c++)
      if      (c>cmax)  /* No c<c> */         { *CVar[c]       = CDummy[c]; }
      else if (sg->Crenorm[c]==SW_BOOL_FALSE) {  CVar[c]->real = blk->data_real[c+2 + Ncol_real*(i+XSize*j)]; } // No renormalisation
      else if (CMax[c]==CMin[c])  /* Ooops */ {  CVar[c]->real = (gsl_finite(blk->data_real[c+2 + Ncol_real*(i+XSize*j)]))?0.5:(GSL_NAN); }
      else if (!CLog[c]) /* Linear */         {  CVar[c]->real = (blk->data_real[c+2 + Ncol_real*(i+XSize*j)] - CMin[c]) / (CMax[c] - CMin[c]); }
      else               /* Logarithmic */    {  CVar[c]->real = log(blk->data_real[c+2 + Ncol_real*(i+XSize*j)] / CMin[c]) / log(CMax[c] / CMin[c]); }

#define SET_RGB_COLOR \
     /* Check if mask criterion is satisfied */ \
     { \
     int       colspace; \
     double    comp[4]={0,0,0,0}; \
     const int stkLevelOld = x->c->stackPtr; \
     if (sg->MaskExpr!=NULL) \
      { \
       pplObj *v; \
       int lOP; \
       v = ppl_expEval(x->c, (pplExpr *)sg->MaskExpr, &lOP, 1, x->iterDepth+1); \
       if (x->c->errStat.status) { sprintf(x->c->errcontext.tempErrStr, "Could not evaluate mask expression <%s>.", ((pplExpr *)sg->MaskExpr)->ascii); ppl_error(&x->c->errcontext,ERR_NUMERICAL,-1,-1,NULL); ppl_tbWrite(x->c); ppl_tbClear(x->c); return 1; } \
       if (v->real==0) { component_r = TRANS_R; component_g = TRANS_G; component_b = TRANS_B; goto write_rgb; } \
      } \
 \
     /* Compute RGB, HSB or CMYK components */ \
     if (sg->ColMapExpr == NULL) \
      { \
       colspace = SW_COLSPACE_RGB; \
       comp[0] = comp[1] = comp[2] = C1Var->real; \
      } \
     else \
      { \
       pplObj *v; \
       int lOP, outcol; \
       unsigned char d1, d2; \
       v = ppl_expEval(x->c, (pplExpr *)sg->ColMapExpr, &lOP, 1, x->iterDepth+1); \
       if (x->c->errStat.status) { sprintf(x->c->errcontext.tempErrStr, "Could not evaluate color expression <%s>.", ((pplExpr *)sg->ColMapExpr)->ascii); ppl_error(&x->c->errcontext,ERR_NUMERICAL,-1,-1,NULL); ppl_tbWrite(x->c); ppl_tbClear(x->c); return 1; } \
       lOP = ppl_colorFromObj(x->c, v, &outcol, &colspace, NULL, comp, comp+1, comp+2, comp+3, &d1, &d2); \
       if (lOP) { ppl_error(&x->c->errcontext,ERR_NUMERICAL,-1,-1,NULL); return 1; } \
       if (outcol!=0) \
        { \
         colspace = SW_COLSPACE_CMYK; \
         comp[0] = *(double *)ppl_fetchSettingName(&x->c->errcontext, outcol, SW_COLOR_INT, (void *)SW_COLOR_CMYK_C, sizeof(double)); \
         comp[1] = *(double *)ppl_fetchSettingName(&x->c->errcontext, outcol, SW_COLOR_INT, (void *)SW_COLOR_CMYK_M, sizeof(double)); \
         comp[2] = *(double *)ppl_fetchSettingName(&x->c->errcontext, outcol, SW_COLOR_INT, (void *)SW_COLOR_CMYK_Y, sizeof(double)); \
         comp[3] = *(double *)ppl_fetchSettingName(&x->c->errcontext, outcol, SW_COLOR_INT, (void *)SW_COLOR_CMYK_K, sizeof(double)); \
        } \
       if ((!gsl_finite(comp[0]))||(!gsl_finite(comp[1]))||(!gsl_finite(comp[2]))||(!gsl_finite(comp[3]))) \
           { component_r = TRANS_R; component_g = TRANS_G; component_b = TRANS_B; goto write_rgb; } \
      } \
 \
     /* Convert to RGB */ \
     switch (colspace) \
      { \
       case SW_COLSPACE_RGB: /* Convert RGB --> RGB */ \
        break; \
       case SW_COLSPACE_HSB: /* Convert HSB --> RGB */ \
        { \
         double h2, ch, x, m; int h2i; \
         CLIP_COMPS; \
         ch  = comp[1]*comp[2]; \
         h2i = (int)(h2 = comp[0] * 6); \
         x   = ch*(1.0-fabs(fmod(h2,2)-1.0)); \
         m   = comp[2] - ch; \
         switch (h2i) \
          { \
           case 0 : comp[0]=ch; comp[1]=x ; comp[2]=0 ; break; \
           case 1 : comp[0]=x ; comp[1]=ch; comp[2]=0 ; break; \
           case 2 : comp[0]=0 ; comp[1]=ch; comp[2]=x ; break; \
           case 3 : comp[0]=0 ; comp[1]=x ; comp[2]=ch; break; \
           case 4 : comp[0]=x ; comp[1]=0 ; comp[2]=ch; break; \
           case 5 : \
           case 6 : comp[0]=ch; comp[1]=0 ; comp[2]=x ; break; /* case 6 is for hue=1.0 only */ \
           default: comp[0]=0 ; comp[1]=0 ; comp[2]=0 ; break; \
          } \
         comp[0]+=m; comp[1]+=m; comp[2]+=m; \
         break; \
        } \
       case SW_COLSPACE_CMYK: /* Convert CMYK --> RGB */ \
        comp[0] = 1.0 - (comp[0]+comp[3]); \
        comp[1] = 1.0 - (comp[1]+comp[3]); \
        comp[2] = 1.0 - (comp[2]+comp[3]); \
        break; \
       default: /* Unknown color space */ \
        comp[0] = comp[1] = comp[2] = 0.0; \
        break; \
      } \
     CLIP_COMPS; \
 \
     /* Store RGB components */ \
     component_r = (unsigned char)floor(comp[0] * 255.99); \
     component_g = (unsigned char)floor(comp[1] * 255.99); \
     component_b = (unsigned char)floor(comp[2] * 255.99); \
     if ((component_r==TRANS_R)&&(component_g==TRANS_G)&&(component_b==TRANS_B)) component_b++; \
 \
write_rgb: \
     img.data[p++] = component_r; \
     img.data[p++] = component_g; \
     img.data[p++] = component_b; \
     EPS_STACK_POP; \
     }

     C1Var = CVar[0];
     SET_RGB_COLOR;
    }

  // Restore variables c1...c4 in the user's variable space
  for (i=0; i<4; i++)
   {
    char v[3]={'c','1'+i,'\0'};
    ppl_contextRestoreVarPointer(x->c, v, CDummy+i);
   }

#define COMPRESS_POSTSCRIPT_IMAGE \
  /* Consider converting RGB data into a paletted image */ \
  if ((img.depth == 24) && (img.type==BMP_COLOUR_BMP    )) ppl_bmp_colour_count(&x->c->errcontext,&img);  /* Check full color image to ensure more than 256 colors */ \
  if ((img.depth ==  8) && (img.type==BMP_COLOUR_PALETTE)) ppl_bmp_grey_check(&x->c->errcontext,&img);    /* Check paletted images for greyscale conversion */ \
  if ((img.type == BMP_COLOUR_PALETTE) && (img.pal_len <= 16) && (img.depth == 8)) ppl_bmp_compact(&x->c->errcontext,&img); /* Compact images with few palette entries */ \
 \
  /* Apply compression to image data */ \
  switch (img.TargetCompression) \
   { \
    case BMP_ENCODING_FLATE: \
     zlen   = img.data_len*1.01+12; /* Nasty guess at size of buffer needed. */ \
     imagez = (unsigned char *)ppl_memAlloc(zlen); \
     if (imagez == NULL) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1,"Out of memory (m)."); img.TargetCompression = BMP_ENCODING_NULL; break; } \
     if (DEBUG) { ppl_log(&x->c->errcontext,"Calling zlib to compress image data."); } \
     j = compress2(imagez,&zlen,img.data,img.data_len,9); /* Call zlib to do deflation */ \
 \
     if (j!=0) \
      { \
       if (DEBUG) { sprintf(x->c->errcontext.tempErrStr, "zlib returned error code %d\n",j); ppl_log(&x->c->errcontext,NULL); } \
       img.TargetCompression = BMP_ENCODING_NULL; /* Give up trying to compress data */ \
       break; \
      } \
     if (DEBUG) { sprintf(x->c->errcontext.tempErrStr, "zlib has completed compression. Before flate: %ld bytes. After flate: %ld bytes.", img.data_len, (long)zlen); ppl_log(&x->c->errcontext,NULL); } \
     if (zlen >= img.data_len) \
      { \
       if (DEBUG) { ppl_log(&x->c->errcontext,"Using original uncompressed data since zlib made it bigger than it was to start with."); } \
       img.TargetCompression = BMP_ENCODING_NULL; /* Give up trying to compress data; result was larger than original data size */ \
       break; \
      } \
     img.data = imagez; /* Replace old data with new compressed data */ \
     img.data_len = zlen; \
     break; \
   } \

#ifdef FLATE_DISABLE
if (img.TargetCompression==BMP_ENCODING_FLATE) img.TargetCompression=BMP_ENCODING_NULL;
#endif
  COMPRESS_POSTSCRIPT_IMAGE;

  // Write out postscript image
  fprintf(x->epsbuffer, "gsave\n");
  fprintf(x->epsbuffer, "[ %.2f %.2f %.2f %.2f %.2f %.2f ] concat\n", Lx*sin(ThetaX), Lx*cos(ThetaX), Ly*sin(ThetaY), Ly*cos(ThetaY), xo, yo);

#define WRITE_POSTSCRIPT_IMAGE \
  if      (img.colour == BMP_COLOUR_RGB ) fprintf(x->epsbuffer, "/DeviceRGB setcolorspace\n");  /* RGB palette */ \
  else if (img.colour == BMP_COLOUR_GREY) fprintf(x->epsbuffer, "/DeviceGray setcolorspace\n"); /* Greyscale palette */ \
  else if (img.colour == BMP_COLOUR_PALETTE) /* Indexed palette */ \
   { \
    fprintf(x->epsbuffer, "[/Indexed /DeviceRGB %d <~\n", img.pal_len-1); \
    ppl_bmp_A85(&x->c->errcontext, x->epsbuffer, img.palette, 3*img.pal_len); \
    fprintf(x->epsbuffer, "] setcolorspace\n\n"); \
   } \
 \
  fprintf(x->epsbuffer, "<<\n /ImageType %d\n /Width %d\n /Height %d\n /ImageMatrix [%d 0 0 %d 0 %d]\n", (img.trans==NULL)?1:4, img.width, img.height, img.width, -img.height, img.height); \
  fprintf(x->epsbuffer, " /DataSource currentfile /ASCII85Decode filter"); /* Image data is stored in currentfile, but need to apply filters to decode it */ \
  if (img.TargetCompression == BMP_ENCODING_FLATE) fprintf(x->epsbuffer, " /FlateDecode filter"); \
  fprintf(x->epsbuffer, "\n /BitsPerComponent %d\n /Decode [0 %d%s]\n", (img.colour==BMP_COLOUR_RGB)?(img.depth/3):(img.depth), \
                                                                        (img.type==BMP_COLOUR_PALETTE)?((1<<img.depth)-1):1, \
                                                                        (img.colour==BMP_COLOUR_RGB)?" 0 1 0 1":""); \
  if (img.trans != NULL) \
   { \
    fprintf(x->epsbuffer," /MaskColor ["); \
    if (img.colour == BMP_COLOUR_RGB) fprintf(x->epsbuffer, "%d %d %d]\n",(int)img.trans[0], (int)img.trans[1], (int)img.trans[2]); \
    else                             fprintf(x->epsbuffer, "%d]\n"      ,(int)img.trans[0]); \
   } \
  fprintf(x->epsbuffer, ">> image\n"); \
  ppl_bmp_A85(&x->c->errcontext, x->epsbuffer, img.data, img.data_len); \

  WRITE_POSTSCRIPT_IMAGE;
  fprintf(x->epsbuffer, "grestore\n");
  return 0;
 }

int  eps_plot_colormap_DrawScales(EPSComm *x, double origin_x, double origin_y, double width, double height, double zdepth)
 {
  int              i, j, k;
  long             p;
  double           xmin,xmax,ymin,ymax;
  char            *errtext;
  canvas_plotdesc *pd;
  pplset_graph    *sg= &x->current->settings;
  withWords        ww;

  errtext = ppl_memAlloc(LSTR_LENGTH);
  if (errtext==NULL) { ppl_error(&x->c->errcontext,ERR_MEMORY,-1,-1,"Out of memory (n)."); return 1; }

  // Set color for painting axes
  ppl_withWordsZero(x->c, &ww);
  if (x->current->settings.AxesColor > 0) { ww.color = x->current->settings.AxesColor; ww.USEcolor = 1; }
  else                                    { ww.Col1234Space = x->current->settings.AxesCol1234Space; ww.color1 = x->current->settings.AxesColor1; ww.color2 = x->current->settings.AxesColor2; ww.color3 = x->current->settings.AxesColor3; ww.color4 = x->current->settings.AxesColor4; ww.USEcolor1234 = 1; }
  ww.linewidth = EPS_AXES_LINEWIDTH; ww.USElinewidth = 1;
  ww.linetype  = 1;                  ww.USElinetype  = 1;
  eps_core_SetColor(x, &ww, 1);
  IF_NOT_INVISIBLE eps_core_SetLinewidth(x, EPS_AXES_LINEWIDTH * EPS_DEFAULT_LINEWIDTH, 1, 0.0);
  else return 0;

  // Find extrema of box filled by graph canvas
  if (!x->current->ThreeDim)
   {
    xmin = origin_x;
    xmax = origin_x + width;
    ymin = origin_y;
    ymax = origin_y + height;
   }
  else
   {
    xmin = xmax = origin_x;
    ymin = ymax = origin_y;
    for (k=0; k<8; k++)
     {
      double x0,y0,z0;
      eps_plot_ThreeDimProject((k&1),(k&2)!=0,(k&4)!=0,sg,origin_x,origin_y,width,height,zdepth,&x0,&y0,&z0);
      if (x0<xmin) xmin=x0; if (x0>xmax) xmax=x0;
      if (y0<ymin) ymin=y0; if (y0>ymax) ymax=y0;
     }
   }

  // Loop over all datasets
  pd = x->current->plotitems;
  k  = 0;
  while (pd != NULL) // loop over all datasets
   {
    dataTable *data = x->current->plotdata[k];
    if (pd->CRangeDisplay)
     {
      double              XSize = 1024, YSize = 1; // Dimensions of bitmap image
      unsigned char       component_r, component_g, component_b, transparent[3] = {TRANS_R, TRANS_G, TRANS_B};
      char                v[3]="c1";
      double              x1,y1,x2,y2  ,  x3,y3,x4,y4  ,  theta,dummy  ,  CMin,CMax;
      double              Lx, Ly, ThetaX, ThetaY;
      unsigned char       CLog;
      const double        MARGIN = EPS_COLORSCALE_MARGIN * M_TO_PS;
      const double        WIDTH  = EPS_COLORSCALE_WIDTH * M_TO_PS;
      const unsigned char Lr = (sg->ColKeyPos==SW_COLKEYPOS_B)||(sg->ColKeyPos==SW_COLKEYPOS_R);
      uLongf              zlen; // Length of buffer passed to zlib
      unsigned char      *imagez;
      bitmap_data         img;
      pplObj             *CVar=NULL, *C1Var, CDummy;

      if      (sg->ColKeyPos==SW_COLKEYPOS_T) { x1 = xmin; x2 = xmax; y1 = y2 = x->current->PlotTopMargin   +MARGIN; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_B) { x1 = xmin; x2 = xmax; y1 = y2 = x->current->PlotBottomMargin-MARGIN; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_L) { y1 = ymin; y2 = ymax; x1 = x2 = x->current->PlotLeftMargin  -MARGIN; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_R) { y1 = ymin; y2 = ymax; x1 = x2 = x->current->PlotRightMargin +MARGIN; }
      else                                    { continue; }

      if      (sg->ColKeyPos==SW_COLKEYPOS_T) { x3 = x1; x4 = x2; y3 = y4 = y1 + WIDTH; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_B) { x3 = x1; x4 = x2; y3 = y4 = y1 - WIDTH; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_L) { y3 = y1; y4 = y2; x3 = x4 = x1 - WIDTH; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_R) { y3 = y1; y4 = y2; x3 = x4 = x1 + WIDTH; }
      else                                    { continue; }

      if      (sg->ColKeyPos==SW_COLKEYPOS_T) { Lx = xmax-xmin; Ly = WIDTH; ThetaX =   M_PI/2; ThetaY =   M_PI  ; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_B) { Lx = xmax-xmin; Ly = WIDTH; ThetaX =   M_PI/2; ThetaY =   0     ; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_L) { Lx = ymax-ymin; Ly = WIDTH; ThetaX =   0     ; ThetaY =   M_PI/2; }
      else if (sg->ColKeyPos==SW_COLKEYPOS_R) { Lx = ymax-ymin; Ly = WIDTH; ThetaX =   0     ; ThetaY = 3*M_PI/2; }
      else                                    { continue; }

      // Paint bitmap image portion of colorscale
      img.type     = BMP_COLOUR_BMP;
      img.colour   = BMP_COLOUR_RGB;
      img.pal_len  = 0;
      img.palette  = NULL;
      img.trans    = transparent;
      img.height   = YSize;
      img.width    = XSize;
      img.depth    = 24;
      img.data_len = 3*XSize*YSize;
      img.data     = ppl_memAlloc(3*XSize*YSize);
      img.TargetCompression = BMP_ENCODING_FLATE;
      if (img.data==NULL) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1,"Out of memory (o)."); goto POST_BITMAP; }

      // Set CMin, CMax
      CMin = pd->CMinFinal;
      CMax = pd->CMaxFinal;
      CLog = (sg->Clog[0]==SW_BOOL_TRUE);

      // Get pointer to variable c1 in the user's variable space
      ppl_contextGetVarPointer(x->c, v, &CVar, &CDummy);
      if (sg->Crenorm[0]==SW_BOOL_FALSE) { *CVar = data->firstEntries[2]; CVar->flagComplex=0; CVar->imag=0.0; }
      else pplObjNum(CVar,0,0,0); // c1...c4 are dimensionless numbers in range 0-1, regardless of units of input data

      // Populate bitmap data array
      for (p=0, j=YSize-1; j>=0; j--) // Postscript images are top-first. Data block is bottom-first.
       for (i=0; i<XSize; i++)
        {
         // Set value of c1
         if (sg->Crenorm[0]==SW_BOOL_FALSE)
          {
           if (!CLog) CVar->real = CMin+(CMax-CMin)*((double)i)/(XSize-1);
           else       CVar->real = CMin*pow(CMax/CMin,((double)i)/(XSize-1));
          }
         else         CVar->real = ((double)i)/(XSize-1);

         C1Var = CVar;
         SET_RGB_COLOR;
        }
#ifdef FLATE_DISABLE
if (img.TargetCompression==BMP_ENCODING_FLATE) img.TargetCompression=BMP_ENCODING_NULL;
#endif
       COMPRESS_POSTSCRIPT_IMAGE;

       // Write postscript image
       fprintf(x->epsbuffer, "gsave\n");
       fprintf(x->epsbuffer, "[ %.2f %.2f %.2f %.2f %.2f %.2f ] concat\n", Lx*sin(ThetaX), Lx*cos(ThetaX), Ly*sin(ThetaY), Ly*cos(ThetaY), x3, y3);
       WRITE_POSTSCRIPT_IMAGE;
       fprintf(x->epsbuffer, "grestore\n");

POST_BITMAP:
      // Restore variable c1 in the user's variable space
      if (CVar!=NULL)
       {
        char v[3]="c1";
        ppl_contextRestoreVarPointer(x->c, v, &CDummy);
       }

      // Paint inner-facing scale
      theta = ((sg->ColKeyPos==SW_COLKEYPOS_T)||(sg->ColKeyPos==SW_COLKEYPOS_B))?M_PI:(M_PI/2);
      if (Lr) theta=theta+M_PI;
      eps_plot_axispaint(x, &ww, &pd->C1Axis, 0, GSL_NAN, Lr, x1, y1, NULL, x2, y2, NULL, theta, theta, &dummy, 0);

      // Paint lines at top/bottom of scale
      eps_core_SetLinewidth(x, EPS_AXES_LINEWIDTH * EPS_DEFAULT_LINEWIDTH, 1, 0.0);
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n", x1, y1, x3, y3);
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n", x2, y2, x4, y4);

      // Paint outer-facing scale
      eps_plot_axispaint(x, &ww, &pd->C1Axis, 0, GSL_NAN, !Lr, x3, y3, NULL, x4, y4, NULL, theta+M_PI, theta+M_PI, &dummy, 1);
     }
    pd=pd->next; k++;
   }
  return 0;
 }

