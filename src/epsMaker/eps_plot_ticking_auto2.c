// eps_plot_ticking_auto2.c
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

// This file contain an algorithm for the automatic placement of ticks along axes.

// METHOD 2: PyXPlot 0.7 automatic ticking.

#define _PPL_EPS_PLOT_TICKING_AUTO2_C 1

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
#include "epsMaker/eps_plot_ticking_auto.h"
#include "epsMaker/eps_plot_ticking_auto2.h"

#define MAX_FACTORS 32 /* Maximum number of factors of LogBase which we consider */
#define FACTOR_MULTIPLY 2.0 /* Factorise LogBase**2, so that 0.00,0.25,0.50,0.75,1.00 is a valid factorisation */
#define TICKS_MAXIMUM 256 /* Maximum number of ticks allowed along any given axis */

typedef struct TickScheme {
  int    mantissa[TICKS_MAXIMUM];
  int    Nmantissas;
  double ticksep;
  double offset;
 } TickScheme;

// Print a ticking scheme for debugging purposes
static void TickScheme2_sprintf(char *out, const int OutLen, const double UnitMultiplier, const double *TL, const double TL_len)
 {
  int i=0,j=0,k=0;
  out[j++]='(';
  for (i=0; ((i<TL_len)&&(j<OutLen-8)); i++)
   {
    if (k!=0) out[j++]=',';
    snprintf(out+j,OutLen-j-8,"%g",TL[i]/UnitMultiplier);
    out[OutLen-7]='\0';
    j+=strlen(out+j);
    k++;
   }
  out[j++]=')';
  out[j++]='.';
  out[j++]='\0';
  return;
 }

static void GenerateLogTickSchemes(TickScheme *TickSchemes, int *NTickSchemes, int LogBase)
 {
  unsigned char stopping;
  int           i, divisor;

  for (stopping=0,divisor=1; (!stopping); divisor++)
   {
    if (*NTickSchemes > TICKS_MAXIMUM) return;
    for (i=0; i<divisor; i++)
     {
      TickSchemes[*NTickSchemes].mantissa[i] = floor(pow(LogBase,((double)i)/divisor)+0.5);
      if ((i>0)&&(TickSchemes[*NTickSchemes].mantissa[i-1]==TickSchemes[*NTickSchemes].mantissa[i])) stopping=1;
     }
    if (!stopping)
     {
      TickSchemes[*NTickSchemes].Nmantissas = divisor;
      TickSchemes[*NTickSchemes].ticksep    = 1.0;
      TickSchemes[*NTickSchemes].offset     = 0.0;
      (*NTickSchemes)++;
     }
   }
  if (*NTickSchemes < TICKS_MAXIMUM)
   {
    for (i=1; ((i<LogBase)&&(i<TICKS_MAXIMUM)); i++) TickSchemes[*NTickSchemes].mantissa[i] = i;
    TickSchemes[*NTickSchemes].Nmantissas = i;
    TickSchemes[*NTickSchemes].ticksep    = 1.0;
    TickSchemes[*NTickSchemes].offset     = 0.0;
    (*NTickSchemes)++;
   }
  return;
 }

static void GenerateTickSchemes(TickScheme *TickSchemes, int *NTickSchemes, double OoM, unsigned char IsLog, int LogBase, int *FactorsTen, int NFactors)
 {
  int i, LevelDescend = 1;
  *NTickSchemes = 0;
  while (pow(10.0,LevelDescend-1) < (10.0 * TICKS_MAXIMUM))
   {
    double OoMscan = OoM / pow(10.0, LevelDescend);
    if (IsLog && (OoMscan>0.09) && (OoMscan<0.11))
     {
      GenerateLogTickSchemes(TickSchemes, NTickSchemes, LogBase);
     }
    else
     {
      for (i=NFactors-1; i>=0; i--)
       {
        double t;
        TickSchemes[*NTickSchemes].mantissa[0] = 1;
        TickSchemes[*NTickSchemes].Nmantissas  = 1;
        TickSchemes[*NTickSchemes].ticksep     = t = FactorsTen[i] * OoMscan / pow(10.0, FACTOR_MULTIPLY-1);
        TickSchemes[*NTickSchemes].offset      = 0.0;
        if ((!IsLog) || (ppl_dblEqual(t,floor(t+0.5)))) (*NTickSchemes)++; // Fractional steps (i.e. sqrts) not allowed
       }
     }
    LevelDescend++;
   }
  return;
 }

