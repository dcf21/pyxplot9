// eps_plot_contourmap.c
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

#define _PPL_EPS_PLOT_CONTOURMAP_C 1

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <zlib.h>

#include <gsl/gsl_math.h>

#include "coreUtils/memAlloc.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "mathsTools/dcfmath.h"
#include "epsMaker/canvasDraw.h"
#include "canvasItems.h"
#include "coreUtils/errorReport.h"
#include "settings/settings.h"
#include "settings/withWords_fns.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/garbageCollector.h"
#include "userspace/unitsDisp.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/pplObj_fns.h"
#include "userspace/contextVarDef.h"

#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "settings/epsColors.h"
#include "epsMaker/eps_plot.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_contourmap.h"
#include "epsMaker/eps_plot_legend.h"
#include "epsMaker/eps_settings.h"
#include "epsMaker/eps_style.h"

// Codes for the four sides of a cell
#define FACE_ALL -1
#define FACE_T    1
#define FACE_R    2
#define FACE_B    3
#define FACE_L    4

#define MAX_CONTOUR_PATHS 1024

typedef struct ContourDesc
 {
  long           Nvertices_min, Nvertices_max;
  long           i, segment_flatest;
  unsigned char  closepath;
  double        *posdata;
  double         area;
  double         vreal;
 } ContourDesc;

static int ContourCmp(const void *xv, const void *yv)
 {
  const ContourDesc *x = (const ContourDesc *)xv;
  const ContourDesc *y = (const ContourDesc *)yv;

  if      (x->area < y->area) return  1.0;
  else if (x->area > y->area) return -1.0;
  else                        return  0.0;
 }

// Yield up text items which label contours on a contourmap
void eps_plot_contourmap_YieldText(EPSComm *x, dataTable *data, pplset_graph *sg, canvas_plotdesc *pd)
 {
  dataBlock     *blk;
  int            XSize = (x->current->settings.SamplesXAuto==SW_BOOL_TRUE) ? x->current->settings.samples : x->current->settings.SamplesX;
  int            YSize = (x->current->settings.SamplesYAuto==SW_BOOL_TRUE) ? x->current->settings.samples : x->current->settings.SamplesY;
  int            i, j, k, Ncol;
  double         CMin, CMax, min_prelim, max_prelim, OoM, UnitMultiplier;
  unsigned char  CMinAuto, CMinSet, CMaxAuto, CMaxSet, CLog;
  char          *UnitString;

  // Check that we have some data
  if ((data==NULL) || (data->Nrows<1)) return; // No data present
  Ncol = data->Ncolumns_real;
  blk  = data->first;

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

  // If there's no spread of data, make a spread up
  if ( (fabs(CMax) <= fabs(CMin)) || (fabs(CMin-CMax) <= fabs(1e-14*CMax)) )
   {
    if (!CLog)
     {
      double step = ppl_max(1.0,1e-3*fabs(CMin));
      CMin -= step; CMax += step;
     }
    else
     {
      if (CMin > 1e-300) CMin /= 10.0;
      if (CMax <  1e300) CMax *= 10.0;
     }
   }

  // Work out units in which contours will be labelled
  pd->CRangeUnit             = data->FirstEntries[2];
  pd->CRangeUnit.flagComplex = 0;
  pd->CRangeUnit.imag        = 0.0;
  pd->CRangeUnit.real        = CLog ?  (CMin * sqrt(CMax/CMin))
                                    : ((CMin + CMax)/2);
  UnitString = ppl_printUnit(x->c,&pd->CRangeUnit,&UnitMultiplier,NULL,0,0,SW_DISPLAY_L);
  UnitMultiplier /= pd->CRangeUnit.real;
  if (!gsl_finite(UnitMultiplier)) UnitMultiplier=1.0;

  // Round range outwards to round endpoints
  min_prelim = CMin * UnitMultiplier;
  max_prelim = CMax * UnitMultiplier;

  if (CLog) { min_prelim = log10(min_prelim); max_prelim = log10(max_prelim); }

  OoM = pow(10.0, floor(log10(fabs(max_prelim - min_prelim)/5)));
  min_prelim = floor(min_prelim / OoM) * OoM;
  max_prelim = ceil (max_prelim / OoM) * OoM;

  if (CLog) { min_prelim = pow(10.0,min_prelim); max_prelim = pow(10.0,max_prelim); }

  min_prelim /= UnitMultiplier;
  max_prelim /= UnitMultiplier;

  if (gsl_finite(min_prelim) && (CLog||(min_prelim>1e-300))) CMin = min_prelim;
  if (gsl_finite(max_prelim) && (CLog||(min_prelim>1e-300))) CMax = max_prelim;

  // Loop over all contours submitting labels
  for (k=0;
       (k<MAX_CONTOURS) && (  ((sg->ContoursListLen< 0) && (k<sg->ContoursN))  ||
                              ((sg->ContoursListLen>=0) && (k<sg->ContoursListLen))  );
       k++)
   {
    char *text;
    CanvasTextItem *i;
    pplObj          v = pd->CRangeUnit;

    if (sg->ContoursListLen< 0) v.real = CLog?(CMin*pow(CMax/CMin,(k+0.5*CMinAuto)/(sg->ContoursN-0.5*((!CMinAuto)+(!CMaxAuto)))))
                                             :(CMin+(CMax-CMin)*(k+0.5*CMinAuto)/(sg->ContoursN-0.5*((!CMinAuto)+(!CMaxAuto))));
    else                        v.real = sg->ContoursList[k];

    sprintf(UnitString,"%s",ppl_unitsNumericDisplay(x->c,&v,0,SW_DISPLAY_L,0));
    text = ppl_memAlloc(strlen(UnitString)+1);
    if (text!=NULL) { strcpy(text, UnitString); YIELD_TEXTITEM(text); }
   }

  // Store range of values for which contours will be drawn
  pd->CMinFinal     = CMin;
  pd->CMaxFinal     = CMax;
  return;
 }

