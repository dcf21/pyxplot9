// eps_plot_linkedaxes.c
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

#define _PPL_EPS_PLOT_LINKEDAXES 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_multimin.h>

#include "commands/eqnsolve.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/memAlloc.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "mathsTools/dcfmath.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_linkedaxes.h"
#include "epsMaker/eps_plot_ticking.h"
#include "epsMaker/eps_settings.h"

// Whenever we update the usage variables MinUsed and MaxUsed for an axis, this
// procedure is called. It checks whether the axis is linked, and if so,
// updates the usage variables for the axis which it is linked to. This process
// may then iteration down a hierarchy of linked axes. As a rule, it is the
// axis at the bottom of the linkage hierarchy (i.e. at the end of the linked
// list) that has the canonical usage variables. Axes further up may not have
// complete information about the usage of the set of linked axes, since usage
// does not propagate UP the hierarchy.

void eps_plot_LinkedAxisBackPropagate(EPSComm *x, pplset_axis *source)
 {
  int            IterDepth;
  pplset_axis *target;
  canvas_item   *item;

  if (DEBUG) { sprintf(x->c->errcontext.tempErrStr, "Back-propagating axis usage for axis %c%d on plot %d", "xyzc"[source->xyz], source->axis_n, source->canvas_id); ppl_log(&x->c->errcontext, NULL); }
  item       = x->current;

  // Propagating MinUsed and MaxUsed variables along links between axes
  IterDepth = 0;
  while (1) // loop over as many iterations of linkage as may be necessary
   {
    if (IterDepth++ > 100) return;
    if (!(source->linked && (source->MinUsedSet || source->MaxUsedSet || source->DataUnitSet))) { break; } // proceed only if axis is linked and has usage information
    if (source->LinkedAxisCanvasID <= 0)
     { } // Linked to an axis on the same graph; do not change item
    else // Linked to an axis on a different canvas item
     {
      item = x->itemlist->first;
      while ((item != NULL) && ((item->id)<source->LinkedAxisCanvasID)) item=item->next;
      if ((item == NULL) || (item->id != source->LinkedAxisCanvasID) || (item->type != CANVAS_PLOT) || (item->XAxes==NULL) || (item->YAxes==NULL) || (item->ZAxes==NULL)) { break; }
     }
    if      (source->LinkedAxisToXYZ == 0) target = item->XAxes + source->LinkedAxisToNum;
    else if (source->LinkedAxisToXYZ == 1) target = item->YAxes + source->LinkedAxisToNum;
    else                                   target = item->ZAxes + source->LinkedAxisToNum;
    if (source->linkusing != NULL)
     {
      eps_plot_LinkUsingBackPropagate(x, source->MinUsed, target, source);
      eps_plot_LinkUsingBackPropagate(x, source->MaxUsed, target, source);
     }
    else
     {
      if ((target->DataUnitSet && source->DataUnitSet) && (!ppl_unitsDimEqual(&target->DataUnit , &source->DataUnit)))
       {
        sprintf(x->c->errcontext.tempErrStr,"Axis %c%d of plot %d is linked to axis %c%d of plot %d, but axes have data plotted against them with conflicting physical units. The former has units of <%s> whilst the latter has units of <%s>.","xyzc"[source->xyz],source->axis_n,source->canvas_id,"xyzc"[target->xyz],target->axis_n,target->canvas_id,ppl_printUnit(x->c,&target->DataUnit,NULL,NULL,0,1,0),ppl_printUnit(x->c,&source->DataUnit,NULL,NULL,1,1,0));
        ppl_warning(&x->c->errcontext, ERR_GENERIC, NULL);
        break;
       }
      else
       {
        if ((source->MinUsedSet) && ((!target->MinUsedSet) || (target->MinUsed > source->MinUsed))) { target->MinUsed=source->MinUsed; target->MinUsedSet=1; }
        if ((source->MaxUsedSet) && ((!target->MaxUsedSet) || (target->MaxUsed < source->MaxUsed))) { target->MaxUsed=source->MaxUsed; target->MaxUsedSet=1; }
        if (source->DataUnitSet) { target->DataUnit = source->DataUnit; target->DataUnitSet = 1; }
       }
     }
    source     = target;
   }
  return;
 }

