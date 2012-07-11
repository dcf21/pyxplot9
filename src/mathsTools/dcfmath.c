// dcfmath.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

// A selection of useful mathematical functions which are not included in the standard C math library

#define _PPL_DCFMATH_C 1

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include "dcfmath.h"

double ppl_machineEpsilon;

void ppl_makeMachineEpsilon()
 {
  ppl_machineEpsilon = 1.0;
  do { ppl_machineEpsilon /= 2.0; } while ((1.0 + (ppl_machineEpsilon/2.0)) != 1.0);
  return;
 }

double ppl_max(double x, double y)
 {
  if (x>y) return x;
  return y;
 }

double ppl_min(double x, double y)
 {
  if (x<y) return x;
  return y;
 }

double ppl_max3(double x, double y, double z)
 {
  double o = x>y?x:y;
  if (o>z) return o;
  return z;
 }

double ppl_min3(double x, double y, double z)
 {
  double o = x<y?x:y;
  if (o<z) return o;
  return z;
 }

int ppl_sgn(double x)
 {
  if (x==0) return  0;
  if (x< 0) return -1;
  return 1;
 }

void ppl_linRaster(double *out, double min, double max, int Nsteps)
 {
  int i;
  if (Nsteps < 2) Nsteps = 2; // Avoid division by zero
  for (i=0; i<Nsteps; i++) out[i] = min + (max - min) * ((double)i / (double)(Nsteps-1));
  return;
 }

void ppl_logRaster(double *out, double min, double max, int Nsteps)
 {
  int i;
  if (min < 1e-200) min = 1e-200; // Avoid log of negative numbers or zero
  if (max < 1e-200) max = 1e-200;
  if (Nsteps < 2) Nsteps = 2; // Avoid division by zero
  for (i=0; i<Nsteps; i++) out[i] = min * pow(max / min , (double)i / (double)(Nsteps-1));
  return;
 }

double ppl_degs(double rad)
 {
  return rad*180/M_PI;
 }

double ppl_rads(double degrees)
 {
  return degrees*M_PI/180;
 }

int ppl_dblSort(const void *a, const void *b)
 {
  const double *da = (const double *)a;
  const double *db = (const double *)b;
  if (*da<*db) return -1;
  if (*da>*db) return  1;
  return 0;
 }

