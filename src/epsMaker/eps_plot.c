// eps_plot.c
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

#define _PPL_EPS_PLOT 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/memAlloc.h"
#include "settings/settingTypes.h"
#include "settings/colors.h"
#include "settings/withWords_fns.h"
#include "mathsTools/dcfmath.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"
#include "datafile.h"
#include "commands/interpolate_2d_engine.h"
#include "epsMaker/canvasDraw.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_plot.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_colourmap.h"
#include "epsMaker/eps_plot_contourmap.h"
#include "epsMaker/eps_plot_styles.h"
#include "epsMaker/eps_plot_threedimbuff.h"
#include "epsMaker/eps_settings.h"

// If a plot dataset has any with_words of the form "with linewidth $4", these
// need to be evaluated for every datapoint. We do this by adding additional
// items to the UsingList for these datasets. First, we need to check that the
// UsingList supplied by the user is of an acceptable form. If it is of the
// wrong length, we do nothing; it will fail in due course in ppl_datafile
// anyway. If the list is empty, we auto-generate a default list.

int eps_plot_AddUsingItemsForWithWords(ppl_context *c, withWords *ww, int *NExpect, unsigned char *AutoUsingList, list *UsingList, int *NObjs)
 {
  int i, UsingLen;
  char *AutoItem, *temp, *temp2;

  *NObjs = 0;

  *AutoUsingList = 0;
  UsingLen = ppl_listLen(UsingList);
  if (ww->linespoints == SW_STYLE_CONTOURMAP) return 0; // Contourplot evaluate expressions in terms of c1

  // If using list was empty, generate an automatic list before we start
  if (UsingLen==0)
   {
    for (i=0; i<*NExpect; i++)
     {
      AutoItem = (char *)ppl_memAlloc(10);
      if (AutoItem == NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1, "Out of memory"); return 1; }
      sprintf(AutoItem, "%d", i+1); // !!! NEED TO COMPILE TO EXPRESSION
      ppl_listAppend(UsingList, (void *)AutoItem);
     }
    UsingLen = *NExpect;
    *AutoUsingList = 1;
   }
  else if ((UsingLen==1) && (*NExpect==2)) // Prepend data point number if only one number specified in using statement
   {
    temp = (char *)ppl_listPop(UsingList);
    temp2 = (char *)ppl_memAlloc(2);
    if (temp2==NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1, "Out of memory"); return 1; }
    strcpy(temp2, "0"); // !!! NEED TO COMPILE TO EXPRESSION
    ppl_listAppend(UsingList, (void *)temp2);
    ppl_listAppend(UsingList, (void *)temp);
    UsingLen++;
   }

  // If using list is wrong length, give up and let ppl_datafile return an error
  if (UsingLen != *NExpect) return 0;

#define ADD_FAKE_USING_ITEM(X) \
 { \
  ppl_listAppend(UsingList, (void *)X); \
  (*NExpect)++; \
 }

  // Now cycle through all with_words which can be item-specific
  if (ww->EXPlinetype       != NULL)   ADD_FAKE_USING_ITEM(ww->EXPlinetype      );
  if (ww->EXPlinewidth      != NULL)   ADD_FAKE_USING_ITEM(ww->EXPlinewidth     );
  if (ww->EXPpointlinewidth != NULL)   ADD_FAKE_USING_ITEM(ww->EXPpointlinewidth);
  if (ww->EXPpointsize      != NULL)   ADD_FAKE_USING_ITEM(ww->EXPpointsize     );
  if (ww->EXPpointtype      != NULL)   ADD_FAKE_USING_ITEM(ww->EXPpointtype     );
  if (ww->EXPcolor          != NULL) { ADD_FAKE_USING_ITEM(ww->EXPcolor         ); (*NObjs)++; }
  if (ww->EXPfillcolor      != NULL) { ADD_FAKE_USING_ITEM(ww->EXPfillcolor     ); (*NObjs)++; }

  return 0;
 }