// Loop through all of the axes on a single plot item, and decide its range. If
// the axis is a linked axis, must first decide the range of the axis at the
// bottom of the linkage hierarchy. The function
// eps_plot_LinkedAxisForwardPropagate deals with this.

void eps_plot_DecideAxisRanges(EPSComm *x)
 {
  int            i, j;
  pplset_axis *axes=NULL;

  // Decide the range of each axis in turn
  for (j=0; j<3; j++)
   {
    if      (j==0) axes = x->current->XAxes;
    else if (j==1) axes = x->current->YAxes;
    else if (j==2) axes = x->current->ZAxes;
    else           ppl_fatal(&x->c->errcontext,__FILE__,__LINE__,"Internal fail.");
    for (i=0; i<MAX_AXES; i++)
     {
      if (!axes[i].RangeFinalised   ) { eps_plot_LinkedAxisForwardPropagate(x, &axes[i], 1); if (*x->status) return; }
      if (!axes[i].TickListFinalised) { eps_plot_ticking(x, &axes[i], x->current->settings.AxisUnitStyle, NULL); if (*x->status) return; }
     }
   }
  return;
 }

// As part of the process of determining the range of axis xyz[axis_n], check
// whether the axis is linking, and if so fetch usage information from the
// bottom of the linkage hierarchy. Propagate this information up through all
// intermediate levels of the hierarchy before calling
// eps_plot_DecideAxisRange().

