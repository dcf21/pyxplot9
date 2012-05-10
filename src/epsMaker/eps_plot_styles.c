// eps_plot_styles.c
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

#define _PPL_EPS_PLOT_STYLES_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gsl/gsl_math.h>

#include "coreUtils/memAlloc.h"

#include "epsMaker/canvasDraw.h"
#include "canvasItems.h"
#include "coreUtils/errorReport.h"
#include "settings/epsColors.h"
#include "settings/settingTypes.h"
#include "userspace/unitsDisp.h"
#include "userspace/unitsArithmetic.h"

#include "epsMaker/eps_arrow.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_plot.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_filledregion.h"
#include "epsMaker/eps_plot_linedraw.h"
#include "epsMaker/eps_plot_styles.h"
#include "epsMaker/eps_plot_threedimbuff.h"
#include "epsMaker/eps_settings.h"
#include "epsMaker/eps_style.h"

// Work out the default set of with words for a plot item
void eps_withwords_default(EPSComm *x, withWords *output, pplset_graph *sg, unsigned char functions, int Ccounter, int LTcounter, int PTcounter, unsigned char color)
 {
  int i,j;

  ppl_withWordsZero(x->c, output);
  if (!color) { output->color = COLOR_BLACK; output->USEcolor = 1; }
  else
   {
    for (j=0; j<PALETTE_LENGTH; j++) if (x->c->set->palette_current[j]==-1) break; // j now contains length of palette
    i = Ccounter % j; // i is now the palette color number to use
    while (i<0) i+=j;
    if (x->c->set->palette_current[i] > 0) { output->color  = x->c->set->palette_current[i]; output->USEcolor = 1; }
    else                                   { output->Col1234Space = x->c->set->paletteS_current[i]; output->color1 = x->c->set->palette1_current[i]; output->color2 = x->c->set->palette2_current[i]; output->color3 = x->c->set->palette3_current[i]; output->color4 = x->c->set->palette4_current[i]; output->USEcolor1234 = 1; }
    output->AUTOcolor = Ccounter+5;
   }
  output->linespoints    = (functions ? SW_STYLE_LINES : SW_STYLE_POINTS);
  output->USElinespoints = 1;
  output->fillcolor     = COLOR_NULL;
  output->USEfillcolor  = 1;
  output->linetype       = color ? 1 : LTcounter+1;
  output->pointtype      =              PTcounter+1;
  output->USElinetype    = output->USEpointtype  = 1;
  output->AUTOlinetype   = !color;
  output->AUTOpointtype  = PTcounter+6;
  output->linewidth      = sg->LineWidth;
  output->pointlinewidth = sg->PointLineWidth;
  output->pointsize      = sg->PointSize;
  output->USElinewidth   = output->USEpointlinewidth = output->USEpointsize = 1;
  return;
 }

void eps_withwords_default_counterinc(EPSComm *x, int *Ccounter, int *LTcounter, int *PTcounter, unsigned char color, withWords *ww_final, pplset_graph *sg)
 {
  int style = ww_final->linespoints;

  if (ww_final->AUTOcolor && (style!=SW_STYLE_COLORMAP) && (style!=SW_STYLE_CONTOURMAP)) { (*Ccounter)++; }

  if (ww_final->AUTOlinetype && ((style==SW_STYLE_ARROWS_HEAD)||(style==SW_STYLE_ARROWS_NOHEAD)||(style==SW_STYLE_ARROWS_TWOHEAD)||(style==SW_STYLE_BOXES)||(style==SW_STYLE_FILLEDREGION)||(style==SW_STYLE_FSTEPS)||(style==SW_STYLE_HISTEPS)||(style==SW_STYLE_IMPULSES)||(style==SW_STYLE_LINES)||(style==SW_STYLE_LINESPOINTS)||(style==SW_STYLE_STEPS)||(style==SW_STYLE_SURFACE)||(style==SW_STYLE_WBOXES)||(style==SW_STYLE_XERRORBARS)||(style==SW_STYLE_XERRORRANGE)||(style==SW_STYLE_XYERRORBARS)||(style==SW_STYLE_XYERRORRANGE)||(style==SW_STYLE_XYZERRORBARS)||(style==SW_STYLE_XYZERRORRANGE)||(style==SW_STYLE_YERRORBARS)||(style==SW_STYLE_YERRORRANGE)||(style==SW_STYLE_YZERRORBARS)||(style==SW_STYLE_YZERRORRANGE)||(style==SW_STYLE_ZERRORBARS)||(style==SW_STYLE_ZERRORRANGE))) { (*LTcounter)++; }

  if (ww_final->AUTOpointtype && ((style==SW_STYLE_LINESPOINTS)||(style==SW_STYLE_POINTS)||(style==SW_STYLE_STARS))) { (*PTcounter)++; }

  if (style==SW_STYLE_CONTOURMAP)
   {
    int Ncontours = (sg->ContoursListLen>=0) ? sg->ContoursListLen : sg->ContoursN;
    if (ww_final->AUTOcolor)   { (*Ccounter )+=Ncontours; }
    if (ww_final->AUTOlinetype) { (*LTcounter)+=Ncontours; }
   }

  return;
 }

// Return the number of columns of data which are required to plot in any given plot style
int eps_plot_styles_NDataColumns(EPSComm *x, int style, unsigned char ThreeDim)
 {
  if      (style == SW_STYLE_POINTS         ) return 2 + (ThreeDim!=0);
  else if (style == SW_STYLE_LINES          ) return 2 + (ThreeDim!=0);
  else if (style == SW_STYLE_LINESPOINTS    ) return 2 + (ThreeDim!=0);
  else if (style == SW_STYLE_XERRORBARS     ) return 3 + (ThreeDim!=0);
  else if (style == SW_STYLE_YERRORBARS     ) return 3 + (ThreeDim!=0);
  else if (style == SW_STYLE_ZERRORBARS     ) return 3 + 1;
  else if (style == SW_STYLE_XYERRORBARS    ) return 4 + (ThreeDim!=0);
  else if (style == SW_STYLE_XZERRORBARS    ) return 4 + 1;
  else if (style == SW_STYLE_YZERRORBARS    ) return 4 + 1;
  else if (style == SW_STYLE_XYZERRORBARS   ) return 5 + 1;
  else if (style == SW_STYLE_XERRORRANGE    ) return 4 + (ThreeDim!=0);
  else if (style == SW_STYLE_YERRORRANGE    ) return 4 + (ThreeDim!=0);
  else if (style == SW_STYLE_ZERRORRANGE    ) return 4 + 1;
  else if (style == SW_STYLE_XYERRORRANGE   ) return 6 + (ThreeDim!=0);
  else if (style == SW_STYLE_XZERRORRANGE   ) return 6 + 1;
  else if (style == SW_STYLE_YZERRORRANGE   ) return 6 + 1;
  else if (style == SW_STYLE_XYZERRORRANGE  ) return 8 + 1;
  else if (style == SW_STYLE_FILLEDREGION   ) return 2;
  else if (style == SW_STYLE_YERRORSHADED   ) return 3;
  else if (style == SW_STYLE_UPPERLIMITS    ) return 2 + (ThreeDim!=0);
  else if (style == SW_STYLE_LOWERLIMITS    ) return 2 + (ThreeDim!=0);
  else if (style == SW_STYLE_DOTS           ) return 2 + (ThreeDim!=0);
  else if (style == SW_STYLE_IMPULSES       ) return 2 + (ThreeDim!=0);
  else if (style == SW_STYLE_BOXES          ) return 2;
  else if (style == SW_STYLE_WBOXES         ) return 3;
  else if (style == SW_STYLE_STEPS          ) return 2;
  else if (style == SW_STYLE_FSTEPS         ) return 2;
  else if (style == SW_STYLE_HISTEPS        ) return 2;
  else if (style == SW_STYLE_STARS          ) return 2 + (ThreeDim!=0);
  else if (style == SW_STYLE_ARROWS_HEAD    ) return 4 + 2*(ThreeDim!=0);
  else if (style == SW_STYLE_ARROWS_NOHEAD  ) return 4 + 2*(ThreeDim!=0);
  else if (style == SW_STYLE_ARROWS_TWOHEAD ) return 4 + 2*(ThreeDim!=0);
  else if (style == SW_STYLE_SURFACE        ) return 3;
  else if (style == SW_STYLE_COLORMAP      ) return 3;
  else if (style == SW_STYLE_CONTOURMAP     ) return 3;

  ppl_fatal(&x->c->errcontext,__FILE__,__LINE__,"Unrecognised style type passed to eps_plot_styles_NDataColumns()");
  return -1;
 }

// UpdateUsage... get content of row X from data table
#define UUR(X) blk->data_real[X + Ncolumns*j]

// UpdateUsage... check whether position is within range of axis
#define UUC(X,Y) \
{ \
 if (InRange && (!eps_plot_axis_InRange(X,Y))) InRange=0; \
}

// Memory recall on within-range flag, adding previous flag to ORed list of points to be checked
#define UUD(X,Y) \
{ \
 PartiallyInRange = PartiallyInRange || InRange; \
 InRange = InRangeMemory; \
 UUC(X,Y); \
}

// Store current within-range flag to memory
#define UUE(X,Y) \
{ \
 InRangeMemory = InRange; \
 UUC(X,Y); \
}

// Simultaneously update usage with UUU and check whether position is within range
#define UUF(X,Y) \
{ \
 UUC(X,logaxis?exp(Y):(Y)) \
 UUU(X,logaxis?exp(Y):(Y)) \
}

// Reset flags used to test whether a datapoint is within range before using it to update ranges of other axes
#define UUC_RESET \
{ \
 InRange=1; PartiallyInRange=0; InRangeMemory=1; \
}

// UpdateUsage... update axis X with ordinate value Y
#define UUU(X,Y) \
{ \
 if (InRange || PartiallyInRange) \
  { \
   z = Y; \
   if ( (gsl_finite(z)) && ((!X->MinUsedSet) || (X->MinUsed > z)) && ((X->LogFinal != SW_BOOL_TRUE) || (z>0.0)) ) { X->MinUsedSet=1; X->MinUsed=z; } \
   if ( (gsl_finite(z)) && ((!X->MaxUsedSet) || (X->MaxUsed < z)) && ((X->LogFinal != SW_BOOL_TRUE) || (z>0.0)) ) { X->MaxUsedSet=1; X->MaxUsed=z; } \
  } \
}

// UpdateUsage... update axis X to include value of BoxFrom
#define UUUBF(X) \
{ \
 if (!(X->DataUnitSet && (!ppl_unitsDimEqual(&sg->BoxFrom, &X->DataUnit)))) UUU(X,sg->BoxFrom.real); \
}

// UpdateUsage... get physical unit of row X from data table
#define UURU(X) data->firstEntries[X]

