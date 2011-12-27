// calendars.h
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

#ifndef _CALENDARS_H
#define _CALENDARS_H 1

#include "userspace/context.h"

void   ppl_switchOverCalDate  (ppl_context *ct, double *LastJulian, double *FirstGregorian);
double ppl_switchOverJD       (ppl_context *ct);
char  *ppl_getMonthName       (ppl_context *ct, int i);
char  *ppl_getWeekDayName     (ppl_context *ct, int i);
void   ppl_getHebrewNewYears  (ppl_context *ct, int GregYear, int *YearNumbers, double *JDs, int *YearTypes);
double ppl_hebrewToUnixTime   (ppl_context *ct, int year, int month, int day, int hour, int min, int sec, int *status, char *errText);
void   ppl_hebrewFromUnixTime (ppl_context *ct, double UT, int *year, int *month, int *day, int *status, char *errText);
double ppl_islamicToUnixTime  (ppl_context *ct, int year, int month, int day, int hour, int min, int sec, int *status, char *errText);
void   ppl_islamicFromUnixTime(ppl_context *ct, double UT, int *year, int *month, int *day, int *status, char *errText);
double ppl_toUnixTime         (ppl_context *ct, int year, int month, int day, int hour, int min, int sec, int *status, char *errText);
void   ppl_fromUnixTime       (ppl_context *ct, double UT, int *year, int *month, int *day, int *hour, int *min, double *sec, int *status, char *errText);
void   ppl_dateString         (ppl_context *ct, char *out, double UT, const char *format, int *status, char *errText);
void   ppl_timeDiffStr        (ppl_context *ct, char *out, double UT1, double UT2, const char *format, int *status, char *errText);

#endif
