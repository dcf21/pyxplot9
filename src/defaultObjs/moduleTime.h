// moduleTime.h
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

#ifndef _PPL_MODULE_TIME_H
#define _PPL_MODULE_TIME_H 1

#include "userspace/context.h"
#include "userspace/pplObj.h"

void pplfunc_timefromCalendar(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_timefromJD      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_timefromMJD     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_timefromUnix    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_timeInterval    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_timeIntervalStr (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_timenow         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_timestring      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sleep           (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);
void pplfunc_sleepUntil      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText);

#endif