// Routines for tracking the paths of contours

// -----+-----+-----+-----+
//      |     |     |     |  blk->split[X] contains flags indicating whether crossing points on lines
//      |     |     |     |  A (in bit 1) and B (in bit 2) have been used.
//      |     |--A--|     |  on successive passes, bits (3,4), (5,6) are used, etc.
// -----+-----X-----+-----+
//      |    ||     |     |
//      |    B|     |     |
//      |    ||     |     |
// -----+-----+-----+-----+
//      |     |     |     |
//      |     |     |     |
//      |     |     |     |
// -----+-----+-----+-----+


static int IsBetween(double x, double a, double b, double *f)
 {
  *f=0.5;
  if  (a==b)                  {                 return x==a; }
  if ((a< b)&&(x>=a)&&(x<=b)) { *f=(x-a)/(b-a); return 1;    }
  if ((a> b)&&(x<=a)&&(x>=b)) { *f=(x-a)/(b-a); return 1;    }
  return 0;
 }

// See whether cell X contains any unused starting points for contours
static int GetStartPoint(double c1, dataTable *data, unsigned char **flags, int XSize, int YSize, int x0, int y0, int EntryFace, int *ExitFace, int *xcell, int *ycell, double *Xout, double *Yout)
 {
  dataBlock *blk  = data->first;
  int        Ncol = data->Ncolumns_real;
  const int  pass = 0;
  double     f;

  *xcell = x0;
  *ycell = y0;

  if (((EntryFace<0)||(EntryFace==FACE_T)) && (x0<=XSize-2) && (x0>= 0) && (y0>= 0) && (y0<=YSize-1) &&
       (((blk->split[(x0  )+(y0  )*XSize]>>(0+2*pass))&1)==0) &&
       IsBetween(c1, blk->data_real[2+Ncol*((x0  )+(y0  )*XSize)], blk->data_real[2+Ncol*((x0+1)+(y0  )*XSize)],&f))
   { *flags = &blk->split[(x0  )+(y0  )*XSize]; *ExitFace=FACE_T; *Xout=x0+f; *Yout=y0; return 1; }

  if (((EntryFace<0)||(EntryFace==FACE_R)) && (x0<=XSize-2) && (x0>=-1) && (y0>= 0) && (y0<=YSize-2) &&
       (((blk->split[(x0+1)+(y0  )*XSize]>>(1+2*pass))&1)==0) &&
       IsBetween(c1, blk->data_real[2+Ncol*((x0+1)+(y0  )*XSize)], blk->data_real[2+Ncol*((x0+1)+(y0+1)*XSize)],&f))
   { *flags = &blk->split[(x0+1)+(y0  )*XSize]; *ExitFace=FACE_R; *Xout=x0+1; *Yout=y0+f; return 1; }

  if (((EntryFace<0)||(EntryFace==FACE_B)) && (x0<=XSize-2) && (x0>= 0) && (y0>=-1) && (y0<=YSize-2) &&
       (((blk->split[(x0  )+(y0+1)*XSize]>>(0+2*pass))&1)==0) &&
       IsBetween(c1, blk->data_real[2+Ncol*((x0  )+(y0+1)*XSize)], blk->data_real[2+Ncol*((x0+1)+(y0+1)*XSize)],&f))
   { *flags = &blk->split[(x0  )+(y0+1)*XSize]; *ExitFace=FACE_B; *Xout=x0+f; *Yout=y0+1; return 1; }

  if (((EntryFace<0)||(EntryFace==FACE_L)) && (x0<=XSize-1) && (x0>= 0) && (y0>= 0) && (y0<=YSize-2) &&
       (((blk->split[(x0  )+(y0  )*XSize]>>(1+2*pass))&1)==0) &&
       IsBetween(c1, blk->data_real[2+Ncol*((x0  )+(y0  )*XSize)], blk->data_real[2+Ncol*((x0  )+(y0+1)*XSize)],&f))
   { *flags = &blk->split[(x0  )+(y0  )*XSize]; *ExitFace=FACE_L; *Xout=x0; *Yout=y0+f; return 1; }
  return 0;
 }

