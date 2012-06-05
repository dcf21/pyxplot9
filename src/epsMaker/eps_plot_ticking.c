// eps_plot_ticking.c
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

// This file contains routines for working out the ranges of axes, and where to
// put axis ticks along them

#define _PPL_EPS_PLOT_TICKING_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "mathsTools/dcfmath.h"
#include "stringTools/asciidouble.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "epsMaker/eps_core.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_ticking.h"
#include "epsMaker/eps_plot_ticking_auto.h"
#include "epsMaker/eps_plot_ticking_auto2.h"
#include "epsMaker/eps_plot_ticking_auto3.h"

void eps_plot_ticking(EPSComm *x, pplset_axis *axis, int AxisUnitStyle, pplset_axis *linkedto)
 {
  int i,j,MajMin,N,xrn;
  const double logmin = 1e-10;
  double UnitMultiplier=1.0;
  char *UnitString=NULL;
  pplObj CentralValue;
  unsigned char AutoTicks[2] = {0,0};

  if (!axis->FinalActive) { axis->RangeFinalised = 0; return; } // Axis is not in use

  // First of all, work out what axis range to use
  if (!axis->RangeFinalised)
   {
    // Work out axis range
    unsigned char MinSet=1;

    if       (axis->HardMinSet) axis->MinFinal = axis->HardMin;
    else if  (axis->MinUsedSet) axis->MinFinal = axis->MinUsed;
    else                        MinSet = 0;

    if       (axis->HardMaxSet) axis->MaxFinal = axis->HardMax;
    else if  (axis->MaxUsedSet) axis->MaxFinal = axis->MaxUsed;
    else if  (MinSet)           axis->MaxFinal = (axis->LogFinal == SW_BOOL_TRUE) ? axis->MinFinal * 100
                                                                                  : axis->MinFinal +  20;
    else                        axis->MaxFinal = (axis->LogFinal == SW_BOOL_TRUE) ? 10.0 : 10.0;

    // Check that log axes do not venture too close to zero
    if ((axis->LogFinal == SW_BOOL_TRUE) && (axis->MaxFinal <= 1e-200)) { axis->MaxFinal = logmin; sprintf(x->c->errcontext.tempErrStr, "Range for logarithmic axis %c%d set below zero; defaulting to 1e-10.", "xyzc"[axis->xyz], axis->axis_n); ppl_warning(&x->c->errcontext, ERR_NUMERICAL, NULL); }
    if (!MinSet) axis->MinFinal = (axis->LogFinal == SW_BOOL_TRUE) ? (axis->MaxFinal / 100) : (axis->MaxFinal - 20);
    if ((axis->LogFinal == SW_BOOL_TRUE) && (axis->MinFinal <= 1e-200)) { axis->MinFinal = logmin; sprintf(x->c->errcontext.tempErrStr, "Range for logarithmic axis %c%d set below zero; defaulting to 1e-10.", "xyzc"[axis->xyz], axis->axis_n); ppl_warning(&x->c->errcontext, ERR_NUMERICAL, NULL); }

    // If there's no spread of data on the axis, make a spread up
    if ( (fabs(axis->MinFinal-axis->MaxFinal) <= fabs(1e-14*axis->MinFinal)) || (fabs(axis->MinFinal-axis->MaxFinal) <= fabs(1e-14*axis->MaxFinal)) )
     {
      if (axis->HardMinSet && axis->HardMaxSet) { sprintf(x->c->errcontext.tempErrStr, "Specified minimum and maximum range limits for axis %c%d are equal; reverting to alternative limits.", "xyzc"[axis->xyz], axis->axis_n); ppl_warning(&x->c->errcontext, ERR_NUMERICAL, NULL); }
      if (axis->LogFinal != SW_BOOL_TRUE)
       {
        double step = ppl_max(1.0,1e-3*fabs(axis->MinFinal));
        axis->MinFinal -= step; axis->MaxFinal += step;
       }
      else
       {
        if (axis->MinFinal > 1e-300) axis->MinFinal /= 10.0;
        if (axis->MaxFinal <  1e300) axis->MaxFinal *= 10.0;
       }
     }

   }

  // Finalise the physical unit to be associated with data on this axis
  if (!axis->DataUnitSet)
   {
    if (axis->HardUnitSet) { axis->DataUnitSet=1; axis->DataUnit=axis->HardUnit; }
    else                   { pplObjNum(&axis->DataUnit,0,0,0); }
   }
  CentralValue = axis->DataUnit;
  CentralValue.flagComplex = 0;
  CentralValue.imag = 0.0;
  CentralValue.real = (axis->format==NULL) ? eps_plot_axis_InvGetPosition(0.5, axis) : 1.0;
  if (CentralValue.real==0.0) CentralValue.real = eps_plot_axis_InvGetPosition(0.25, axis);
  UnitString = ppl_printUnit(x->c,&CentralValue,&UnitMultiplier,NULL,0,0,SW_DISPLAY_L);
  UnitMultiplier /= CentralValue.real;
  if (!gsl_finite(UnitMultiplier)) UnitMultiplier=1.0;

  if (!axis->RangeFinalised)
   {
    double min_prelim, max_prelim, OoM;

    // If axis does not have a user-specified range, round it outwards towards a round endpoint
    min_prelim = axis->MinFinal * UnitMultiplier;
    max_prelim = axis->MaxFinal * UnitMultiplier;

    if (axis->LogFinal == SW_BOOL_TRUE) { min_prelim = log10(min_prelim); max_prelim = log10(max_prelim); }

    OoM = pow(10.0, floor(log10(fabs(max_prelim - min_prelim)/5)));
    min_prelim = floor(min_prelim / OoM) * OoM;
    max_prelim = ceil (max_prelim / OoM) * OoM;

    if (axis->LogFinal == SW_BOOL_TRUE) { min_prelim = pow(10.0,min_prelim); max_prelim = pow(10.0,max_prelim); }

    min_prelim /= UnitMultiplier;
    max_prelim /= UnitMultiplier;

    if (gsl_finite(min_prelim) && (!axis->HardMinSet) && ((axis->LogFinal!=SW_BOOL_TRUE)||(min_prelim>1e-300))) axis->MinFinal = min_prelim;
    if (gsl_finite(max_prelim) && (!axis->HardMaxSet) && ((axis->LogFinal!=SW_BOOL_TRUE)||(min_prelim>1e-300))) axis->MaxFinal = max_prelim;

    // Print out debugging report
    if (DEBUG)
     {
      sprintf(x->c->errcontext.tempErrStr,"Determined range for axis %c%d of plot %d. Usage was [", "xyzc"[axis->xyz], axis->axis_n, axis->canvas_id);
      i = strlen(x->c->errcontext.tempErrStr);
      if (axis->MinUsedSet) { sprintf(x->c->errcontext.tempErrStr+i, "%f", axis->MinUsed); i+=strlen(x->c->errcontext.tempErrStr+i); }
      else                  x->c->errcontext.tempErrStr[i++] = '*';
      x->c->errcontext.tempErrStr[i++] = ':';
      if (axis->MaxUsedSet) { sprintf(x->c->errcontext.tempErrStr+i, "%f", axis->MaxUsed); i+=strlen(x->c->errcontext.tempErrStr+i); }
      else                  x->c->errcontext.tempErrStr[i++] = '*';
      sprintf(x->c->errcontext.tempErrStr+i,"]. Final range was [%f:%f].",axis->MinFinal,axis->MaxFinal);
      ppl_log(&x->c->errcontext,NULL);
     }

    // Flip axis range if it is reversed
    if (axis->RangeReversed)
     {
      double swap;
      swap=axis->MinFinal; axis->MinFinal=axis->MaxFinal; axis->MaxFinal=swap;
     }

    // Set flag to show that we have finalised the range of this axis
    axis->RangeFinalised = 1;
   }

  // Secondly, decide what ticks to place on this axis
  if (!axis->TickListFinalised)
   {
    int OutContext;

    // If ticks have been manually specified, check that units are right
    if ((!ppl_unitsDimEqual(&axis->unit,&axis->DataUnit)) && ((axis->tics.tickList!=NULL)||(((axis->log==SW_BOOL_TRUE)?(axis->tics.tickMinSet):(axis->tics.tickStepSet))!=0)||(axis->ticsM.tickList!=NULL)||(((axis->log==SW_BOOL_TRUE)?(axis->ticsM.tickMinSet):(axis->ticsM.tickStepSet))!=0)))
     {
      sprintf(x->c->errcontext.tempErrStr, "Cannot put any ticks on axis %c%d because their positions are specified in units of <%s> whilst the axis has units of <%s>.", "xyzc"[axis->xyz], axis->axis_n, ppl_printUnit(x->c,&axis->unit,NULL,NULL,0,1,0), ppl_printUnit(x->c,&axis->DataUnit,NULL,NULL,1,1,0));
      ppl_error(&x->c->errcontext,ERR_GENERIC,-1,-1,NULL);
      axis->TickListPositions = NULL; axis->TickListStrings = NULL;
      axis->TickListPositions = NULL; axis->TickListStrings = NULL;
      return;
     }

    OutContext = ppl_memAlloc_GetMemContext();

    // Finalise the label to be placed on the axis, quoting a physical unit as necessary
    if ((axis->DataUnit.dimensionless) || (axis->format != NULL))
     { axis->FinalAxisLabel = axis->label; } // No units to append
    else
     {
      i = 1024;
      if (axis->label != NULL) i+=strlen(axis->label);
      axis->FinalAxisLabel = (char *)ppl_memAlloc(i);
      if (axis->FinalAxisLabel==NULL) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1, "Out of memory (u)."); return; }
      if      (AxisUnitStyle == SW_AXISUNITSTY_BRACKET) sprintf(axis->FinalAxisLabel, "%s ($%s$)", (axis->label != NULL)?axis->label:"", UnitString);
      else if (AxisUnitStyle == SW_AXISUNITSTY_RATIO)   sprintf(axis->FinalAxisLabel, "%s / $%s$", (axis->label != NULL)?axis->label:"", UnitString);
      else                                              sprintf(axis->FinalAxisLabel, "%s [$%s$]", (axis->label != NULL)?axis->label:"", UnitString);
     }

    // Minor ticks. Then major ticks.
    for (MajMin=0; MajMin<2; MajMin++)
     {
      double        TickMax, TickMin, TickStep;
      unsigned char TickMaxSet, TickMinSet, TickStepSet;
      double       *TickList;
      char        **TickStrs;
      double      **TickListPositions;
      char       ***TickListStrings;

      if (MajMin==0) { TickMax = axis->ticsM.tickMax; TickMin = axis->ticsM.tickMin; TickStep = axis->ticsM.tickStep; TickMaxSet = axis->ticsM.tickMaxSet; TickMinSet = axis->ticsM.tickMinSet; TickStepSet = axis->ticsM.tickStepSet; TickList = axis->ticsM.tickList; TickStrs = axis->ticsM.tickStrs; TickListPositions = &axis->MTickListPositions; TickListStrings = &axis->MTickListStrings; }
      else           { TickMax = axis->tics .tickMax; TickMin = axis->tics .tickMin; TickStep = axis->tics .tickStep; TickMaxSet = axis->tics .tickMaxSet; TickMinSet = axis->tics .tickMinSet; TickStepSet = axis->tics .tickStepSet; TickList = axis->tics .tickList; TickStrs = axis->tics .tickStrs; TickListPositions = &axis->TickListPositions;  TickListStrings = &axis->TickListStrings;  }

      if (TickList != NULL) // Ticks have been specified as an explicit list
       {
        for (N=0; TickStrs[N]!=NULL; N++); // Find length of list of ticks
        *TickListPositions = (double  *)ppl_memAlloc((N+1) * (axis->AxisValueTurnings+1) * sizeof(double));
        *TickListStrings   = (char   **)ppl_memAlloc((N+1) * (axis->AxisValueTurnings+1) * sizeof(char *));
        if ((*TickListPositions==NULL) || (*TickListStrings==NULL)) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1, "Out of memory (v)."); *TickListPositions = NULL; *TickListStrings = NULL; return; }
        for (i=j=0; i<N; i++)
         for (xrn=0; xrn<=axis->AxisValueTurnings; xrn++)
          {
           (*TickListPositions)[j] = eps_plot_axis_GetPosition(TickList[i], axis, xrn, 0);
           if ( (!gsl_finite((*TickListPositions)[j])) || ((*TickListPositions)[j]<0.0) || ((*TickListPositions)[j]>1.0) ) continue; // Filter out ticks which are off the end of the axis
           if      (TickStrs[i][0]!='\xFF')    (*TickListStrings)[j] = TickStrs[i];
           else if (MajMin==0)                 (*TickListStrings)[j] = "";
           else if (axis->format == NULL)        TickLabelAutoGen(x, &(*TickListStrings)[j], TickList[i] * UnitMultiplier, axis->tics.logBase , OutContext);
           else                                  TickLabelFromFormat(x, &(*TickListStrings)[j], axis->format, TickList[i], &axis->DataUnit, axis->xyz, OutContext);
           j++;
          }
        (*TickListStrings)[j] = NULL; // null terminate list
       }
      else if (TickStepSet)
       {
        double TMin, TStep, TMax, tmp;
        unsigned char inverted=0;
        *TickListPositions = (double  *)ppl_memAlloc(102 * (axis->AxisValueTurnings+1) * sizeof(double));
        *TickListStrings   = (char   **)ppl_memAlloc(102 * (axis->AxisValueTurnings+1) * sizeof(char *));
        if ((*TickListPositions==NULL) || (*TickListStrings==NULL)) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1, "Out of memory (w)."); *TickListPositions = NULL; *TickListStrings = NULL; return; }
        TStep= TickStep;
        if (TStep<0) { TStep=-TStep; inverted=1; }
        if (axis->LogFinal == SW_BOOL_TRUE) { if (TStep<1) { TStep=1/TStep; inverted=1; } else inverted=0; }

        for (xrn=j=0; xrn<=axis->AxisValueTurnings; xrn++)
         {
          if (axis->AxisLinearInterpolation!=NULL)
           {
            double RegionMin, RegionMax, first;
            first     = axis->AxisLinearInterpolation[axis->AxisTurnings[0    ]];
            RegionMin = axis->AxisLinearInterpolation[axis->AxisTurnings[xrn  ]];
            RegionMax = axis->AxisLinearInterpolation[axis->AxisTurnings[xrn+1]];
            if (RegionMax<RegionMin) { tmp=RegionMin; RegionMin=RegionMax; RegionMax=tmp; }

            if (!inverted) { TMin = TickMinSet ? TickMin : first;  TMax = TickMaxSet ? TickMax : RegionMax; }
            else           { TMin = TickMaxSet ? TickMax : first;  TMax = TickMinSet ? TickMin : RegionMax; }

            if ((TMin < RegionMin) || ((!inverted)&&(!TickMinSet)) || ((inverted)&&(!TickMaxSet)))
             {
              if (axis->LogFinal == SW_BOOL_TRUE) TMin *= exp(ceil ((log(RegionMin / TMin))/log(TStep)) * log(TStep));
              else                                TMin +=     ceil ((    RegionMin - TMin )/    TStep ) *     TStep  ;
             }
            if (TMax > RegionMax)
             {
              if (axis->LogFinal == SW_BOOL_TRUE) TMax /= exp(floor((log(TMax / RegionMax))/log(TStep)) * log(TStep));
              else                                TMax -=     floor((    TMax - RegionMax )/    TStep ) *     TStep  ;
             }
           }
          else
           {
            if (!inverted) { TMin = TickMinSet ? TickMin : ppl_min(axis->MinFinal , axis->MaxFinal);  TMax = TickMaxSet ? TickMax : ppl_max(axis->MinFinal , axis->MaxFinal); }
            else           { TMin = TickMaxSet ? TickMax : ppl_min(axis->MinFinal , axis->MaxFinal);  TMax = TickMinSet ? TickMin : ppl_max(axis->MinFinal , axis->MaxFinal); }

            if (TMin < ppl_min(axis->MinFinal , axis->MaxFinal))
             {
              if (axis->LogFinal == SW_BOOL_TRUE) TMin *= exp(ceil ((log(axis->MinFinal / TMin))/log(TStep)) * log(TStep));
              else                                TMin +=     ceil ((    axis->MinFinal - TMin )/    TStep ) *     TStep  ;
             }
            if (TMax > ppl_max(axis->MinFinal , axis->MaxFinal))
             {
              if (axis->LogFinal == SW_BOOL_TRUE) TMax /= exp(floor((log(TMax / axis->MaxFinal))/log(TStep)) * log(TStep));
              else                                TMax -=     floor((    TMax - axis->MaxFinal )/    TStep ) *     TStep  ;
             }
           }
          for (i=0; i<100; i++)
           {
            if (!inverted)
             {
              if (axis->LogFinal == SW_BOOL_TRUE) tmp = TMin * pow(TStep, i);
              else                                tmp = TMin + i*TStep;
             } else {
              if (axis->LogFinal == SW_BOOL_TRUE) tmp = TMax * pow(TStep,-i);
              else                                tmp = TMax - i*TStep;
             }
            if (((!inverted)&&(tmp>TMax)) || ((inverted)&&(tmp<TMin))) break;
            if ((axis->LogFinal != SW_BOOL_TRUE) && (fabs(tmp)<fabs(TStep*1e-14))) tmp=0; // Enforce that ticks close to zero are at zero
            (*TickListPositions)[j] = eps_plot_axis_GetPosition(tmp, axis, xrn, 0);
            if ( (!gsl_finite((*TickListPositions)[j])) || ((*TickListPositions)[j]<0.0) || ((*TickListPositions)[j]>1.0) ) continue; // Filter out ticks which are off the end of the axis
            if      (MajMin==0)            (*TickListStrings)[j] = "";
            else if (axis->format == NULL) TickLabelAutoGen(x, &(*TickListStrings)[j], tmp * UnitMultiplier, axis->tics.logBase, OutContext);
            else                           TickLabelFromFormat(x, &(*TickListStrings)[j], axis->format, tmp, &axis->DataUnit, axis->xyz, OutContext);
            if ((*TickListStrings)[j]==NULL) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1, "Out of memory (x)."); *TickListPositions = NULL; *TickListStrings = NULL; return; }
            j++;
           }
         }
        (*TickListStrings)[j] = NULL; // null terminate list
       }
      else
       {
        AutoTicks[MajMin] = 1;
       }
     }

    // Do automatic ticking as required
    if (AutoTicks[1])
     {
      if ((axis->format != NULL) || (axis->AxisLinearInterpolation != NULL))
        eps_plot_ticking_auto(x, axis, UnitMultiplier, AutoTicks, linkedto);
      else
       {
        if (linkedto!=NULL) eps_plot_ticking_auto3(x, axis, UnitMultiplier, AutoTicks, linkedto);
        else                eps_plot_ticking_auto2(x, axis, UnitMultiplier, AutoTicks, linkedto);
       }
     }

    // Set flag to show that we have finalised the ticking of this axis
    axis->TickListFinalised = 1;
   }

  return;
 }

