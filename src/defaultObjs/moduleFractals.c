// moduleFractals.c
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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include <gsl/gsl_cdf.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>
#include <gsl/gsl_math.h>

#include "coreUtils/dict.h"

#include "settings/settings.h"

#include "stringTools/asciidouble.h"

#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

#include "defaultObjs/moduleFractals.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

void pplfunc_julia       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "julia(z,cz,MaxIter)";
  CHECK_NEEDLONG(in[2], "MaxIter", "function's third argument must be");
   {
    double  y = in[0].imag  ,  x = in[0].real, x2;
    double cy = in[1].imag  , cx = in[1].real;
    long MaxIter=(long)(in[2].real), iter;

    for (iter=0; ((iter<MaxIter)&&((x*x+y*y)<4)); iter++)
     {
      x2 = x*x - y*y + cx;
      y  = 2*x*y     + cy;
      x  = x2;
     }
    OUTPUT.real = (double)iter;
   }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_mandelbrot  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "mandelbrot(z,MaxIter)";
  CHECK_NEEDLONG(in[1], "MaxIter", "function's second argument must be");
   {
    double  y = in[0].imag  ,  x = in[0].real, x2;
    double cy = in[0].imag  , cx = in[0].real;
    long MaxIter=(long)(in[1].real), iter;

    for (iter=0; ((iter<MaxIter)&&((x*x+y*y)<4)); iter++)
     {
      x2 = x*x - y*y + cx;
      y  = 2*x*y     + cy;
      x  = x2;
     }
    OUTPUT.real = (double)iter;
   }
  CHECK_OUTPUT_OKAY;
 }

