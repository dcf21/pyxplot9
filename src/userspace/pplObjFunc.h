// pplObjFunc.h
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

#ifndef _PPLOBJFUNC_H
#define _PPLOBJFUNC_H 1

// Structures for describing functions

#include <gsl/gsl_spline.h>

#ifdef HAVE_FFTW3
#include <fftw3.h>
#else
#include <fftw.h>
#endif

#include "pplObj.h"

#define PPL_FUNC_USERDEF    32050
#define PPL_FUNC_SYSTEM     32051
#define PPL_FUNC_MAGIC      32052
#define PPL_FUNC_SPLINE     32053
#define PPL_FUNC_INTERP2D   32054
#define PPL_FUNC_BMPDATA    32055
#define PPL_FUNC_HISTOGRAM  32056
#define PPL_FUNC_FFT        32057
#define PPL_FUNC_SUBROUTINE 32058

typedef struct FunctionDescriptor
 {
  int     functionType, refCount;
  int     minArgs , maxArgs;
  void   *functionPtr;
  char   *argList;
  pplObj *min, *max; // Range of values over which this function definition can be used; used in function splicing
  unsigned char *minActive, *maxActive, numOnly, notNan, realOnly, dimlessOnly;
  struct FunctionDescriptor *next; // A linked list of spliced alternative function definitions
  char   *LaTeX;
  char   *description, *descriptionShort;
 } pplFunc;

typedef struct SubroutineDescriptor
 {
  int        nArgs;
  char      *argList;
 } SubroutineDescriptor;


typedef struct SplineDescriptor
 {
  gsl_spline       *SplineObj;
  gsl_interp_accel *accelerator;
  pplObj            UnitX, UnitY, UnitZ;
  long              SizeX, SizeY;
  unsigned char     LogInterp;
  char             *filename, *SplineType;
 } SplineDescriptor;

typedef struct HistogramDescriptor
 {
  long int      Nbins;
  double       *bins;
  double       *binvals;
  unsigned char log;
  pplObj        unit;
  char         *filename;
 } HistogramDescriptor;

typedef struct FFTDescriptor
 {
  int           Ndims;
  int          *XSize;
  fftw_complex *datagrid;
  pplObj       *range, *invrange, OutputUnit;
  double        normalisation;
 } FFTDescriptor;

#endif

