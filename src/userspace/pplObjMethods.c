// pplObjMethods.c
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

#define _PPLOBJMETHODS_C 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

#include "userspace/calendars.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjCmp.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjMethods.h"
#include "userspace/pplObjPrint.h"
#include "userspace/pplObjUnits.h"
#include "userspace/unitsDisp.h"

dict **pplObjMethods;

#define COPYSTR(X,Y) \
 { \
  X = (char *)malloc(strlen(Y)+1); \
  if (X==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; } \
  strcpy(X, Y); \
 }

// Universal methods of all objects

void pplmethod_class(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  if (t==NULL) pplObjNull(&OUTPUT,0);
  else         pplObjCpy (&OUTPUT,t->objPrototype,0,1);
  OUTPUT.self_lval = NULL;
 }

void pplmethod_data(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int           count=0;
  char         *keys[4096] , *key , *tmp;
  pplObj        v;
  pplObj       *st   = in[-1].self_this;
  pplObj       *iter = in[-1].self_this;
  int           t    = iter->objType;
  pplObj       *item;
  list         *l;
  dictIterator *di;
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  l = (list *)OUTPUT.auxil;
  for ( ; (t==PPLOBJ_MOD)||(t==PPLOBJ_USER) ; iter=iter->objPrototype , t=iter->objType )
   {
    di = ppl_dictIterateInit((dict *)iter->auxil);
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
     {
      int i; for (i=0; i<count; count++) if (strcmp(key,keys[i])==0) continue;
      if (item->objType==PPLOBJ_FUNC) continue; // Non-methods only
      COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
      keys[count++]=key; if (count>4094) break;
     }
   }
  di = ppl_dictIterateInit( pplObjMethods[st->objType] );
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   {
    int i; for (i=0; i<count; count++) if (strcmp(key,keys[i])==0) continue;
    if (item->objType==PPLOBJ_FUNC) continue; // Non-methods only
    COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
    keys[count++]=key; if (count>4094) break;
   }
 }

void pplmethod_contents(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int           count=0;
  char         *keys[4096] , *key , *tmp;
  pplObj        v;
  pplObj       *st   = in[-1].self_this;
  pplObj       *iter = in[-1].self_this;
  int           t    = iter->objType;
  pplObj       *item;
  list         *l;
  dictIterator *di;
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  l = (list *)OUTPUT.auxil;
  for ( ; (t==PPLOBJ_MOD)||(t==PPLOBJ_USER) ; iter=iter->objPrototype , t=iter->objType )
   {
    di = ppl_dictIterateInit((dict *)iter->auxil);
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
     {
      int i; for (i=0; i<count; count++) if (strcmp(key,keys[i])==0) continue;
      COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
      keys[count++]=key; if (count>4094) break;
     }
   }
  di = ppl_dictIterateInit( pplObjMethods[st->objType] );
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   {
    int i; for (i=0; i<count; count++) if (strcmp(key,keys[i])==0) continue;
    COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
    keys[count++]=key; if (count>4094) break;
   }
 }

void pplmethod_methods(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int           count=0;
  char         *keys[4096] , *key , *tmp;
  pplObj        v;
  pplObj       *st   = in[-1].self_this;
  pplObj       *iter = in[-1].self_this;
  int           t    = iter->objType;
  pplObj       *item;
  list         *l;
  dictIterator *di;
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  l = (list *)OUTPUT.auxil;
  for ( ; (t==PPLOBJ_MOD)||(t==PPLOBJ_USER) ; iter=iter->objPrototype , t=iter->objType )
   {
    di = ppl_dictIterateInit((dict *)iter->auxil);
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
     {
      int i; for (i=0; i<count; count++) if (strcmp(key,keys[i])==0) continue;
      if (item->objType!=PPLOBJ_FUNC) continue; // List methods only
      COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
      keys[count++]=key; if (count>4094) break;
     }
   }
  di = ppl_dictIterateInit( pplObjMethods[st->objType] );
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   {
    int i; for (i=0; i<count; count++) if (strcmp(key,keys[i])==0) continue;
    if (item->objType!=PPLOBJ_FUNC) continue; // List methods only
    COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
    keys[count++]=key; if (count>4094) break;
   }
 }

void pplmethod_str(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t   = in[-1].self_this;
  char   *tmp = (char *)malloc(LSTR_LENGTH);
  if (tmp==NULL) { *status=1; sprintf(errText,"Out of memory."); return; }
  pplObjPrint(c,t,NULL,tmp,LSTR_LENGTH,0,0);
  pplObjStr(&OUTPUT,0,1,tmp);
 }

