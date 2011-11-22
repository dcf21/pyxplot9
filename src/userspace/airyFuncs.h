// airyFuncs.h
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

#ifndef _PPL_AIRY_FUNCTIONS_H
#define _PPL_AIRY_FUNCTIONS_H 1

#include <gsl/gsl_complex.h>

void airy_ai     (gsl_complex in, gsl_complex *out, int *status, char *errtext);
void airy_bi     (gsl_complex in, gsl_complex *out, int *status, char *errtext);
void airy_ai_diff(gsl_complex in, gsl_complex *out, int *status, char *errtext);
void airy_bi_diff(gsl_complex in, gsl_complex *out, int *status, char *errtext);

#endif