// Return up to a maximum of MaxFactors factors of in. Factors are returned to array out.
static void factorise(int in, int *out, int MaxFactors, int FactorMax, int *NFactors)
 {
  int i,j=0,k,N=sqrt(in);
  for (i=2; i<=N; i++) if (in%i==0) { out[j]=i; out[MaxFactors-1-j]=in/i; j++; if (j>=MaxFactors/2) break; }
  for (i=MaxFactors-j; i<=MaxFactors-1; i++) { out[j] = out[i]; if (out[j]!=out[j-1]) j++; }
  for (i=0; i<j; i++) if (in/out[i] <= FactorMax) break;
  for (k=0; i<j; i++, k++) out[k] = out[i];
  *NFactors=k;
  return;
 }

// Main entry point for automatic ticking of axes
void eps_plot_ticking_auto2(EPSComm *x, pplset_axis *axis, double UnitMultiplier, unsigned char *AutoTicks, pplset_axis *linkedto)
 {
  int           i, j, k, N, OutContext, ContextRough=-1, NFactorsTen, LogBase, number_ticks, major, OverlayMatch;
  unsigned char IsLog=(axis->LogFinal == SW_BOOL_TRUE);
  int           FactorsTen[MAX_FACTORS];
  double        OoM, axis_min, axis_max, axis_min_l, axis_max_l, outer_min, outer_max;
  int           TL_best_len, TL_trial_len;
  double       *TLP, *TL_best, *TL_trial;
  char        **TLS;
  int           NTickSchemes;
  TickScheme   *TickSchemes;

  if (DEBUG) ppl_log(&x->c->errcontext,"Using eps_plot_ticking_auto2()");

  // Make temporary rough workspace
  OutContext   = ppl_memAlloc_GetMemContext();
  ContextRough = ppl_memAlloc_DescendIntoNewContext();

  // This ticking scheme does not work on axes with non-linear mappings
  if (axis->AxisLinearInterpolation != NULL) goto FAIL;

  // Work out factors of log base of axis. In fact, work out factors of log base ** 2, so that ten divides by four.
  LogBase = IsLog ? axis->tics.logBase : 10;
  factorise(pow(10.0,FACTOR_MULTIPLY), FactorsTen, MAX_FACTORS, axis->PhysicalLengthMinor, &NFactorsTen);

  // Work out order of magnitude of axis range
  axis_min = axis->MinFinal * UnitMultiplier;
  axis_max = axis->MaxFinal * UnitMultiplier;
  if      (axis_min  > axis_max) { double temp; temp = axis_min; axis_min = axis_max; axis_max = temp; }
  else if (axis_min == axis_max) { goto FAIL; }
  if (IsLog && (axis_max < 3*axis_min)) IsLog = 0;
  if (!IsLog)
   {
    OoM       = pow(10.0, ceil(log10(axis_max - axis_min)));
    outer_min = floor(axis_min / OoM) * OoM;
    outer_max = ceil (axis_max / OoM) * OoM;
   }
  else
   {
    axis_min_l = log(axis_min) / log(LogBase);
    axis_max_l = log(axis_max) / log(LogBase);
    OoM        = pow(10.0, ceil(log10(axis_max_l - axis_min_l)));
    outer_min  = floor(axis_min_l / OoM) * OoM;
    outer_max  = ceil (axis_max_l / OoM) * OoM;
   }

  // Malloc list of accepted ticks
  if (AutoTicks[1])
   {
    axis-> TickListPositions = (double  *)ppl_memAlloc_incontext((TICKS_MAXIMUM+1) * sizeof(double), OutContext);
    axis-> TickListStrings   = (char   **)ppl_memAlloc_incontext((TICKS_MAXIMUM+1) * sizeof(char *), OutContext);
   }
  if (AutoTicks[0])
   {
    axis->MTickListPositions = (double  *)ppl_memAlloc_incontext((TICKS_MAXIMUM+1) * sizeof(double), OutContext);
    axis->MTickListStrings   = (char   **)ppl_memAlloc_incontext((TICKS_MAXIMUM+1) * sizeof(char *), OutContext);
   }
  if (    ((AutoTicks[1]) && ((axis-> TickListPositions==NULL) || (axis-> TickListStrings==NULL)))
       || ((AutoTicks[0]) && ((axis->MTickListPositions==NULL) || (axis->MTickListStrings==NULL)))
     ) goto FAIL;

  // Malloc temporary lists of ticks
  TL_best  = (double  *)ppl_memAlloc((TICKS_MAXIMUM+1) * sizeof(double));
  TL_trial = (double  *)ppl_memAlloc((TICKS_MAXIMUM+1) * sizeof(double));
  if ((TL_best==NULL)||(TL_trial==NULL)) goto FAIL;
  TL_best_len = 0;

  // Malloc list of tick schemes
  TickSchemes = (TickScheme *)ppl_memAlloc((TICKS_MAXIMUM+1) * sizeof(TickScheme));
  if (TickSchemes==NULL) goto FAIL;

  // Deal with MAJOR ticks. Then MINOR ticks.
  for (major=1; major>=0; major--)
   if (AutoTicks[major])
    {
     if (major) { TLP = axis-> TickListPositions; TLS = axis-> TickListStrings; }
     else       { TLP = axis->MTickListPositions; TLS = axis->MTickListStrings; }

     // How many ticks can we fit onto this axis?
     number_ticks = (major ? axis->PhysicalLengthMajor : axis->PhysicalLengthMinor) + 1;
     if (number_ticks <             2) number_ticks =             2; // Minimum of two ticks along any given axis
     if (number_ticks > TICKS_MAXIMUM) number_ticks = TICKS_MAXIMUM; // Maximum number of ticks along any given axis

     // Generate list of potential tick schemes
     GenerateTickSchemes(TickSchemes, &NTickSchemes, OoM, IsLog, LogBase, FactorsTen, NFactorsTen);

     // Try each tick scheme in turn
     for (i=0; i<NTickSchemes; i++)
      {
       double ts_min = outer_min  + TickSchemes[i].offset;
       TL_trial_len = 0;
       for (j=0; j<((outer_max-outer_min)/TickSchemes[i].ticksep+1.5); j++)
        {
         if (TL_trial_len>=TICKS_MAXIMUM) break;
         for (k=0; k<TickSchemes[i].Nmantissas; k++)
          {
           double x;
           if (TL_trial_len>=TICKS_MAXIMUM) break;
           x = ts_min + j * TickSchemes[i].ticksep;
           if (fabs(x)<1e-6*TickSchemes[i].ticksep) x=0;
           if (IsLog) x = pow(LogBase, x);
           x *= TickSchemes[i].mantissa[k];
           if ((x<axis_min) || (x>axis_max)) continue;
           TL_trial[TL_trial_len] = x;
           TL_trial_len++;
          }
        }

       // If minor ticks, make sure that this tick scheme overlays major ticks
       OverlayMatch = 1;
       if (!major)
        {
         if (DEBUG)
          {
           int tmp;
           sprintf(x->c->errcontext.tempErrStr, "Trying minor tick scheme: ");
           tmp = strlen(x->c->errcontext.tempErrStr);
           TickScheme2_sprintf(x->c->errcontext.tempErrStr+tmp, LSTR_LENGTH-tmp, UnitMultiplier, TL_trial, TL_trial_len);
           ppl_log(&x->c->errcontext,NULL);
          }
         for (j=0; axis->TickListStrings[j]!=NULL; j++)
          {
           OverlayMatch = 0;
           for (k=0; k<TL_trial_len; k++)
            {
             double pos = eps_plot_axis_GetPosition(TL_trial[k]/UnitMultiplier, axis, 0, 0);
             if (fabs(pos - axis->TickListPositions[j])>1e-4) continue;
             for (k++; k<TL_trial_len; k++) TL_trial[k-1] = TL_trial[k];
             TL_trial_len--;
             OverlayMatch = 1;
             break;
            }
           if (!OverlayMatch) break;
          }
         if (DEBUG && !OverlayMatch)
          {
           sprintf(x->c->errcontext.tempErrStr, "FAIL: Did not overlay major tick at %g (axis pos %.3f)", eps_plot_axis_InvGetPosition(axis->TickListPositions[j],axis), axis->TickListPositions[j]);
           ppl_log(&x->c->errcontext,NULL);
          }
         if (!OverlayMatch) continue;
         if (DEBUG) ppl_log(&x->c->errcontext,"PASS");
        }

       // See whether this tick scheme is better than previous
       if ((IsLog)&&(TickSchemes[i].Nmantissas>=LogBase-1)&&(!major))
        {
         if (TL_trial_len > 3*number_ticks)
          {
           if (DEBUG)
            {
             sprintf(x->c->errcontext.tempErrStr, "Minor tick scheme with %d mantissas rejected because it has too many ticks (%d when we wanted ** %d)", TickSchemes[i].Nmantissas, TL_trial_len, number_ticks);
             ppl_log(&x->c->errcontext,NULL);
            }
           break;
          }
         if (DEBUG) ppl_log(&x->c->errcontext,"Accepted minor tick scheme which marks every mantissa value");
         TL_best_len = TL_trial_len; for (j=0; j<TL_trial_len; j++) TL_best[j] = TL_trial[j];
        }
       else if ((IsLog)&&(TickSchemes[i].Nmantissas>1)&&(TL_trial_len>number_ticks))
        {
         if (DEBUG)
          {
           sprintf(x->c->errcontext.tempErrStr, "Tick scheme with %d mantissas rejected because it has too many ticks (%d when we wanted %d)", TickSchemes[i].Nmantissas, TL_trial_len, number_ticks);
           ppl_log(&x->c->errcontext,NULL);
          }
         continue;
        }
       else if (TL_trial_len > number_ticks) break;
       else if (TL_trial_len > TL_best_len )
        {
         for (j=0; j<TL_trial_len; j++) TL_best[j] = TL_trial[j];
         TL_best_len = TL_trial_len;
        }
      }

     // Commit list of ticks
     for (i=j=0; i<TL_best_len; i++)
      {
       double xtmp = TL_best[i];
       TLP[j] = eps_plot_axis_GetPosition(xtmp/UnitMultiplier, axis, 0, 0);
       if (!gsl_finite(TLP[j])) continue;
       if (major)
        {
         if (axis->format==NULL) TickLabelAutoGen(x, &TLS[j], xtmp, LogBase, OutContext);
         else                    TickLabelFromFormat(x, &TLS[j], axis->format, xtmp/UnitMultiplier, &axis->DataUnit, axis->xyz, OutContext);
        }
       else
        {
         TLS[j] = "";
        }
       j++;
      }
     TLS[j] = NULL;
    }

  // Finished
  goto CLEANUP;

FAIL:
  if (DEBUG) ppl_log(&x->c->errcontext,"eps_plot_ticking_auto2() has failed");

  // A very simple way of putting ticks on axes when clever logic fails
  N = 1 + axis->PhysicalLengthMajor; // Estimate how many ticks we want
  if (N<  3) N=  3;
  if (N>100) N=100;

  axis->TickListPositions = (double  *)ppl_memAlloc_incontext((N+1) * sizeof(double), OutContext);
  axis->TickListStrings   = (char   **)ppl_memAlloc_incontext((N+1) * sizeof(char *), OutContext);
  if ((axis->TickListPositions==NULL) || (axis->TickListStrings==NULL)) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1, "Out of memory (s)."); axis->TickListPositions = NULL; axis->TickListStrings = NULL; goto CLEANUP; }
  for (i=0; i<N; i++)
   {
    double X;
    X = ((double)i)/(N-1);
    axis->TickListPositions[i] = X;
    X = eps_plot_axis_InvGetPosition(X, axis);
    if (axis->format == NULL) TickLabelAutoGen(x, &axis->TickListStrings[i] , X * UnitMultiplier , axis->tics.logBase, OutContext);
    else                      TickLabelFromFormat(x, &axis->TickListStrings[i], axis->format, X, &axis->DataUnit, axis->xyz, OutContext);
    if (axis->TickListStrings[i]==NULL) { ppl_error(&x->c->errcontext,ERR_MEMORY, -1, -1, "Out of memory (t)."); axis->TickListPositions = NULL; axis->TickListStrings = NULL; goto CLEANUP; }
   }
  axis->TickListStrings[i] = NULL; // null terminate list

CLEANUP:
  // Delete rough workspace
  if (ContextRough>0) ppl_memAlloc_AscendOutOfContext(ContextRough);
  return;
 }

