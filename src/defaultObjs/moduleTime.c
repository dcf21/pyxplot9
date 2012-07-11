// moduleTime.c
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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/dict.h"

#include "settings/settings.h"

#include "stringTools/asciidouble.h"

#include "userspace/calendars.h"
#include "userspace/pplObj.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

#include "defaultObjs/moduleTime.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

void pplfunc_timefromUnix(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "fromUnix(t)";
  pplObjDate(&OUTPUT,0,in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_timefromJD  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "fromJD(t)";
  pplObjDate(&OUTPUT,0, 86400.0 * (in[0].real - 2440587.5) );
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_timefromMJD (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "fromMJD(t)";
  pplObjDate(&OUTPUT,0, 86400.0 * (in[0].real - 40587.0) );
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_timefromCalendar(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "fromCalendar(year,month,day,hour,min,sec)";
  double t;
  CHECK_NEEDSINT(in[0], "year" ,"function's first input (year) must be an integer");
  CHECK_NEEDINT (in[1], "month", "function's second input (month) must be an integer");
  CHECK_NEEDINT (in[2], "day"  , "function's third input (day) must be an integer");
  CHECK_NEEDINT (in[3], "hour" , "function's fourth input (hours) must be an integer");
  CHECK_NEEDINT (in[4], "min"  , "function's fifth input (minutes) must be an integer");
  CHECK_NEEDINT (in[5], "sec"  , "function's sixth input (seconds) must be an integer");
  t = ppl_toUnixTime(c, (int)in[0].real, (int)in[1].real, (int)in[2].real, (int)in[3].real, (int)in[4].real, (int)in[5].real, status, errText);
  pplObjDate(&OUTPUT,0,t);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_timeInterval    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "interval(t2,t1)";
  if (in[0].objType!=PPLOBJ_DATE) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a date as its first argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  if (in[1].objType!=PPLOBJ_DATE) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a date as its second argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  pplObjNum(&OUTPUT,0,in[0].real-in[1].real,0);
  CLEANUP_APPLYUNIT(UNIT_TIME);
 }

void pplfunc_timeIntervalStr (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char  *FunctionDescription = "intervalStr(t2,t1,<s>)";
  char  *format=NULL, *out=NULL;
  if (in[0].objType!=PPLOBJ_DATE) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a date as its first argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  if (in[1].objType!=PPLOBJ_DATE) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a date as its second argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[1].objType]); return; }
  if ((nArgs>2)&&(in[2].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a string as its third argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[2].objType]); return; }

  out = (char *)malloc(8192);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }

  if (nArgs>2) // Format specified
   {
    format = (char *)in[2].auxil;
    ppl_timeDiffStr(c, out, in[0].real, in[1].real, format, status, errText);
    if (*status) { *errType=ERR_NUMERICAL; return; }
   }
  else // Format not specified
   {
    int i=0,n,s;
    s = (int)fabs(in[1].real - in[0].real);
    out[0]='\0';
    n = s/3600/24;
    if      (n> 1) sprintf(out+i, "%d days", n);
    else if (n> 0) strcpy (out+i, "1 day");
    i+=strlen(out+i);
    n = (s/3600) % 24;
    if ((n>0)&&(i>0)) out[i++]=' ';
    if      (n> 1) sprintf(out+i, "%d hours", n);
    else if (n> 0) strcpy (out+i, "1 hour");
    i+=strlen(out+i);
    n = (s/60) % 60;
    if ((n>0)&&(i>0)) out[i++]=' ';
    if      (n> 1) sprintf(out+i, "%d minutes", n);
    else if (n> 0) strcpy (out+i, "1 minute");
    i+=strlen(out+i);
    n = s % 60;
    if ((n>0)&&(i>0)) out[i++]=' ';
    if      (n> 1) sprintf(out+i, "%d seconds", n);
    else if (n> 0) strcpy (out+i, "1 second");
    i+=strlen(out+i);
   }
  pplObjStr(&OUTPUT,0,1,out);
 }

void pplfunc_timenow     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "now()";
  pplObjDate(&OUTPUT,0,(double)time(NULL));
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_timestring  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char  *FunctionDescription = "string(t,<s>)";
  char  *format=NULL, *out=NULL;
  if (in[0].objType!=PPLOBJ_DATE) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a date as its first argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  if ((nArgs>1)&&(in[1].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a string as its second argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[1].objType]); return; }

  out = (char *)malloc(8192);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }

  if (nArgs>1) format = (char *)in[1].auxil; // Format specified
  else         format = NULL;                // Format not specified
  ppl_dateString(c, out, in[0].real, format, status, errText);
  if (*status) { *errType=ERR_NUMERICAL; return; }
  pplObjStr(&OUTPUT,0,1,out);
 }

void pplfunc_sleep       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int i;
  char *FunctionDescription = "sleep(t)";
  struct timespec waitperiod, waitperiod2; // A time.h timespec specifier for a 50ms nanosleep wait
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "a time", UNIT_TIME, 1);
  waitperiod.tv_sec  = floor(in[0].real);
  waitperiod.tv_nsec = fmod(in[0].real*1e9 , 1e9);
  for (i=0; i<1e6; i++)
    if (!nanosleep(&waitperiod,&waitperiod2)) break;
    else waitperiod=waitperiod2;
  pplObjNum(&OUTPUT,0,0,0);
 }

void pplfunc_sleepUntil  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int i;
  double p = in[0].real - time(NULL);
  char *FunctionDescription = "sleepUntil(d)";
  struct timespec waitperiod, waitperiod2; // A time.h timespec specifier for a 50ms nanosleep wait
  if (in[0].objType!=PPLOBJ_DATE) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a date as its argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  if (p > 0)
   {
    waitperiod.tv_sec  = floor(p);
    waitperiod.tv_nsec = fmod(p*1e9 , 1e9);
    for (i=0; i<1e6; i++)
      if (!nanosleep(&waitperiod,&waitperiod2)) break;
      else waitperiod=waitperiod2;
   }
  pplObjNum(&OUTPUT,0,0,0);
 }