void eps_plot_LinkedAxisForwardPropagate(EPSComm *x, pplset_axis *axis, int mode)
 {
  int            IterDepth, OriginalMode=mode;
  pplset_axis *source, *target, *target2;
  canvas_item   *item;

  // Propagate MinUsed and MaxUsed variables along links
  IterDepth     = 0;
  target        = axis;
  item          = x->current;

  while (1) // loop over as many iterations of linkage as may be necessary to find MinFinal and MaxFinal at the bottom
   {
    if (IterDepth++ > 100) break;
    if ((!target->linked) || target->RangeFinalised) { break; } // proceed only if axis is linked
    if (target->LinkedAxisCanvasID <= 0)
     { } // Linked to an axis on the same graph; do not change item
    else // Linked to an axis on a different canvas item
     {
      item = x->itemlist->first;
      while ((item != NULL) && (item->id)<target->LinkedAxisCanvasID) item=item->next;
      if ((item == NULL) || (item->id != target->LinkedAxisCanvasID)) { if ((IterDepth==1)&&(mode==0)) { sprintf(x->c->errcontext.tempErrStr,"Axis %c%d of plot %d is linked to axis %c%d of plot %d, but no such plot exists.","xyzc"[target->xyz],target->axis_n,target->canvas_id,"xyzc"[target->LinkedAxisToXYZ],target->LinkedAxisToNum,target->LinkedAxisCanvasID); ppl_warning(&x->c->errcontext, ERR_GENERIC, NULL); } break; }
      if (item->type != CANVAS_PLOT) { if ((IterDepth==1)&&(mode==0)) { sprintf(x->c->errcontext.tempErrStr,"Axis %c%d of plot %d is linked to axis %c%d of plot %d, but this canvas item is not a plot.","xyzc"[target->xyz],target->axis_n,target->canvas_id,"xyzc"[target->LinkedAxisToXYZ],target->LinkedAxisToNum,target->LinkedAxisCanvasID); ppl_warning(&x->c->errcontext, ERR_GENERIC, NULL); } break; }
      if ((item->XAxes==NULL)||(item->YAxes==NULL)||(item->ZAxes==NULL)) { if ((IterDepth==1)&&(mode==0)) { sprintf(x->c->errcontext.tempErrStr,"Axis %c%d of plot %d is linked to axis %c%d of plot %d, but this item has NULL axes.","xyzc"[target->xyz],target->axis_n,target->canvas_id,"xyzc"[target->LinkedAxisToXYZ],target->LinkedAxisToNum,target->LinkedAxisCanvasID); ppl_warning(&x->c->errcontext, ERR_INTERNAL, NULL); } break; }
     }
    if      (target->LinkedAxisToXYZ == 0) target2 = item->XAxes + target->LinkedAxisToNum;
    else if (target->LinkedAxisToXYZ == 1) target2 = item->YAxes + target->LinkedAxisToNum;
    else                                   target2 = item->ZAxes + target->LinkedAxisToNum;
    if (target->DataUnitSet && target2->DataUnitSet && (!ppl_unitsDimEqual(&target->DataUnit , &target2->DataUnit))) break; // If axes are dimensionally incompatible, stop
    target        = target2;
   }
  if ((mode==1) && (!target->RangeFinalised)) { eps_plot_ticking(x, target, x->current->settings.AxisUnitStyle, NULL); if (*x->status) return; }
  IterDepth -= 2;
  source     = target;
  for ( ; IterDepth>=0 ; IterDepth--) // loop over as many iterations of linkage as may be necessary
   {
    int k;
    target = axis;
    for (k=0; k<IterDepth; k++)
     {
      if (target->LinkedAxisCanvasID <= 0)
       { item = x->current; } // Linked to an axis on the same graph
      else // Linked to an axis on a different canvas item
       {
        item = x->itemlist->first;
        while ((item != NULL) && (item->id)<target->LinkedAxisCanvasID) item=item->next;
        if ((item == NULL) || (item->id != target->LinkedAxisCanvasID) || (item->type != CANVAS_PLOT) || (item->XAxes==NULL) || (item->YAxes==NULL) || (item->ZAxes==NULL)) { break; }
       }
      if      (target->LinkedAxisToXYZ == 0) target2 = item->XAxes + target->LinkedAxisToNum;
      else if (target->LinkedAxisToXYZ == 1) target2 = item->YAxes + target->LinkedAxisToNum;
      else                                   target2 = item->ZAxes + target->LinkedAxisToNum;
      target        = target2;
     }

    if (target->RangeFinalised) { break; }
    if (!target->linked) { break; } // proceed only if axis is linked
    if ((OriginalMode==0)&&(target->Mode0BackPropagated)) continue;
    target->Mode0BackPropagated = 1;
    if (mode==0) // MODE 0: Propagate HardMin, HardMax, HardUnit
     {
      if (target->linkusing != NULL)
       {
        if (source->HardMinSet && source->HardMaxSet)
         {
          source->MinFinal    = source->HardMin;
          source->MaxFinal    = source->HardMax;
          source->DataUnit    = source->HardUnit;
          source->DataUnitSet = 1;
          mode=1;
         }
        else
         {
          target->HardMinSet = target->HardMaxSet = target->HardUnitSet = 0;
         }
       }
      else
       {
        target->HardMinSet     = source->HardMinSet;
        target->HardMin        = source->HardMin;
        target->HardMaxSet     = source->HardMaxSet;
        target->HardMax        = source->HardMax;
        target->HardUnitSet    = source->HardUnitSet;
        target->HardUnit       = source->HardUnit;
        target->HardAutoMinSet = source->HardAutoMinSet;
        target->HardAutoMaxSet = source->HardAutoMaxSet;
       }
     }
    if (mode==1) // MODE 1: Propagate finalised ranges
     {
      target->LogFinal = source->LogFinal;
      target->MinFinal = source->MinFinal;
      target->MaxFinal = source->MaxFinal;
      if (target->linkusing != NULL)
       {
        if (eps_plot_LinkedAxisLinkUsing(x, target, source)) break;
       }
      else
       {
        if (target->DataUnitSet && source->DataUnitSet && (!ppl_unitsDimEqual(&target->DataUnit , &source->DataUnit))) break; // If axes are dimensionally incompatible, stop
        target->DataUnit = source->DataUnit;
        target->DataUnitSet = source->DataUnitSet;
        target->AxisValueTurnings = source->AxisValueTurnings;
        target->AxisLinearInterpolation = source->AxisLinearInterpolation;
        target->AxisTurnings = source->AxisTurnings;
       }
      target->RangeFinalised = 1;
      eps_plot_ticking(x, target, x->current->settings.AxisUnitStyle, source);
     }
    source = target;
   }
  return;
 }

