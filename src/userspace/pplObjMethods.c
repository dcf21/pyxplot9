// pplObjMethods.c
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

#define _PPLOBJMETHODS_C 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "gsl/gsl_blas.h"
#include "gsl/gsl_eigen.h"
#include "gsl/gsl_linalg.h"
#include "gsl/gsl_matrix.h"
#include "gsl/gsl_permutation.h"
#include "gsl/gsl_vector.h"

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

#include "mathsTools/dcfmath.h"

#include "userspace/calendars.h"
#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjCmp.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjMethods.h"
#include "userspace/pplObjPrint.h"
#include "userspace/pplObjUnits.h"
#include "userspace/unitsArithmetic.h"
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
  else         pplObjCpy (&OUTPUT,t->objPrototype,0,0,1);
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
  v.refCount=1;
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
  v.refCount=1;
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
  v.refCount=1;
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
  else         pplObjCpy (&OUTPUT,&pplObjPrototypes[t->objType],0,0,1);
  OUTPUT.self_lval = NULL;
 }

// String methods

void pplmethod_strUpper(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i=0;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  COPYSTR(tmp, work);
  while (tmp[i]!='\0') { tmp[i] = toupper(tmp[i]); i++; }
  pplObjStr(&OUTPUT,0,1,tmp);
 }

void pplmethod_strLower(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i=0;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  COPYSTR(tmp, work);
  while (tmp[i]!='\0') { tmp[i] = tolower(tmp[i]); i++; }
  pplObjStr(&OUTPUT,0,1,tmp);
 }

void pplmethod_strStrip(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i,j;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  COPYSTR(tmp, work);
  for (i=0; ((tmp[i]>'\0')&&(tmp[i]<=' ')); i++);
  for (j=0; tmp[i]!='\0'; i++,j++) tmp[j]=tmp[i];
  for (   ; ((i>=0)&&(tmp[i]>'\0')&&(tmp[i]<=' ')); i--) tmp[j]='\0';
  pplObjStr(&OUTPUT,0,1,tmp);
 }

void pplmethod_strisalpha(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *s = (char *)t->auxil;
  int i, l=strlen(s), out=1;
  for (i=0; i<l; i++) if (!isalpha(s[i])) { out=0; break; }
  pplObjBool(&OUTPUT,0,out);
 }

void pplmethod_strisdigit(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *s = (char *)t->auxil;
  int i, l=strlen(s), out=1;
  for (i=0; i<l; i++) if (!isdigit(s[i])) { out=0; break; }
  pplObjBool(&OUTPUT,0,out);
 }

void pplmethod_strisalnum(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *s = (char *)t->auxil;
  int i, l=strlen(s), out=1;
  for (i=0; i<l; i++) if (!isalnum(s[i])) { out=0; break; }
  pplObjBool(&OUTPUT,0,out);
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
  else       { pplObjBool(&OUTPUT,0,strncmp(instr,cmpstr,cl)==0); }
 }

void pplmethod_strFind(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *instr = (char *)t->auxil, *cmpstr;
  int     il, cl, pmax, p;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The find() method requires a single string argument."); return; }
  cmpstr = (char *)in[0].auxil;
  il     = strlen(instr);
  cl     = strlen(cmpstr);
  pmax   = il-cl;
  for (p=0; p<=pmax; p++) if (strncmp(instr+p,cmpstr,cl)==0) { pplObjNum(&OUTPUT,0,p,0); return; }
  pplObjNum(&OUTPUT,0,-1,0);
 }

