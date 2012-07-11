// eps_plot_ticking_auto3.c
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

// This file contain an algorithm for the automatic placement of ticks along axes.

// METHOD 3: This axis is linked to another axis of the same length. Use the
// same ticking scheme.

#define _PPL_EPS_PLOT_TICKING_AUTO3_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"
#include "mathsTools/dcfmath.h"
#include "stringTools/asciidouble.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/unitsDisp.h"

#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_ticking.h"
#include "epsMaker/eps_plot_ticking_auto2.h"
#include "epsMaker/eps_plot_ticking_auto3.h"

void eps_plot_ticking_auto3(EPSComm *x, pplset_axis *axis, double UnitMultiplier, unsigned char *AutoTicks, pplset_axis *linkedto)
 {
  int i, Nticks_maj, Nticks_min, OutContext;

  if (DEBUG) ppl_log(&x->c->errcontext,"Using eps_plot_ticking_auto3()");
  OutContext = ppl_memAlloc_GetMemContext();

  if (linkedto==NULL) goto FAIL;
  if (axis->linkusing != NULL) goto FAIL;
  if (linkedto->PhysicalLengthMinor != axis->PhysicalLengthMinor) goto FAIL;
  if ((linkedto->TickListStrings==NULL)||(linkedto->MTickListStrings==NULL)) goto FAIL;

  for (Nticks_maj=0; linkedto-> TickListStrings[Nticks_maj]!=NULL; Nticks_maj++);
  for (Nticks_min=0; linkedto->MTickListStrings[Nticks_min]!=NULL; Nticks_min++);

  axis-> TickListPositions = linkedto-> TickListPositions;
  axis->MTickListPositions = linkedto->MTickListPositions;

  if (axis->format==NULL)
   {
    axis-> TickListStrings = linkedto-> TickListStrings;
   }
  else
   {
    axis-> TickListStrings   = (char   **)ppl_memAlloc((Nticks_maj+1) * sizeof(char *));
    if (axis->TickListStrings==NULL) goto FAIL;

    for (i=0; i<Nticks_maj; i++)
     {
      double xtmpA,xtmp,xtmpB;
      xtmpA = eps_plot_axis_InvGetPosition(axis->TickListPositions[i]-1e8, axis) * UnitMultiplier;
      xtmp  = eps_plot_axis_InvGetPosition(axis->TickListPositions[i]    , axis) * UnitMultiplier;
      xtmpB = eps_plot_axis_InvGetPosition(axis->TickListPositions[i]    , axis) * UnitMultiplier;
      if ((gsl_finite(xtmpA))&&(gsl_finite(xtmpB))&&(((xtmpA<=0.0)&&(xtmpB>=0.0))||((xtmpA>=0.0)&&(xtmpB<=0.0)))) xtmp=0.0;
      TickLabelFromFormat(x, &axis->TickListStrings[i], axis->format, xtmp, &axis->DataUnit, axis->xyz, OutContext);
     }
    axis->TickListStrings[i] = NULL;
   }

  axis->MTickListStrings   = (char   **)ppl_memAlloc((Nticks_min+1) * sizeof(char *));
  if (axis->MTickListStrings == NULL) goto FAIL;
  for (i=0; i<Nticks_min; i++) axis->MTickListStrings[i] = "";
  axis->MTickListStrings[i] = NULL;

  // Finished
  goto CLEANUP;

FAIL:
  if (DEBUG) ppl_log(&x->c->errcontext,"eps_plot_ticking_auto3() has failed");
  eps_plot_ticking_auto2(x, axis, UnitMultiplier, AutoTicks, linkedto);

CLEANUP:
  return;
 }