int eps_plot_LinkedAxisLinkUsing(EPSComm *X, pplset_axis *out, pplset_axis *in)
 {
  const int stkLevelOld = X->c->stackPtr;
  int       l,xrn=0,lOP;
  int       oldsgn=-10,newsgn;
  double    p,x;
  char     *VarName;
  pplObj    DummyTemp, *outVal, *VarVal;
  if      (in->xyz==0) VarName = "x";
  else if (in->xyz==1) VarName = "y";
  else if (in->xyz==2) VarName = "z";
  else                 VarName = "c";

  // Look up variable in user space and get pointer to its value
  ppl_contextGetVarPointer(X->c, VarName, &VarVal, &DummyTemp);

  out->AxisLinearInterpolation = (double *)ppl_memAlloc(AXISLINEARINTERPOLATION_NPOINTS * sizeof(double));
  out->AxisValueTurnings       = 0;
  out->AxisTurnings            = (int *)ppl_memAlloc((AXISLINEARINTERPOLATION_NPOINTS+2) * sizeof(int));

  if ((out->AxisLinearInterpolation==NULL)||(out->AxisTurnings==NULL)) goto FAIL;
  out->AxisTurnings[xrn++] = 0;

  for (l=0; l<AXISLINEARINTERPOLATION_NPOINTS; l++) // Loop over samples we are going to take from axis linkage function
   {
    p = ((double)l)/(AXISLINEARINTERPOLATION_NPOINTS-1);
    x = eps_plot_axis_InvGetPosition(p,in);
    pplExpr *olu = (pplExpr *)out->linkusing;

    // Set value of x (or y/z)
    {
     int om = VarVal->amMalloced;
     int rc = VarVal->refCount;
     *VarVal = in->DataUnit;
     VarVal->amMalloced  = om;
     VarVal->refCount    = rc;
     VarVal->self_lval   = NULL;
     VarVal->self_this   = NULL;
     VarVal->imag        = 0.0;
     VarVal->flagComplex = 0;
     VarVal->real        = x;
    }

    // Generate a sample from the axis linkage function
    outVal = ppl_expEval(X->c, olu, &lOP, 0, X->iterDepth+1);
    if (X->c->errStat.status) { sprintf(X->c->errcontext.tempErrStr, "Error encountered whilst evaluating axis linkage expression: %s",olu->ascii); ppl_error(&X->c->errcontext, ERR_GENERIC, -1, -1, NULL); ppl_tbWrite(X->c); ppl_tbClear(X->c); goto FAIL; }
    if ((outVal->objType != PPLOBJ_NUM)&&(outVal->objType != PPLOBJ_DATE)&&(outVal->objType != PPLOBJ_BOOL))
     { sprintf(X->c->errcontext.tempErrStr, "Error encountered whilst evaluating axis linkage expression: %s\nObject was not a number but an object of type <%s>",olu->ascii,pplObjTypeNames[outVal->objType]); ppl_error(&X->c->errcontext, ERR_TYPE, -1, -1, NULL); goto FAIL; }
    if (outVal->flagComplex) { sprintf(X->c->errcontext.tempErrStr, "Error encountered whilst evaluating axis linkage expression: %s",olu->ascii); ppl_error(&X->c->errcontext, ERR_GENERIC, -1, -1, NULL); ppl_error(&X->c->errcontext, ERR_GENERIC, -1, -1, "Received a complex number; axes must have strictly real values at all points."); goto FAIL; }
    if (!gsl_finite(outVal->real)) { sprintf(X->c->errcontext.tempErrStr, "Error encountered whilst evaluating axis linkage expression: %s",olu->ascii); ppl_error(&X->c->errcontext, ERR_GENERIC, -1, -1, NULL); ppl_error(&X->c->errcontext, ERR_GENERIC, -1, -1, "Expression returned non-finite result."); goto FAIL; }
    if (!out->DataUnitSet) { out->DataUnit = *outVal; out->DataUnitSet = 1; }
    if (!ppl_unitsDimEqual(&out->DataUnit,outVal))  { sprintf(X->c->errcontext.tempErrStr, "Error encountered whilst evaluating axis linkage expression: %s",olu->ascii); ppl_error(&X->c->errcontext, ERR_GENERIC, -1, -1, NULL); sprintf(X->c->errcontext.tempErrStr, "Axis linkage function produces axis values with dimensions of <%s> whilst data plotted on this axis has dimensions of <%s>.", ppl_printUnit(X->c,outVal,NULL,NULL,0,1,0), ppl_printUnit(X->c,&out->DataUnit,NULL,NULL,1,1,0)); ppl_error(&X->c->errcontext, ERR_GENERIC, -1, -1, NULL); goto FAIL; }
    out->AxisLinearInterpolation[l] = outVal->real;
    if (l>0) // Check for turning points
     {
      newsgn = ppl_sgn(out->AxisLinearInterpolation[l] - out->AxisLinearInterpolation[l-1]);
      if (newsgn==0) continue;
      if ((newsgn!=oldsgn) && (oldsgn>-10)) out->AxisTurnings[xrn++] = l-1;
      oldsgn = newsgn;
     }
    { EPSComm *x=X; EPS_STACK_POP; }
   }

  // Restore original value of x (or y/z)
  ppl_contextRestoreVarPointer(X->c, VarName, &DummyTemp);
  out->AxisTurnings[xrn--] = AXISLINEARINTERPOLATION_NPOINTS-1;
  out->AxisValueTurnings = xrn;
  return 0;

FAIL:
  { EPSComm *x=X; EPS_STACK_POP; }
  ppl_contextRestoreVarPointer(X->c, VarName, &DummyTemp);
  out->AxisLinearInterpolation = NULL;
  out->AxisValueTurnings       = 0;
  out->AxisTurnings            = NULL;
  return 1;
 }

