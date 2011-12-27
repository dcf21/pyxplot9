// modulePhy.h
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

#ifndef _PPL_MODULE_PHY_H
#define _PPL_MODULE_PHY_H 1

#include "coreUtils/dict.h"
#include "settings/settings.h"
#include "userspace/pplObj.h"

void pplfunc_planck_Bv   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_planck_Bvmax(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);

#endif