// UpdateUsage... assert that axis X should be dimensionally compatible with unit Y
#define UUAU(XYZ,XYZN,X,Y) \
 if ((X->HardUnitSet) && (!ppl_unitsDimEqual(&X->HardUnit , &(Y)))) { sprintf(x->c->errcontext.tempErrStr, "Axis %c%d on plot %d has data plotted against it with conflicting physical units of <%s> as compared to range of axis, which has units of <%s>.", "xyzc"[XYZ], XYZN, id,  ppl_printUnit(x->c,&(Y),NULL,NULL,0,1,0),  ppl_printUnit(x->c,&X->HardUnit,NULL,NULL,1,1,0)); ppl_error(&x->c->errcontext, ERR_GENERAL, -1, -1, NULL); return 1; } \
 if ((X->DataUnitSet) && (!ppl_unitsDimEqual(&X->DataUnit , &(Y)))) { sprintf(x->c->errcontext.tempErrStr, "Axis %c%d on plot %d has data plotted against it with conflicting physical units of <%s> and <%s>.", "xyzc"[XYZ], XYZN, id,  ppl_printUnit(x->c,&X->DataUnit,NULL,NULL,0,1,0),  ppl_printUnit(x->c,&(Y),NULL,NULL,1,1,0)); ppl_error(&x->c->errcontext, ERR_GENERAL, -1, -1, NULL); return 1; } \
 if (!X->DataUnitSet) \
  { \
   X->DataUnitSet = 1; \
   X->DataUnit = Y; \
  }