void pplmethod_type(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  if (t==NULL) pplObjNull(&OUTPUT,0);
  else         pplObjCpy (&OUTPUT,&pplObjPrototypes[t->objType],0,1);
  OUTPUT.self_lval = NULL;
 }

// String methods

void pplmethod_strUpper(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i=0;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  while (work[i]!='\0') { work[i] = toupper(work[i]); i++; }
  COPYSTR(tmp, work);
  pplObjStr(&OUTPUT,0,1,tmp);
 }

void pplmethod_strLower(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i=0;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  while (work[i]!='\0') { work[i] = tolower(work[i]); i++; }
  COPYSTR(tmp, work);
  pplObjStr(&OUTPUT,0,1,tmp);
 }

void pplmethod_strBeginsWith(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char *instr = (char *)t->auxil, *cmpstr;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The beginswith() method requires a single string argument."); return; }
  cmpstr = (char *)in[0].auxil;
  pplObjBool(&OUTPUT,0,strncmp(instr,cmpstr,strlen(cmpstr))==0);
 }

void pplmethod_strEndsWith(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *instr = (char *)t->auxil, *cmpstr;
  int     il, cl;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The beginswith() method requires a single string argument."); return; }
  cmpstr = (char *)in[0].auxil;
  il     = strlen(instr);
  cl     = strlen(cmpstr);
  instr += il-cl;
  if (il<cl) { pplObjBool(&OUTPUT,0,0); }
  else       { pplObjBool(&OUTPUT,0,strncmp(instr,cmpstr,strlen(cmpstr))==0); }
 }

void pplmethod_strLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char *instr = (char *)t->auxil;
  pplObjNum(&OUTPUT,0,strlen(instr),0);
 }

// Date methods

void pplmethod_dateToDayOfMonth(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  ppl_fromUnixTime(c,t->real,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,day,0);
 }

void pplmethod_dateToDayWeekName(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char *tmp;
  COPYSTR(tmp , ppl_getWeekDayName(c, floor( fmod(t->real/3600/24+3 , 7))));
  pplObjStr(&OUTPUT,0,1,tmp);
 }

void pplmethod_dateToDayWeekNum(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  pplObjNum(&OUTPUT,0,floor( fmod(t->real/3600/24+4 , 7))+1,0);
 }

void pplmethod_dateToHour(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  ppl_fromUnixTime(c,t->real,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,hour,0);
 }

void pplmethod_dateToJD(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  pplObjNum(&OUTPUT,0, t->real / 86400.0 + 2440587.5 ,0);
 }

void pplmethod_dateToMinute(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  ppl_fromUnixTime(c,t->real,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,day,0);
 }

void pplmethod_dateToMJD(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  pplObjNum(&OUTPUT,0, t->real / 86400.0 + 40587.0 ,0);
 }