void TickLabelAutoGen(EPSComm *X, char **output, double x, double log_base, int OutContext)
 {
  int    SF = X->c->set->term_current.SignificantFigures;
  double ApproxMargin;

  ApproxMargin = pow(10,-SF+1);
  if (ApproxMargin < 1e-15) ApproxMargin = 1e-15;

  if ((fabs(x)<DBL_MIN*100) || ((fabs(x)>1e-3) && (fabs(x)<1e5))) { sprintf(X->c->errcontext.tempErrStr,"%s",ppl_numericDisplay(x,X->c->numdispBuff[0],SF,1)); }
  else
   {
    double e,m;
    unsigned char sgn=0;
    if (x<0) { sgn=1; x=-x; }
    e = floor(log(x)/log(log_base));
    m = x / pow(log_base,e);
    if (fabs(m-log_base)<ApproxMargin) { e++; m=1; } // Avoid 10 x 10^2
    if (ppl_dblApprox(m,1,pow(10,-SF+1))) sprintf(X->c->errcontext.tempErrStr,"%s%d^{%s}",sgn?"-":"",(int)log_base,ppl_numericDisplay(e,X->c->numdispBuff[0],SF,1));
    else                                  sprintf(X->c->errcontext.tempErrStr,"%s%s\\times %d^{%s}",sgn?"-":"",ppl_numericDisplay(m,X->c->numdispBuff[0],SF,1),(int)log_base,ppl_numericDisplay(e,X->c->numdispBuff[1],SF,1));
   }
  *output = (char *)ppl_memAlloc_incontext(strlen(X->c->errcontext.tempErrStr)+3, OutContext);
  if ((*output)==NULL) return;
  sprintf(*output,"$%s$",X->c->errcontext.tempErrStr);
  return;
 }