void pplmethod_strFindAll(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *instr = (char *)t->auxil, *cmpstr;
  int     il, cl, pmax, p;
  pplObj  v;
  list   *l;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The findAll() method requires a single string argument."); return; }
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  v.refCount=1;
  l = (list *)OUTPUT.auxil;
  cmpstr = (char *)in[0].auxil;
  il     = strlen(instr);
  cl     = strlen(cmpstr);
  pmax   = il-cl;
  for (p=0; p<=pmax; p++)
   if (strncmp(instr+p,cmpstr,cl)==0)
    { pplObjNum(&v,0,p,0); ppl_listAppendCpy(l, &v, sizeof(v)); }
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

// Vector methods

void pplmethod_vectorAppend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int         i;
  pplObj     *st = in[-1].self_this;
  gsl_vector *v  = ((pplVector *)st->auxil)->v;
  gsl_vector *vo;
  if (!ppl_unitsDimEqual(st, &in[0]))
   {
    if (st->dimensionless)
     { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector is dimensionless, but number has units of <%s>.", ppl_printUnit(c, &in[0], NULL, NULL, 1, 1, 0) ); }
    else if (in[0].dimensionless)
     { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector has units of <%s>, while number is dimensionless.", ppl_printUnit(c, st, NULL, NULL, 0, 1, 0) ); }
    else
     { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector has units of <%s>, while number has units of <%s>.", ppl_printUnit(c, st, NULL, NULL, 0, 1, 0), ppl_printUnit(c, &in[0], NULL, NULL, 1, 1, 0) ); }
    *errType=ERR_UNIT; *status = 1; return;
   }
  if (pplObjVector(&OUTPUT,0,1,v->size+1)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplVector *)(OUTPUT.auxil))->v;
  for (i=0; i<v->size; i++) gsl_vector_set(vo , i , gsl_vector_get(v,i));
  gsl_vector_set(vo, i, in[0].real);
  ppl_unitsDimCpy(&OUTPUT,st);
  if (st->self_lval!=NULL)
   {
    pplObj *o = st->self_lval;
    int     om=o->amMalloced;
    int     rc=o->refCount;
    o->amMalloced=0;
    o->refCount=1;
    ppl_garbageObject(o);
    o->refCount=rc;
    pplObjCpy(o,&OUTPUT,0,om,1);
   }
 }

void pplmethod_vectorExtend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int         i,l=0;
  pplObj     *st = in[-1].self_this;
  gsl_vector *va = ((pplVector *)st->auxil)->v;
  gsl_vector *vo;
  int         t  = in[0].objType;

  if      (t==PPLOBJ_VEC)
   {
    if (!ppl_unitsDimEqual(st, &in[0]))
     {
      if (st->dimensionless)
       { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector is dimensionless, but number has units of <%s>.", ppl_printUnit(c, &in[0], NULL, NULL, 1, 1, 0) ); }
      else if (in[0].dimensionless)
       { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector has units of <%s>, while number is dimensionless.", ppl_printUnit(c, st, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector has units of <%s>, while number has units of <%s>.", ppl_printUnit(c, st, NULL, NULL, 0, 1, 0), ppl_printUnit(c, &in[0], NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
    l = ((pplVector *)in[0].auxil)->v->size;
   }
  else if (t==PPLOBJ_LIST)
   {
    l = ((list *)in[0].auxil)->length;
   }
  else
   {
    *status=1; *errType=ERR_TYPE; sprintf(errText, "Argument to the extend(x) method must be either a list or a vector. Supplied argument had type <%s>.", pplObjTypeNames[t]); return;
   }

  if (pplObjVector(&OUTPUT,0,1,va->size+l)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplVector *)(OUTPUT.auxil))->v;
  for (i=0; i<va->size; i++) gsl_vector_set(vo , i , gsl_vector_get(va,i));
  ppl_unitsDimCpy(&OUTPUT,st);

  if (t==PPLOBJ_VEC)
   {
    int j;
    gsl_vector *vi = ((pplVector *)in[0].auxil)->v;
    for (j=0; j<vi->size; j++,i++) gsl_vector_set(vo, i, gsl_vector_get(vi,j));
   }
  else if (t==PPLOBJ_LIST)
   {
    list *listin = (list *)in[0].auxil;
    listIterator *li = ppl_listIterateInit(listin);
    pplObj *item;
    while ((item = (pplObj*)ppl_listIterate(&li))!=NULL)
     {
      if (item->objType!=PPLOBJ_NUM)
       { *status=1; *errType=ERR_TYPE; sprintf(errText, "Can only append numbers to vectors; supplied object has type <%s>.",pplObjTypeNames[item->objType]); return; } 
      if (!ppl_unitsDimEqual(st, item))
       {
        if (st->dimensionless)
         { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector is dimensionless, but number has units of <%s>.", ppl_printUnit(c, item, NULL, NULL, 1, 1, 0) ); }
        else if (item->dimensionless)
         { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector has units of <%s>, while number is dimensionless.", ppl_printUnit(c, st, NULL, NULL, 0, 1, 0) ); }
        else
         { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector has units of <%s>, while number has units of <%s>.", ppl_printUnit(c, st, NULL, NULL, 0, 1, 0), ppl_printUnit(c, item, NULL, NULL, 1, 1, 0) ); }
        *errType=ERR_UNIT; *status = 1; return;
       }
      if (item->flagComplex) { *status=1; *errType=ERR_TYPE; sprintf(errText, "Can only append real numbers to vectors; supplied value is complex."); return; }
      gsl_vector_set(vo, i++, item->real);
     }
   }
  if (st->self_lval!=NULL)
   {
    pplObj *o = st->self_lval;
    int     om=o->amMalloced;
    int     rc=o->refCount;
    o->amMalloced=0;
    o->refCount=1;
    ppl_garbageObject(o);
    o->refCount=rc;
    pplObjCpy(o,&OUTPUT,0,om,1);
   }
 }

void pplmethod_vectorInsert(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long    p,i;
  pplObj *st = in[-1].self_this;
  gsl_vector *v  = ((pplVector *)st->auxil)->v;
  gsl_vector *vo;
  if (in[0].objType!=PPLOBJ_NUM) { *status=1; *errType=ERR_TYPE; sprintf(errText, "First argument to the insert(n,x) method must be a number. Supplied argument had type <%s>.", pplObjTypeNames[in[0].objType]); return; }
  if (in[0].flagComplex) { *status=1; *errType=ERR_TYPE; sprintf(errText, "First argument to the insert(n,x) method must be a real number. Supplied argument is complex."); return; }
  if (in[0].real < 0) in[0].real += v->size+1;
  p = (long)round(in[0].real);
  if (p<0) { *status=1; *errType=ERR_RANGE; sprintf(errText, "Attempt to insert a vector item before the beginning of a vector."); return; }
  if (p>v->size) { *status=1; *errType=ERR_RANGE; sprintf(errText, "Vector index out of range."); return; }
  if (!ppl_unitsDimEqual(st, &in[1]))
   {
    if (st->dimensionless)
     { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector is dimensionless, but number has units of <%s>.", ppl_printUnit(c, &in[1], NULL, NULL, 1, 1, 0) ); }
    else if (in[1].dimensionless)
     { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector has units of <%s>, while number is dimensionless.", ppl_printUnit(c, st, NULL, NULL, 0, 1, 0) ); }
    else
     { sprintf(errText, "Attempt to append a number to a vector with conflicting dimensions: vector has units of <%s>, while number has units of <%s>.", ppl_printUnit(c, st, NULL, NULL, 0, 1, 0), ppl_printUnit(c, &in[1], NULL, NULL, 1, 1, 0) ); }
    *errType=ERR_UNIT; *status = 1; return;
   }
  if (pplObjVector(&OUTPUT,0,1,v->size+1)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplVector *)(OUTPUT.auxil))->v;
  gsl_vector_set(vo, p, in[1].real);
  for (i=0; i<v->size; i++) gsl_vector_set(vo , i+(i>=p) , gsl_vector_get(v,i));
  ppl_unitsDimCpy(&OUTPUT,st);
  if (st->self_lval!=NULL)
   {
    pplObj *o = st->self_lval;
    int     om=o->amMalloced;
    int     rc=o->refCount;
    o->amMalloced=0;
    o->refCount=1;
    ppl_garbageObject(o);
    o->refCount=rc;
    pplObjCpy(o,&OUTPUT,0,om,1);
   }
 }

void pplmethod_vectorLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  gsl_vector *v = ((pplVector *)in[-1].self_this->auxil)->v;
  pplObjNum(&OUTPUT,0,v->size,0);
 }

void pplmethod_vectorNorm(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st = in[-1].self_this;
  gsl_vector *v = ((pplVector *)st->auxil)->v;
  pplObjNum(&OUTPUT,0,gsl_blas_dnrm2(v),0);
  ppl_unitsDimCpy(&OUTPUT, st);
 }

void pplmethod_vectorReverse(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  gsl_vector *v = ((pplVector *)in[-1].self_this->auxil)->v;
  long i,j;
  long n = v->size;
  if (v->size<2) return;
  for ( i=0 , j=n-1 ; i<j ; i++ , j-- ) { double tmp = gsl_vector_get(v,i); gsl_vector_set(v,i,gsl_vector_get(v,j)); gsl_vector_set(v,j,tmp); }
 }

void pplmethod_vectorSort(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  gsl_vector *v = ((pplVector *)in[-1].self_this->auxil)->v;
  if (v->size<2) return;
  qsort((void *)v->data , v->size , sizeof(double)*v->stride , ppl_dblSort);
 }

// List methods

void pplmethod_listAppend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj  v;
  pplObj *st = in[-1].self_this;
  list   *l  = (list *)st->auxil;
  v.refCount=1;
  pplObjCpy(&v,&in[0],0,1,1);
  ppl_listAppendCpy(l, &v, sizeof(pplObj));
  pplObjCpy(&OUTPUT,st,0,0,1);
 }

void pplmethod_listExtend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj  v;
  pplObj *st = in[-1].self_this;
  list   *l  = (list *)st->auxil;
  int     t  = in[0].objType;

  if (t==PPLOBJ_VEC)
   {
    int i;
    gsl_vector *vi = ((pplVector *)in[0].auxil)->v;
    pplObjNum(&v,1,0,0);
    ppl_unitsDimCpy(&v, &in[0]);
    for (i=0; i<vi->size; i++)
     {
      v.real = gsl_vector_get(vi,i);
      ppl_listAppendCpy(l, &v, sizeof(pplObj));
     }
   }
  else if (t==PPLOBJ_LIST)
   {
    list *listin = (list *)in[0].auxil;
    listIterator *li = ppl_listIterateInit(listin);
    pplObj *item;
    while ((item = (pplObj*)ppl_listIterate(&li))!=NULL)
     {
      v.refCount=1;
      pplObjCpy(&v,item,0,1,1);
      ppl_listAppendCpy(l, &v, sizeof(pplObj));
     }
   }
  else
   {
    *status=1; *errType=ERR_TYPE; sprintf(errText, "Argument to the extend(x) method must be either a list or a vector. Supplied argument had type <%s>.", pplObjTypeNames[t]); return;
   }
  pplObjCpy(&OUTPUT,st,0,0,1);
 }

void pplmethod_listInsert(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long    p;
  pplObj  v;
  pplObj *st = in[-1].self_this;
  list   *l  = (list *)st->auxil;
  if (in[0].objType!=PPLOBJ_NUM) { *status=1; *errType=ERR_TYPE; sprintf(errText, "First argument to the insert(n,x) method must be a number. Supplied argument had type <%s>.", pplObjTypeNames[in[0].objType]); return; }
  if (in[0].flagComplex) { *status=1; *errType=ERR_TYPE; sprintf(errText, "First argument to the insert(n,x) method must be a real number. Supplied argument is complex."); return; }
  if (in[0].real < 0) in[0].real += l->length+1;
  p = (long)round(in[0].real);
  if (p<0) { *status=1; *errType=ERR_RANGE; sprintf(errText, "Attempt to insert a list item before the beginning of a list."); return; }
  if (p>l->length) { *status=1; *errType=ERR_RANGE; sprintf(errText, "List index out of range."); return; }
  v.refCount=1;
  pplObjCpy(&v,&in[1],0,1,1);
  ppl_listInsertCpy(l, p, &v, sizeof(pplObj));
  pplObjCpy(&OUTPUT,st,0,0,1);
 }

void pplmethod_listLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  list *l = (list *)in[-1].self_this->auxil;
  pplObjNum(&OUTPUT,0,l->length,0);
 }

void pplmethod_listPop(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *obj;
  pplObj *st = in[-1].self_this;
  list   *l  = (list *)st->auxil;
  if (nArgs==0)
   {
    obj = ppl_listPop(l);
   }
  else
   {
    long p;
    if (in[0].objType!=PPLOBJ_NUM) { *status=1; *errType=ERR_TYPE; sprintf(errText, "Optional argument to the pop(n) method must be a number. Supplied argument had type <%s>.", pplObjTypeNames[in[0].objType]); return; }
    if (in[0].flagComplex) { *status=1; *errType=ERR_TYPE; sprintf(errText, "Optional argument to the pop(n) method must be a real number. Supplied argument is complex."); return; }
    if (in[0].real < 0) in[0].real += l->length;
    p = (long)round(in[0].real);
    if (p<0) { *status=1; *errType=ERR_RANGE; sprintf(errText, "Attempt to pop a list item before the beginning of a list."); return; }
    if (p>=l->length) { *status=1; *errType=ERR_RANGE; sprintf(errText, "List index out of range."); return; }
    obj = ppl_listPopItem(l,p);
   }
  pplObjCpy(&OUTPUT,obj,0,0,1);
  ppl_garbageObject(obj);
 }

void pplmethod_listReverse(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long      i;
  list     *l     = (list *)in[-1].self_this->auxil;
  listItem *l1    = l->first;
  listItem *l2    = l->last;
  long      n     = l->length;
  if (n<2) return;

  for (i=0; i<n/2; i++)
   {
    void *tmp = l1->data; l1->data=l2->data; l2->data=tmp;
    l1=l1->next; l2=l2->prev;
   }
 }

void pplmethod_listSort(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long     i;
  list    *l     = (list *)in[-1].self_this->auxil;
  long     n     = l->length;
  pplObj **items;
  listIterator *li = ppl_listIterateInit(l);
  if (n<2) return;
  items = (pplObj **)malloc(n * sizeof(pplObj *));
  if (items==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }
  i=0; while (li!=NULL) { items[i++]=(pplObj*)li->data; ppl_listIterate(&li); }
  qsort(items, n, sizeof(pplObj*), pplObjCmpQuiet);
  li = ppl_listIterateInit(l);
  i=0; while (li!=NULL) { li->data=(void*)items[i++]; ppl_listIterate(&li); }
  free(items);
 }

// Dictionary methods

void pplmethod_dictHasKey(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char         *key;
  pplObj       *item;
  dictIterator *di = ppl_dictIterateInit((dict *)in[-1].self_this->auxil);
  char         *instr;
  if ((nArgs!=1)&&(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function hasKey() requires a string as its argument; supplied argument had type <%s>.", pplObjTypeNames[in[0].objType]); return; }
  instr = (char *)in[0].auxil;
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   { if (strcmp(instr,key)==0) { pplObjBool(&OUTPUT,0,1); return; } }
  pplObjBool(&OUTPUT,0,0);
  return;
 }

void pplmethod_dictItems(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char         *key, *tmp;
  pplObj        v, va, vb, *item;
  list         *l, *l2;
  dictIterator *di = ppl_dictIterateInit((dict *)in[-1].self_this->auxil);
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  l  = (list *)OUTPUT.auxil;
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   {
    v .refCount=1;
    va.refCount=1;
    vb.refCount=1;
    COPYSTR(tmp,key);
    if (pplObjStr(&va,1,1,tmp )==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    pplObjCpy(&vb,item,0,1,1);
    if (pplObjList(&v,1,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    l2 = (list *)v.auxil;
    ppl_listAppendCpy(l2, (void *)&va, sizeof(pplObj));
    ppl_listAppendCpy(l2, (void *)&vb, sizeof(pplObj));
    ppl_listAppendCpy(l , (void *)&v , sizeof(pplObj));
   }
 }

void pplmethod_dictKeys(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char         *key, *tmp;
  pplObj        v, *item;
  list         *l;
  dictIterator *di = ppl_dictIterateInit((dict *)in[-1].self_this->auxil);
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  v.refCount=1;
  l  = (list *)OUTPUT.auxil;
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   { COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj)); }
 }

void pplmethod_dictLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  dict *d = (dict *)in[-1].self_this->auxil;
  pplObjNum(&OUTPUT,0,d->length,0);
 }

void pplmethod_dictValues(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char         *key, *tmp;
  pplObj        v, *item;
  list         *l;
  dictIterator *di = ppl_dictIterateInit((dict *)in[-1].self_this->auxil);
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  v.refCount=1;
  l  = (list *)OUTPUT.auxil;
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   { COPYSTR(tmp,key); pplObjCpy(&v,item,0,1,1); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj)); }
 }

// Matrix methods

void pplmethod_matrixDet(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj          *st  = in[-1].self_this;
  gsl_matrix      *m   = ((pplMatrix *)in[-1].self_this->auxil)->m;
  double           d   = 0;
  int              n   = m->size1, i;
  int              s;
  gsl_matrix      *tmp = NULL;
  gsl_permutation *p   = NULL;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "The determinant is only defined for square matrices."); return; }
  if ((tmp=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp,m);
  if ((p = gsl_permutation_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if (gsl_linalg_LU_decomp(tmp,p,&s)!=0) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "LU decomposition failed whilst computing matrix determinant."); return; }
  d = gsl_linalg_LU_det(tmp,s);
  gsl_permutation_free(p);
  gsl_matrix_free(tmp);
  pplObjNum(&OUTPUT,0,d,0);
  ppl_unitsDimCpy(&OUTPUT, st);
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) OUTPUT.exponent[i] *= n;
 }

void pplmethod_matrixDiagonal(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int i,j;
  gsl_matrix *m = ((pplMatrix *)in[-1].self_this->auxil)->m;
  if (m->size1 != m->size2) { pplObjBool(&OUTPUT,0,0); return; }
  for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) if ((i!=j) && (gsl_matrix_get(m,i,j)!=0)) { pplObjBool(&OUTPUT,0,0); return; }
  pplObjBool(&OUTPUT,0,1);
 }

void pplmethod_matrixEigenvalues(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int         i,j;
  pplObj     *st  = in[-1].self_this;
  gsl_matrix *m   = ((pplMatrix *)st->auxil)->m;
  gsl_matrix *tmp = NULL;
  gsl_vector *vo;
  int         n   = m->size1;
  gsl_eigen_symm_workspace *w;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "Eigenvalues are only defined for square matrices."); return; }
  for (i=0; i<m->size1; i++) for (j=0; j<i; j++) if (gsl_matrix_get(m,i,j) != gsl_matrix_get(m,j,i)) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "Eigenvalues can only be computed for symmetric matrices; supplied matrix is not symmetric."); return; }
  if ((w=gsl_eigen_symm_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if ((tmp=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp,m);
  if (pplObjVector(&OUTPUT,0,1,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplVector *)OUTPUT.auxil)->v;
  if (gsl_eigen_symm(tmp, vo, w)!=0) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "Numerical failure whilst trying to compute eigenvalues."); return; }
  gsl_matrix_free(tmp);
  gsl_eigen_symm_free(w);
  ppl_unitsDimCpy(&OUTPUT, st);
 }

