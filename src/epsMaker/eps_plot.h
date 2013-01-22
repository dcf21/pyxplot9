// eps_plot.h
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

#ifndef _PPL_EPS_PLOT_H
#define _PPL_EPS_PLOT_H 1

#include "coreUtils/list.h"
#include "expressions/expCompile.h"
#include "settings/settings.h"
#include "epsMaker/eps_comm.h"
#include "userspace/pplObj.h"

int  eps_plot_AddUsingItemsForWithWords(ppl_context *c, withWords *ww, int *NExpect, unsigned char *autoUsingList, pplExpr ***usingList, int *Nusing, int *NObjs, char *errtext);
void eps_plot_WithWordsFromUsingItems(ppl_context *c, withWords *ww, double *DataRow, pplObj *ObjRow, int Ncolumns_real, int Ncolumns_obj);
int  eps_plot_WithWordsCheckUsingItemsDimLess(ppl_context *c, withWords *ww, pplObj *firstValues, int Ncolumns_real, int Ncolumns_obj, int *NDataCols);

void eps_plot_ReadAccessibleData(EPSComm *x);
void eps_plot_SampleFunctions(EPSComm *x);
void eps_plot_DecideAxisRanges(EPSComm *x);
void eps_plot_YieldUpText(EPSComm *x);
void eps_plot_RenderEPS(EPSComm *x);


#endif