void TickLabelFromFormat(EPSComm *X, char **output, pplExpr *FormatExp, double x, pplObj *xunit, int xyz, int OutContext)
 {
  int       lOP;
  const int stkLevelOld = X->c->stackPtr;
  pplObj   *outval;
  char     *VarName, *tmp_string;
  pplObj    DummyTemp, *VarVal;

  if      (xyz==0) VarName = "x";
  else if (xyz==1) VarName = "y";
  else if (xyz==2) VarName = "z";
  else             VarName = "c";

  // Look up variable in user space and get pointer to its value
  ppl_contextGetVarPointer(X->c, VarName, &VarVal, &DummyTemp);

  // Set value of x (or y/z)
  *VarVal = *xunit;
  VarVal->imag        = 0.0;
  VarVal->flagComplex = 0;
  VarVal->real        = x;

  // Generate tick string
  outval = ppl_expEval(X->c, FormatExp, &lOP, 0, X->iterDepth+1);
  if (X->c->errStat.status) { sprintf(X->c->errcontext.tempErrStr, "Error encountered whilst using format string: %s",FormatExp->ascii); ppl_error(&X->c->errcontext,ERR_PREFORMED, -1, -1, NULL); ppl_tbWrite(X->c); ppl_tbClear(X->c); tmp_string = "{\\bf ?}"; }
  else if (outval->objType != PPLOBJ_STR) { sprintf(X->c->errcontext.tempErrStr, "Error encountered whilst using format string: %s",FormatExp->ascii); ppl_error(&X->c->errcontext,ERR_PREFORMED, -1, -1, NULL); ppl_error(&X->c->errcontext,ERR_PREFORMED, -1, -1, "Tick label was not a string."); tmp_string = "{\\bf ?}"; }
  else                       { tmp_string = (char *)outval->auxil; }
  *output = (char *)ppl_memAlloc_incontext(strlen(tmp_string)+3, OutContext);
  if ((*output)==NULL) return;
  sprintf(*output,"%s",tmp_string);
  { EPSComm *x=X; EPS_STACK_POP; }

  // Restore original value of x (or y/z)
  ppl_contextRestoreVarPointer(X->c, VarName, &DummyTemp);
  return;
 }