void pplmethod_matrixEigenvectors(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int         i,j;
  gsl_matrix *m    = ((pplMatrix *)in[-1].self_this->auxil)->m;
  gsl_matrix *tmp1 = NULL;
  gsl_matrix *tmp2 = NULL;
  gsl_vector *vtmp = NULL;
  list       *lo;
  pplObj      v;
  gsl_vector *vo;
  int         n    = m->size1;
  gsl_eigen_symmv_workspace *w;
  v.refCount=1;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "Eigenvectors are only defined for square matrices."); return; }
  for (i=0; i<m->size1; i++) for (j=0; j<i; j++) if (gsl_matrix_get(m,i,j) != gsl_matrix_get(m,j,i)) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "Eigenvectors can only be computed for symmetric matrices; supplied matrix is not symmetric."); return; }
  if ((w=gsl_eigen_symmv_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if ((tmp1=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if ((tmp2=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if ((vtmp=gsl_vector_alloc(n)  )==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp1,m);
  if (gsl_eigen_symmv(tmp1, vtmp, tmp2, w)!=0) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "Numerical failure whilst trying to compute eigenvectors."); return; }
  gsl_matrix_free(tmp1);
  gsl_eigen_symmv_free(w);
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  lo = (list*)OUTPUT.auxil;
  for (i=0; i<n; i++)
   {
    if (pplObjVector(&v,1,1,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    vo = ((pplVector *)v.auxil)->v;
    for (j=0; j<n; j++) gsl_vector_set(vo,j,gsl_matrix_get(tmp2,i,j));
    ppl_listAppendCpy(lo, (void*)&v, sizeof(pplObj));
   }
  gsl_matrix_free(tmp2);
 }

void pplmethod_matrixInv(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj          *st  = in[-1].self_this;
  gsl_matrix      *m   = ((pplMatrix *)st->auxil)->m;
  int              n   = m->size1;
  int              s;
  gsl_matrix      *tmp = NULL;
  gsl_matrix      *mo  = NULL;
  gsl_permutation *p   = NULL;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "The inverse is only defined for square matrices."); return; }
  if ((tmp=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp,m);
  if ((p = gsl_permutation_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if (gsl_linalg_LU_decomp(tmp,p,&s)!=0) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "LU decomposition failed whilst computing matrix determinant."); return; }
  if (pplObjMatrix(&OUTPUT,0,1,n,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  mo = ((pplMatrix *)OUTPUT.auxil)->m;
  if (gsl_linalg_LU_invert(tmp,p,mo)) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "Numerical failure while computing matrix inverse."); return; }
  gsl_permutation_free(p);
  gsl_matrix_free(tmp);
  ppl_unitsDimInverse(&OUTPUT,st);
 }

void pplmethod_matrixSize(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  gsl_matrix *m = ((pplMatrix *)in[-1].self_this->auxil)->m;
  gsl_vector *vo;
  if (pplObjVector(&OUTPUT,0,1,2)==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }
  vo = ((pplVector *)OUTPUT.auxil)->v;
  gsl_vector_set(vo,0,m->size1);
  gsl_vector_set(vo,1,m->size2);
 }

void pplmethod_matrixSymmetric(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int i,j;
  gsl_matrix *m = ((pplMatrix *)in[-1].self_this->auxil)->m;
  if (m->size1 != m->size2) { pplObjBool(&OUTPUT,0,0); return; }
  for (i=0; i<m->size1; i++) for (j=0; j<i; j++) if (gsl_matrix_get(m,i,j) != gsl_matrix_get(m,j,i)) { pplObjBool(&OUTPUT,0,0); return; }
  pplObjBool(&OUTPUT,0,1);
 }

void pplmethod_matrixTrans(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj          *st  = in[-1].self_this;
  gsl_matrix      *m = ((pplMatrix *)st->auxil)->m;
  int              n = m->size1, i, j;
  gsl_matrix      *vo = NULL;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "The transpose is only defined for square matrices."); return; }
  if (pplObjMatrix(&OUTPUT,0,1,n,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplMatrix *)OUTPUT.auxil)->m;
  for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) gsl_matrix_set(vo,i,j,gsl_matrix_get(m,j,i));
  ppl_unitsDimCpy(&OUTPUT,st);
 }

// File methods

void pplmethod_fileClose(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (f->open)
   {
    f->open=0;
    if (!f->pipe) fclose(f->file);
    else          pplObjNum(&OUTPUT,0,pclose(f->file),0);
    f->file=NULL;
   }
 }

void pplmethod_fileEof(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  pplObjBool(&OUTPUT,0,feof(f->file)!=0);
 }

void pplmethod_fileFlush(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  if (fflush(f->file)<0) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }
 }

