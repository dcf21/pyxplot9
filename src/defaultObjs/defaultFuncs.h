// defaultFuncs.h
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

#ifndef _PPL_DEFAULT_FUNCTIONS_H
#define _PPL_DEFAULT_FUNCTIONS_H 1

#include "coreUtils/dict.h"
#include "settings/settings.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

void ppl_addMagicFunction(dict *n, char *name, int id, char *shortdesc, char *latex, char *desc);
void ppl_addSystemFunc   (dict *n, char *name, int minArgs, int maxArgs, int numOnly, int notNan, int realOnly, int dimlessOnly, void *fn, char *shortdesc, char *latex, char *desc);
void pplfunc_abs         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acos        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acosh       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acot        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acoth       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acsc        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_acsch       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_airy_ai     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_airy_ai_diff(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_airy_bi     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_airy_bi_diff(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_arg         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_asec        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_asech       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_asin        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_asinh       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_atan        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_atanh       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_atan2       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besseli     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselI     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselj     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselJ     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselk     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselK     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_bessely     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_besselY     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_beta        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ceil        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_chr         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_classOf     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_cmyk        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_conjugate   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_copy        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_cos         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_cosh        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_cot         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_coth        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_cross       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_csc         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_csch        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_degrees     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_deepcopy    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ellK        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ellE        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ellP        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_erf         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_erfc        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_exp         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_expm1       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_expint      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_factors     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_finite      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_floor       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_gamma       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_globals     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_heaviside   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hsb         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_0F1  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_1F1  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_2F0  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_2F1  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hyperg_U    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_hypot       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_imag        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_jacobi_cn   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_jacobi_dn   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_jacobi_sn   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lambert_W0  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lambert_W1  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ldexp       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_legendreP   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_legendreQ   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_len         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_locals      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_log         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_log10       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_logn        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_lrange      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_max         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_min         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_mod         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_open        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ord         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_ordinal     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_pow         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_prime       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_primefactors(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_radians     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_raise       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_range       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_rgb         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_real        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_root        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sec         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sech        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sgn         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sin         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sinc        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sinh        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sqrt        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sum         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tan         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tanh        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_tophat      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_typeOf      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_zernike     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_zernikeR    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_zeta        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);

#endif

