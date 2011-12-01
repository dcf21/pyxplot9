// defaultFuncs.h
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

#ifndef _PPL_DEFAULT_FUNCTIONS_H
#define _PPL_DEFAULT_FUNCTIONS_H 1

#include "coreUtils/dict.h"
#include "settings/settings.h"
#include "userspace/pplObj.h"

void ppl_addSystemFunc   (dict *n, char *name, int minArgs, int maxArgs, int numOnly, int notNan, int realOnly, int dimlessOnly, void *fn, char *shortdesc, char *latex, char *desc);
void pplfunc_abs         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acos        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acosh       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acot        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acoth       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acsc        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acsch       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_airy_ai     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_airy_ai_diff(pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_airy_bi     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_airy_bi_diff(pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_arg         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_asec        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_asech       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_asin        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_asinh       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_atan        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_atanh       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_atan2       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besseli     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselI     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselj     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselJ     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselk     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselK     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_bessely     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselY     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_beta        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ceil        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_conjugate   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_cos         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_cosh        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_cot         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_coth        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_csc         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_csch        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_degrees     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ellK        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ellE        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ellP        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_erf         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_erfc        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_exp         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_expm1       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_expint      (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_finite      (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_floor       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_gamma       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_heaviside   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_0F1  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_1F1  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_2F0  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_2F1  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_U    (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hypot       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_imag        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_jacobi_cn   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_jacobi_dn   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_jacobi_sn   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lambert_W0  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lambert_W1  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ldexp       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_legendreP   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_legendreQ   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_log         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_log10       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_logn        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_max         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_min         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_mod         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_pow         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_prime       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_radians     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_real        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_root        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sec         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sech        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sin         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sinc        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sinh        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sqrt        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tan         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tanh        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tophat      (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_zernike     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_zernikeR    (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_zeta        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText);

#endif

