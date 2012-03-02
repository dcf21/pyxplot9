// axes_fns.h
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

#ifndef _PPLSET_AXES_FNS_H
#define _PPLSET_AXES_FNS_H 1

#include "settings/settings.h"
#include "userspace/context.h"

void          pplaxis_destroy  (ppl_context *context, pplset_axis *in);
void          pplaxis_copy     (ppl_context *context, pplset_axis *out, const pplset_axis *in);
void          pplaxis_copyTics (ppl_context *context, pplset_axis *out, const pplset_axis *in);
void          pplaxis_copyMTics(ppl_context *context, pplset_axis *out, const pplset_axis *in);
unsigned char pplaxis_cmpTics  (ppl_context *context, const pplset_axis *a, const pplset_axis *b);
unsigned char pplaxis_cmpMTics (ppl_context *context, const pplset_axis *a, const pplset_axis *b);

#endif