#define PROJ_DBL \
 { \
  dbl = DataRow[i--]; \
  if (i<0) i=0; \
  if (!gsl_finite(dbl)) dbl=0.0; \
 }

#define PROJ0_1 \
 { \
  PROJ_DBL; \
  if (dbl < 0.0) dbl= 0.0; \
  if (dbl > 1.0) dbl= 1.0; \
 }

#define PROJ_INT \
 { \
  PROJ_DBL; \
  if (dbl < INT_MIN) dbl=INT_MIN+1; \
  if (dbl > INT_MAX) dbl=INT_MAX-1; \
 }

void eps_plot_WithWordsFromUsingItems(ppl_context *c, withWords *ww, double *DataRow, pplObj *ObjRow, int Ncolumns, int Ncol_obj)
 {
  int i=Ncolumns-1, j=Ncol_obj-1;
  double dbl;

  if (ww->linespoints == SW_STYLE_CONTOURMAP) return; // Contourplot evaluate expressions in terms of c1

  if (ww->EXPfillcolor != NULL)
   {
    ppl_colorFromObj(c, &ObjRow[j--], &ww->color, &ww->Col1234Space, &ww->EXPcolor, &ww->color1, &ww->color2, &ww->color3, &ww->color4, &ww->USEcolor, &ww->USEcolor1234);
   }

  if (ww->EXPcolor != NULL)
   {
    ppl_colorFromObj(c, &ObjRow[j--], &ww->color, &ww->Col1234Space, &ww->EXPcolor, &ww->color1, &ww->color2, &ww->color3, &ww->color4, &ww->USEcolor, &ww->USEcolor1234);
   }

  if (ww->EXPpointtype      != NULL) { PROJ_INT ; ww->USEpointtype      = 1; ww->pointtype      = (int)dbl; ww->AUTOpointtype = 0; }
  if (ww->EXPpointsize      != NULL) { PROJ_DBL ; ww->USEpointsize      = 1; ww->pointsize      = dbl; }
  if (ww->EXPpointlinewidth != NULL) { PROJ_DBL ; ww->USEpointlinewidth = 1; ww->pointlinewidth = dbl; }
  if (ww->EXPlinewidth      != NULL) { PROJ_DBL ; ww->USElinewidth      = 1; ww->linewidth      = dbl; }
  if (ww->EXPlinetype       != NULL) { PROJ_INT ; ww->USElinetype       = 1; ww->linetype       = (int)dbl; ww->AUTOlinetype  = 0; }

  return;
 }

#define WWCUID(X) \
 if (!FirstValues[i].dimensionless) { sprintf(c->errcontext.tempErrStr, "The expression specified for the %s should have been dimensionless, but instead had units of <%s>. Cannot plot this dataset.", X, ppl_printUnit(c, FirstValues+i, NULL, NULL, 0, 1, 0)); ppl_error(&c->errcontext, ERR_NUMERIC, -1, -1, NULL); return 1; } \
 i--; \
 if (i<0) i=0;

int eps_plot_WithWordsCheckUsingItemsDimLess(ppl_context *c, withWords *ww, pplObj *FirstValues, int Ncolumns, int Ncolumns_obj, int *NDataCols)
 {
  int i = Ncolumns-1;

  if (ww->linespoints == SW_STYLE_CONTOURMAP) return 0; // Contourplot evaluate expressions in terms of c1

  if (ww->EXPpointtype      != NULL) { WWCUID("point type"); }
  if (ww->EXPpointsize      != NULL) { WWCUID("point size"); }
  if (ww->EXPpointlinewidth != NULL) { WWCUID("point line width"); }
  if (ww->EXPlinewidth      != NULL) { WWCUID("line width"); }
  if (ww->EXPlinetype       != NULL) { WWCUID("line type"); }
  if (NDataCols!=NULL) *NDataCols=i+1; // The number of columns which contain data which is not from with .... expressions
  return 0;
 }