// Given a contour already tracking through cell X from a given face, where to go next?
static int GetNextPoint(double c1, dataTable *data, int pass, int XSize, int YSize, int x0, int y0, int EntryFace, int *ExitFace, int *xcell, int *ycell, double *Xout, double *Yout)
 {
  dataBlock *blk  = data->first;
  int        Ncol = data->Ncolumns_real;
  int        j;
  double     f;

  for (j=0; j<2; j++)
   {
    if (((j==1       )||(EntryFace==FACE_B)) && (x0<=XSize-2) && (x0>= 0) && (y0>= 0) && (y0<=YSize-1) &&
         (((blk->split[(x0  )+(y0  )*XSize]>>(0+2*pass))&1)==0) &&
         IsBetween(c1, blk->data_real[2+Ncol*((x0  )+(y0  )*XSize)], blk->data_real[2+Ncol*((x0+1)+(y0  )*XSize)],&f))
     { blk->split[(x0  )+(y0  )*XSize] |= 1<<(0+2*pass); *ExitFace=FACE_T; *xcell=x0; *ycell=y0-1; *Xout=x0+f; *Yout=y0; return 1; }

    if (((j==1       )||(EntryFace==FACE_L)) && (x0<=XSize-2) && (x0>=-1) && (y0>= 0) && (y0<=YSize-2) &&
         (((blk->split[(x0+1)+(y0  )*XSize]>>(1+2*pass))&1)==0) &&
         IsBetween(c1, blk->data_real[2+Ncol*((x0+1)+(y0  )*XSize)], blk->data_real[2+Ncol*((x0+1)+(y0+1)*XSize)],&f))
     { blk->split[(x0+1)+(y0  )*XSize] |= 1<<(1+2*pass); *ExitFace=FACE_R; *xcell=x0+1; *ycell=y0; *Xout=x0+1; *Yout=y0+f; return 1; }

    if (((j==1       )||(EntryFace==FACE_T)) && (x0<=XSize-2) && (x0>= 0) && (y0>=-1) && (y0<=YSize-2) &&
         (((blk->split[(x0  )+(y0+1)*XSize]>>(0+2*pass))&1)==0) &&
         IsBetween(c1, blk->data_real[2+Ncol*((x0  )+(y0+1)*XSize)], blk->data_real[2+Ncol*((x0+1)+(y0+1)*XSize)],&f))
     { blk->split[(x0  )+(y0+1)*XSize] |= 1<<(0+2*pass); *ExitFace=FACE_B; *xcell=x0; *ycell=y0+1; *Xout=x0+f; *Yout=y0+1; return 1; }

    if (((j==1       )||(EntryFace==FACE_R)) && (x0<=XSize-1) && (x0>= 0) && (y0>= 0) && (y0<=YSize-2) &&
         (((blk->split[(x0  )+(y0  )*XSize]>>(1+2*pass))&1)==0) &&
         IsBetween(c1, blk->data_real[2+Ncol*((x0  )+(y0  )*XSize)], blk->data_real[2+Ncol*((x0  )+(y0+1)*XSize)],&f))
     { blk->split[(x0  )+(y0  )*XSize] |= 1<<(1+2*pass); *ExitFace=FACE_L; *xcell=x0-1; *ycell=y0; *Xout=x0; *Yout=y0+f; return 1; }
   }
  return 0;
 }

static void FollowContour(EPSComm *x, dataTable *data, ContourDesc *cd, pplObj *v, unsigned char *flags, int XSize, int YSize, int xcell, int ycell, int face, double xpos, double ypos, double Lx, double ThetaX, double Ly, double ThetaY)
 {
  long   i, j, i_flatest=0;
  int    xcelli=xcell, ycelli=ycell;
  double xposi =xpos , yposi =ypos , grad_flatest = 1e100;
  int    facei =face;
  double xold=xpos, yold=ypos;

  // Starting point has been used on pass zero
  *flags |= 1<<(((face==FACE_L)||(face==FACE_R)) + 0);

  // Trace path, looking for flattest segment and counting length
  for (i=1; (GetNextPoint(v->real, data, 0, XSize, YSize, xcell, ycell, face, &face, &xcell, &ycell, &xpos, &ypos)!=0); i++)
   {
    double grad;
    if (xpos!=xold)
     {
      double x0, y0, x1, y1;
      x0 = Lx*xold/(XSize-1)*sin(ThetaX) + Ly*yold/(YSize-1)*sin(ThetaY);
      y0 = Lx*xold/(XSize-1)*cos(ThetaX) + Ly*yold/(YSize-1)*cos(ThetaY);
      x1 = Lx*xpos/(XSize-1)*sin(ThetaX) + Ly*ypos/(YSize-1)*sin(ThetaY);
      y1 = Lx*xpos/(XSize-1)*cos(ThetaX) + Ly*ypos/(YSize-1)*cos(ThetaY);
      grad = fabs((y1-y0)/(x1-x0));
      if (grad<grad_flatest) { grad_flatest=grad; i_flatest=i; }
     }
    xold=xpos; yold=ypos;
   }

  // Fill out information in contour descriptor
  cd->Nvertices_min = cd->Nvertices_max = i;
  cd->segment_flatest = i_flatest;
  cd->posdata = (double *)ppl_memAlloc((i+8) * 2 * sizeof(double));
  if (cd->posdata==NULL) return;

  // Begin pass one
  xcell=xcelli; ycell=ycelli; xpos=xposi; ypos=yposi; face=facei;

  // Starting point has been used on pass one
  *flags |= 1<<(((face==FACE_L)||(face==FACE_R)) + 2);

  // Trace path, looking for flattest segment and counting length
  cd->posdata[2*0  ] = xpos;
  cd->posdata[2*0+1] = ypos;
  for (j=1; (GetNextPoint(v->real, data, 1, XSize, YSize, xcell, ycell, face, &face, &xcell, &ycell, &xpos, &ypos)!=0); j++)
   {
    cd->posdata[2*j  ] = xpos;
    cd->posdata[2*j+1] = ypos;
   }
  return;
 }

