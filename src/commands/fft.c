// fft.c
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

#define _FFT_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#ifdef HAVE_FFTW3
#include <fftw3.h>
#else
#include <fftw.h>
#endif

#include "commands/fft.h"
#include "parser/parser.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

// Window functions for FFTs
// static void fftwindow_rectangular (pplObj *x, int Ndim, int *Npos, int *Nstep) { }
// static void fftwindow_hamming     (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=0.54-0.46*cos(2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
// static void fftwindow_hann        (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=0.5*(1.0-cos(2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1)))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
// static void fftwindow_cosine      (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=sin(M_PI*((double)Npos[i])/((double)(Nstep[i]-1))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
// static void fftwindow_lanczos     (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0,z; int i; for (i=0; i<Ndim; i++) { z=2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))-M_PI; y*=sin(z)/z; } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
// static void fftwindow_bartlett    (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=(2.0/(Nstep[i]-1))*((Nstep[i]-1)/2.0-fabs(Npos[i]-(Nstep[i]-1)/2.0)); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
// static void fftwindow_triangular  (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=(2.0/(Nstep[i]  ))*((Nstep[i]  )/2.0-fabs(Npos[i]-(Nstep[i]-1)/2.0)); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
// static void fftwindow_gauss       (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; const double sigma=0.5; int i; for (i=0; i<Ndim; i++) { y*=exp(-0.5*pow((Npos[i]-((Nstep[i]-1)/2.0))/(sigma*(Nstep[i]-1)/2.0),2.0)); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
// static void fftwindow_bartletthann(pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=0.62 - 0.48*(((double)Npos[i])/((double)(Nstep[i]-1))-0.5) - 0.38*cos(2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
// static void fftwindow_blackman    (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; const double alpha=0.16; int i; for (i=0; i<Ndim; i++) { y*=(1.0-alpha)/2.0 - 0.5*cos(2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))) + alpha/2.0*cos(4*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }

// Main entry point for the FFT command
void directive_fft(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  return;
 }

// Function which is called whenever an FFT function is evaluated, to extract value out of data grid
void ppl_fft_evaluate(ppl_context *c, char *FuncName, FFTDescriptor *desc, pplObj *in, pplObj *out, int *status, char *errout)
 {
  int i, j;
  double tempDbl;

  *out = desc->outputUnit;

  // Issue user with a warning if complex arithmetic is not enabled
  if (c->set->term_current.ComplexNumbers != SW_ONOFF_ON) ppl_warning(&c->errcontext, ERR_NUMERIC, "Attempt to evaluate a Fourier transform function whilst complex arithmetic is disabled. Fourier transforms are almost invariably complex and so this is unlikely to work.");

  // Check dimensions of input arguments and ensure that they are all real
  for (i=0; i<desc->Ndims; i++)
   {
    if (!ppl_unitsDimEqual(in+i, &desc->invRange[i]))
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errout, "The %s(x) function expects argument %d to have dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, i+1, ppl_printUnit(c, &desc->invRange[i], NULL, NULL, 0, 1, 0), ppl_printUnit(c, in+i, NULL, NULL, 1, 1, 0)); }
      else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
      *status=1;
      return;
     }
    if ((in+i)->flagComplex)
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errout, "The %s(x) function expects argument %d to be real, but the supplied argument has an imaginary component.", FuncName, i+1); }
      else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
      *status=1;
      return;
     }
   }

  // Work out closest datapoint in FFT datagrid to the one we want
  j=0;
  for (i=0; i<desc->Ndims; i++)
   {
    tempDbl = floor((in+i)->real * desc->range[i].real + 0.5);
    if      ((tempDbl >= 0.0) && (tempDbl <= desc->XSize[i]/2)) { } // Positive frequencies stored in lower half of array
    else if ((tempDbl <  0.0) && (tempDbl >=-desc->XSize[i]/2)) { tempDbl += desc->XSize[i]; } // Negative frequencies stored in upper half of array
    else                                                        { return; } // Query out of range; return zero with appropriate output unit
    j *= desc->XSize[i];
    j += (int)tempDbl;
   }

  // Write output value to out
  #ifdef HAVE_FFTW3
  out->real = desc->datagrid[j][0];
  if (desc->datagrid[j][1] == 0.0) { out->flagComplex = 0; out->imag = 0.0;                  }
  else                             { out->flagComplex = 1; out->imag = desc->datagrid[j][1]; }
  #else
  out->real = desc->datagrid[j].re;
  if (desc->datagrid[j].im == 0.0) { out->flagComplex = 0; out->imag = 0.0;                  }
  else                             { out->flagComplex = 1; out->imag = desc->datagrid[j].im; }
  #endif
  return;
 }