// Routines for the back propagation of usage information along non-linear links

typedef struct LAUComm {
 EPSComm      *x;
 pplExpr      *expr;
 char         *VarName;
 pplObj       *VarValue;
 int           GoneNaN, mode, iter2;
 double        norm;
 pplObj        target;
 double        WorstScore;
 int           fail;
 } LAUComm;

#define WORSTSCORE_INIT -(DBL_MAX/1e10)

double eps_plot_LAUSlave(const gsl_vector *x, void *params)
 {
  int       i, lOP;
  double    output;
  pplObj   *outVal;
  LAUComm  *data = (LAUComm *)params;
  const int stkLevelOld = data->x->c->stackPtr;

  if (data->fail) return GSL_NAN; // We've previously had a serious error... so don't do any more work
  if (data->mode==0) data->VarValue->real                   = ppl_optimise_LogToReal(gsl_vector_get(x, 0) , data->iter2 , &data->norm);
  else               data->VarValue->exponent[data->mode-1] = ppl_optimise_LogToReal(gsl_vector_get(x, 0) , data->iter2 , &data->norm);

  data->VarValue->dimensionless=1;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) if (ppl_dblEqual(data->VarValue->exponent[i], 0) == 0) { data->VarValue->dimensionless=0; break; }
  outVal = ppl_expEval(data->x->c, data->expr, &lOP, 0, data->x->iterDepth+1);

  // If a numerical error happened; ignore it for now, but return NAN
  if (data->x->c->errStat.status) { sprintf(data->x->c->errcontext.tempErrStr, "An algebraic error was encountered at %s=%s:", data->VarName, ppl_unitsNumericDisplay(data->x->c, data->VarValue,0,0,0)); ppl_error(&data->x->c->errcontext,ERR_PREFORMED, -1, -1, NULL); ppl_tbWrite(data->x->c); ppl_tbClear(data->x->c); output=GSL_NAN; goto end; }
  if ((outVal->objType != PPLOBJ_NUM)&&(outVal->objType != PPLOBJ_DATE)&&(outVal->objType != PPLOBJ_BOOL))
   { sprintf(data->x->c->errcontext.tempErrStr, "Error encountered whilst evaluating axis linkage expression: %s\nObject was not a number but an object of type <%s>",data->expr->ascii,pplObjTypeNames[outVal->objType]); ppl_error(&data->x->c->errcontext, ERR_TYPE, -1, -1, NULL); output=GSL_NAN; goto end; }


