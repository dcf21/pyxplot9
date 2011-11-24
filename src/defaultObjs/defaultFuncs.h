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

#endif