// Update the usage of axes to include data from a particular data table, plotted in a particular style
int eps_plot_styles_UpdateUsage(EPSComm *x, dataTable *data, int style, unsigned char ThreeDim, pplset_axis *a1, pplset_axis *a2, pplset_axis *a3, pplset_graph *sg, int xyz1, int xyz2, int xyz3, int n1, int n2, int n3, int id)
 {
  int i, j, Ncolumns;
  double z;
  double ptAx, ptBx=0, ptCx=0, lasty=0;
  unsigned char ptAset=0, ptBset=0, ptCset=0;
  unsigned char InRange, PartiallyInRange, InRangeMemory;
  dataBlock *blk;

  if ((data==NULL) || (data->Nrows<1)) return 0; // No data present

  // Cycle through data table acting upon the physical units of all of the columns
  if      (style == SW_STYLE_POINTS         ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_LINES          ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_LINESPOINTS    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_XERRORBARS     ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz1,n1,a1,UURU(2+ThreeDim)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_YERRORBARS     ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz2,n2,a2,UURU(2+ThreeDim)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_ZERRORBARS     ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz3,n3,a3,UURU(3)); }
  else if (style == SW_STYLE_XYERRORBARS    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz1,n1,a1,UURU(2+ThreeDim)); UUAU(xyz2,n2,a2,UURU(3+ThreeDim)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_XZERRORBARS    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz1,n1,a1,UURU(3)); UUAU(xyz3,n3,a3,UURU(4)); }
  else if (style == SW_STYLE_YZERRORBARS    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz2,n2,a2,UURU(3)); UUAU(xyz3,n3,a3,UURU(4)); }
  else if (style == SW_STYLE_XYZERRORBARS   ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz1,n1,a1,UURU(3)); UUAU(xyz2,n2,a2,UURU(4)); UUAU(xyz3,n3,a3,UURU(5)); }
  else if (style == SW_STYLE_XERRORRANGE    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz1,n1,a1,UURU(2+ThreeDim)); UUAU(xyz1,n1,a1,UURU(3+ThreeDim)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_YERRORRANGE    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz2,n2,a2,UURU(2+ThreeDim)); UUAU(xyz2,n2,a2,UURU(3+ThreeDim)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_ZERRORRANGE    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz3,n3,a3,UURU(3)); UUAU(xyz3,n3,a3,UURU(4)); }
  else if (style == SW_STYLE_XYERRORRANGE   ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz1,n1,a1,UURU(2+ThreeDim)); UUAU(xyz1,n1,a1,UURU(3+ThreeDim)); UUAU(xyz2,n2,a2,UURU(4+ThreeDim)); UUAU(xyz2,n2,a2,UURU(5+ThreeDim)); if (ThreeDim) UUAU(xyz3,n3,a3,UURU(2)); }
  else if (style == SW_STYLE_XZERRORRANGE   ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz1,n1,a1,UURU(3)); UUAU(xyz1,n1,a1,UURU(4)); UUAU(xyz3,n3,a3,UURU(5)); UUAU(xyz3,n3,a3,UURU(6)); }
  else if (style == SW_STYLE_YZERRORRANGE   ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz2,n2,a2,UURU(3)); UUAU(xyz2,n2,a2,UURU(4)); UUAU(xyz3,n3,a3,UURU(5)); UUAU(xyz3,n3,a3,UURU(6)); }
  else if (style == SW_STYLE_XYZERRORRANGE  ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz1,n1,a1,UURU(3)); UUAU(xyz1,n1,a1,UURU(4)); UUAU(xyz2,n2,a2,UURU(5)); UUAU(xyz2,n2,a2,UURU(6)); UUAU(xyz3,n3,a3,UURU(7)); UUAU(xyz3,n3,a3,UURU(8)); }
  else if (style == SW_STYLE_FILLEDREGION   ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); }
  else if (style == SW_STYLE_YERRORSHADED   ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz2,n2,a2,UURU(2)); }
  else if (style == SW_STYLE_UPPERLIMITS    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_LOWERLIMITS    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_DOTS           ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_IMPULSES       ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_BOXES          ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); }
  else if (style == SW_STYLE_WBOXES         ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz1,n1,a1,UURU(2)); }
  else if (style == SW_STYLE_STEPS          ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); }
  else if (style == SW_STYLE_FSTEPS         ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); }
  else if (style == SW_STYLE_HISTEPS        ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); }
  else if (style == SW_STYLE_STARS          ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_ARROWS_HEAD    ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz1,n1,a1,UURU(2+ThreeDim)); UUAU(xyz2,n2,a2,UURU(3+ThreeDim)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz3,n3,a3,UURU(5)); } }
  else if (style == SW_STYLE_ARROWS_NOHEAD  ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz1,n1,a1,UURU(2+ThreeDim)); UUAU(xyz2,n2,a2,UURU(3+ThreeDim)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz3,n3,a3,UURU(5)); } }
  else if (style == SW_STYLE_ARROWS_TWOHEAD ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); UUAU(xyz1,n1,a1,UURU(2+ThreeDim)); UUAU(xyz2,n2,a2,UURU(3+ThreeDim)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); UUAU(xyz3,n3,a3,UURU(5)); } }
  else if (style == SW_STYLE_SURFACE        ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); if (ThreeDim) { UUAU(xyz3,n3,a3,UURU(2)); } }
  else if (style == SW_STYLE_COLORMAP      ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); }
  else if (style == SW_STYLE_CONTOURMAP     ) { UUAU(xyz1,n1,a1,UURU(0)); UUAU(xyz2,n2,a2,UURU(1)); }

  // Cycle through data table, ensuring that axis ranges are sufficient to include all data
  Ncolumns = data->Ncolumns_real;
  blk = data->first;
  i=0;
  while (blk != NULL)
   {
    for (j=0; j<blk->blockPosition; j++)
     {
      UUC_RESET;
      if      (style == SW_STYLE_POINTS         ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_LINES          ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_LINESPOINTS    ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_XERRORBARS     ) { UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2)); UUE(a1, UUR(0)-UUR(2+ThreeDim));
                                                                                                    UUD(a1, UUR(0)                );
                                                                                                    UUD(a1, UUR(0)+UUR(2+ThreeDim));
                                                    UUU(a1, UUR(0)); UUU(a1, UUR(0)-UUR(2+ThreeDim)); UUU(a1, UUR(0)+UUR(2+ThreeDim)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_YERRORBARS     ) { UUC(a1, UUR(0)); if (ThreeDim) UUC(a3, UUR(2)); UUE(a2, UUR(1)-UUR(2+ThreeDim));
                                                                                                    UUD(a2, UUR(1)                );
                                                                                                    UUD(a2, UUR(1)+UUR(2+ThreeDim));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a2, UUR(1)-UUR(2+ThreeDim)); UUU(a2, UUR(1)+UUR(2+ThreeDim)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_ZERRORBARS     ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); UUE(a3, UUR(2)); UUD(a3, UUR(2)-UUR(3)); UUD(a3, UUR(2)+UUR(3));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a3, UUR(2)); UUU(a3, UUR(2)-UUR(3)); UUU(a3, UUR(2)+UUR(3)); }
      else if (style == SW_STYLE_XYERRORBARS    ) { if (ThreeDim) UUC(a3, UUR(2)); UUE(a1, UUR(0)                ); UUC(a2, UUR(1)-UUR(3+ThreeDim));
                                                                                   UUD(a1, UUR(0)                ); UUC(a2, UUR(1)                );
                                                                                   UUD(a1, UUR(0)                ); UUC(a2, UUR(1)+UUR(3+ThreeDim));
                                                                                   UUD(a1, UUR(0)-UUR(2+ThreeDim)); UUC(a2, UUR(1)                );
                                                                                   UUD(a1, UUR(0)+UUR(2+ThreeDim)); UUC(a2, UUR(1)                );
                                                    UUU(a1, UUR(0)); UUU(a1, UUR(0)-UUR(2+ThreeDim)); UUU(a1, UUR(0)+UUR(2+ThreeDim)); UUU(a2, UUR(1)); UUU(a2, UUR(1)-UUR(3+ThreeDim)); UUU(a2, UUR(1)+UUR(3+ThreeDim)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_XZERRORBARS    ) { UUC(a2, UUR(1)); UUE(a1, UUR(0)       ); UUC(a3, UUR(2)-UUR(4));
                                                                     UUD(a1, UUR(0)       ); UUC(a3, UUR(2)       );
                                                                     UUD(a1, UUR(0)       ); UUC(a3, UUR(2)+UUR(4));
                                                                     UUD(a1, UUR(0)-UUR(3)); UUC(a3, UUR(2)       );
                                                                     UUD(a1, UUR(0)+UUR(3)); UUC(a3, UUR(2)       );
                                                    UUU(a1, UUR(0)); UUU(a1, UUR(0)-UUR(3)); UUU(a1, UUR(0)+UUR(3)); UUU(a2, UUR(1)); UUU(a3, UUR(2)); UUU(a3, UUR(2)-UUR(4)); UUU(a3, UUR(2)+UUR(4)); }
      else if (style == SW_STYLE_YZERRORBARS    ) { UUC(a1, UUR(0)); UUE(a2, UUR(1)       ); UUC(a3, UUR(2)-UUR(4));
                                                                     UUD(a2, UUR(1)       ); UUC(a3, UUR(2)       );
                                                                     UUD(a2, UUR(1)       ); UUC(a3, UUR(2)+UUR(4));
                                                                     UUD(a2, UUR(1)-UUR(3)); UUC(a3, UUR(2)       );
                                                                     UUD(a2, UUR(1)+UUR(3)); UUC(a3, UUR(2)       );
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a2, UUR(1)-UUR(3)); UUU(a2, UUR(1)+UUR(3)); UUU(a3, UUR(2)); UUU(a3, UUR(2)-UUR(4)); UUU(a3, UUR(2)+UUR(4)); }
      else if (style == SW_STYLE_XYZERRORBARS   ) { UUC(a1, UUR(0)       ); UUC(a2, UUR(1)       ); UUC(a3, UUR(2)-UUR(5));
                                                    UUD(a1, UUR(0)       ); UUC(a2, UUR(1)       ); UUC(a3, UUR(2)       );
                                                    UUD(a1, UUR(0)       ); UUC(a2, UUR(1)       ); UUC(a3, UUR(2)+UUR(5));
                                                    UUD(a1, UUR(0)       ); UUC(a2, UUR(1)-UUR(4)); UUC(a3, UUR(2)       );
                                                    UUD(a1, UUR(0)       ); UUC(a2, UUR(1)+UUR(4)); UUC(a3, UUR(2)       );
                                                    UUD(a1, UUR(0)-UUR(3)); UUC(a2, UUR(1)       ); UUC(a3, UUR(2)       );
                                                    UUD(a1, UUR(0)+UUR(3)); UUC(a2, UUR(1)       ); UUC(a3, UUR(2)       );
                                                    UUU(a1, UUR(0)); UUU(a1, UUR(0)-UUR(3)); UUU(a1, UUR(0)+UUR(3)); UUU(a2, UUR(1)); UUU(a2, UUR(1)-UUR(4)); UUU(a2, UUR(1)+UUR(4)); UUU(a3, UUR(2)); UUU(a3, UUR(2)-UUR(5)); UUU(a3, UUR(2)+UUR(5)); }
      else if (style == SW_STYLE_XERRORRANGE    ) { UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2)); UUE(a1, UUR(2+ThreeDim));
                                                                                                    UUD(a1, UUR(0)         );
                                                                                                    UUD(a1, UUR(3+ThreeDim));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a1, UUR(2+ThreeDim)); UUU(a1, UUR(3+ThreeDim)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_YERRORRANGE    ) { UUC(a1, UUR(0)); if (ThreeDim) UUC(a3, UUR(2)); UUE(a2, UUR(2+ThreeDim));
                                                                                                    UUD(a2, UUR(1)         );
                                                                                                    UUD(a2, UUR(3+ThreeDim));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a2, UUR(2+ThreeDim)); UUU(a2, UUR(3+ThreeDim)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_ZERRORRANGE    ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); UUE(a3, UUR(2)); UUD(a3, UUR(3)); UUD(a3, UUR(4));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a3, UUR(2)); UUU(a3, UUR(3)); UUU(a3, UUR(4)); }
      else if (style == SW_STYLE_XYERRORRANGE   ) { if (ThreeDim) UUC(a3, UUR(2)); UUE(a1, UUR(0)         ); UUC(a2, UUR(4+ThreeDim));
                                                                                   UUD(a1, UUR(0)         ); UUC(a2, UUR(1)         );
                                                                                   UUD(a1, UUR(0)         ); UUC(a2, UUR(5+ThreeDim));
                                                                                   UUD(a1, UUR(2+ThreeDim)); UUC(a2, UUR(1)         );
                                                                                   UUD(a1, UUR(3+ThreeDim)); UUC(a2, UUR(1)         );
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a1, UUR(2+ThreeDim)); UUU(a1, UUR(3+ThreeDim)); UUU(a2, UUR(4+ThreeDim)); UUU(a2, UUR(5+ThreeDim)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_XZERRORRANGE   ) { UUC(a2, UUR(1)); UUE(a1, UUR(0)); UUC(a3, UUR(5));
                                                                     UUD(a1, UUR(0)); UUC(a3, UUR(2));
                                                                     UUD(a1, UUR(0)); UUC(a3, UUR(6));
                                                                     UUD(a1, UUR(3)); UUC(a3, UUR(2));
                                                                     UUD(a1, UUR(4)); UUC(a3, UUR(2));
                                                    UUU(a2, UUR(1)); UUU(a1, UUR(0)); UUU(a1, UUR(3)); UUU(a1, UUR(4)); UUU(a3, UUR(2)); UUU(a3, UUR(5)); UUU(a3, UUR(6)); }
      else if (style == SW_STYLE_YZERRORRANGE   ) { UUC(a1, UUR(0)); UUE(a2, UUR(1)); UUC(a3, UUR(5));
                                                                     UUD(a2, UUR(1)); UUC(a3, UUR(2));
                                                                     UUD(a2, UUR(1)); UUC(a3, UUR(6));
                                                                     UUD(a2, UUR(3)); UUC(a3, UUR(2));
                                                                     UUD(a2, UUR(4)); UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a1, UUR(3)); UUU(a1, UUR(4)); UUU(a2, UUR(1)); UUU(a2, UUR(5)); UUU(a2, UUR(6)); UUU(a3, UUR(2)); UUU(a3, UUR(7)); UUU(a3, UUR(8)); }
      else if (style == SW_STYLE_XYZERRORRANGE  ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); UUC(a3, UUR(7));
                                                    UUD(a1, UUR(0)); UUC(a2, UUR(1)); UUC(a3, UUR(2));
                                                    UUD(a1, UUR(0)); UUC(a2, UUR(1)); UUC(a3, UUR(8));
                                                    UUD(a1, UUR(0)); UUC(a2, UUR(5)); UUC(a3, UUR(2));
                                                    UUD(a1, UUR(0)); UUC(a2, UUR(6)); UUC(a3, UUR(2));
                                                    UUD(a1, UUR(3)); UUC(a2, UUR(1)); UUC(a3, UUR(2));
                                                    UUD(a1, UUR(4)); UUC(a2, UUR(1)); UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a1, UUR(0)-UUR(3)); UUU(a1, UUR(0)+UUR(3)); UUU(a2, UUR(1)); UUU(a2, UUR(1)-UUR(4)); UUU(a2, UUR(1)+UUR(4)); UUU(a3, UUR(2)); UUU(a3, UUR(2)-UUR(5)); UUU(a3, UUR(2)+UUR(5)); }
      else if (style == SW_STYLE_FILLEDREGION   ) { UUC(a1, UUR(0)); UUC(a2, UUR(1));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); }
      else if (style == SW_STYLE_YERRORSHADED   ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); UUC(a2, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a2, UUR(2)); }
      else if (style == SW_STYLE_LOWERLIMITS    ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_UPPERLIMITS    ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_DOTS           ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if (style == SW_STYLE_IMPULSES       ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); UUUBF(a2); }
      else if (style == SW_STYLE_WBOXES         ) { UUC(a2, UUR(1)); UUE(a1, UUR(0)); UUD(a1, UUR(0)-UUR(2)); UUD(a1, UUR(0)+UUR(2));
                                                    UUU(a2, UUR(1)); UUU(a1, UUR(0)); UUU(a1, UUR(0)-UUR(2)); UUU(a1, UUR(0)+UUR(2)); UUUBF(a2); }
      else if (style == SW_STYLE_STARS          ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if ((style == SW_STYLE_ARROWS_HEAD) || (style == SW_STYLE_ARROWS_NOHEAD) || (style == SW_STYLE_ARROWS_TWOHEAD))
                                                  { UUC(a1, UUR(0         )); UUC(a2, UUR(1         )); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUD(a1, UUR(2+ThreeDim)); UUC(a2, UUR(3+ThreeDim)); if (ThreeDim) UUC(a3, UUR(5));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); UUU(a1, UUR(2+ThreeDim)); UUU(a2, UUR(3+ThreeDim)); if (ThreeDim) { UUU(a3, UUR(2)); UUU(a3, UUR(5)); } }
      else if (style == SW_STYLE_SURFACE        ) { UUC(a1, UUR(0)); UUC(a2, UUR(1)); if (ThreeDim) UUC(a3, UUR(2));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); if (ThreeDim) UUU(a3, UUR(2)); }
      else if ((style == SW_STYLE_COLORMAP) || (style == SW_STYLE_CONTOURMAP))
                                                  { UUC(a1, UUR(0)); UUC(a2, UUR(1));
                                                    UUU(a1, UUR(0)); UUU(a2, UUR(1)); }
      else if ((style == SW_STYLE_BOXES) || (style == SW_STYLE_STEPS) || (style == SW_STYLE_FSTEPS) || (style == SW_STYLE_HISTEPS))
       {
        // Boxes and steps need slightly more complicated logic to take into account finite width of boxes/steps

#define FINISH_FACTORING_BOXES \
 { \
  /* Logic to take account of final boxes/steps */ \
  if (ptCset && ((style == SW_STYLE_BOXES) || (style == SW_STYLE_STEPS) || (style == SW_STYLE_FSTEPS) || (style == SW_STYLE_HISTEPS))) \
   { \
    unsigned char logaxis = (a1->LogFinal==SW_BOOL_TRUE); \
    UUC_RESET; \
    UUC(a2, lasty); \
    if (ptBset) /* We have one final box/step to process */ \
     { \
            if      ((style == SW_STYLE_BOXES) && (sg->BoxWidth.real<1e-200)) { UUF(a1, (ptCx - (ptCx-ptBx)/2      )); UUF(a1, (ptCx + (ptCx-ptBx)/2      )); UUUBF(a2); } \
            else if (style == SW_STYLE_BOXES)                                 { UUF(a1, (ptCx - sg->BoxWidth.real/2)); UUF(a1, (ptCx + sg->BoxWidth.real/2)); UUUBF(a2); } \
            else if (style == SW_STYLE_HISTEPS)                               { UUF(a1, (ptCx - (ptCx-ptBx)/2      )); UUF(a1, (ptCx + (ptCx-ptBx)/2      )); } \
            else if (style == SW_STYLE_STEPS)  { UUF(a1, ((ptBx+ptCx)/2 - (ptCx-ptBx)/2)); UUF(a1, ((ptBx+ptCx)/2 + (ptCx-ptBx)/2)); } \
            else if (style == SW_STYLE_FSTEPS) { UUF(a1, ptCx); } \
     } \
    else if (ptCset) /* We have a dataset with only a single box/step */ \
     { \
      if     ((style == SW_STYLE_BOXES) && (sg->BoxWidth.real<1e-200)) { UUF(a1, (ptCx - 0.5)); UUF(a1, (ptCx + 0.5)); UUUBF(a2); } \
      else if (style == SW_STYLE_BOXES)                                { UUF(a1, (ptCx - sg->BoxWidth.real/2)); UUF(a1, (ptCx + sg->BoxWidth.real/2)); UUUBF(a2); } \
      else if (sg->BoxWidth.real<1e-200)                               { UUF(a1, (ptCx - 0.5)); UUF(a1, (ptCx + 0.5)); } \
      else                                                             { UUF(a1, (ptCx - sg->BoxWidth.real/2)); UUF(a1, (ptCx + sg->BoxWidth.real/2)); } \
     } \
   } \
 }

        unsigned char logaxis = (a1->LogFinal==SW_BOOL_TRUE);
        if (blk->split[j]) { FINISH_FACTORING_BOXES; }
        UUC(a2, UUR(1)); // y-coordinates are easy
        ptAx=ptBx; ptAset=ptBset;
        ptBx=ptCx; ptBset=ptCset;
        ptCx=logaxis?log(UUR(0)):(UUR(0)); ptCset=1;
        if (ptBset)
         {
          if (ptAset) // We are processing a box in the midst of many
           {
            if      ((style == SW_STYLE_BOXES) && (sg->BoxWidth.real<1e-200)) { UUF(a1, ((ptBx+(ptAx+ptCx)/2)/2 - (ptCx-ptAx)/4)); UUF(a1, ((ptBx+(ptAx+ptCx)/2)/2 + (ptCx-ptAx)/4)); UUUBF(a2); }
            else if (style == SW_STYLE_BOXES)                                 { UUF(a1, (ptBx - sg->BoxWidth.real/2)); UUF(a1, (ptBx + sg->BoxWidth.real/2)); UUUBF(a2); }
            else if (style == SW_STYLE_HISTEPS)                               { UUF(a1, ((ptBx+(ptAx+ptCx)/2)/2 - (ptCx-ptAx)/4)); UUF(a1, ((ptBx+(ptAx+ptCx)/2)/2 + (ptCx-ptAx)/4)); UUUBF(a2); }
            else if (style == SW_STYLE_STEPS)  { UUF(a1, ((ptAx+ptBx)/2 - (ptBx-ptAx)/2)); UUF(a1, ((ptAx+ptBx)/2 + (ptBx-ptAx)/2)); }
            else if (style == SW_STYLE_FSTEPS) { UUF(a1, ((ptBx+ptCx)/2 - (ptCx-ptBx)/2)); UUF(a1, ((ptBx+ptCx)/2 + (ptCx-ptBx)/2)); }
           }
          else // The first box/step we work out the width of
           {
            if      ((style == SW_STYLE_BOXES) && (sg->BoxWidth.real<1e-200)) { UUF(a1, (ptBx - (ptCx-ptBx)/2));       UUF(a1, (ptBx + (ptCx-ptBx)/2)); UUUBF(a2); }
            else if (style == SW_STYLE_BOXES)                                 { UUF(a1, (ptBx - sg->BoxWidth.real/2)); UUF(a1, (ptBx + sg->BoxWidth.real/2)); UUUBF(a2); }
            else if (style == SW_STYLE_HISTEPS)                               { UUF(a1, (ptBx - (ptCx-ptBx)/2));       UUF(a1, (ptBx + (ptCx-ptBx)/2)); }
            else if (style == SW_STYLE_STEPS)  { UUF(a1, ptBx); }
            else if (style == SW_STYLE_FSTEPS) { UUF(a1, ((ptBx+ptCx)/2 - (ptCx-ptBx)/2)); UUF(a1, ((ptBx+ptCx)/2 + (ptCx-ptBx)/2)); }
           }
         }
        UUU(a2, UUR(1)); // y-coordinates are easy
        lasty = UUR(1);
       }
      i++;
     }
    blk=blk->next;
   }
  FINISH_FACTORING_BOXES;
  return 0;
 }

