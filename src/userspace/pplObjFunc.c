// pplObjFunc.c
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

#include "stdlib.h"
#include "string.h"

#ifdef HAVE_FFTW3
#include <fftw3.h>
#else
#include <fftw.h>
#endif

#include "expressions/expCompile.h"

#include "parser/parser.h"

#include "userspace/pplObj.h"
#include "userspace/pplObjFunc_fns.h"

void pplObjFuncDestroyChain(pplFunc *f)
 {
  pplFunc *next;
  while (f!=NULL)
   {
    next=f->next;
    pplObjFuncDestroy(f);
    f=next;
   }
  return;
 }

void pplObjFuncDestroy(pplFunc *f)
 {
  int type = f->functionType;
  if ((type==PPL_FUNC_SYSTEM)||(type==PPL_FUNC_MAGIC)) return; // Never garbage collect system functions

  switch(type)
   {
    case PPL_FUNC_USERDEF:
     {
      pplExpr *e = (pplExpr *)f->functionPtr;
      f->functionPtr=NULL;
      if (e!=NULL) pplExpr_free(e);
      break;
     }
    case PPL_FUNC_SPLINE:
     {
      splineDescriptor *d = (splineDescriptor *)f->functionPtr;
      f->functionPtr=NULL;
      if (d!=NULL)
       {
        if (d->splineType[1]!='t') // Not stepwise interpolation
         {
          if (d->splineObj   != NULL) gsl_spline_free      (d->splineObj  );
          if (d->accelerator != NULL) gsl_interp_accel_free(d->accelerator);
         }
        else
         {
          if (d->splineObj   != NULL) free(d->splineObj);
         }
        free(d);
       }
      break;
     }
    case PPL_FUNC_INTERP2D:
    case PPL_FUNC_BMPDATA:
     {
      splineDescriptor *d = (splineDescriptor *)f->functionPtr;
      f->functionPtr=NULL;
      if (d!=NULL)
       {
        if (d->splineObj != NULL) free(d->splineObj);
        free(d);
       }
      break;
     }
    case PPL_FUNC_HISTOGRAM:
     {
      histogramDescriptor *h = (histogramDescriptor *)f->functionPtr;
      f->functionPtr=NULL;
      if (h!=NULL)
       {
        if (h->bins   != NULL ) free(h->bins   );
        if (h->binvals!= NULL ) free(h->binvals);
        free(h);
       }
      break;
     }
    case PPL_FUNC_FFT:
     {
      FFTDescriptor *d = (FFTDescriptor *)f->functionPtr;
      f->functionPtr=NULL;
      if (d!=NULL)
       {
        if (d->datagrid != NULL) fftw_free(d->datagrid);
        free(d);
       }
      break;
     }
    case PPL_FUNC_SUBROUTINE:
     {
      parserLine *s = (parserLine *)f->functionPtr;
      f->functionPtr=NULL;
      if (s!=NULL) ppl_parserLineFree(s);
      break;
     }
   }

  if (f->argList    !=NULL) free(f->argList);
  if (f->min        !=NULL) free(f->min);
  if (f->max        !=NULL) free(f->max);
  if (f->minActive  !=NULL) free(f->minActive);
  if (f->maxActive  !=NULL) free(f->maxActive);
  if (f->LaTeX      !=NULL) free(f->LaTeX);
  free(f);
  return;
 }

pplFunc *pplObjFuncCpy(pplFunc *f)
 {
  pplFunc *o;
  int i,j;
  int Nargs;
  if (f->functionType != PPL_FUNC_USERDEF) return NULL;

  Nargs=f->maxArgs;
  for (j=0,i=0;i<Nargs;i++) while (f->argList[j++]!='\0');

  if ((o = (pplFunc *)malloc(sizeof(pplFunc)))==NULL) return NULL;
       o->functionType = PPL_FUNC_USERDEF;
       o->minArgs      = f->minArgs;
       o->maxArgs      = f->maxArgs;
  if ((o->functionPtr  = (void   *)pplExpr_cpy((pplExpr *)f->functionPtr))==NULL) return NULL;
  if ((o->argList      = (char   *)malloc(j                    ))==NULL) return NULL; memcpy( o->argList    , f->argList   , j );
  if ((o->min          = (pplObj *)malloc(Nargs*sizeof(pplObj) ))==NULL) return NULL; memcpy( o->min        , f->min       , Nargs*sizeof(pplObj));
  if ((o->max          = (pplObj *)malloc(Nargs*sizeof(pplObj) ))==NULL) return NULL; memcpy( o->max        , f->max       , Nargs*sizeof(pplObj));
  if ((o->minActive    = (unsigned char *)malloc(Nargs         ))==NULL) return NULL; memcpy( o->minActive  , f->minActive , Nargs);
  if ((o->maxActive    = (unsigned char *)malloc(Nargs         ))==NULL) return NULL; memcpy( o->maxActive  , f->maxActive , Nargs);
       o->next         = NULL;
       o->description  = NULL;
       o->descriptionShort = NULL;
       o->LaTeX        = NULL;
  return o;
 }

