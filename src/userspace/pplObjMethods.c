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

// Vector methods

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

void pplmethod_listLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  list *l = (list *)in[-1].self_this->auxil;
  pplObjNum(&OUTPUT,0,l->length,0);
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
    COPYSTR(tmp,key);
    if (pplObjStr(&va,1,1,tmp )==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    pplObjCpy(&vb,item,1,1);
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
  l  = (list *)OUTPUT.auxil;
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   { COPYSTR(tmp,key); pplObjCpy(&v,item,1,1); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj)); }
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
  gsl_matrix      *vo  = NULL;
  gsl_permutation *p   = NULL;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "The inverse is only defined for square matrices."); return; }
  if ((tmp=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp,m);
  if ((p = gsl_permutation_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if (gsl_linalg_LU_decomp(tmp,p,&s)!=0) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "LU decomposition failed whilst computing matrix determinant."); return; }
  if (pplObjMatrix(&OUTPUT,0,1,n,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplMatrix *)OUTPUT.self_this->auxil)->m;
  if (gsl_linalg_LU_invert(tmp,p,vo)) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "Numerical failure while computing matrix inverse."); return; }
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
  gsl_matrix      *m = ((pplMatrix *)in[-1].self_this->auxil)->m;
  int              n = m->size1, i, j;
  gsl_matrix      *vo = NULL;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, "The transpose is only defined for square matrices."); return; }
  if (pplObjMatrix(&OUTPUT,0,1,n,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplMatrix *)OUTPUT.self_this->auxil)->m;
  for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) gsl_matrix_set(vo,i,j,gsl_matrix_get(m,j,i));
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

  // Vector methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"len",0,0,1,1,1,1,(void *)pplmethod_vectorLen, "len()", "\\mathrm{len}@<@>", "len() returns the number of dimensions of a vector");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"norm",0,0,1,1,1,1,(void *)pplmethod_vectorNorm, "norm()", "\\mathrm{norm}@<@>", "norm() returns the norm (quadrature sum) of a vector's elements");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"reverse",0,0,1,1,1,1,(void *)pplmethod_vectorReverse, "reverse()", "\\mathrm{reverse}@<@>", "reverse() reverses the order of the elements of a vector");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_VEC],"sort",0,0,1,1,1,1,(void *)pplmethod_vectorSort, "sort()", "\\mathrm{sort}@<@>", "sort() sorts the members of a vector");

  // List methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"len",0,0,1,1,1,1,(void *)pplmethod_listLen, "len()", "\\mathrm{len}@<@>", "len() returns the length of a list");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"reverse",0,0,1,1,1,1,(void *)pplmethod_listReverse, "reverse()", "\\mathrm{reverse}@<@>", "reverse() reverses the order of the members of a list");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_LIST],"sort",0,0,1,1,1,1,(void *)pplmethod_listSort, "sort()", "\\mathrm{sort}@<@>", "sort() sorts the members of a list");

  // Dictionary methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DICT],"hasKey",1,1,0,0,0,0,(void *)pplmethod_dictHasKey, "hasKey()", "\\mathrm{hasKey}@<@1@>", "hasKey(x) returns a boolean indicating whether the key x exists in the dictionary");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DICT],"items" ,0,0,1,1,1,1,(void *)pplmethod_dictItems , "items()", "\\mathrm{items}@<@>", "items() returns a list of the [key,value] pairs in a dictionary");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DICT],"keys"  ,0,0,1,1,1,1,(void *)pplmethod_dictKeys  , "keys()", "\\mathrm{keys}@<@>", "keys() returns a list of the keys defined in a dictionary");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DICT],"len"   ,0,0,1,1,1,1,(void *)pplmethod_dictLen   , "len()", "\\mathrm{len}@<@>", "len() returns the number of entries in a dictionary");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_DICT],"values",0,0,1,1,1,1,(void *)pplmethod_dictValues, "values()", "\\mathrm{values}@<@>", "values() returns a list of the values in a dictionary");

  // Matrix methods
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"det" ,0,0,1,1,1,1,(void *)pplmethod_matrixDet,"det()", "\\mathrm{det}@<@>", "det() returns the determinant of a square matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"eigenvalues",0,0,1,1,1,1,(void *)pplmethod_matrixEigenvalues,"eigenvalues()", "\\mathrm{eigenvalues}@<@>", "eigenvalues() returns a vector containing the eigenvalues of a square symmetric matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"eigenvectors",0,0,1,1,1,1,(void *)pplmethod_matrixEigenvectors,"eigenvectors()", "\\mathrm{eigenvectors}@<@>", "eigenvectors() returns a list of the eigenvectors of a square symmetric matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"inv" ,0,0,1,1,1,1,(void *)pplmethod_matrixInv,"inv()", "\\mathrm{inv}@<@>", "inv() returns the inverse of a square matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"size",0,0,1,1,1,1,(void *)pplmethod_matrixSize, "size()", "\\mathrm{size}@<@>", "size() returns the dimensions of a matrix");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"symmetric",0,0,1,1,1,1,(void *)pplmethod_matrixSymmetric, "symmetric()", "\\mathrm{symmetric}@<@>", "symmetric() returns a boolean indicating whether a matrix is symmetric");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_MAT],"transpose",0,0,1,1,1,1,(void *)pplmethod_matrixTrans, "transpose()", "\\mathrm{transpose}@<@>", "transpose() returns the transpose of a matrix");

  return;
 }