// Render a dataset to postscript
int  eps_plot_dataset(EPSComm *x, dataTable *data, int style, unsigned char ThreeDim, pplset_axis *a1, pplset_axis *a2, pplset_axis *a3, int xn, int yn, int zn, pplset_graph *sg, canvas_plotdesc *pd, double origin_x, double origin_y, double width, double height, double zdepth)
 {
  int             i, j, Ncolumns, Ncol_obj, pt=0, xrn, yrn, zrn;
  double          xpos, ypos, depth, xap, yap, zap, scale_x, scale_y, scale_z;
  char            epsbuff[FNAME_LENGTH], *last_colstr=NULL;
  LineDrawHandle *ld;
  pplset_axis    *a[3] = {a1,a2,a3};
  dataBlock      *blk;

  if ((data==NULL) || (data->Nrows<1)) return 0; // No data present

  Ncolumns = data->Ncolumns_real;
  Ncol_obj = data->Ncolumns_obj;
  if (eps_plot_WithWordsCheckUsingItemsDimLess(x->c, &pd->ww_final, data->firstEntries, Ncolumns, Ncol_obj, NULL)) return 1;

  if (!ThreeDim) { scale_x=width; scale_y=height; scale_z=1.0;    }
  else           { scale_x=width; scale_y=height; scale_z=zdepth; }

  // If axes have value-turning points, loop over all monotonic regions of axis space
  for (xrn=0; xrn<=a[xn]->AxisValueTurnings; xrn++)
  for (yrn=0; yrn<=a[yn]->AxisValueTurnings; yrn++)
  for (zrn=0; zrn<=(ThreeDim ? a[zn]->AxisValueTurnings : 0); zrn++)
    {

  blk = data->first;

  if ((style == SW_STYLE_LINES) || (style == SW_STYLE_LINESPOINTS)) // LINES
   {
    ld = LineDraw_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z);
    last_colstr=NULL;

    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++)
       {
        // Work out style information for next point
        eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
        eps_core_SetColor(x, &pd->ww_final, 0);
        if (blk->split[j]) { LineDraw_PenUp(x, ld); }
        IF_NOT_INVISIBLE
         {
          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
          LineDraw_Point(x, ld, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
         } else { LineDraw_PenUp(x, ld); }
       }
      blk=blk->next;
     }
    LineDraw_PenUp(x, ld);
   }

  if ((style == SW_STYLE_POINTS) || (style == SW_STYLE_LINESPOINTS) || (style == SW_STYLE_STARS) || (style == SW_STYLE_DOTS)) // POINTS, DOTS, STARS
   {
    last_colstr=NULL;

    blk = data->first;
    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++)
       {
        double final_pointsize=0.0;
        eps_plot_GetPosition(&xpos, &ypos, &depth, &xap, &yap, &zap, NULL, NULL, NULL, ThreeDim, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, origin_x, origin_y, scale_x, scale_y, scale_z, 0);
        if (!gsl_finite(xpos)) // Position of point is off side of graph
         {
          if ((blk->text[j] != NULL) && (blk->text[j][0] != '\0')) x->LaTeXpageno++;
          continue;
         }

        // Work out style information for next point
        eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
        final_pointsize = pd->ww_final.pointsize;
        if      (style == SW_STYLE_DOTS ) final_pointsize *=  0.05; // Dots are 1/20th size of points
        else if (style == SW_STYLE_STARS) final_pointsize *= 12.0 ; // Stars are BIG
        eps_core_SetColor(x, &pd->ww_final, 0);
        IF_NOT_INVISIBLE
         {
          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
          if (style != SW_STYLE_STARS)
           {
            pt = (style == SW_STYLE_DOTS) ? 16 : ((pd->ww_final.pointtype-1) % N_POINTTYPES); // Dots are always pt 17 (filled circle)
            while (pt<0) pt+=N_POINTTYPES;
            x->PointTypesUsed[pt] = 1;
            sprintf(epsbuff, "%.2f %.2f pt%d", xpos, ypos, pt+1);
            eps_core_BoundingBox(x, xpos, ypos, 2 * final_pointsize * eps_PointSize[pt] * EPS_DEFAULT_PS);
           } else {
            pt = ((pd->ww_final.pointtype-1) % N_STARTYPES);
            while (pt<0) pt+=N_STARTYPES;
            x->StarTypesUsed[pt] = 1;
            sprintf(epsbuff, "/angle { 40 } def %.2f %.2f st%d", xpos, ypos, pt+1);
            eps_core_BoundingBox(x, xpos, ypos, 2 * final_pointsize * eps_StarSize[pt] * EPS_DEFAULT_PS);
           }
          ThreeDimBuffer_writeps(x, depth, 1, pd->ww_final.pointlinewidth, 0.0, final_pointsize, last_colstr, epsbuff);
         }

        // label point if instructed to do so
        if ((blk->text[j] != NULL) && (blk->text[j][0] != '\0'))
         {
          char *text=NULL;
          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
          canvas_EPSRenderTextItem(x, &text, x->LaTeXpageno++,
             xpos/M_TO_PS - (x->current->settings.TextHAlign - SW_HALIGN_CENT) * final_pointsize * eps_PointSize[pt] * EPS_DEFAULT_PS / M_TO_PS * 1.1,
             ypos/M_TO_PS + (x->current->settings.TextVAlign - SW_VALIGN_CENT) * final_pointsize * eps_PointSize[pt] * EPS_DEFAULT_PS / M_TO_PS * 1.1,
             x->current->settings.TextHAlign, x->current->settings.TextVAlign, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
          if (text!=NULL) ThreeDimBuffer_writeps(x, depth, 1, 1, 0, 1, last_colstr, text);
         }
       }
      blk=blk->next;
     }
   }

  if ((style == SW_STYLE_XERRORBARS) || (style == SW_STYLE_YERRORBARS) || (style == SW_STYLE_ZERRORBARS) || (style == SW_STYLE_XYERRORBARS) || (style == SW_STYLE_YZERRORBARS) || (style == SW_STYLE_XZERRORBARS) || (style == SW_STYLE_XYZERRORBARS) || (style == SW_STYLE_XERRORRANGE) || (style == SW_STYLE_YERRORRANGE) || (style == SW_STYLE_ZERRORRANGE) || (style == SW_STYLE_XYERRORRANGE) || (style == SW_STYLE_YZERRORRANGE) || (style == SW_STYLE_XZERRORRANGE) || (style == SW_STYLE_XYZERRORRANGE)) // XERRORBARS , YERRORBARS , ZERRORBARS
   {
    unsigned char ac[3]={0,0,0};
    double b,ps,min[3],max[3];
    int NDirections=0;

    b  = 0.0005 * sg->bar;
    ps = pd->ww_final.pointsize * EPS_DEFAULT_PS / M_TO_PS;

    ld = LineDraw_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z);
    last_colstr=NULL;

    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++)
       {
        // Work out style information for next point
        eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
        eps_core_SetColor(x, &pd->ww_final, 0);
        LineDraw_PenUp(x, ld);
        IF_NOT_INVISIBLE
         {
          for (i=0;i<3;i++) { min[i]=max[i]=UUR(i); }
          if      (style == SW_STYLE_XERRORBARS   ) { NDirections = 1; ac[0]=1; min[0] = UUR(0) - UUR(2+ThreeDim); max[0] = UUR(0) + UUR(2+ThreeDim); }
          else if (style == SW_STYLE_YERRORBARS   ) { NDirections = 1; ac[1]=1; min[1] = UUR(1) - UUR(2+ThreeDim); max[1] = UUR(1) + UUR(2+ThreeDim); }
          else if (style == SW_STYLE_ZERRORBARS   ) { NDirections = 1; ac[2]=1; min[2] = UUR(2) - UUR(3         ); max[2] = UUR(2) + UUR(3         ); }
          else if (style == SW_STYLE_XERRORRANGE  ) { NDirections = 1; ac[0]=1; min[0] = UUR(2+ThreeDim); max[0] = UUR(3+ThreeDim); }
          else if (style == SW_STYLE_YERRORRANGE  ) { NDirections = 1; ac[1]=1; min[1] = UUR(2+ThreeDim); max[1] = UUR(3+ThreeDim); }
          else if (style == SW_STYLE_ZERRORRANGE  ) { NDirections = 1; ac[2]=1; min[2] = UUR(3         ); max[2] = UUR(4         ); }
          else if (style == SW_STYLE_XYERRORBARS  ) { NDirections = 2; ac[0]=ac[1]=1; min[0] = UUR(0) - UUR(2+ThreeDim); max[0] = UUR(0) + UUR(2+ThreeDim); min[1] = UUR(1) - UUR(3+ThreeDim); max[1] = UUR(1) + UUR(3+ThreeDim); }
          else if (style == SW_STYLE_XZERRORBARS  ) { NDirections = 2; ac[0]=ac[2]=1; min[0] = UUR(0) - UUR(3         ); max[0] = UUR(0) + UUR(3         ); min[2] = UUR(2) - UUR(4         ); max[2] = UUR(2) + UUR(4         ); }
          else if (style == SW_STYLE_YZERRORBARS  ) { NDirections = 2; ac[1]=ac[2]=1; min[1] = UUR(1) - UUR(3         ); max[1] = UUR(1) + UUR(3         ); min[2] = UUR(2) - UUR(4         ); max[2] = UUR(2) + UUR(4         ); }
          else if (style == SW_STYLE_XYERRORRANGE ) { NDirections = 2; ac[0]=ac[1]=1; min[0] = UUR(2+ThreeDim); max[0] = UUR(3+ThreeDim); min[1] = UUR(4+ThreeDim); max[1] = UUR(5+ThreeDim); }
          else if (style == SW_STYLE_YZERRORRANGE ) { NDirections = 2; ac[1]=ac[2]=1; min[1] = UUR(3         ); max[1] = UUR(4         ); min[2] = UUR(5         ); max[2] = UUR(6         ); }
          else if (style == SW_STYLE_XZERRORRANGE ) { NDirections = 2; ac[0]=ac[2]=1; min[0] = UUR(3         ); max[0] = UUR(4         ); min[2] = UUR(5         ); max[2] = UUR(6         ); }
          else if (style == SW_STYLE_XYZERRORBARS ) { NDirections = 3; ac[0]=ac[1]=ac[2]=1; min[0] = UUR(0) - UUR(3); max[0] = UUR(0) + UUR(3); min[1] = UUR(1) - UUR(4); max[1] = UUR(1) + UUR(4); min[2] = UUR(2) - UUR(5); max[2] = UUR(2) + UUR(6); }
          else if (style == SW_STYLE_XYZERRORRANGE) { NDirections = 3; ac[0]=ac[1]=ac[2]=1; min[0] = UUR(3); max[0] = UUR(4); min[1] = UUR(5); max[1] = UUR(6); min[2] = UUR(7); max[2] = UUR(8); }

          if (a[0]->LogFinal==SW_BOOL_TRUE) { if (min[0]<DBL_MIN) min[0]=DBL_MIN; if (max[0]<DBL_MIN) max[0]=DBL_MIN; }
          if (a[1]->LogFinal==SW_BOOL_TRUE) { if (min[1]<DBL_MIN) min[1]=DBL_MIN; if (max[1]<DBL_MIN) max[1]=DBL_MIN; }
          if (a[2]->LogFinal==SW_BOOL_TRUE) { if (min[2]<DBL_MIN) min[2]=DBL_MIN; if (max[2]<DBL_MIN) max[2]=DBL_MIN; }

          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }

          for (i=0; i<3; i++) if (ac[i])
           {
            LineDraw_Point(x, ld, (i==xn)?(min[xn]):(UUR(xn)), (i==yn)?(min[yn]):(UUR(yn)), ThreeDim ? ((i==zn)?(min[zn]):(UUR(zn))) : 0.0, 0,0,0,
                  (i==xn)?( b ):0, (i==yn)?( b ):0, (i==zn)?( b ):0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_Point(x, ld, (i==xn)?(min[xn]):(UUR(xn)), (i==yn)?(min[yn]):(UUR(yn)), ThreeDim ? ((i==zn)?(min[zn]):(UUR(zn))) : 0.0, 0,0,0,
                  (i==xn)?(-b ):0, (i==yn)?(-b ):0, (i==zn)?(-b ):0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_PenUp(x, ld);
            LineDraw_Point(x, ld, (i==xn)?(min[xn]):(UUR(xn)), (i==yn)?(min[yn]):(UUR(yn)), ThreeDim ? ((i==zn)?(min[zn]):(UUR(zn))) : 0.0, 0,0,0,
                                0,               0,               0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_Point(x, ld, (i==xn)?(max[xn]):(UUR(xn)), (i==yn)?(max[yn]):(UUR(yn)), ThreeDim ? ((i==zn)?(max[zn]):(UUR(zn))) : 0.0, 0,0,0,
                                0,               0,               0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_PenUp(x, ld);
            LineDraw_Point(x, ld, (i==xn)?(max[xn]):(UUR(xn)), (i==yn)?(max[yn]):(UUR(yn)), ThreeDim ? ((i==zn)?(max[zn]):(UUR(zn))) : 0.0, 0,0,0,
                  (i==xn)?( b ):0, (i==yn)?( b ):0, (i==zn)?( b ):0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_Point(x, ld, (i==xn)?(max[xn]):(UUR(xn)), (i==yn)?(max[yn]):(UUR(yn)), ThreeDim ? ((i==zn)?(max[zn]):(UUR(zn))) : 0.0, 0,0,0,
                  (i==xn)?(-b ):0, (i==yn)?(-b ):0, (i==zn)?(-b ):0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_PenUp(x, ld);
            if (NDirections!=1) continue; // Only put a central bar through errorbars which only go in a single direction
            LineDraw_Point(x, ld,                   (UUR(xn)),                   (UUR(yn)), ThreeDim ? (                   UUR(zn) ) : 0.0, 0,0,0,
                  (i==xn)?( ps):0, (i==yn)?( ps):0, (i==zn)?( ps):0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_Point(x, ld,                   (UUR(xn)),                   (UUR(yn)), ThreeDim ? (                   UUR(zn) ) : 0.0, 0,0,0,
                  (i==xn)?(-ps):0, (i==yn)?(-ps):0, (i==zn)?(-ps):0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_PenUp(x, ld);
           }
         }

        // label point if instructed to do so
        if ((blk->text[j] != NULL) && (blk->text[j][0] != '\0'))
         {
          char *text=NULL;
          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
          eps_plot_GetPosition(&xpos, &ypos, &depth, &xap, &yap, &zap, NULL, NULL, NULL, ThreeDim, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, origin_x, origin_y, scale_x, scale_y, scale_z, 0);
          if (!gsl_finite(xpos)) { x->LaTeXpageno++; continue; } // Position of point is off side of graph
          canvas_EPSRenderTextItem(x, &text, x->LaTeXpageno++, xpos/M_TO_PS, ypos/M_TO_PS, x->current->settings.TextHAlign, x->current->settings.TextVAlign, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
          if (text!=NULL) ThreeDimBuffer_writeps(x, depth, 1, 1, 0, 1, last_colstr, text);
         }
       }
      blk=blk->next;
     }
   }

  else if (style == SW_STYLE_IMPULSES       ) // IMPULSES
   {
    ld = LineDraw_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z);
    last_colstr=NULL;

    if (a[yn]->DataUnitSet && (!ppl_unitsDimEqual(&sg->BoxFrom, &a[yn]->DataUnit))) { sprintf(x->c->errcontext.tempErrStr, "Data with units of <%s> plotted with impulses when BoxFrom is set to a value with units of <%s>.", ppl_printUnit(x->c,&a[yn]->DataUnit,NULL,NULL,0,1,0),  ppl_printUnit(x->c,&sg->BoxFrom,NULL,NULL,1,1,0)); ppl_error(&x->c->errcontext, ERR_GENERAL, -1, -1, NULL); }
    else
     {
      while (blk != NULL)
       {
        for (j=0; j<blk->blockPosition; j++)
         {
          // Work out style information for next point
          eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
          eps_core_SetColor(x, &pd->ww_final, 0);
          LineDraw_PenUp(x, ld);
          IF_NOT_INVISIBLE
           {
            double boxfrom = sg->BoxFrom.real;
            if ((a[yn]->LogFinal==SW_BOOL_TRUE) && (boxfrom<DBL_MIN)) boxfrom=DBL_MIN;
            if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
            LineDraw_Point(x, ld, (xn==1)?boxfrom:(UUR(xn)), (yn==1)?boxfrom:(UUR(yn)), ThreeDim ? ((zn==1)?boxfrom:(UUR(zn))) : 0.0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
            LineDraw_Point(x, ld,                  UUR(xn) ,                  UUR(yn) , ThreeDim ?                   UUR(zn)   : 0.0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr);
           }
          LineDraw_PenUp(x, ld);

          // label point if instructed to do so
          if ((blk->text[j] != NULL) && (blk->text[j][0] != '\0'))
           {
            char *text=NULL;
            if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
            eps_plot_GetPosition(&xpos, &ypos, &depth, &xap, &yap, &zap, NULL, NULL, NULL, ThreeDim, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, origin_x, origin_y, scale_x, scale_y, scale_z, 0);
            if (!gsl_finite(xpos)) { x->LaTeXpageno++; continue; } // Position of point is off side of graph
            canvas_EPSRenderTextItem(x, &text, x->LaTeXpageno++, xpos/M_TO_PS, ypos/M_TO_PS, x->current->settings.TextHAlign, x->current->settings.TextVAlign, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
            if (text!=NULL) ThreeDimBuffer_writeps(x, depth, 1, 1, 0, 1, last_colstr, text);
           }
         }
        blk=blk->next;
       }
     }
   }

  else if ((style == SW_STYLE_LOWERLIMITS) || (style == SW_STYLE_UPPERLIMITS)) // LOWERLIMITS, UPPERLIMITS
   {
    int    an;
    double ps, sgn, lw, theta[3], x2, y2, x3, y3, x4, y4, x5, y5;
    double offset_barx, offset_bary, offset_arrx, offset_arry;
    ps  = pd->ww_final.pointsize * EPS_DEFAULT_PS / M_TO_PS;
    sgn = ( (style == SW_STYLE_UPPERLIMITS) ^ (a[yn]->MaxFinal > a[yn]->MinFinal) ) ? 1.0 : -1.0;
    lw  = pd->ww_final.pointlinewidth;

    ld = LineDraw_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z);
    last_colstr=NULL;

    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++)
       {
        // Work out style information for next point
        eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
        eps_core_SetColor(x, &pd->ww_final, 0);
        LineDraw_PenUp(x, ld);
        IF_NOT_INVISIBLE
         {
          eps_plot_GetPosition(&xpos, &ypos, &depth, &xap, &yap, &zap, &theta[0], &theta[1], &theta[2], ThreeDim, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, origin_x, origin_y, scale_x, scale_y, scale_z, 0);
          if (!gsl_finite(xpos)) // Position of point is off side of graph
           {
            if ((blk->text[j] != NULL) && (blk->text[j][0] != '\0')) x->LaTeXpageno++;
            continue;
           }

          if (xn==1) an=0; else if (yn==1) an=1; else an=2;
          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }

          offset_barx = ps * sin(theta[an] + M_PI/2);
          offset_bary = ps * cos(theta[an] + M_PI/2);
          offset_arrx = sgn*2*ps * sin(theta[an]);
          offset_arry = sgn*2*ps * cos(theta[an]);

          LineDraw_Point(x, ld, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0,-offset_barx,-offset_bary,0,0,0,0, pd->ww_final.linetype, lw, last_colstr);
          LineDraw_Point(x, ld, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0, offset_barx, offset_bary,0,0,0,0, pd->ww_final.linetype, lw, last_colstr);
          LineDraw_PenUp(x, ld);
          LineDraw_Point(x, ld, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0,           0,           0,0,0,0,0, pd->ww_final.linetype, lw, last_colstr);
          LineDraw_Point(x, ld, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0, offset_arrx, offset_arry,0,0,0,0, pd->ww_final.linetype, lw, last_colstr);
          LineDraw_PenUp(x, ld);
          x2 = xpos + sgn*2*ps*M_TO_PS * sin(theta[an]);
          y2 = ypos + sgn*2*ps*M_TO_PS * cos(theta[an]);
          if (sgn<0.0) theta[an] += M_PI;
          x3 = x2 - EPS_ARROW_HEADSIZE/2 * lw * sin(theta[an] - EPS_ARROW_ANGLE / 2); // Pointy back of arrowhead on one side
          y3 = y2 - EPS_ARROW_HEADSIZE/2 * lw * cos(theta[an] - EPS_ARROW_ANGLE / 2);
          x5 = x2 - EPS_ARROW_HEADSIZE/2 * lw * sin(theta[an] + EPS_ARROW_ANGLE / 2); // Pointy back of arrowhead on other side
          y5 = y2 - EPS_ARROW_HEADSIZE/2 * lw * cos(theta[an] + EPS_ARROW_ANGLE / 2);
          x4 = x2 - EPS_ARROW_HEADSIZE/2 * lw * sin(theta[an]) * (1.0 - EPS_ARROW_CONSTRICT) * cos(EPS_ARROW_ANGLE / 2); // Point where back of arrowhead crosses stalk
          y4 = y2 - EPS_ARROW_HEADSIZE/2 * lw * cos(theta[an]) * (1.0 - EPS_ARROW_CONSTRICT) * cos(EPS_ARROW_ANGLE / 2);
          sprintf(epsbuff, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\n%.2f %.2f lineto\n%.2f %.2f lineto\nclosepath\nfill\n", x4,y4,x3,y3,x2,y2,x5,y5);
          ThreeDimBuffer_writeps(x, depth, 1, lw, 0.0, 1.0, last_colstr, epsbuff);
         }

        // label point if instructed to do so
        if ((blk->text[j] != NULL) && (blk->text[j][0] != '\0'))
         {
          char *text=NULL;
          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
          canvas_EPSRenderTextItem(x, &text, x->LaTeXpageno++, xpos/M_TO_PS, ypos/M_TO_PS, x->current->settings.TextHAlign, x->current->settings.TextVAlign, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
          if (text!=NULL) ThreeDimBuffer_writeps(x, depth, 1, 1, 0, 1, last_colstr, text);
         }
       }
      blk=blk->next;
     }
   }

  else if ((style == SW_STYLE_BOXES) || (style == SW_STYLE_WBOXES) || (style == SW_STYLE_STEPS) || (style == SW_STYLE_FSTEPS) || (style == SW_STYLE_HISTEPS)) // BOXES, WBOXES, STEPS, FSTEPS, HISTEPS
   {
    double ptAx, ptBx=0, ptBy=0, ptCx=0, ptCy=0;
    unsigned char ptAset=0, ptBset=0, ptCset=0;

    ld = LineDraw_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z);
    last_colstr=NULL;

#define MAKE_STEP(X0,Y0,WIDTH) \
 IF_NOT_INVISIBLE \
  { \
   double pA[3]={(X0)-(WIDTH),(Y0),0}, pB[3]={(X0)+(WIDTH),(Y0),0}; \
   if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); } \
   if (logaxis) { pA[0]=exp(pA[0]); pB[0]=exp(pB[0]); } \
   LineDraw_Point(x, ld, pA[xn], pA[yn], ThreeDim?(pA[zn]):0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr); \
   LineDraw_Point(x, ld, pB[xn], pB[yn], ThreeDim?(pB[zn]):0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr); \
  } \

#define MAKE_BOX(X0,Y0,WIDTH) \
 { \
  int    an; \
  double pA[3] = {(X0)-(WIDTH), sg->BoxFrom.real, 0}; \
  double pB[3] = {(X0)-(WIDTH), (Y0)            , 0}; \
  double pC[3] = {(X0)+(WIDTH), (Y0)            , 0}; \
  double pD[3] = {(X0)+(WIDTH), sg->BoxFrom.real, 0}; \
  if (logaxis) { pA[0]=exp(pA[0]); pB[0]=exp(pB[0]); pC[0]=exp(pC[0]); pD[0]=exp(pD[0]); } \
  if (xn==1) an=0; else if (yn==1) an=1; else an=2; \
  if ((a[an]->LogFinal==SW_BOOL_TRUE) && (pA[1]<DBL_MIN)) pA[1]=DBL_MIN; \
  if ((a[an]->LogFinal==SW_BOOL_TRUE) && (pB[1]<DBL_MIN)) pB[1]=DBL_MIN; \
  if ((a[an]->LogFinal==SW_BOOL_TRUE) && (pC[1]<DBL_MIN)) pC[1]=DBL_MIN; \
  if ((a[an]->LogFinal==SW_BOOL_TRUE) && (pD[1]<DBL_MIN)) pD[1]=DBL_MIN; \
\
  /* Set fill color of box */ \
  eps_core_SetFillColor(x, &pd->ww_final); \
  eps_core_SwitchTo_FillColor(x,0); \
\
  /* Fill box */ \
  IF_NOT_INVISIBLE if (!ThreeDim) \
   { \
    FilledRegionHandle *fr; \
    fr = FilledRegion_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z); \
    FilledRegion_Point(x, fr, pA[xn], pA[yn]); \
    FilledRegion_Point(x, fr, pB[xn], pB[yn]); \
    FilledRegion_Point(x, fr, pC[xn], pC[yn]); \
    FilledRegion_Point(x, fr, pD[xn], pD[yn]); \
    FilledRegion_Finish(x, fr, pd->ww_final.linetype, pd->ww_final.linewidth, 0); \
   } \
  eps_core_SwitchFrom_FillColor(x,0); \
\
  /* Stroke outline of box */ \
  IF_NOT_INVISIBLE \
   { \
    if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); } \
    LineDraw_Point(x, ld, pA[xn], pA[yn], ThreeDim?pA[zn]:0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr); \
    LineDraw_Point(x, ld, pB[xn], pB[yn], ThreeDim?pB[zn]:0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr); \
    LineDraw_Point(x, ld, pC[xn], pC[yn], ThreeDim?pC[zn]:0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr); \
    LineDraw_Point(x, ld, pD[xn], pD[yn], ThreeDim?pD[zn]:0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr); \
    LineDraw_Point(x, ld, pA[xn], pA[yn], ThreeDim?pA[zn]:0, 0,0,0,0,0,0, pd->ww_final.linetype, pd->ww_final.linewidth, last_colstr); \
    LineDraw_PenUp(x, ld); \
   } \
 } \