void pplmethod_dateToMonthName(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int year, month, day, hour, minute; double second;
  char *tmp;
  pplObj *t = in[-1].self_this;
  ppl_fromUnixTime(c,t->real,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  COPYSTR(tmp, ppl_getMonthName(c,month));
  pplObjStr(&OUTPUT,0,1,tmp);
 }

void pplmethod_dateToMonthNum(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  ppl_fromUnixTime(c,t->real,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,month,0);
 }

void pplmethod_dateToSecond(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  ppl_fromUnixTime(c,t->real,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,second,0);
 }

void pplmethod_dateToStr(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char  *FunctionDescription = "str(<s>)";
  pplObj *t = in[-1].self_this;
  char  *format=NULL, *out=NULL;
  if ((nArgs>0)&&(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a string as its first argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  out = (char *)malloc(8192);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }

  if (nArgs>0) format = (char *)in[0].auxil; // Format specified
  else         format = NULL;                // Format not specified
  ppl_dateString(c, out, t->real, format, status, errText);
  if (*status) { *errType=ERR_NUMERIC; return; }
  pplObjStr(&OUTPUT,0,1,out);
 }

void pplmethod_dateToUnix(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  pplObjNum(&OUTPUT,0,t->real,0);
 }

void pplmethod_dateToYear(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  ppl_fromUnixTime(c,t->real,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,year,0);
 }

// Color methods

void pplmethod_colToRGB(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *a = in[-1].self_this;
  double r,g,b;
  gsl_vector *v;
  if      (round(a->exponent[0])==SW_COLSPACE_RGB ) { r=a->exponent[8]; g=a->exponent[9]; b=a->exponent[10]; }
  else if (round(a->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(a->exponent[8],a->exponent[9],a->exponent[10],a->exponent[11],&r,&g,&b);
  else                                              pplcol_HSBtoRGB(a->exponent[8],a->exponent[9],a->exponent[10],&r,&g,&b);
  pplObjVector(&OUTPUT,0,1,3);
  v = ((pplVector *)OUTPUT.auxil)->v;
  gsl_vector_set(v,0,r);
  gsl_vector_set(v,1,g);
  gsl_vector_set(v,2,b);
 }

void pplmethod_colToCMYK(ppl_context *dummy, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *a = in[-1].self_this;
  double c,m,y,k;
  gsl_vector *v;
  if      (round(a->exponent[0])==SW_COLSPACE_RGB ) pplcol_RGBtoCMYK(a->exponent[8],a->exponent[9],a->exponent[10],&c,&m,&y,&k);
  else if (round(a->exponent[0])==SW_COLSPACE_CMYK) { c=a->exponent[8]; m=a->exponent[9]; y=a->exponent[10]; k=a->exponent[11]; }
  else                                              pplcol_HSBtoCMYK(a->exponent[8],a->exponent[9],a->exponent[10],&c,&m,&y,&k);
  pplObjVector(&OUTPUT,0,1,4);
  v = ((pplVector *)OUTPUT.auxil)->v;
  gsl_vector_set(v,0,c);
  gsl_vector_set(v,1,m);
  gsl_vector_set(v,2,y);
  gsl_vector_set(v,2,k);
 }

void pplmethod_colToHSB(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *a = in[-1].self_this;
  double h,s,b;
  gsl_vector *v;
  if      (round(a->exponent[0])==SW_COLSPACE_RGB ) pplcol_RGBtoHSB (a->exponent[8],a->exponent[9],a->exponent[10],&h,&s,&b);
  else if (round(a->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoHSB(a->exponent[8],a->exponent[9],a->exponent[10],a->exponent[11],&h,&s,&b);
  else                                              { h=a->exponent[8]; s=a->exponent[9]; b=a->exponent[10]; }
  pplObjVector(&OUTPUT,0,1,3);
  v = ((pplVector *)OUTPUT.auxil)->v;
  gsl_vector_set(v,0,h);
  gsl_vector_set(v,1,s);
  gsl_vector_set(v,2,b);
 }

// Build dictionaries of the above methods

void pplObjMethodsInit(ppl_context *c)
 {
  int i;
  const int n = PPLOBJ_USER+1;
  pplObjMethods = (dict **)malloc(n * sizeof(dict *));
  if (pplObjMethods==NULL) ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Out of memory.");
  for (i=0; i<n; i++)
   {
    dict *d = pplObjMethods[i] = ppl_dictInit(HASHSIZE_LARGE,1);
    if (d==NULL) ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Out of memory.");
    ppl_addSystemFunc(d,"class"   ,0,0,1,1,1,1,(void *)&pplmethod_class   , "class()", "\\mathrm{class}@<@>", "class() returns the class prototype of an object");
    ppl_addSystemFunc(d,"contents",0,0,1,1,1,1,(void *)&pplmethod_contents, "contents()", "\\mathrm{contents}@<@>", "contents() returns a list of all the methods and internal variables of an object");
    ppl_addSystemFunc(d,"data"    ,0,0,1,1,1,1,(void *)&pplmethod_data    , "data()"    , "\\mathrm{data}@<@>", "data() returns a list of all the internal variables (not methods) of an object");
    ppl_addSystemFunc(d,"methods" ,0,0,1,1,1,1,(void *)&pplmethod_methods , "methods()", "\\mathrm{methods}@<@>", "methods() returns a list of the methods of an object");
    ppl_addSystemFunc(d,"str"     ,0,0,1,1,1,1,(void *)&pplmethod_str     , "str()", "\\mathrm{str}@<@>", "str() returns a string representation of an object");
    ppl_addSystemFunc(d,"type"    ,0,0,1,1,1,1,(void *)&pplmethod_type    , "type()", "\\mathrm{type}@<@>", "type() returns the type of an object");
   }

  // String methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"beginswith",1,1,0,0,0,0,(void *)&pplmethod_strBeginsWith, "beginswith(x)", "\\mathrm{beginswith}@<@1@>", "beginswith(x) returns a boolean indicating whether a string begins with the substring x");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"endswith"  ,1,1,0,0,0,0,(void *)&pplmethod_strEndsWith  , "endswith(x)", "\\mathrm{endswith}@<@1@>", "endswith(x) returns a boolean indicating whether a string ends with the substring x");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"len"       ,0,0,1,1,1,1,(void *)&pplmethod_strLen       , "len()", "\\mathrm{len}@<@>", "len() returns the length of a string");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"lower"     ,0,0,1,1,1,1,(void *)&pplmethod_strLower     , "lower()", "\\mathrm{lower}@<@>", "lower() converts a string to lowercase");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"upper"     ,0,0,1,1,1,1,(void *)&pplmethod_strUpper     , "upper()", "\\mathrm{upper}@<@>", "upper() converts a string to uppercase");

  // Date methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toDayOfMonth",0,0,1,1,1,1,(void *)pplmethod_dateToDayOfMonth, "toDayOfMonth()", "\\mathrm{toDayOfMonth}@<@>", "toDayOfMonth() returns the day of the month of a date object in the current calendar");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toDayWeekName",0,0,1,1,1,1,(void *)pplmethod_dateToDayWeekName, "toDayWeekName()", "\\mathrm{toDayWeekName}@<@>", "toDayWeekName() returns the name of the day of the week of a date object");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toDayWeekNum",0,0,1,1,1,1,(void *)pplmethod_dateToDayWeekNum, "toDayWeekNum()", "\\mathrm{toDayWeekNum}@<@>", "toDayWeekNum() returns the day of the week (1-7) of a date object");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toHour",0,0,1,1,1,1,(void *)pplmethod_dateToHour, "toHour()", "\\mathrm{toHour}@<@>", "toHour() returns the integer hour component (0-23) of a date object");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toJD",0,0,1,1,1,1,(void *)pplmethod_dateToJD, "toJD()", "\\mathrm{toJD}@<@>", "toJD() converts a date object to a Julian Date");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toMinute",0,0,1,1,1,1,(void *)pplmethod_dateToMinute, "toMinute()", "\\mathrm{toMinute}@<@>", "toMinute() returns the integer minute component (0-59) of a date object");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toMJD",0,0,1,1,1,1,(void *)pplmethod_dateToMJD, "toMJD()", "\\mathrm{toMJD}@<@>", "toMJD() converts a date object to a Modified Julian Date");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toMonthName",0,0,1,1,1,1,(void *)pplmethod_dateToMonthName, "toMonthName()", "\\mathrm{toMonthName}@<@>", "toMonthName() returns the name of the month in which a date object falls");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toMonthNum",0,0,1,1,1,1,(void *)pplmethod_dateToMonthNum, "toMonthNum()", "\\mathrm{toMonthNum}@<@>", "toMonthNum() returns the number (1-12) of the month in which a date object falls");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toSecond",0,0,1,1,1,1,(void *)pplmethod_dateToSecond, "toSecond()", "\\mathrm{toSecond}@<@>", "toSecond() returns the seconds component (0.0-60.0) of a date object");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"str",0,1,0,0,0,0,(void *)pplmethod_dateToStr, "str(<s>)", "\\mathrm{str}@<@0@>", "str(<s>) converts a date object to a string with an optional format string");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toUnix",0,0,1,1,1,1,(void *)pplmethod_dateToUnix, "toUnix()", "\\mathrm{toUnix}@<@>", "toUnix() converts a date object to a unix time");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DATE],"toYear",0,0,1,1,1,1,(void *)pplmethod_dateToYear, "toYear()", "\\mathrm{toYear}@<@>", "toYear() returns the year in which a date object falls in the current calendar");

  // Color methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_COL],"toCMYK",0,0,1,1,1,1,(void *)pplmethod_colToCMYK, "toCMYK()", "\\mathrm{toCMYK}@<@>", "toCMYK() returns a vector CMYK representation of a color");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_COL],"toHSB",0,0,1,1,1,1,(void *)pplmethod_colToHSB, "toHSB()", "\\mathrm{toHSB}@<@>", "toHSB() returns a vector HSB representation of a color");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_COL],"toRGB",0,0,1,1,1,1,(void *)pplmethod_colToRGB, "toRGB()", "\\mathrm{toRGB}@<@>", "toRGB() returns a vector RGB representation of a color");

  return;
 }

