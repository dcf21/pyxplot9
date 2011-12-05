// pplObjFunc.c
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

#include "stdlib.h"

#ifdef HAVE_FFTW3
#include <fftw3.h>
#else
#include <fftw.h>
#endif

#include "userspace/pplObj.h"
#include "userspace/pplObjFunc_fns.h"

void pplObjFuncDestroy(pplFunc *f)
 {
  int type = f->functionType;
  if ((type==PPL_FUNC_SYSTEM)||(type==PPL_FUNC_MAGIC)) return; // Never garbage collect system functions

  switch(type)
   {
    case PPL_FUNC_USERDEF:
     {
      break;
     }
    case PPL_FUNC_SPLINE:
     {
      SplineDescriptor *d = (SplineDescriptor *)f->functionPtr;
      f->functionPtr=NULL;
      if (d!=NULL)
       {
        if (d->SplineType[1]!='t') // Not stepwise interpolation
         {
          if (d->SplineObj   != NULL) gsl_spline_free      (d->SplineObj  );
          if (d->accelerator != NULL) gsl_interp_accel_free(d->accelerator);
         }
        else
         {
          if (d->SplineObj   != NULL) free(d->SplineObj);
         }
       }
      break;
     }
    case PPL_FUNC_INTERP2D:
    case PPL_FUNC_BMPDATA:
     {
      break;
     }
    case PPL_FUNC_HISTOGRAM:
     {
      HistogramDescriptor *h = (HistogramDescriptor *)f->functionPtr;
      f->functionPtr=NULL;
      if (h!=NULL)
       {
        if (h->bins   != NULL ) free(h->bins   );
        if (h->binvals!= NULL ) free(h->binvals);
       }
      free(h);
      break;
     }
    case PPL_FUNC_FFT:
     {
      FFTDescriptor *d = (FFTDescriptor *)f->functionPtr;
      f->functionPtr=NULL;
      if (d!=NULL)
       {
        if (d->XSize    != NULL )      free(d->XSize   );
        if (d->range    != NULL )      free(d->range   );
        if (d->datagrid != NULL ) fftw_free(d->datagrid);
       }
      break;
     }
    case PPL_FUNC_SUBROUTINE:
     {
      break;
     }
   }
  return;
 }