#define FINISH_ROW_OF_BOXES \
 { \
  if (ptBset) /*/ We have one final box/step to process */ \
   { \
    if      ((style == SW_STYLE_BOXES) && (sg->BoxWidth.real<1e-200)) { MAKE_BOX ( ptCx                 , ptCy, (ptCx-ptBx)/2      ); } \
    else if (style == SW_STYLE_BOXES)                                 { MAKE_BOX ( ptCx                 , ptCy, sg->BoxWidth.real/2); } \
    else if (style == SW_STYLE_HISTEPS)                               { MAKE_STEP( ptCx                 , ptCy, (ptCx-ptBx)/2      ); } \
    else if (style == SW_STYLE_STEPS)  { MAKE_STEP((ptBx+ptCx)/2, ptCy, (ptCx-ptBx)/2); } \
    else if (style == SW_STYLE_FSTEPS) { MAKE_STEP( ptCx        , ptCy, 0.0          ); } \
   } \
  else if (ptCset) /* We have a dataset with only a single box/step */ \
   { \
    if     ((style == SW_STYLE_BOXES) && (sg->BoxWidth.real<1e-200)) { MAKE_BOX (ptCx, ptCy, 0.5); } \
    else if (style == SW_STYLE_BOXES)                                { MAKE_BOX (ptCx, ptCy, sg->BoxWidth.real/2); } \
    else if (sg->BoxWidth.real<1e-200)                               { MAKE_STEP(ptCx, ptCy, 0.5); } \
    else                                                             { MAKE_STEP(ptCx, ptCy, sg->BoxWidth.real/2); } \
   } \
  ptAset = ptBset = ptCset = 0; \
  LineDraw_PenUp(x, ld); \
 } \

    if (a[yn]->DataUnitSet && (!ppl_unitsDimEqual(&sg->BoxFrom, &a[yn]->DataUnit))) { sprintf(x->c->errcontext.tempErrStr, "Data with units of <%s> plotted as boxes/steps when BoxFrom is set to a value with units of <%s>.", ppl_printUnit(x->c,&a[yn]->DataUnit,NULL,NULL,0,1,0),  ppl_printUnit(x->c,&sg->BoxFrom,NULL,NULL,1,1,0)); ppl_error(&x->c->errcontext, ERR_GENERAL, -1, -1, NULL); }
    else if (a[xn]->DataUnitSet && (sg->BoxWidth.real>0.0) && (!ppl_unitsDimEqual(&sg->BoxWidth, &a[xn]->DataUnit))) { sprintf(x->c->errcontext.tempErrStr, "Data with ordinate units of <%s> plotted as boxes/steps when BoxWidth is set to a value with units of <%s>.", ppl_printUnit(x->c,&a[xn]->DataUnit,NULL,NULL,0,1,0),  ppl_printUnit(x->c,&sg->BoxWidth,NULL,NULL,1,1,0)); ppl_error(&x->c->errcontext, ERR_GENERAL, -1, -1, NULL); }
    else
     {
      unsigned char logaxis = (a[xn]->LogFinal==SW_BOOL_TRUE);
      while (blk != NULL)
       {
        for (j=0; j<blk->blockPosition; j++)
         {
          if (blk->split[j]) { FINISH_ROW_OF_BOXES; }
          ptAx=ptBx;            ptAset=ptBset;
          ptBx=ptCx; ptBy=ptCy; ptBset=ptCset;
          ptCx=logaxis?log(UUR(0)):(UUR(0)); ptCy=UUR(1); ptCset=1;
          if (ptBset)
           {
            if (ptAset) // We are processing a box in the midst of many
             {
              if      ((style == SW_STYLE_BOXES) && (sg->BoxWidth.real<1e-200)) { MAKE_BOX ((ptBx+(ptAx+ptCx)/2)/2, ptBy, (ptCx-ptAx)/4      ); }
              else if (style == SW_STYLE_BOXES)                                 { MAKE_BOX ( ptBx                 , ptBy, sg->BoxWidth.real/2); }
              else if (style == SW_STYLE_HISTEPS)                               { MAKE_STEP((ptBx+(ptAx+ptCx)/2)/2, ptBy, (ptCx-ptAx)/4      ); }
              else if (style == SW_STYLE_STEPS)  { MAKE_STEP((ptAx+ptBx)/2, ptBy, (ptBx-ptAx)/2); }
              else if (style == SW_STYLE_FSTEPS) { MAKE_STEP((ptBx+ptCx)/2, ptBy, (ptCx-ptBx)/2); }
             }
            else // The first box/step we work out the width of
             {
              if      ((style == SW_STYLE_BOXES) && (sg->BoxWidth.real<1e-200)) { MAKE_BOX ( ptBx                 , ptBy, (ptCx-ptBx)/2      ); }
              else if (style == SW_STYLE_BOXES)                                 { MAKE_BOX ( ptBx                 , ptBy, sg->BoxWidth.real/2); }
              else if (style == SW_STYLE_HISTEPS)                               { MAKE_STEP( ptBx                 , ptBy, (ptCx-ptBx)/2      ); }
              else if (style == SW_STYLE_STEPS)  { MAKE_STEP( ptBx        , ptBy, 0.0          ); }
              else if (style == SW_STYLE_FSTEPS) { MAKE_STEP((ptBx+ptCx)/2, ptBy, (ptCx-ptBx)/2); }
             }
           }
          // Work out style information for next point
          eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
          eps_core_SetColor(x, &pd->ww_final, 0);
          if (style == SW_STYLE_WBOXES) { MAKE_BOX(ptCx, ptCy, UUR(2)/2); }
         }
        blk=blk->next;
       }
      FINISH_ROW_OF_BOXES;
     }
    LineDraw_PenUp(x, ld);
   }

  else if ((style == SW_STYLE_ARROWS_HEAD) || (style == SW_STYLE_ARROWS_NOHEAD) || (style == SW_STYLE_ARROWS_TWOHEAD)) // ARROWS_HEAD, ARROWS_NOHEAD, ARROWS_TWOHEAD
   {
    double xpos2,ypos2,depth2,xap2,yap2,zap2,lw,theta;
    lw  = pd->ww_final.linewidth;

    ld = LineDraw_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z);
    last_colstr=NULL;

    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++)
       {
        // Work out style information for next point
        eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
        eps_core_SetColor(x, &pd->ww_final, 0);
        LineDraw_PenUp(x, ld);
        IF_NOT_INVISIBLE
         {
          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
          LineDraw_Point(x, ld, UUR(xn           ), UUR(yn           ), ThreeDim ? UUR(zn  ) : 0.0, 0,0,0,0,0,0, pd->ww_final.linetype, lw, last_colstr);
          LineDraw_Point(x, ld, UUR(xn+2+ThreeDim), UUR(yn+2+ThreeDim), ThreeDim ? UUR(zn+3) : 0.0, 0,0,0,0,0,0, pd->ww_final.linetype, lw, last_colstr);
          LineDraw_PenUp(x, ld);

          eps_plot_GetPosition(&xpos , &ypos , &depth , &xap , &yap , &zap , NULL, NULL, NULL, ThreeDim, UUR(xn           ), UUR(yn           ), ThreeDim ? UUR(zn  ) : 0.0, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, origin_x, origin_y, scale_x, scale_y, scale_z, 1);
          eps_plot_GetPosition(&xpos2, &ypos2, &depth2, &xap2, &yap2, &zap2, NULL, NULL, NULL, ThreeDim, UUR(xn+2+ThreeDim), UUR(yn+2+ThreeDim), ThreeDim ? UUR(zn+3) : 0.0, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, origin_x, origin_y, scale_x, scale_y, scale_z, 1);
          if ((!gsl_finite(xpos))||(!gsl_finite(ypos))||(!gsl_finite(xpos2))||(!gsl_finite(ypos2)) || (ThreeDim&&((!gsl_finite(depth))||(!gsl_finite(depth2)))) )
             { if ((blk->text[j] != NULL) && (blk->text[j][0] != '\0')) x->LaTeXpageno++; continue; }
          theta = atan2(xpos2-xpos,ypos2-ypos);
          if (!gsl_finite(theta)) theta=0.0;

          if ((style == SW_STYLE_ARROWS_TWOHEAD) && (xap>=0.0)&&(xap<=1.0)&&(yap>=0.0)&&(yap<=1.0)&&((!ThreeDim)||((zap>=0.0)&&(zap<=1.0))))
           {
            double x2=xpos, y2=ypos, x3, y3, x4, y4, x5, y5, theta_y = theta + M_PI;
            x3 = x2 - EPS_ARROW_HEADSIZE * lw * sin(theta_y - EPS_ARROW_ANGLE / 2); // Pointy back of arrowhead on one side
            y3 = y2 - EPS_ARROW_HEADSIZE * lw * cos(theta_y - EPS_ARROW_ANGLE / 2);
            x5 = x2 - EPS_ARROW_HEADSIZE * lw * sin(theta_y + EPS_ARROW_ANGLE / 2); // Pointy back of arrowhead on other side
            y5 = y2 - EPS_ARROW_HEADSIZE * lw * cos(theta_y + EPS_ARROW_ANGLE / 2);
            x4 = x2 - EPS_ARROW_HEADSIZE * lw * sin(theta_y) * (1.0 - EPS_ARROW_CONSTRICT) * cos(EPS_ARROW_ANGLE / 2); // Point where back of arrowhead crosses stalk
            y4 = y2 - EPS_ARROW_HEADSIZE * lw * cos(theta_y) * (1.0 - EPS_ARROW_CONSTRICT) * cos(EPS_ARROW_ANGLE / 2);
            sprintf(epsbuff, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\n%.2f %.2f lineto\n%.2f %.2f lineto\nclosepath\nfill\n", x4,y4,x3,y3,x2,y2,x5,y5);
            ThreeDimBuffer_writeps(x, depth, 1, lw, 0.0, 1.0, last_colstr, epsbuff);
           }

          if (((style == SW_STYLE_ARROWS_TWOHEAD) || (style == SW_STYLE_ARROWS_HEAD)) && (xap2>=0.0)&&(xap2<=1.0)&&(yap2>=0.0)&&(yap2<=1.0)&&((!ThreeDim)||((zap2>=0.0)&&(zap2<=1.0))))
           {
            double x2=xpos2, y2=ypos2, x3, y3, x4, y4, x5, y5, theta_y = theta;
            x3 = x2 - EPS_ARROW_HEADSIZE * lw * sin(theta_y - EPS_ARROW_ANGLE / 2); // Pointy back of arrowhead on one side
            y3 = y2 - EPS_ARROW_HEADSIZE * lw * cos(theta_y - EPS_ARROW_ANGLE / 2);
            x5 = x2 - EPS_ARROW_HEADSIZE * lw * sin(theta_y + EPS_ARROW_ANGLE / 2); // Pointy back of arrowhead on other side
            y5 = y2 - EPS_ARROW_HEADSIZE * lw * cos(theta_y + EPS_ARROW_ANGLE / 2);
            x4 = x2 - EPS_ARROW_HEADSIZE * lw * sin(theta_y) * (1.0 - EPS_ARROW_CONSTRICT) * cos(EPS_ARROW_ANGLE / 2); // Point where back of arrowhead crosses stalk
            y4 = y2 - EPS_ARROW_HEADSIZE * lw * cos(theta_y) * (1.0 - EPS_ARROW_CONSTRICT) * cos(EPS_ARROW_ANGLE / 2);
            sprintf(epsbuff, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\n%.2f %.2f lineto\n%.2f %.2f lineto\nclosepath\nfill\n", x4,y4,x3,y3,x2,y2,x5,y5);
            ThreeDimBuffer_writeps(x, depth2, 1, lw, 0.0, 1.0, last_colstr, epsbuff);
           }
         }

        // label point if instructed to do so
        if ((blk->text[j] != NULL) && (blk->text[j][0] != '\0'))
         {
          char *text=NULL;
          if ((xap2<0.0)||(xap2>1.0)||(yap2<0.0)||(yap2>1.0)||(ThreeDim&&((zap2<0.0)||(zap2>1.0)))) { x->LaTeXpageno++; continue; }
          if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
          canvas_EPSRenderTextItem(x, &text, x->LaTeXpageno++, xpos2/M_TO_PS, ypos2/M_TO_PS, x->current->settings.TextHAlign, x->current->settings.TextVAlign, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
          if (text!=NULL) ThreeDimBuffer_writeps(x, depth2, 1, 1, 0, 1, last_colstr, text);
         }
       }
      blk=blk->next;
     }
   }

  else if (style == SW_STYLE_FILLEDREGION) // FILLEDREGION
   {
    FilledRegionHandle *fr;
    fr = FilledRegion_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z);

    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++)
       {
        // Work out style information for next point
        eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
        FilledRegion_Point(x, fr, UUR(xn), UUR(yn));
       }
      blk=blk->next;
     }
    eps_core_SetColor(x, &pd->ww_final, 1);
    eps_core_SetFillColor(x, &pd->ww_final);
    eps_core_SwitchTo_FillColor(x,0);
    FilledRegion_Finish(x, fr, pd->ww_final.linetype, pd->ww_final.linewidth, 1);
    eps_core_SwitchFrom_FillColor(x,1);
   }

  else if (style == SW_STYLE_YERRORSHADED) // YERRORSHADED
   {
    FilledRegionHandle *fr;
    int BlkNo=0;

    fr = FilledRegion_Init(x, a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, ThreeDim, origin_x, origin_y, scale_x, scale_y, scale_z);

    // First add all of the points along the tops of the error bars, moving from left to right
    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++) FilledRegion_Point(x, fr, UUR(xn), UUR(yn));
      blk=blk->next;
      BlkNo++;
     }

    // Now add the points along the bottoms of all of the error bars, moving from right to left
    for (BlkNo-- ; BlkNo>=0; BlkNo--)
     {
      blk = data->first;
      for (j=0; j<BlkNo; j++) blk=blk->next;
      for (j=blk->blockPosition-1; j>=0; j--)
       {
        // Work out style information for next point
        eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj);
        if (xn==0) FilledRegion_Point(x, fr, UUR(0), UUR(2));
        else       FilledRegion_Point(x, fr, UUR(2), UUR(0));
       }
     }

    eps_core_SetColor(x, &pd->ww_final, 1);
    eps_core_SetFillColor(x, &pd->ww_final);
    eps_core_SwitchTo_FillColor(x,0);
    FilledRegion_Finish(x, fr, pd->ww_final.linetype, pd->ww_final.linewidth, 1);
    eps_core_SwitchFrom_FillColor(x,1);
   }

  else if ((style == SW_STYLE_SURFACE)&&(xrn==0)&&(yrn==0)&&(zrn==0)) // SURFACE
   {
    int  fill;
    int  XSize = pd->GridXSize;
    int  YSize = pd->GridYSize;
    long X,Y; long j;
    double xap,yap,zap,theta_x,theta_y,theta_z,depth;
    double x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4;

#define SURFACE_POINT(XO,YO,ZO) \
   eps_plot_GetPosition(&XO,&YO,&ZO, &xap, &yap, &zap, &theta_x, &theta_y, &theta_z, ThreeDim, UUR(xn), UUR(yn), ThreeDim ? UUR(zn) : 0.0,  a[xn], a[yn], a[zn], xrn, yrn, zrn, sg, origin_x, origin_y, scale_x, scale_y, scale_z, 1); \
   if ((xap<0)||(xap>1)||(yap<0)||(yap>1)||(zap<0)||(zap>1)) continue; \
   if ( (!gsl_finite(XO))||(!gsl_finite(YO))||(!gsl_finite(ZO)) ) continue;

    if (blk->blockPosition != ((long)XSize) * YSize)
     {
      ppl_error(&x->c->errcontext, ERR_INTERNAL,-1,-1,"Surface plot yielded data grid of the wrong size.");
     }
    else
     {
      last_colstr=NULL;
      for (fill=1; fill>=0; fill--)
       for (Y=0; Y<YSize-1; Y++)
        for (X=0; X<XSize-1; X++)
         {
          j = X + Y*((long)XSize);
          eps_plot_WithWordsFromUsingItems(x->c, &pd->ww_final, &blk->data_real[Ncolumns*j], &blk->data_obj[Ncol_obj*j], Ncolumns, Ncol_obj); // Work out style information for next point
          if (fill)
           {
            eps_core_SetFillColor(x, &pd->ww_final);
            eps_core_SwitchTo_FillColor(x,0);
           }
          else
           {
            eps_core_SetColor(x, &pd->ww_final, 0);
           }
          IF_NOT_INVISIBLE
           {
            if ((last_colstr==NULL)||(strcmp(last_colstr,x->CurrentColor)!=0)) { last_colstr = (char *)ppl_memAlloc(strlen(x->CurrentColor)+1); if (last_colstr==NULL) break; strcpy(last_colstr, x->CurrentColor); }
            SURFACE_POINT(x1,y1,z1); j++; SURFACE_POINT(x2,y2,z2); j+=XSize; SURFACE_POINT(x3,y3,z3); j--; SURFACE_POINT(x4,y4,z4); // Draw a square
            depth = (z1+z2+z3+z4)/4.0;
            if (fill) depth += 1e-6*fabs(depth);
            sprintf(epsbuff, "newpath %.2f %.2f moveto %.2f %.2f lineto %.2f %.2f lineto %.2f %.2f lineto closepath %s\n", x1,y1,x2,y2,x3,y3,x4,y4,fill?"eofill":"stroke");
            ThreeDimBuffer_writeps(x, depth, pd->ww_final.linetype, pd->ww_final.linewidth, 0.0, 1, last_colstr, epsbuff);
           }
         }
     }
   }

  else if (style == SW_STYLE_COLORMAP) // COLORMAP
   {
    // Dealt with in advance of drawing backmost axes
   }

  else if ((style == SW_STYLE_CONTOURMAP)&&(xrn==0)&&(yrn==0)&&(zrn==0)) // CONTOURMAP
   {
    // Dealt with in advance of drawing backmost axes
   }

  // End looping over monotonic regions of axis space
   }

  return 0;
 }

