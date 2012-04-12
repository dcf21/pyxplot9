// colors.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "expressions/expCompile_fns.h"
#include "expressions/traceback_fns.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "settings/colors.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "pplConstants.h"

int ppl_colorFromDict  (ppl_context *c, parserOutput *in, parserLine *pl, const int *ptab, 
                        int fillColor, int *outcol, int *outcolspace, pplExpr **EXPoutcol,
                        double *outcol1, double *outcol2, double *outcol3, double *outcol4,
                        unsigned char *USEcol, unsigned char *USEcol1234)
 {
  int     pos = ptab[ fillColor ? PARSE_INDEX_fillcolor : PARSE_INDEX_color];
  pplObj *col = &in->stk[pos];
  int s;
  if (pos<0) return 0;
  s = ppl_colorFromObj(c, col, outcol, outcolspace, EXPoutcol, outcol1, outcol2, outcol3, outcol4, USEcol, USEcol1234);
  if (s) ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, NULL);
  return s;
 }

int ppl_colorFromObj   (ppl_context *c, const pplObj *col, int *outcol, int *outcolspace, pplExpr **EXPoutcol,
                        double *outcol1, double *outcol2, double *outcol3, double *outcol4,
                        unsigned char *USEcol, unsigned char *USEcol1234)
 {
  switch (col->objType)
   {
    case PPLOBJ_ZOM: // no color specified
      return 0;
    case PPLOBJ_NUM: // color specified as palette index
     {
      int j,palette_index;
      for (j=1; j<PALETTE_LENGTH; j++) if (c->set->palette_current[j]==-1) break;
      palette_index = ((int)col->real-1)%j;
      while (palette_index < 0) palette_index+=j;
      *outcol      = c->set->palette_current [palette_index];
      *outcolspace = c->set->paletteS_current[palette_index];
      *outcol1     = c->set->palette1_current[palette_index];
      *outcol2     = c->set->palette2_current[palette_index];
      *outcol3     = c->set->palette3_current[palette_index];
      *outcol4     = c->set->palette4_current[palette_index];
      if (USEcol    !=NULL) *USEcol      = (*outcol >0);
      if (USEcol1234!=NULL) *USEcol1234  = (*outcol==0);
      return 0;
     }
    case PPLOBJ_COL: // color specified as a color object
     {
      *outcol      = (int)round(col->exponent[2]);
      *outcolspace = (int)round(col->exponent[0]);
      *outcol1     = col->exponent[ 8];
      *outcol2     = col->exponent[ 9];
      *outcol3     = col->exponent[10];
      *outcol4     = col->exponent[11];
      if (USEcol    !=NULL) *USEcol      = (*outcol >0);
      if (USEcol1234!=NULL) *USEcol1234  = (*outcol==0);
      return 0;
     }
    case PPLOBJ_EXP: // colour expressed as an expression via %C
     {
      if (EXPoutcol == NULL)
       {
        sprintf(c->errcontext.tempErrStr, "Colour specified as expression via %%C token, but no pointer given for output of this expression.");
        return 1;
       }
      if (*EXPoutcol == NULL) pplExpr_free(*EXPoutcol);
      *EXPoutcol = (pplExpr *)col->auxil;
      (*EXPoutcol)->refCount++;
      if (USEcol    !=NULL) *USEcol      = 0;
      if (USEcol1234!=NULL) *USEcol1234  = 0;
      return 0;
     }
    default:
     {
      sprintf(c->errcontext.tempErrStr, "Colour specified as wrong type of object (type %d).", col->objType);
      return 1;
     }
   }
  return 1;
 }

