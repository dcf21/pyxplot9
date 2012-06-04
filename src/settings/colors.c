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
#include "settings/epsColors.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjCmp.h"
#include "pplConstants.h"

int ppl_colorFromDict  (ppl_context *c, parserOutput *in, parserLine *pl, const int *ptab, int stkbase,
                        int fillColor, int *outcol, int *outcolspace, pplExpr **EXPoutcol,
                        double *outcol1, double *outcol2, double *outcol3, double *outcol4,
                        unsigned char *USEcol, unsigned char *USEcol1234)
 {
  int     pos = ptab[ fillColor ? PARSE_INDEX_fillcolor : PARSE_INDEX_color];
  pplObj *col = &in->stk[stkbase+pos];
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
      int    j,k1,k2,palette_index1,palette_index2;
      double w1,w2;
      for (j=1; j<PALETTE_LENGTH; j++) if (c->set->palette_current[j]==-1) break;
      k1 = (int)floor(col->real);
      if (k1 > INT_MAX-8) k1=INT_MAX-8;
      if (k1 < INT_MIN+8) k1=INT_MIN+8;
      k2 = k1+1; // Mix color indexes k1 and k2
      w2 = col->real - k1; // weights for two colors
      if (w2<=0) w2=0; if (w2>=1) w2=1;
      w1 = 1-w2;
      palette_index1 = (k1-1)%j; while (palette_index1 < 0) palette_index1+=j;
      palette_index2 = (k2-1)%j; while (palette_index2 < 0) palette_index2+=j;

// ALWAYS DO COLOR MIXING -- OTHERWISE CMYK COLORS MAY RENDER DIFFERENTLY FROM CMYK COLORS
//
//    if (w2<1e-3) // If weight for color 2 is very small, use color 1 (preserving original color space)
//     {
//      *outcol      = c->set->palette_current [palette_index1];
//      *outcolspace = c->set->paletteS_current[palette_index1];
//      *outcol1     = c->set->palette1_current[palette_index1];
//      *outcol2     = c->set->palette2_current[palette_index1];
//      *outcol3     = c->set->palette3_current[palette_index1];
//      *outcol4     = c->set->palette4_current[palette_index1];
//      if (USEcol    !=NULL) *USEcol      = (*outcol >0);
//      if (USEcol1234!=NULL) *USEcol1234  = (*outcol==0);
//     }
//    else if (w1<1e-3) // If weight for color 1 is very small, use color 2 (preserving original color space)
//     {
//      *outcol      = c->set->palette_current [palette_index2];
//      *outcolspace = c->set->paletteS_current[palette_index2];
//      *outcol1     = c->set->palette1_current[palette_index2];
//      *outcol2     = c->set->palette2_current[palette_index2];
//      *outcol3     = c->set->palette3_current[palette_index2];
//      *outcol4     = c->set->palette4_current[palette_index2];
//      if (USEcol    !=NULL) *USEcol      = (*outcol >0);
//      if (USEcol1234!=NULL) *USEcol1234  = (*outcol==0);
//     }
//    else // Otherwise do weighted color mixing in RGB space

       {
        double r1,g1,b1,r2,g2,b2;
        double i11,i12,i13,i14,i21,i22,i23,i24;
        int s1,s2;
        if (c->set->palette_current[palette_index1]>0)
         {
          s1  = SW_COLSPACE_CMYK;
          i11 = *(double *)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[palette_index1], SW_COLOR_INT, (void *)SW_COLOR_CMYK_C, sizeof(double));
          i12 = *(double *)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[palette_index1], SW_COLOR_INT, (void *)SW_COLOR_CMYK_M, sizeof(double));
          i13 = *(double *)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[palette_index1], SW_COLOR_INT, (void *)SW_COLOR_CMYK_Y, sizeof(double));
          i14 = *(double *)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[palette_index1], SW_COLOR_INT, (void *)SW_COLOR_CMYK_K, sizeof(double));
         }
        else
         {
          s1  = c->set->paletteS_current[palette_index1];
          i11 = c->set->palette1_current[palette_index1];
          i12 = c->set->palette2_current[palette_index1];
          i13 = c->set->palette3_current[palette_index1];
          i14 = c->set->palette4_current[palette_index1];
         }
        if (c->set->palette_current[palette_index2]>0)
         {
          s2  = SW_COLSPACE_CMYK;
          i21 = *(double *)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[palette_index2], SW_COLOR_INT, (void *)SW_COLOR_CMYK_C, sizeof(double));
          i22 = *(double *)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[palette_index2], SW_COLOR_INT, (void *)SW_COLOR_CMYK_M, sizeof(double));
          i23 = *(double *)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[palette_index2], SW_COLOR_INT, (void *)SW_COLOR_CMYK_Y, sizeof(double));
          i24 = *(double *)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[palette_index2], SW_COLOR_INT, (void *)SW_COLOR_CMYK_K, sizeof(double));
         }
        else
         {
          s2  = c->set->paletteS_current[palette_index2];
          i21 = c->set->palette1_current[palette_index2];
          i22 = c->set->palette2_current[palette_index2];
          i23 = c->set->palette3_current[palette_index2];
          i24 = c->set->palette4_current[palette_index2];
         }
        if      (s1==SW_COLSPACE_RGB ) { r1=i11; g1=i12; b1=i13; }
        else if (s1==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(i11,i12,i13,i14,&r1,&g1,&b1);
        else                           pplcol_HSBtoRGB (i11,i12,i13,&r1,&g1,&b1);
        if      (s2==SW_COLSPACE_RGB ) { r2=i21; g2=i22; b2=i23; }
        else if (s2==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(i21,i22,i23,i24,&r2,&g2,&b2);
        else                           pplcol_HSBtoRGB (i21,i22,i23,&r2,&g2,&b2);
        *outcol      = 0;
        *outcolspace = SW_COLSPACE_RGB;
        *outcol1     = r1*w1 + r2*w2;
        *outcol2     = g1*w1 + g2*w2;
        *outcol3     = b1*w1 + b2*w2;
        *outcol4     = 0.0;
        if (USEcol    !=NULL) *USEcol      = 0;
        if (USEcol1234!=NULL) *USEcol1234  = 1;
       }
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
    case PPLOBJ_EXP: // color expressed as an expression via %C
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