// Produce an icon representing a dataset on the graph's legend
void eps_plot_LegendIcon(EPSComm *x, int i, canvas_plotdesc *pd, double xpos, double ypos, double scale, pplset_axis *a1, pplset_axis *a2, pplset_axis *a3, int xn, int yn, int zn)
 {
  int            style;
  dataTable     *data;
  pplset_axis   *a[3] = {a1,a2,a3};

  data  = x->current->plotdata[i];
  style = pd->ww_final.linespoints;
  if ((data==NULL) || (data->Nrows<1)) return; // No data present

  if ((style==SW_STYLE_LINES) || (style==SW_STYLE_LINESPOINTS) || (style==SW_STYLE_IMPULSES) || (style==SW_STYLE_BOXES) || (style==SW_STYLE_WBOXES) || (style==SW_STYLE_STEPS) || (style==SW_STYLE_FSTEPS) || (style==SW_STYLE_HISTEPS) || (style==SW_STYLE_SURFACE) || (style==SW_STYLE_CONTOURMAP))
   {
    eps_core_SetColor(x, &pd->ww_final, 1);
    eps_core_SetLinewidth(x, EPS_DEFAULT_LINEWIDTH * pd->ww_final.linewidth, pd->ww_final.linetype, 0);
    IF_NOT_INVISIBLE
     {
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n", xpos-scale*0.60/2, ypos, xpos+scale*0.60/2, ypos);
      eps_core_BoundingBox(x, xpos-scale*0.60/2, ypos, pd->ww_final.linewidth);
      eps_core_BoundingBox(x, xpos+scale*0.60/2, ypos, pd->ww_final.linewidth);
     }
   }

  if ((style==SW_STYLE_POINTS) || (style==SW_STYLE_LINESPOINTS) || (style==SW_STYLE_STARS) || (style==SW_STYLE_DOTS))
   {
    double final_pointsize = pd->ww_final.pointsize;
    if (style==SW_STYLE_DOTS ) final_pointsize *= 0.05; // Dots are 1/20th size of points
    if (style==SW_STYLE_STARS) final_pointsize =  1;
    eps_core_SetColor(x, &pd->ww_final, 1);
    eps_core_SetLinewidth(x, EPS_DEFAULT_LINEWIDTH * pd->ww_final.pointlinewidth, 1, 0);
    IF_NOT_INVISIBLE
     {
      int pt = (pd->ww_final.pointtype-1) % N_POINTTYPES;
      if (style==SW_STYLE_DOTS ) pt = 16; // Dots are always pt 17 (circle)
      if (style==SW_STYLE_STARS) pt = 24; // Stars are always pt 25 (star)
      while (pt<0) pt+=N_POINTTYPES;
      x->PointTypesUsed[pt] = 1;
      fprintf(x->epsbuffer, "/ps { %f } def %.2f %.2f pt%d\n", final_pointsize * EPS_DEFAULT_PS, xpos, ypos, pt+1);
      eps_core_BoundingBox(x, xpos, ypos, 2 * final_pointsize * eps_PointSize[pt] * EPS_DEFAULT_PS);
     }
   }

  else if ((style==SW_STYLE_LOWERLIMITS) || (style==SW_STYLE_UPPERLIMITS))
   {
    double ps, sgn, eah_old=EPS_ARROW_HEADSIZE;
    ps  = pd->ww_final.pointsize * EPS_DEFAULT_PS;
    sgn = ( (style == SW_STYLE_UPPERLIMITS) ^ (a[yn]->MaxFinal > a[yn]->MinFinal) ) ? 1.0 : -1.0;
    eps_core_SetColor(x, &pd->ww_final, 1);
    eps_core_SetLinewidth(x, EPS_DEFAULT_LINEWIDTH * pd->ww_final.pointlinewidth, pd->ww_final.linetype, 0);
    IF_NOT_INVISIBLE
     {
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n", xpos-ps, ypos-ps*sgn, xpos+ps, ypos-ps*sgn);
      eps_core_BoundingBox(x, xpos-ps, ypos-ps*sgn, pd->ww_final.linewidth);
      eps_core_BoundingBox(x, xpos+ps, ypos-ps*sgn, pd->ww_final.linewidth);
      EPS_ARROW_HEADSIZE /=2;
      eps_primitive_arrow(x, SW_ARROWTYPE_HEAD, xpos, ypos-ps*sgn, NULL, xpos, ypos+ps*sgn, NULL, &pd->ww_final);
      EPS_ARROW_HEADSIZE = eah_old;
     }
   }

  else if ((style==SW_STYLE_ARROWS_HEAD) || (style==SW_STYLE_ARROWS_NOHEAD) || (style==SW_STYLE_ARROWS_TWOHEAD))
   {
    int ArrowStyle;
    if      (style==SW_STYLE_ARROWS_TWOHEAD) ArrowStyle = SW_ARROWTYPE_TWOWAY;
    else if (style==SW_STYLE_ARROWS_NOHEAD)  ArrowStyle = SW_ARROWTYPE_NOHEAD;
    else                                     ArrowStyle = SW_ARROWTYPE_HEAD;
    eps_core_SetColor(x, &pd->ww_final, 1);
    eps_core_SetLinewidth(x, EPS_DEFAULT_LINEWIDTH * pd->ww_final.pointlinewidth, pd->ww_final.linetype, 0);
    IF_NOT_INVISIBLE eps_primitive_arrow(x, ArrowStyle, xpos-scale*0.60/2, ypos, NULL, xpos+scale*0.60/2, ypos, NULL, &pd->ww_final);
   }

  else if ((style == SW_STYLE_FILLEDREGION) || (style == SW_STYLE_YERRORSHADED) || (style == SW_STYLE_COLORMAP))
   {
    double s=scale*0.45/2;
    eps_core_SetColor(x, &pd->ww_final, 1);
    eps_core_SetFillColor(x, &pd->ww_final);
    eps_core_SwitchTo_FillColor(x,1);
    IF_NOT_INVISIBLE
     {
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto %.2f %.2f lineto %.2f %.2f lineto closepath fill\n", xpos-s, ypos-s, xpos+s, ypos-s, xpos+s, ypos+s, xpos-s, ypos+s);
      eps_core_BoundingBox(x, xpos-s, ypos-s, 0);
      eps_core_BoundingBox(x, xpos+s, ypos-s, 0);
      eps_core_BoundingBox(x, xpos-s, ypos+s, 0);
      eps_core_BoundingBox(x, xpos+s, ypos+s, 0);
     }
    eps_core_SwitchFrom_FillColor(x,1);
    eps_core_SetLinewidth(x, EPS_DEFAULT_LINEWIDTH * pd->ww_final.pointlinewidth, pd->ww_final.linetype, 0);
    IF_NOT_INVISIBLE
     {
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto %.2f %.2f lineto %.2f %.2f lineto closepath stroke\n", xpos-s, ypos-s, xpos+s, ypos-s, xpos+s, ypos+s, xpos-s, ypos+s);
      eps_core_BoundingBox(x, xpos-s, ypos-s, pd->ww_final.linewidth);
      eps_core_BoundingBox(x, xpos+s, ypos-s, pd->ww_final.linewidth);
      eps_core_BoundingBox(x, xpos-s, ypos+s, pd->ww_final.linewidth);
      eps_core_BoundingBox(x, xpos+s, ypos+s, pd->ww_final.linewidth);
     }
   }

  else if ((style==SW_STYLE_XERRORBARS)||(style==SW_STYLE_YERRORBARS)||(style==SW_STYLE_ZERRORBARS)||(style==SW_STYLE_XYERRORBARS)||(style==SW_STYLE_XZERRORBARS)||(style==SW_STYLE_XZERRORBARS)||(style==SW_STYLE_XZERRORBARS)||(style==SW_STYLE_XERRORRANGE)||(style==SW_STYLE_YERRORRANGE)||(style==SW_STYLE_ZERRORRANGE)||(style==SW_STYLE_XYERRORRANGE)||(style==SW_STYLE_XZERRORRANGE)||(style==SW_STYLE_YZERRORRANGE))
   {
    double s  = scale*0.6/2;
    double b  = 0.0005 * x->current->settings.bar * M_TO_PS;
    double ps = pd->ww_final.pointsize * EPS_DEFAULT_PS;

    eps_core_SetColor(x, &pd->ww_final, 1);
    IF_NOT_INVISIBLE
     {
      eps_core_SetLinewidth(x, EPS_DEFAULT_LINEWIDTH * pd->ww_final.linewidth, pd->ww_final.linetype, 0);
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n",xpos-s,ypos   ,xpos+s,ypos   );
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n",xpos-s,ypos-b ,xpos-s,ypos+b );
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n",xpos+s,ypos-b ,xpos+s,ypos+b );
      fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto stroke\n",xpos  ,ypos-ps,xpos  ,ypos+ps);
      eps_core_BoundingBox(x, xpos-s, ypos-ps, pd->ww_final.linewidth);
      eps_core_BoundingBox(x, xpos-s, ypos+ps, pd->ww_final.linewidth);
      eps_core_BoundingBox(x, xpos+s, ypos-ps, pd->ww_final.linewidth);
      eps_core_BoundingBox(x, xpos+s, ypos+ps, pd->ww_final.linewidth);
     }
   }

  return;
 }