// Render a contourmap to postscript
int  eps_plot_contourmap(EPSComm *x, dataTable *data, unsigned char ThreeDim, int xn, int yn, int zn, pplset_graph *sg, canvas_plotdesc *pd, int pdn, double origin_x, double origin_y, double width, double height, double zdepth)
 {
  double         scale_x, scale_y, scale_z;
  dataBlock     *blk;
  int            XSize = (x->current->settings.SamplesXAuto==SW_BOOL_TRUE) ? x->current->settings.samples : x->current->settings.SamplesX;
  int            YSize = (x->current->settings.SamplesYAuto==SW_BOOL_TRUE) ? x->current->settings.samples : x->current->settings.SamplesY;
  int            i, j, k, cn, pass, face, xcell, ycell;
  double         xo, yo, Lx, Ly, ThetaX, ThetaY, CMin, CMax, xpos, ypos;
  double         col=GSL_NAN,col1=-1,col2=-1,col3=-1,col4=-1,fc=GSL_NAN,fc1=-1,fc2=-1,fc3=-1,fc4=-1;
  unsigned char  CLog, CMinAuto, CMaxAuto, CRenorm, *flags;
  char          *errtext, c1name[]="c1";
  pplObj        *CVar=NULL, CDummy;
  ContourDesc   *clist;
  int            cpos=0;
  // int         Ncol_real, Ncol_obj;

  if ((data==NULL) || (data->Nrows<1)) return 0; // No data present
  // Ncol_real = data->Ncolumns_real;
  // Ncol_obj  = data->Ncolumns_obj;
  // if (eps_plot_WithWordsCheckUsingItemsDimLess(&pd->ww_final, data->FirstEntries, Ncol_real, NULL, Ncol_obj)) return 1;
  if (!ThreeDim) { scale_x=width; scale_y=height; scale_z=1.0;    }
  else           { scale_x=width; scale_y=height; scale_z=zdepth; }
  blk = data->first;

  clist   = (ContourDesc *)ppl_memAlloc(MAX_CONTOUR_PATHS * sizeof(ContourDesc));
  errtext = ppl_memAlloc(LSTR_LENGTH);
  if ((clist==NULL)||(errtext==NULL)) { ppl_error(&x->c->errcontext,ERR_MEMORY,-1,-1,"Out of memory."); return 1; }

  // Work out orientation of contourmap
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

  // Look up normalisation of variable c1
  CLog     = (sg->Clog[0]==SW_BOOL_TRUE);
  CMin     = pd->CMinFinal;
  CMax     = pd->CMaxFinal;
  CMinAuto = (sg->Cminauto[0]==SW_BOOL_TRUE);
  CMaxAuto = (sg->Cmaxauto[0]==SW_BOOL_TRUE);
  CRenorm  = (sg->Crenorm [0]==SW_BOOL_TRUE);

  // If no data present, stop now
  if ((CMin==0)&&(CMax==0))
   {
    sprintf(x->c->errcontext.tempErrStr, "No data supplied to determine range for variable c1");
    return 0;
   }

  // Output result to debugging output
  if (DEBUG)
   {
    int SF = x->c->set->term_current.SignificantFigures;
    sprintf(x->c->errcontext.tempErrStr, "Range for variable c1 is [%s:%s]", ppl_numericDisplay(CMin,x->c->numdispBuff[0],SF,0), ppl_numericDisplay(CMax,x->c->numdispBuff[1],SF,0));
    ppl_log(&x->c->errcontext,NULL);
   }

  // Check that variable c1 has appropriate units
  if ( ((!CMinAuto)||(!CMaxAuto)) && (!ppl_unitsDimEqual(&data->FirstEntries[2] , (sg->Cminauto[0]==SW_BOOL_TRUE)?(&sg->Cmax[0]):(&sg->Cmin[0]))) )
   {
    sprintf(x->c->errcontext.tempErrStr, "Column 3 of data supplied to the colormap plot style has conflicting units with those set in the 'set crange' command. The former has units of <%s> whilst the latter has units of <%s>.", ppl_printUnit(x->c,&data->FirstEntries[2], NULL, NULL, 0, 1, 0), ppl_printUnit(x->c,(sg->Cminauto[0]==SW_BOOL_TRUE)?(&sg->Cmax[0]):(&sg->Cmin[0]), NULL, NULL, 1, 1, 0));
    ppl_error(&x->c->errcontext,ERR_NUMERIC,-1,-1,NULL);
    return 1;
   }

  // Get pointer to variable c1 in the user's variable space
  ppl_contextGetVarPointer(x->c, c1name, &CVar, &CDummy);
  if (!CRenorm) { *CVar = pd->CRangeUnit; CVar->flagComplex=0; CVar->imag=0.0; }
  else pplObjNum(CVar,0,0,0); // c1 is a dimensionless number in range 0-1, regardless of units of input data

  // Loop over contours
  for (k=0;
       (k<MAX_CONTOURS) && (  ((sg->ContoursListLen< 0) && (k<sg->ContoursN))  ||
                              ((sg->ContoursListLen>=0) && (k<sg->ContoursListLen))  );
       k++)
   {
    pplObj v = pd->CRangeUnit;

    if (sg->ContoursListLen< 0) v.real = CLog?(CMin*pow(CMax/CMin,(k+0.5*CMinAuto)/(sg->ContoursN-0.5*((!CMinAuto)+(!CMaxAuto)))))
                                             :(CMin+(CMax-CMin)*(k+0.5*CMinAuto)/(sg->ContoursN-0.5*((!CMinAuto)+(!CMaxAuto))));
    else                        v.real = sg->ContoursList[k];

    // Write debugging output
    if (DEBUG)
     {
      sprintf(x->c->errcontext.tempErrStr, "Beginning to trace path of contour at c1=%g.", v.real);
      ppl_log(&x->c->errcontext,NULL);
     }

    // Reset contour map usage flags
    for (j=0; j<YSize; j++)
     for (i=0; i<XSize; i++)
      blk->split[i+XSize*j] = 0;

    // Set value of c1
    if (CRenorm)
     {
      if (sg->ContoursListLen< 0)  CVar->real = ((double)(k+1)/(sg->ContoursN+1));
      else                         CVar->real = CLog?(log(v.real/CMin) / log(CMax/CMin))
                                                    :((v.real-CMin)/(CMax-CMin));
      if (CVar->real<0.0)          CVar->real=0.0;
      if (CVar->real>1.0)          CVar->real=1.0;
      if (!gsl_finite(CVar->real)) CVar->real=0.5;
     }
    else                           CVar->real = v.real;

    // Scan edges of plot looking for contour start points
    for (i=0; i<XSize-1; i++) // Top face
     if (GetStartPoint(v.real, data, &flags, XSize, YSize, i        , 0        , FACE_T,&face,&xcell,&ycell,&xpos,&ypos))
      {
       clist[cpos].i=k; clist[cpos].vreal=CVar->real;
       clist[cpos].closepath = 0;
       FollowContour(x, data, &clist[cpos], &v, flags, XSize, YSize, xcell, ycell, face, xpos, ypos, Lx, ThetaX, Ly, ThetaY);
       if (++cpos>=MAX_CONTOUR_PATHS) goto GOT_CONTOURS;
      }
    for (i=0; i<YSize-1; i++) // Right face
     if (GetStartPoint(v.real, data, &flags, XSize, YSize, XSize-2  , i        , FACE_R,&face,&xcell,&ycell,&xpos,&ypos))
      {
       clist[cpos].i=k; clist[cpos].vreal=CVar->real;
       clist[cpos].closepath = 0;
       FollowContour(x, data, &clist[cpos], &v, flags, XSize, YSize, xcell, ycell, face, xpos, ypos, Lx, ThetaX, Ly, ThetaY);
       if (++cpos>=MAX_CONTOUR_PATHS) goto GOT_CONTOURS;
      }
    for (i=0; i<XSize-1; i++) // Bottom face
     if (GetStartPoint(v.real, data, &flags, XSize, YSize, XSize-2-i, YSize-2  , FACE_B,&face,&xcell,&ycell,&xpos,&ypos))
      {
       clist[cpos].i=k; clist[cpos].vreal=CVar->real;
       clist[cpos].closepath = 0;
       FollowContour(x, data, &clist[cpos], &v, flags, XSize, YSize, xcell, ycell, face, xpos, ypos, Lx, ThetaX, Ly, ThetaY);
       if (++cpos>=MAX_CONTOUR_PATHS) goto GOT_CONTOURS;
      }
    for (i=0; i<YSize-1; i++) // Left face
     if (GetStartPoint(v.real, data, &flags, XSize, YSize, 0        , YSize-2-i, FACE_L,&face,&xcell,&ycell,&xpos,&ypos))
      {
       clist[cpos].i=k; clist[cpos].vreal=CVar->real;
       clist[cpos].closepath = 0;
       FollowContour(x, data, &clist[cpos], &v, flags, XSize, YSize, xcell, ycell, face, xpos, ypos, Lx, ThetaX, Ly, ThetaY);
       if (++cpos>=MAX_CONTOUR_PATHS) goto GOT_CONTOURS;
      }

    // Scan body of plot looking for undrawn contours
    for (j=0; j<YSize-1; j++)
     for (i=0; i<XSize-1; i++)
      if (GetStartPoint(v.real, data, &flags, XSize, YSize, i, j, FACE_ALL,&face,&xcell,&ycell,&xpos,&ypos))
       {
        clist[cpos].i=k; clist[cpos].vreal=CVar->real;
        clist[cpos].closepath = 1;
        FollowContour(x, data, &clist[cpos], &v, flags, XSize, YSize, xcell, ycell, face, xpos, ypos, Lx, ThetaX, Ly, ThetaY);
        if (++cpos>=MAX_CONTOUR_PATHS) goto GOT_CONTOURS;
       }
   } // Finish looping over contours we are to trace

GOT_CONTOURS:

#define XPOS_TO_POSTSCRIPT \
 { \
  xps = xo + Lx*xpos/(XSize-1)*sin(ThetaX) + Ly*ypos/(YSize-1)*sin(ThetaY); \
  yps = yo + Lx*xpos/(XSize-1)*cos(ThetaX) + Ly*ypos/(YSize-1)*cos(ThetaY); \
 }

  // Add corners of plot to paths of non-closed contours
  for (cn=0; cn<cpos; cn++)
  if (!clist[cn].closepath)
   {
    int    faceA, faceB;
    int    n  = clist[cn].Nvertices_min;
    if (clist[cn].posdata==NULL) continue;

    if      (clist[cn].posdata[          0] <        0.0001 ) faceA = FACE_L;
    else if (clist[cn].posdata[          0] > (XSize-0.0001)) faceA = FACE_R;
    else if (clist[cn].posdata[          1] <        0.0001 ) faceA = FACE_T;
    else                                                      faceA = FACE_B;

    if      (clist[cn].posdata[2*(n-1) + 0] <        0.0001 ) faceB = FACE_L;
    else if (clist[cn].posdata[2*(n-1) + 0] > (XSize-0.0001)) faceB = FACE_R;
    else if (clist[cn].posdata[2*(n-1) + 1] <        0.0001 ) faceB = FACE_T;
    else                                                      faceB = FACE_B;

    if      ( ((faceA==FACE_L)&&(faceB==FACE_T)) || ((faceA==FACE_T)&&(faceB==FACE_L)) )
     {
      clist[cn].posdata[2*n+0] = 0;
      clist[cn].posdata[2*n+1] = 0;
      clist[cn].Nvertices_max += 1;
     }
    else if ( ((faceA==FACE_R)&&(faceB==FACE_T)) || ((faceA==FACE_T)&&(faceB==FACE_R)) )
     {
      clist[cn].posdata[2*n+0] = XSize;
      clist[cn].posdata[2*n+1] = 0;
      clist[cn].Nvertices_max += 1;
     }
    else if ( ((faceA==FACE_L)&&(faceB==FACE_B)) || ((faceA==FACE_B)&&(faceB==FACE_L)) )
     {
      clist[cn].posdata[2*n+0] = 0;
      clist[cn].posdata[2*n+1] = YSize;
      clist[cn].Nvertices_max += 1;
     }
    else if ( ((faceA==FACE_R)&&(faceB==FACE_B)) || ((faceA==FACE_B)&&(faceB==FACE_R)) )
     {
      clist[cn].posdata[2*n+0] = XSize;
      clist[cn].posdata[2*n+1] = YSize;
      clist[cn].Nvertices_max += 1;
     }
    else if ((faceA==FACE_L)&&(faceB==FACE_R))
     {
      clist[cn].posdata[2*n+0] = XSize;
      clist[cn].posdata[2*n+1] = 0;
      clist[cn].posdata[2*n+2] = 0;
      clist[cn].posdata[2*n+3] = 0;
      clist[cn].Nvertices_max += 2;
     }
    else if ((faceA==FACE_R)&&(faceB==FACE_L))
     {
      clist[cn].posdata[2*n+0] = 0;
      clist[cn].posdata[2*n+1] = 0;
      clist[cn].posdata[2*n+2] = XSize;
      clist[cn].posdata[2*n+3] = 0;
      clist[cn].Nvertices_max += 2;
     }
    else if ((faceA==FACE_T)&&(faceB==FACE_B))
     {
      clist[cn].posdata[2*n+0] = 0;
      clist[cn].posdata[2*n+1] = YSize;
      clist[cn].posdata[2*n+2] = 0;
      clist[cn].posdata[2*n+3] = 0;
      clist[cn].Nvertices_max += 2;
     }
    else if ((faceA==FACE_B)&&(faceB==FACE_T))
     {
      clist[cn].posdata[2*n+0] = 0;
      clist[cn].posdata[2*n+1] = 0;
      clist[cn].posdata[2*n+2] = 0;
      clist[cn].posdata[2*n+3] = YSize;
      clist[cn].Nvertices_max += 2;
     }
   }

  // Calculate area of each contour
  for (cn=0; cn<cpos; cn++)
   {
    int i;
    double area=0.0;
    if (clist[cn].posdata!=NULL)
     {
      for (i=0; i<clist[cn].Nvertices_max-1; i++)
       {
        area +=   clist[cn].posdata[2*(i  )+0] * clist[cn].posdata[2*(i+1)+1]
                - clist[cn].posdata[2*(i+1)+0] * clist[cn].posdata[2*(i  )+1];
       }
      area +=   clist[cn].posdata[2*(i  )+0] * clist[cn].posdata[2*(0  )+1]
              - clist[cn].posdata[2*(0  )+0] * clist[cn].posdata[2*(i  )+1]; // Close sum around a closed path
     }
    clist[cn].area=fabs(area/2);
   }

  // Sort contours into order of descending enclosed area
  qsort((void *)clist, cpos, sizeof(ContourDesc), ContourCmp);

  // Now loop over the contours that we have traced, drawing them
  for (pass=1; pass<=4; pass++) // Fill contour, Stroke contour, Label contour
   {
    if ((pass==2)&&(sg->ContoursLabel!=SW_ONOFF_ON)) continue;
    if ((pass==2)&&(sg->ContoursLabel==SW_ONOFF_ON)) fprintf(x->epsbuffer, "gsave\n");
    if ((pass==4)&&(sg->ContoursLabel==SW_ONOFF_ON)) fprintf(x->epsbuffer, "grestore\n");

    for (cn=0; cn<cpos; cn++)
     {
      const int stkLevelOld = x->c->stackPtr;

      if (clist[cn].Nvertices_max<1) continue;

      // Set value of c1
      if (clist[cn].posdata==NULL) continue;
      CVar->real = clist[cn].vreal;

      if ((pass==1)||(pass==3)) // Set before filling and stroking
       {
        // Evaluate any expressions in style information for next contour
        for (i=0 ; ; i++)
         {
          int            lOP;
          pplExpr       *expr [] = { pd->ww_final.EXPlinetype ,  pd->ww_final.EXPlinewidth ,  pd->ww_final.EXPpointlinewidth ,  pd->ww_final.EXPpointsize ,  pd->ww_final.EXPpointtype ,  pd->ww_final.EXPcolor     ,  pd->ww_final.EXPfillcolor     , NULL};
          double        *outD [] = { NULL                     , &pd->ww_final.linewidth    , &pd->ww_final.pointlinewidth    , &pd->ww_final.pointsize    ,  NULL                      , &col                       , &fc                            , NULL};
          int           *outI [] = {&pd->ww_final.linetype    ,  NULL                      ,  NULL                           ,  NULL                      , &pd->ww_final.pointtype    ,  NULL                      ,  NULL                          , NULL};
          unsigned char *flagU[] = {&pd->ww_final.USElinetype , &pd->ww_final.USElinewidth , &pd->ww_final.USEpointlinewidth , &pd->ww_final.USEpointsize , &pd->ww_final.USEpointtype ,  NULL                      ,  NULL                          , NULL};
          int           *flagA[] = {&pd->ww_final.AUTOlinetype,  NULL                      ,  NULL                           ,  NULL                      , &pd->ww_final.AUTOpointtype,  NULL                      ,  NULL                          , NULL};
          unsigned char  clip [] = {0,0,0,0,0,1,1,1,1,1,1,1,1,2};
          pplObj *outval; double dbl; int errpos=-1;

          if (clip[i]>1) break;
          if (expr[i]==NULL) continue;

          outval = ppl_expEval(x->c, expr[i], &lOP, 1, x->iterDepth+1);

          if (errpos>=0) { sprintf(x->c->errcontext.tempErrStr, "Could not evaluate the style expression <%s>. The error, encountered at character position %d, was: '%s'", expr[i]->ascii, errpos, errtext); ppl_error(&x->c->errcontext,ERR_NUMERIC,-1,-1,NULL); continue; }
          if (!outval->dimensionless) { sprintf(x->c->errcontext.tempErrStr, "The style expression <%s> yielded a result which was not a dimensionless number.", expr[i]->ascii); ppl_error(&x->c->errcontext,ERR_NUMERIC,-1,-1,NULL); continue; }
          if (outval->flagComplex) { sprintf(x->c->errcontext.tempErrStr, "The style expression <%s> yielded a result which was a complex number.", expr[i]->ascii); ppl_error(&x->c->errcontext,ERR_NUMERIC,-1,-1,NULL); continue; }
          if (!gsl_finite(outval->real)) { sprintf(x->c->errcontext.tempErrStr, "The style expression <%s> yielded a result which was not a finite number.", expr[i]->ascii); ppl_error(&x->c->errcontext,ERR_NUMERIC,-1,-1,NULL); continue; }
          dbl = outval->real;

          if (clip[i]) { if (dbl<0.0) dbl=0.0; if (dbl>1.0) dbl=1.0; }
          if (outD[i]!=NULL) *outD[i] = dbl;
          if (outI[i]!=NULL) { if (dbl<INT_MIN) dbl=INT_MIN+1; if (dbl>INT_MAX) dbl=INT_MAX-1; *outI[i] = (int)dbl; }
          if (flagU[i]!=NULL) *flagU[i] = 1;
          if (flagA[i]!=NULL) *flagA[i] = 0;
          EPS_STACK_POP;
         }
        EPS_STACK_POP;

        if ((col1>=0.0)&&(col2>=0.0)&&(col3>=0.0)&&((pd->ww_final.    Col1234Space!=SW_COLSPACE_CMYK)||(col4>=0.0))) { pd->ww_final.color1=col1; pd->ww_final.color2=col2; pd->ww_final.color3=col3; pd->ww_final.color4=col4; pd->ww_final.USEcolor=0; pd->ww_final.USEcolor1234=1; pd->ww_final.AUTOcolor=0; }
        if ((fc1 >=0.0)&&(fc2 >=0.0)&&(fc3 >=0.0)&&((pd->ww_final.FillCol1234Space!=SW_COLSPACE_CMYK)||(fc4 >=0.0))) { pd->ww_final.fillcolor1=fc1; pd->ww_final.fillcolor2=fc2; pd->ww_final.fillcolor3=fc3; pd->ww_final.fillcolor4=fc4; pd->ww_final.USEfillcolor=0; pd->ww_final.USEfillcolor1234=1; }

        if (gsl_finite(col))
         {
          int j, palette_index;
          for (j=1; j<PALETTE_LENGTH; j++) if (x->c->set->palette_current[j]==-1) break;
          palette_index = (((int)col)-1)%j;
          while (palette_index < 0) palette_index+=j;
          pd->ww_final.color        = x->c->set->palette_current [palette_index];
          pd->ww_final.Col1234Space = x->c->set->paletteS_current[palette_index];
          pd->ww_final.color1       = x->c->set->palette1_current[palette_index];
          pd->ww_final.color2       = x->c->set->palette2_current[palette_index];
          pd->ww_final.color3       = x->c->set->palette3_current[palette_index];
          pd->ww_final.color4       = x->c->set->palette4_current[palette_index];
          pd->ww_final.USEcolor1234 = (pd->ww_final.color == 0);
          pd->ww_final.AUTOcolor    = 0;
          pd->ww_final.USEcolor     = (pd->ww_final.color >  0);
         }
        if (gsl_finite(fc ))
         {
          int j, palette_index;
          for (j=1; j<PALETTE_LENGTH; j++) if (x->c->set->palette_current[j]==-1) break;
          palette_index = (((int)fc)-1)%j;
          while (palette_index < 0) palette_index+=j;                                                                
          pd->ww_final.fillcolor        = x->c->set->palette_current [palette_index];
          pd->ww_final.FillCol1234Space = x->c->set->paletteS_current[palette_index];
          pd->ww_final.fillcolor1       = x->c->set->palette1_current[palette_index];
          pd->ww_final.fillcolor2       = x->c->set->palette2_current[palette_index];
          pd->ww_final.fillcolor3       = x->c->set->palette3_current[palette_index];
          pd->ww_final.fillcolor4       = x->c->set->palette4_current[palette_index];
          pd->ww_final.USEfillcolor1234 = (pd->ww_final.fillcolor == 0);
          pd->ww_final.USEfillcolor     = (pd->ww_final.fillcolor >  0);
         }

        // Advance automatic plot styles
        if (pd->ww_final.AUTOcolor)
         {
          int i,j;
          for (j=0; j<PALETTE_LENGTH; j++) if (x->c->set->palette_current[j]==-1) break; // j now contains length of palette
          i = ((pd->ww_final.AUTOcolor+clist[cn].i)-5) % j; // i is now the palette color number to use
          while (i<0) i+=j;
          if (x->c->set->palette_current[i] > 0) { pd->ww_final.color  = x->c->set->palette_current[i]; pd->ww_final.USEcolor = 1; }
          else                                   { pd->ww_final.Col1234Space = x->c->set->paletteS_current[i]; pd->ww_final.color1 = x->c->set->palette1_current[i]; pd->ww_final.color2 = x->c->set->palette2_current[i]; pd->ww_final.color3 = x->c->set->palette3_current[i]; pd->ww_final.color4 = x->c->set->palette4_current[i]; pd->ww_final.USEcolor1234 = 1; }
         }
        else if (pd->ww_final.AUTOlinetype) pd->ww_final.linetype = pd->ww_final.AUTOlinetype + clist[cn].i;
       }

      // PASS 1: Fill path, if required
      if (pass==1)
       {
//      eps_core_SetFillColour(x, &pd->ww_final);
//      eps_core_SwitchTo_FillColour(x,1);
//      IF_NOT_INVISIBLE
//       {
//        double xps, yps; long c=0;
//        xpos = clist[cn].posdata[c++];
//        ypos = clist[cn].posdata[c++];
//        XPOS_TO_POSTSCRIPT;
//        fprintf(x->epsbuffer, "newpath %.2f %.2f moveto\n", xps, yps);
//        while (c<2*clist[cn].Nvertices_max)
//         {
//          xpos = clist[cn].posdata[c++];
//          ypos = clist[cn].posdata[c++];
//          XPOS_TO_POSTSCRIPT;
//          fprintf(x->epsbuffer, "%.2f %.2f lineto\n", xps, yps);
//         }
//        fprintf(x->epsbuffer, "closepath fill\n");
//       }
       }

      // PASS 2: Set clip path before stroking
      else if (pass==2)
       {
        long   n=clist[cn].Nvertices_max;
        if (n<1) continue;
        if (sg->ContoursLabel==SW_ONOFF_ON)
         {
          int    page = x->current->DatasetTextID[pdn]+clist[cn].i;
          long   i    = clist[cn].segment_flatest;
          double xlab0= (clist[cn].posdata[2*((i  )%n)+0] + clist[cn].posdata[2*((i+1)%n)+0] )/2;
          double ylab0= (clist[cn].posdata[2*((i  )%n)+1] + clist[cn].posdata[2*((i+1)%n)+1] )/2;
          double xlab, ylab, wlab, hlab;
          double bb_top,bb_bottom,ab_left,ab_right;
          postscriptPage *dviPage;
  
          xlab = xo + Lx*xlab0/(XSize-1)*sin(ThetaX) + Ly*ylab0/(YSize-1)*sin(ThetaY);
          ylab = yo + Lx*xlab0/(XSize-1)*cos(ThetaX) + Ly*ylab0/(YSize-1)*cos(ThetaY);
          if (x->dvi == NULL) { continue; }
          dviPage = (postscriptPage *)ppl_listGetItem(x->dvi->output->pages, page+1);
          if (dviPage== NULL) { continue; }
          //bb_left   = dviPage->boundingBox[0];
          bb_bottom = dviPage->boundingBox[1];
          //bb_right  = dviPage->boundingBox[2];
          bb_top    = dviPage->boundingBox[3];
          ab_left   = dviPage->textSizeBox[0];
          //ab_bottom = dviPage->textSizeBox[1];
          ab_right  = dviPage->textSizeBox[2];
          //ab_top    = dviPage->textSizeBox[3];
          hlab      = (fabs(bb_top - bb_bottom) + 2) * x->current->settings.FontSize;
          wlab      = (fabs(ab_right - ab_left) + 2) * x->current->settings.FontSize;

          fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto %.2f %.2f lineto %.2f %.2f lineto closepath %.2f %.2f %.2f 0 360 arc closepath eoclip\n", xlab-wlab/2, ylab-hlab/2, xlab+wlab/2, ylab-hlab/2, xlab+wlab/2, ylab+hlab/2, xlab-wlab/2, ylab+hlab/2, xlab, ylab, 2*(Lx+Ly));
         }
       }

      // PASS 3: Stroke path
      else if (pass==3)
       {
        eps_core_SetColour(x, &pd->ww_final, 1);
        eps_core_SetLinewidth(x, EPS_DEFAULT_LINEWIDTH * pd->ww_final.linewidth, pd->ww_final.linetype, 0);
        IF_NOT_INVISIBLE
         {
          double xps, yps; long c=0;
          long   n=clist[cn].Nvertices_max;
          if (n<1) continue;
          xpos = clist[cn].posdata[c++];
          ypos = clist[cn].posdata[c++];
          XPOS_TO_POSTSCRIPT;
          fprintf(x->epsbuffer, "newpath %.2f %.2f moveto\n", xps, yps);
          while (c<2*clist[cn].Nvertices_min)
           {
            xpos = clist[cn].posdata[c++];
            ypos = clist[cn].posdata[c++];
            XPOS_TO_POSTSCRIPT;
            fprintf(x->epsbuffer, "%.2f %.2f lineto\n", xps, yps);
           }
          fprintf(x->epsbuffer, "%sstroke\n", clist[cn].closepath?"closepath ":"");
         }
       }

      // PASS 4: Label contours
      else if ((pass==4) && (sg->ContoursLabel==SW_ONOFF_ON))
       {
        long   i = clist[cn].segment_flatest;
        long   n = clist[cn].Nvertices_max;
        withWords ww;

        if (n<1) continue;
        ppl_withWordsZero(x->c,&ww);
        if (x->current->settings.TextColour > 0) { ww.color = x->current->settings.TextColour; ww.USEcolor = 1; }
        else                                     { ww.Col1234Space = x->current->settings.TextCol1234Space; ww.color1 = x->current->settings.TextColour1; ww.color2 = x->current->settings.TextColour2; ww.color3 = x->current->settings.TextColour3; ww.color4 = x->current->settings.TextColour4; ww.USEcolor1234 = 1; }
        eps_core_SetColour(x, &ww, 1);

        IF_NOT_INVISIBLE
         {
          double xlab = (clist[cn].posdata[2*((i  )%n)+0] + clist[cn].posdata[2*((i+1)%n)+0] )/2;
          double ylab = (clist[cn].posdata[2*((i  )%n)+1] + clist[cn].posdata[2*((i+1)%n)+1] )/2;
          canvas_EPSRenderTextItem(x, NULL, x->current->DatasetTextID[pdn]+clist[cn].i,
                                   (xo + Lx*xlab/(XSize-1)*sin(ThetaX) + Ly*ylab/(YSize-1)*sin(ThetaY))/M_TO_PS,
                                   (yo + Lx*xlab/(XSize-1)*cos(ThetaX) + Ly*ylab/(YSize-1)*cos(ThetaY))/M_TO_PS,
                                   SW_HALIGN_CENT, SW_VALIGN_CENT, x->CurrentColour, x->current->settings.FontSize,
                                   0.0, NULL, NULL);
         }
       }
     }
   }

  // Reset value of variable c1
  if (CVar!=NULL) ppl_contextRestoreVarPointer(x->c, c1name, &CDummy);

  return 0;
 }

