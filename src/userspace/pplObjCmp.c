// expEvalOps.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
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
#include <math.h>
#include <string.h>

#include <gsl/gsl_math.h>

#include "mathsTools/dcfmath.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

void pplcol_RGBtoHSB(double ri, double gi, double bi, double *ho, double *so, double *bo)
 {
  double M = ppl_max3(ri,gi,bi);
  double m = ppl_min3(ri,gi,bi);
  double C = M-m;
  if      (C== 0) *ho = 0;
  else if (C==ri) *ho = fmod((gi-bi)/C , 6);
  else if (C==gi) *ho = (bi-ri)/C + 2;
  else            *ho = (ri-gi)/C + 4;
  *ho /= 6.0;
  *so = C/M;
  *bo = M;
  if (!gsl_finite(*so)) *so=0;
  if (*ho<0) *ho=0; if (*ho>1) *ho=1;
  if (*so<0) *so=0; if (*so>1) *so=1;
  if (*bo<0) *bo=0; if (*bo>1) *bo=1;
 }

void pplcol_CMYKtoRGB(double ci, double mi, double yi, double ki, double *ro, double *go, double *bo)
 {
  *ro = 1.0 - (ci+ki);
  *go = 1.0 - (mi+ki);
  *bo = 1.0 - (yi+ki);
  if (*ro<0) *ro=0; if (*ro>1) *ro=1;
  if (*go<0) *go=0; if (*go>1) *go=1;
  if (*bo<0) *bo=0; if (*bo>1) *bo=1;
 }

void pplcol_HSBtoRGB(double hi, double si, double bi, double *ro, double *go, double *bo)
 {
  double ch  = si*bi;
  double h2;
  int    h2i = (int)(h2 = hi * 6);
  double x   = ch*(1.0-fabs(fmod(h2,2)-1.0));
  double m   = bi - ch;
  switch (h2i)
   {
    case 0 : *ro=ch; *go=x ; *bo=0 ; break;
    case 1 : *ro=x ; *go=ch; *bo=0 ; break;
    case 2 : *ro=0 ; *go=ch; *bo=x ; break;
    case 3 : *ro=0 ; *go=x ; *bo=ch; break;
    case 4 : *ro=x ; *go=0 ; *bo=ch; break;
    case 5 :
    case 6 : *ro=ch; *go=0 ; *bo=x ; break; // case 6 is for hue=1.0 only
    default: *ro=0 ; *go=0 ; *bo=0 ; break;
   }
  *ro+=m; *go+=m; *bo+=m;

  if (*ro<0) *ro=0; if (*ro>1) *ro=1;
  if (*go<0) *go=0; if (*go>1) *go=1;
  if (*bo<0) *bo=0; if (*bo>1) *bo=1;
 }

void pplcol_CMYKtoHSB(double ci, double mi, double yi, double ki, double *ho, double *so, double *bo)
 {
  double r,g,b;
  pplcol_CMYKtoRGB(ci,mi,yi,ki,&r,&g,&b);
  pplcol_RGBtoHSB(r,g,b,ho,so,bo);
 }