#define TWINLOG(X) ((X>1e-200) ? log(X) : (2*log(1e-200) - log(2e-200-X)))
  if (data->mode==0) output = pow( TWINLOG(data->target.real                  ) - TWINLOG(outVal->real                   ) ,2);
  else               output = pow( TWINLOG(data->target.exponent[data->mode-1]) - TWINLOG(outVal->exponent[data->mode-1] ) ,2);
  if ((!gsl_finite(output)) && (data->WorstScore > WORSTSCORE_INIT)) { data->WorstScore *= 1.1; return data->WorstScore; }
  if (( gsl_finite(output)) && (data->WorstScore < output)         ) { data->WorstScore = output; }

end:
  { EPSComm *x=data->x; EPS_STACK_POP; }
  return output;
 }

void eps_plot_LAUFitter(LAUComm *commlink)
 {
  size_t                              iter = 0,iter2 = 0;
  int                                 i, status=0;
  double                              size=0,sizelast=0,sizelast2=0,testval;
  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex; // We don't use nmsimplex2 here because it was new in gsl 1.12
  gsl_multimin_fminimizer            *s;
  gsl_vector                         *x, *ss;
  gsl_multimin_function               fn;

  fn.n = 1;
  fn.f = &eps_plot_LAUSlave;
  fn.params = (void *)commlink;

  x  = gsl_vector_alloc( 1 );
  ss = gsl_vector_alloc( 1 );

  iter2=0;
  do
   {
    iter2++;
    if (commlink->mode!=0) iter2=999;
    commlink->iter2 = iter2;
    sizelast2 = size;
    gsl_vector_set(x , 0, ppl_optimise_RealToLog( (commlink->mode==0) ? commlink->VarValue->real : 1.0 , iter2, &commlink->norm));
    gsl_vector_set(ss, 0, (fabs(gsl_vector_get(x,0))>1e-6) ? 0.1 * (gsl_vector_get(x,0)) : 0.1);
    s = gsl_multimin_fminimizer_alloc (T, fn.n);
    gsl_multimin_fminimizer_set (s, &fn, x, ss);

    // If initial value we are giving the minimiser produces an algebraic error, it's not worth continuing
    commlink->GoneNaN    = 0;
    commlink->WorstScore = WORSTSCORE_INIT;
    testval = eps_plot_LAUSlave(x,(void *)commlink);
    if (!gsl_finite(testval)) { commlink->fail=1; return; }
    iter                 = 0;
    commlink->GoneNaN    = 0;
    commlink->WorstScore = WORSTSCORE_INIT;
    do
     {
      iter++;
      for (i=0; i<4; i++) { status = gsl_multimin_fminimizer_iterate(s); if (status) break; } // Run optimiser four times for good luck
      if (status) break;
      sizelast = size;
      size     = gsl_multimin_fminimizer_minimum(s); // Do this ten times, or up to 50, until fit stops improving
     }
    while ((iter < 10) || ((size < sizelast) && (iter < 50))); // Iterate 10 times, and then see whether size carries on getting smaller

    // Read off best-fit value from s->x
    if (commlink->mode==0) commlink->VarValue->real                       = ppl_optimise_LogToReal(gsl_vector_get(s->x, 0), iter2, &commlink->norm);
    else                   commlink->VarValue->exponent[commlink->mode-1] = ppl_optimise_LogToReal(gsl_vector_get(s->x, 0), iter2, &commlink->norm);
    gsl_multimin_fminimizer_free(s);
    if (iter2==999) { iter2=2; break; }
   }
  while ((iter2 < 4) || ((commlink->GoneNaN==0) && (!status) && (size < sizelast2) && (iter2 < 20))); // Iterate 2 times, and then see whether size carries on getting smaller

  if (iter2>=20) status=1;
  if (status) { commlink->fail=1; }
  gsl_vector_free(x);
  gsl_vector_free(ss);
  return;
 }