void pplmethod_fileGetpos(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long int fp;
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  if ((fp = ftell(f->file))<0) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }
  pplObjNum(&OUTPUT,0,fp*8,0);
  CLEANUP_APPLYUNIT(UNIT_BIT);
 }

void pplmethod_fileIsOpen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  pplObjBool(&OUTPUT,0,f->open!=0);
 }

void pplmethod_fileRead(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *out=NULL, *new;
  int  chunk = 8192;
  long pos   = 0;
  long size  = chunk;
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  new = (char *)malloc(chunk+4);
  while (!feof(f->file))
   {
    int n;
    if (new==NULL) { if (out!=NULL) free(out); *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    out = new;
    n = fread(out+pos,1,chunk,f->file);
    pos += n;
    if (n<chunk) break;
    size += chunk;
    new = (char *)realloc(out,size+4);
   }
  out[pos]='\0';
  pplObjStr(&OUTPUT,0,1,out);
 }

void pplmethod_fileReadline(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *out=NULL;
  int  chunk = 8192;
  long pos   = 0;
  long size  = chunk;
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  out = (char *)malloc(chunk);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  while (!feof(f->file))
   {
    int n;
    n = fread(out+pos,1,1,f->file);
    if ((n==0)||(out[pos]=='\n')) break;
    pos++;
    if (pos>size-4)
     {
      char *new;
      size += chunk; new = (char *)realloc(out,size);
      if (new==NULL) { if (out!=NULL) free(out); *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
      out = new;
     }
   }
  out[pos]='\0';
  pplObjStr(&OUTPUT,0,1,out);
 }

void pplmethod_fileReadlines(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st = in[-1].self_this;
  pplObj  v;
  pplFile *f = (pplFile *)st->auxil;
  list    *l;
  v.refCount=1;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  if (pplObjList(&v,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  l = (list *)v.auxil;
  while (!feof(f->file))
   {
    in[-1].self_this = st;
    pplmethod_fileReadline(c,in,0,status,errType,errText);
    if (*status) { ppl_garbageObject(&v); return; }
    OUTPUT.amMalloced=1;
    ppl_listAppendCpy(l,&OUTPUT,sizeof(pplObj));
    pplObjZom(&OUTPUT,0);
   }
  memcpy(&OUTPUT,&v,sizeof(pplObj));
 }

void pplmethod_fileSetpos(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char FunctionDescription[] = "setpos(x)";
  int i;
  long int fp = (long int)in[0].real;
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "a number of bytes", UNIT_BIT, 1);
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  if (!in[0].dimensionless) fp/=8;
  if (fseek(f->file,fp,SEEK_SET)!=0) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }
 }

void pplmethod_fileWrite(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  char    *s = (char *)in[0].auxil;
  long int l , o;
  if ((nArgs!=1)&&(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function write() requires a string as its argument; supplied argument had type <%s>.", pplObjTypeNames[in[0].objType]); return; }
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  l = strlen(s);
  o = fwrite(s,1,l,f->file);
  if (o!=l) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }
 }

// Exception methods

void pplmethod_excRaise(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if ((nArgs!=1)&&(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function raise() requires a string as its argument; supplied argument had type <%s>.", pplObjTypeNames[in[0].objType]); return; }
  *status  = 1;
  *errType = (int)round(in[-1].self_this->real);
  strncpy(errText, in[0].auxil, FNAME_LENGTH);
  errText[FNAME_LENGTH-1]='\0';
  return;
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
    ppl_addSystemFunc(d,"data"    ,0,0,1,1,1,1,(void *)&pplmethod_data    , "data()", "\\mathrm{data}@<@>", "data() returns a list of all the internal variables (not methods) of an object");
    ppl_addSystemFunc(d,"methods" ,0,0,1,1,1,1,(void *)&pplmethod_methods , "methods()", "\\mathrm{methods}@<@>", "methods() returns a list of the methods of an object");
    ppl_addSystemFunc(d,"str"     ,0,0,1,1,1,1,(void *)&pplmethod_str     , "str()", "\\mathrm{str}@<@>", "str() returns a string representation of an object");
    ppl_addSystemFunc(d,"type"    ,0,0,1,1,1,1,(void *)&pplmethod_type    , "type()", "\\mathrm{type}@<@>", "type() returns the type of an object");
   }

  // String methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"beginswith",1,1,0,0,0,0,(void *)&pplmethod_strBeginsWith, "beginswith(x)", "\\mathrm{beginswith}@<@1@>", "beginswith(x) returns a boolean indicating whether a string begins with the substring x");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"endswith"  ,1,1,0,0,0,0,(void *)&pplmethod_strEndsWith  , "endswith(x)", "\\mathrm{endswith}@<@1@>", "endswith(x) returns a boolean indicating whether a string ends with the substring x");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"find"      ,1,1,0,0,0,0,(void *)&pplmethod_strFind      , "find(x)", "\\mathrm{find}@<@1@>", "find(x) returns the position of the first occurance of x in a string");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"findAll"   ,1,1,0,0,0,0,(void *)&pplmethod_strFindAll   , "findAll(x)", "\\mathrm{findAll}@<@1@>", "findAll(x) returns a list of the positions where the substring x occurs in a string");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"isalnum"   ,0,0,1,1,1,1,(void *)&pplmethod_strisalnum   , "isalnum()", "\\mathrm{isalnum}@<@>", "isalnum() returns a boolean indicating whether all of the characters of a string are alphanumeric");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"isalpha"   ,0,0,1,1,1,1,(void *)&pplmethod_strisalpha   , "isalpha()", "\\mathrm{isalpha}@<@>", "isalpha() returns a boolean indicating whether all of the characters of a string are alphabetic");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"isdigit"   ,0,0,1,1,1,1,(void *)&pplmethod_strisdigit   , "isdigit()", "\\mathrm{isdigit}@<@>", "isdigit() returns a boolean indicating whether all of the characters of a string are numeric");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"len"       ,0,0,1,1,1,1,(void *)&pplmethod_strLen       , "len()", "\\mathrm{len}@<@>", "len() returns the length of a string");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"lower"     ,0,0,1,1,1,1,(void *)&pplmethod_strLower     , "lower()", "\\mathrm{lower}@<@>", "lower() converts a string to lowercase");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"strip"     ,0,0,1,1,1,1,(void *)&pplmethod_strStrip     , "strip()", "\\mathrm{strip}@<@>", "strip() strips whitespace off the beginning and end of a string");
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

  // Vector methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"append",1,1,1,1,1,0,(void *)pplmethod_vectorAppend, "append(x)", "\\mathrm{append}@<@0@>", "append(x) appends the object x to a vector");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"extend",1,1,0,0,0,0,(void *)pplmethod_vectorExtend, "extend(x)", "\\mathrm{extend}@<@0@>", "extend(x) appends the members of the list x to the end of a vector");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"insert",2,2,0,0,0,0,(void *)pplmethod_vectorInsert, "insert(n,x)", "\\mathrm{insert}@<@0@>", "insert(n,x) inserts the object x into a vector at position n");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"len",0,0,1,1,1,1,(void *)pplmethod_vectorLen, "len()", "\\mathrm{len}@<@>", "len() returns the number of dimensions of a vector");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"norm",0,0,1,1,1,1,(void *)pplmethod_vectorNorm, "norm()", "\\mathrm{norm}@<@>", "norm() returns the norm (quadrature sum) of a vector's elements");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"reverse",0,0,1,1,1,1,(void *)pplmethod_vectorReverse, "reverse()", "\\mathrm{reverse}@<@>", "reverse() reverses the order of the elements of a vector");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"sort",0,0,1,1,1,1,(void *)pplmethod_vectorSort, "sort()", "\\mathrm{sort}@<@>", "sort() sorts the members of a vector");

  // List methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"append",1,1,0,0,0,0,(void *)pplmethod_listAppend, "append(x)", "\\mathrm{append}@<@0}@>", "append(x) appends the object x to a list");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"extend",1,1,0,0,0,0,(void *)pplmethod_listExtend, "extend(x)", "\\mathrm{extend}@<@0}@>", "extend(x) appends the members of the list x to the end of a list");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"insert",2,2,0,0,0,0,(void *)pplmethod_listInsert, "insert(n,x)", "\\mathrm{insert}@<@0@>", "insert(n,x) inserts the object x into a list at position n");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"len",0,0,1,1,1,1,(void *)pplmethod_listLen, "len()", "\\mathrm{len}@<@>", "len() returns the length of a list");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"pop",0,1,1,1,1,1,(void *)pplmethod_listPop, "pop()", "\\mathrm{pop}@<@0@>", "pop(n) removes the nth item from a list and returns it. If n is not specified, the last list item is popped.");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"reverse",0,0,1,1,1,1,(void *)pplmethod_listReverse, "reverse()", "\\mathrm{reverse}@<@>", "reverse() reverses the order of the members of a list");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"sort",0,0,1,1,1,1,(void *)pplmethod_listSort, "sort()", "\\mathrm{sort}@<@>", "sort() sorts the members of a list");

  // Dictionary methods
  for (i=0 ;i<3; i++)
   {
    int pplobj;
    if      (i==0) pplobj = PPLOBJ_DICT;
    else if (i==1) pplobj = PPLOBJ_MOD;
    else           pplobj = PPLOBJ_USER;

    ppl_addSystemFunc(pplObjMethods[pplobj],"hasKey",1,1,0,0,0,0,(void *)pplmethod_dictHasKey, "hasKey()", "\\mathrm{hasKey}@<@1@>", "hasKey(x) returns a boolean indicating whether the key x exists in the dictionary");
    ppl_addSystemFunc(pplObjMethods[pplobj],"items" ,0,0,1,1,1,1,(void *)pplmethod_dictItems , "items()", "\\mathrm{items}@<@>", "items() returns a list of the [key,value] pairs in a dictionary");
    ppl_addSystemFunc(pplObjMethods[pplobj],"keys"  ,0,0,1,1,1,1,(void *)pplmethod_dictKeys  , "keys()", "\\mathrm{keys}@<@>", "keys() returns a list of the keys defined in a dictionary");
    ppl_addSystemFunc(pplObjMethods[pplobj],"len"   ,0,0,1,1,1,1,(void *)pplmethod_dictLen   , "len()", "\\mathrm{len}@<@>", "len() returns the number of entries in a dictionary");
    ppl_addSystemFunc(pplObjMethods[pplobj],"values",0,0,1,1,1,1,(void *)pplmethod_dictValues, "values()", "\\mathrm{values}@<@>", "values() returns a list of the values in a dictionary");
   }

  // Matrix methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"det" ,0,0,1,1,1,1,(void *)pplmethod_matrixDet,"det()", "\\mathrm{det}@<@>", "det() returns the determinant of a square matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"diagonal",0,0,1,1,1,1,(void *)pplmethod_matrixDiagonal,"diagonal()", "\\mathrm{diagonal}@<@>", "diagonal() returns a boolean indicating whether a matrix is diagonal");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"eigenvalues",0,0,1,1,1,1,(void *)pplmethod_matrixEigenvalues,"eigenvalues()", "\\mathrm{eigenvalues}@<@>", "eigenvalues() returns a vector containing the eigenvalues of a square symmetric matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"eigenvectors",0,0,1,1,1,1,(void *)pplmethod_matrixEigenvectors,"eigenvectors()", "\\mathrm{eigenvectors}@<@>", "eigenvectors() returns a list of the eigenvectors of a square symmetric matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"inv" ,0,0,1,1,1,1,(void *)pplmethod_matrixInv,"inv()", "\\mathrm{inv}@<@>", "inv() returns the inverse of a square matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"size",0,0,1,1,1,1,(void *)pplmethod_matrixSize, "size()", "\\mathrm{size}@<@>", "size() returns the dimensions of a matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"symmetric",0,0,1,1,1,1,(void *)pplmethod_matrixSymmetric, "symmetric()", "\\mathrm{symmetric}@<@>", "symmetric() returns a boolean indicating whether a matrix is symmetric");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"transpose",0,0,1,1,1,1,(void *)pplmethod_matrixTrans, "transpose()", "\\mathrm{transpose}@<@>", "transpose() returns the transpose of a matrix");

  // File methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"close",0,0,1,1,1,1,(void *)pplmethod_fileClose,"close()", "\\mathrm{close}@<@>", "close() closes a file handle");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"eof",0,0,1,1,1,1,(void *)pplmethod_fileEof,"eof()", "\\mathrm{eof}@<@>", "eof() returns a boolean flag to indicate whether the end of a file has been reached");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"flush",0,0,1,1,1,1,(void *)pplmethod_fileFlush,"flush()", "\\mathrm{flush}@<@>", "flush() flushes any buffered data which has not yet physically been written to a file");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"getpos",0,0,1,1,1,1,(void *)pplmethod_fileGetpos,"getpos()", "\\mathrm{getpos}@<@>", "getpos() returns a file handle's current position in a file");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"isOpen",0,0,1,1,1,1,(void *)pplmethod_fileIsOpen,"isOpen()", "\\mathrm{isOpen}@<@>", "isOpen() returns a boolean flag indicating whether a file is open");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"read",0,0,1,1,1,1,(void *)pplmethod_fileRead,"read()", "\\mathrm{read}@<@>", "read() returns the contents of a file as a string");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"readline",0,0,1,1,1,1,(void *)pplmethod_fileReadline,"readline()", "\\mathrm{readline}@<@>", "readline() returns a single line of a file as a string");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"readlines",0,0,1,1,1,1,(void *)pplmethod_fileReadlines,"readlines()", "\\mathrm{readlines}@<@>", "readlines() returns the lines of a file as a list of strings");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"setpos",1,1,1,1,1,1,(void *)pplmethod_fileSetpos,"setpos(x)", "\\mathrm{setpos}@<@>", "setpos(x) sets a file handle's current position in a file");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_FILE],"write",1,1,0,0,0,0,(void *)pplmethod_fileWrite,"write(x)", "\\mathrm{write}@<@0@>", "write(x) writes the string x to a file");

  // Exception methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_EXC],"raise",1,1,0,0,0,0,(void *)pplmethod_excRaise,"raise(s)", "\\mathrm{raise}@<@0@>", "raise(s) raises an exception with error string s");

  return;
 }