int pplObjCmp(ppl_context *c, pplObj *a, pplObj *b, int *status, int *errType, char *errText)
 {
  int t1  = a->objType;
  int t2  = b->objType;
  int t1o = pplObjTypeOrder[t1];
  int t2o = pplObjTypeOrder[t2];
  if (t1o < t2o) return -1;
  if (t1o > t2o) return  1;
  if  (t1o==0) return -2; // 0 - nulls are never equal
  if  (t1o==2) // 2 - numbers
   {
    double av,bv;
    if (ppl_unitsDimEqual(a,b)==0)
     {
      *status=1; *errType=ERR_UNIT;
      if (a->dimensionless)
       sprintf(errText, "Attempt to compare a quantity which is dimensionless with one with dimensions of <%s>.", ppl_printUnit(c,b,NULL,NULL,1,1,0));
      else if (b->dimensionless)
       sprintf(errText, "Attempt to compare a quantity with dimensions of <%s> with one which is dimensionless.", ppl_printUnit(c,a,NULL,NULL,0,1,0));
      else
       sprintf(errText, "Attempt to compare a quantity with dimensions of <%s> with one with dimensions of <%s>.", ppl_printUnit(c,a,NULL,NULL,0,1,0), ppl_printUnit(c,b,NULL,NULL,1,1,0));
      return -2;
     }
    if ((c->set->term_current.ComplexNumbers==SW_ONOFF_OFF)&&((a->flagComplex)||(b->flagComplex))) return -2;
    if ((!gsl_finite(a->real))||(!gsl_finite(a->imag))||(!gsl_finite(b->real))||(!gsl_finite(b->imag))) return -2;
    av = hypot(a->real,a->imag);
    bv = hypot(b->real,b->imag);
    if (av <bv) return -1;
    if (av >bv) return  1;
    if (av==bv) return  0;
    return -2;
   }
  else if ((t1o==2) || (t1o==3)) // 2 - booleans; 3 - dates
   {
    if (a->real <b->real) return -1;
    if (a->real >b->real) return  1;
    if (a->real==b->real) return  0;
    return -2;
   }
  else if ((t1o==4)||(t1o==14)||(t1o==15)) // 4 - strings; 14 - data type; 15 - exception
    return strcmp((char*)a->auxil,(char*)b->auxil);
  else if (t1o==5) // 5 - colors
   {
    double h1,s1,b1,h2,s2,b2;
    if      (round(a->exponent[0])==SW_COLSPACE_RGB ) pplcol_RGBtoHSB (a->exponent[8],a->exponent[9],a->exponent[10],&h1,&s1,&b1);
    else if (round(a->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoHSB(a->exponent[8],a->exponent[9],a->exponent[10],a->exponent[11],&h1,&s1,&b1);
    else                                              { h1=a->exponent[8]; s1=a->exponent[9]; b1=a->exponent[10]; }
    if      (round(b->exponent[0])==SW_COLSPACE_RGB ) pplcol_RGBtoHSB (b->exponent[8],b->exponent[9],b->exponent[10],&h2,&s2,&b2);
    else if (round(b->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoHSB(b->exponent[8],b->exponent[9],b->exponent[10],b->exponent[11],&h2,&s2,&b2);
    else                                              { h2=b->exponent[8]; s2=b->exponent[9]; b2=b->exponent[10]; }
    if ((!gsl_finite(h1))||(!gsl_finite(s1))||(!gsl_finite(b1))||(!gsl_finite(h2))||(!gsl_finite(s2))||(!gsl_finite(b2))) return -2;
    if (h1 < h2) return -1;
    if (h1 > h2) return  1;
    if (b1 < b2) return -1;
    if (b1 > b2) return  1;
    if (s1 < s2) return -1;
    if (s1 > s2) return  1;
    return 0;
   }
  else if (t1o==6) // 6 - vector
   {
    gsl_vector *av = ((pplVector *)a->auxil)->v;
    gsl_vector *bv = ((pplVector *)b->auxil)->v;
    if (av->size < bv->size) return -1;
    if (av->size > bv->size) return  1;
    if (av < bv) return -1;
    if (av > bv) return  1;
    return 0;
   }
  else if (t1o==7) // 7 - list
   {
    list *la = (list *)a->auxil;
    list *lb = (list *)b->auxil;
    if (la->length < lb->length) return -1;
    if (la->length > lb->length) return  1;
    if (la < lb) return -1;
    if (la > lb) return  1;
    return 0;
   }
  else if (t1o==8) // 8 - matrix
   {
    gsl_matrix *am = ((pplMatrix *)a->auxil)->m;
    gsl_matrix *bm = ((pplMatrix *)b->auxil)->m;
    if (am->size1*am->size2 < bm->size1*bm->size2) return -1;
    if (am->size1*am->size2 > bm->size1*bm->size2) return  1;
    if (am < bm) return -1;
    if (am > bm) return  1;
    return 0;
   }
  else if ((t1o==9)||(t1o==10)||(t1o==11)) // 9 - dictionary; 10 - instance; 11 - module
   {
    dict *da = (dict *)a->auxil;
    dict *db = (dict *)b->auxil;
    if (da->length < db->length) return -1;
    if (da->length > db->length) return  1;
    if (da < db) return -1;
    if (da > db) return  1;
    return 0;
   }
  else if ((t1o==12)||(t1o==13)) // 12 - function; 13 - file handle
   {
    if (a->auxil < b->auxil) return -1;
    if (a->auxil > b->auxil) return -1;
    return 0;
   }
  return -2;
 }