void eps_plot_LinkUsingBackPropagate(EPSComm *x, double val, pplset_axis *target, pplset_axis *source)
 {
  LAUComm commlink;
  char   *VarName;
  pplObj  DummyTemp, *VarVal;

  if (target->HardMinSet && target->HardMaxSet) return;

  if      (target->xyz==0) VarName = "x";
  else if (target->xyz==1) VarName = "y";
  else if (target->xyz==2) VarName = "z";
  else                     VarName = "c";

  // Look up xyz dummy variable in user space and get pointer to its value
  ppl_contextGetVarPointer(x->c, VarName, &VarVal, &DummyTemp);

  // Set up commlink structure to use in minimiser
  commlink.x           = x;
  commlink.expr        = (pplExpr *)source->linkusing;
  commlink.VarName     = VarName;
  commlink.VarValue    = VarVal;
  commlink.GoneNaN     = commlink.mode = 0;
  commlink.WorstScore  = WORSTSCORE_INIT;
  commlink.target      = source->DataUnit;
  commlink.target.real = val;
  commlink.fail        = 0;

  // First run minimiser to get numerical value of xyz. Then fit each dimension in turn.
  pplObjNum(VarVal,0,1,0);
  for (commlink.mode=0; commlink.mode<UNITS_MAX_BASEUNITS+1; commlink.mode++)
   {
    // if ((commlink.mode==UNIT_ANGLE+1)&&(x->c->set->term_current.UnitAngleDimless == SW_ONOFF_ON)) continue;
    eps_plot_LAUFitter(&commlink);
    if (commlink.fail) break;
   }

  // If there was a problem, throw a warning and proceed no further
  if (commlink.fail)
   {
    sprintf(x->c->errcontext.tempErrStr, "Could not propagate axis range information from axis %c%d of plot %d to axis %c%d of plot %d using expression <%s>. Recommend setting an explicit range for axis %c%d of plot %d.", "xyzc"[source->xyz], source->axis_n, source->canvas_id, "xyzc"[target->xyz], target->axis_n, target->canvas_id, commlink.expr->ascii, "xyzc"[target->xyz], target->axis_n, target->canvas_id);
    ppl_warning(&x->c->errcontext, ERR_GENERIC, NULL);
   }
  else
   {
    int i;
    VarVal->dimensionless=1; // Cycle through powers of final value of xyz dummy value making things which are nearly ints into ints.
    for (i=0; i<UNITS_MAX_BASEUNITS; i++)
     {
      if      (fabs(floor(VarVal->exponent[i]) - VarVal->exponent[i]) < 1e-12) VarVal->exponent[i] = floor(VarVal->exponent[i]);
      else if (fabs(ceil (VarVal->exponent[i]) - VarVal->exponent[i]) < 1e-12) VarVal->exponent[i] = ceil (VarVal->exponent[i]);
      if (VarVal->exponent[i] != 0) VarVal->dimensionless=0;
     }

    // Check that dimension of propagated value fits with existing unit of axis
    if      ((target->HardUnitSet) && (!ppl_unitsDimEqual(&target->HardUnit, VarVal)))
     {
      sprintf(x->c->errcontext.tempErrStr, "Could not propagate axis range information from axis %c%d of plot %d to axis %c%d of plot %d using expression <%s>. Propagated axis range has units of <%s>, but axis %c%d of plot %d has a range set with units of <%s>.", "xyzc"[source->xyz], source->axis_n, source->canvas_id, "xyzc"[target->xyz], target->axis_n, target->canvas_id, commlink.expr->ascii, ppl_printUnit(x->c,VarVal,NULL,NULL,0,1,0), "xyzc"[target->xyz], target->axis_n, target->canvas_id, ppl_printUnit(x->c,&target->HardUnit,NULL,NULL,1,1,0));
      ppl_warning(&x->c->errcontext, ERR_GENERIC, NULL);
     }
    else if ((target->DataUnitSet) && (!ppl_unitsDimEqual(&target->DataUnit, VarVal)))
     {
      sprintf(x->c->errcontext.tempErrStr, "Could not propagate axis range information from axis %c%d of plot %d to axis %c%d of plot %d using expression <%s>. Propagated axis range has units of <%s>, but axis %c%d of plot %d has data plotted against it with units of <%s>.", "xyzc"[source->xyz], source->axis_n, source->canvas_id, "xyzc"[target->xyz], target->axis_n, target->canvas_id, commlink.expr->ascii, ppl_printUnit(x->c,VarVal,NULL,NULL,0,1,0), "xyzc"[target->xyz], target->axis_n, target->canvas_id, ppl_printUnit(x->c,&target->DataUnit,NULL,NULL,1,1,0));
      ppl_warning(&x->c->errcontext, ERR_GENERIC, NULL);
     }
    else if (VarVal->flagComplex)
     {
      sprintf(x->c->errcontext.tempErrStr, "Could not propagate axis range information from axis %c%d of plot %d to axis %c%d of plot %d using expression <%s>. Axis usage was a complex number.", "xyzc"[source->xyz], source->axis_n, source->canvas_id, "xyzc"[target->xyz], target->axis_n, target->canvas_id, commlink.expr->ascii);
      ppl_warning(&x->c->errcontext, ERR_GENERIC, NULL);
     }
    else if (!gsl_finite(VarVal->real))
     {
      sprintf(x->c->errcontext.tempErrStr, "Could not propagate axis range information from axis %c%d of plot %d to axis %c%d of plot %d using expression <%s>. Axis usage was a non-finite number.", "xyzc"[source->xyz], source->axis_n, source->canvas_id, "xyzc"[target->xyz], target->axis_n, target->canvas_id, commlink.expr->ascii);
      ppl_warning(&x->c->errcontext, ERR_GENERIC, NULL);
     }
    else
     {
      if ((!target->MinUsedSet) || (target->MinUsed > VarVal->real)) { target->MinUsed=VarVal->real; target->MinUsedSet=1; }
      if ((!target->MaxUsedSet) || (target->MaxUsed < VarVal->real)) { target->MaxUsed=VarVal->real; target->MaxUsedSet=1; }
      if (source->DataUnitSet) { target->DataUnit = *VarVal; target->DataUnitSet = 1; }
     }
   }

  // Restore original value of x (or y/z)
  ppl_contextRestoreVarPointer(x->c, VarName, &DummyTemp);
  return;
 }

