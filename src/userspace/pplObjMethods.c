// pplObjMethods.c
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

#define _PPLOBJMETHODS_C 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "gsl/gsl_blas.h"
#include "gsl/gsl_eigen.h"
#include "gsl/gsl_linalg.h"
#include "gsl/gsl_math.h"
#include "gsl/gsl_matrix.h"
#include "gsl/gsl_permutation.h"
#include "gsl/gsl_vector.h"

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "expressions/fnCall.h"

#include "mathsTools/dcfmath.h"

#include "userspace/calendars.h"
#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjCmp.h"
#include "userspace/pplObjDump.h"
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

#define STACK_POP_LISTMETHOD \
   { \
    c->stackPtr--; \
    ppl_garbageObject(&c->stack[c->stackPtr]); \
    if (c->stack[c->stackPtr].refCount != 0) { fprintf(stderr,"Stack forward reference detected."); } \
   }

#define TBADD_LISTMETHOD(et) ppl_tbAdd(c,dummy.srcLineN,dummy.srcId,dummy.srcFname,0,et,0,dummy.ascii,"")


// Universal methods of all objects

static void pplmethod_class(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  if (t==NULL) pplObjNull(&OUTPUT,0);
  else         pplObjCpy (&OUTPUT,t->objPrototype,0,0,1);
  OUTPUT.self_lval = NULL;
 }

static void pplmethod_data(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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
      int i,new=1; for (i=0; i<count; i++) if (strcmp(key,keys[i])==0) { new=0; break; }
      if (!new) continue;
      if (item->objType==PPLOBJ_FUNC) continue; // Non-methods only
      COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
      keys[count++]=key; if (count>4094) break;
     }
   }
  di = ppl_dictIterateInit( pplObjMethods[st->objType] );
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   {
    int i,new=1; for (i=0; i<count; i++) if (strcmp(key,keys[i])==0) { new=0; break; }
    if (!new) continue;
    if (item->objType==PPLOBJ_FUNC) continue; // Non-methods only
    COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
    keys[count++]=key; if (count>4094) break;
   }
 }

static void pplmethod_contents(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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
      int i,new=1; for (i=0; i<count; i++) if (strcmp(key,keys[i])==0) { new=0; break; }
      if (!new) continue;
      COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
      keys[count++]=key; if (count>4094) break;
     }
   }
  di = ppl_dictIterateInit( pplObjMethods[st->objType] );
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   {
    int i,new=1; for (i=0; i<count; i++) if (strcmp(key,keys[i])==0) { new=0; break; }
    if (!new) continue;
    COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
    keys[count++]=key; if (count>4094) break;
   }
 }

static void pplmethod_methods(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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
      int i,new=1; for (i=0; i<count; i++) if (strcmp(key,keys[i])==0) { new=0; break; }
      if (!new) continue;
      if (item->objType!=PPLOBJ_FUNC) continue; // List methods only
      COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
      keys[count++]=key; if (count>4094) break;
     }
   }
  di = ppl_dictIterateInit( pplObjMethods[st->objType] );
  while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
   {
    int i,new=1; for (i=0; i<count; i++) if (strcmp(key,keys[i])==0) { new=0; break; }
    if (!new) continue;
    if (item->objType!=PPLOBJ_FUNC) continue; // List methods only
    COPYSTR(tmp,key); pplObjStr(&v,1,1,tmp); ppl_listAppendCpy(l, (void *)&v, sizeof(pplObj));
    keys[count++]=key; if (count>4094) break;
   }
 }

static void pplmethod_str(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t   = in[-1].self_this;
  char   *tmp = (char *)malloc(LSTR_LENGTH);
  if (tmp==NULL) { *status=1; sprintf(errText,"Out of memory."); return; }
  pplObjPrint(c,t,NULL,tmp,LSTR_LENGTH,0,0);
  pplObjStr(&OUTPUT,0,1,tmp);
 }

static void pplmethod_type(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  if (t==NULL) pplObjNull(&OUTPUT,0);
  else         pplObjCpy (&OUTPUT,&pplObjPrototypes[t->objType],0,0,1);
  OUTPUT.self_lval = NULL;
 }

// String methods

static void pplmethod_strUpper(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i=0;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  COPYSTR(tmp, work);
  while (tmp[i]!='\0') { tmp[i] = toupper(tmp[i]); i++; }
  pplObjStr(&OUTPUT,0,1,tmp);
 }

static void pplmethod_strLower(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i=0;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  COPYSTR(tmp, work);
  while (tmp[i]!='\0') { tmp[i] = tolower(tmp[i]); i++; }
  pplObjStr(&OUTPUT,0,1,tmp);
 }

static void pplmethod_strSplit(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *instr = (char *)t->auxil;
  int     i,j;
  list   *lst;
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  lst = (list *)OUTPUT.auxil;
  for (i=0,j=0 ; ; i++)
   if ((instr[i]<=' ')&&(instr[i]>='\0'))
    {
     if (j<i)
      {
       pplObj  v;
       char   *s = (char *)malloc(i-j+1);
       if (s==NULL) break;
       memcpy(s, instr+j, i-j);
       s[i-j]='\0';
       pplObjStr(&v,1,1,s);
       v.refCount=1;
       ppl_listAppendCpy(lst, &v, sizeof(v));
      }
     j=i+1;
     if (instr[i]=='\0') break;
    }
 }

static void pplmethod_strSplitOn(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *instr = (char *)t->auxil;
  int     i,j;
  list   *lst;
  for (i=0; i<nArgs; i++)
   if (in[i].objType!=PPLOBJ_STR) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The splitOn() method requires all its arguments to be strings; argument %d has type <%s>.",i+1,pplObjTypeNames[in[i].objType]); return; }
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  lst = (list *)OUTPUT.auxil;
  for (i=0,j=0 ; ; i++)
   {
    int breaking = (instr[i]=='\0');
    int k,l=0;
    for (k=0; ((k<nArgs)&&(!breaking)); k++)
     {
      char *cmp=(char *)in[k].auxil;
      for (l=0; cmp[l]!='\0'; l++)
       {
        if (instr[i+l]=='\0') break;
        if (instr[i+l]!=cmp[l]) break;
       }
      if (cmp[l]=='\0') breaking=1;
      else              l=0;
     }
    if (breaking)
     {
      pplObj  v;
      char   *s = (char *)malloc(i-j+1);
      if (s==NULL) break;
      memcpy(s, instr+j, i-j);
      s[i-j]='\0';
      pplObjStr(&v,1,1,s);
      v.refCount=1;
      ppl_listAppendCpy(lst, &v, sizeof(v));
      j=i+l;
      i=ppl_max(i,j-1);
     }
    if (instr[i]=='\0') break;
   }
 }

static void pplmethod_strStrip(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i,j;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  COPYSTR(tmp, work);
  for (i=0; ((tmp[i]>'\0')&&(tmp[i]<=' ')); i++);
  for (j=0; tmp[i]!='\0'; i++,j++) tmp[j]=tmp[i];
  j--;
  for (   ; ((j>=0)&&(tmp[j]>'\0')&&(tmp[j]<=' ')); j--) tmp[j]='\0';
  tmp[j+1]='\0';
  pplObjStr(&OUTPUT,0,1,tmp);
 }

static void pplmethod_strLStrip(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i=0,j=0;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  COPYSTR(tmp, work);
  for (i=0; ((tmp[i]>'\0')&&(tmp[i]<=' ')); i++);
  for (j=0; tmp[i]!='\0'; i++,j++) tmp[j]=tmp[i];
  tmp[j]='\0';
  pplObjStr(&OUTPUT,0,1,tmp);
 }

static void pplmethod_strRStrip(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int     i=0,j=0;
  pplObj *t = in[-1].self_this;
  char   *tmp, *work=(char *)t->auxil;
  COPYSTR(tmp, work);
  for (j=0; tmp[i]!='\0'; i++,j++) tmp[j]=tmp[i];
  j--;
  for (   ; ((j>=0)&&(tmp[j]>'\0')&&(tmp[j]<=' ')); j--) tmp[j]='\0';
  tmp[j+1]='\0';
  pplObjStr(&OUTPUT,0,1,tmp);
 }

static void pplmethod_strisalpha(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *s = (char *)t->auxil;
  int i, l=strlen(s), out=1;
  for (i=0; i<l; i++) if (!isalpha(s[i])) { out=0; break; }
  pplObjBool(&OUTPUT,0,out);
 }

static void pplmethod_strisdigit(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *s = (char *)t->auxil;
  int i, l=strlen(s), out=1;
  for (i=0; i<l; i++) if (!isdigit(s[i])) { out=0; break; }
  pplObjBool(&OUTPUT,0,out);
 }

static void pplmethod_strisalnum(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *s = (char *)t->auxil;
  int i, l=strlen(s), out=1;
  for (i=0; i<l; i++) if (!isalnum(s[i])) { out=0; break; }
  pplObjBool(&OUTPUT,0,out);
 }

static void pplmethod_strAppend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char *instr = (char *)t->auxil, *astr;
  int   alen;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The append() method requires a single string argument."); return; }
  if ((t==NULL)||(t->self_lval==NULL)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The append() method can only be called on a string lvalue."); return; }
  if ((t->immutable)||(t->self_lval->immutable)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The append() method cannot act on immutable strings."); return; }
  t = t->self_lval;
  astr = (char *)in[0].auxil;
  alen = strlen(astr);
  if (in[0].auxilMalloced)
   {
    int ilen   = t->auxilLen - 1;
    int newlen = t->auxilLen + alen;
    instr = (char *)realloc(t->auxil, newlen);
    if (instr==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    t->auxilLen = newlen;
    strcpy(instr+ilen, astr);
   }
  else
   {
    int   ilen   = strlen(instr);
    int   newlen = ilen + alen + 1;
    char *ns     = (char *)malloc(newlen);
    if (ns==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    t->auxilLen = newlen;
    sprintf(ns, "%s%s", instr, astr);
    instr = ns;
   }
  t->auxil = (void *)instr;
  pplObjCpy(&OUTPUT,t,0,0,1);
 }

static void pplmethod_strBeginsWith(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char *instr = (char *)t->auxil, *cmpstr;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The beginsWith() method requires a single string argument."); return; }
  cmpstr = (char *)in[0].auxil;
  pplObjBool(&OUTPUT,0,strncmp(instr,cmpstr,strlen(cmpstr))==0);
 }

static void pplmethod_strEndsWith(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char   *instr = (char *)t->auxil, *cmpstr;
  int     il, cl;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The endsWith() method requires a single string argument."); return; }
  cmpstr = (char *)in[0].auxil;
  il     = strlen(instr);
  cl     = strlen(cmpstr);
  instr += il-cl;
  if (il<cl) { pplObjBool(&OUTPUT,0,0); }
  else       { pplObjBool(&OUTPUT,0,strncmp(instr,cmpstr,cl)==0); }
 }

static void pplmethod_strFind(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_strFindAll(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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
    { pplObjNum(&v,1,p,0); ppl_listAppendCpy(l, &v, sizeof(v)); }
 }

static void pplmethod_strLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  char *instr = (char *)t->auxil;
  pplObjNum(&OUTPUT,0,strlen(instr),0);
 }

// Date methods

#define TZ_INIT \
 double offset; \
 { \
  if ((nArgs>0)&&(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a string as its first argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; } \
  if (nArgs>0) ppl_calendarTimezoneSet(c, 1, (char*)in[0].auxil); \
  else         ppl_calendarTimezoneSet(c, 0, NULL              ); \
  ppl_calendarTimezoneOffset(c, t->real, NULL, &offset); \
  ppl_calendarTimezoneUnset(c); \
 }

static void pplmethod_dateToDayOfMonth(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toDayOfMonth(<timezone>)";
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  TZ_INIT;
  ppl_fromUnixTime(c,t->real+offset,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,day,0);
 }

static void pplmethod_dateToDayWeekName(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toDayWeekName(<timezone>)";
  pplObj *t = in[-1].self_this;
  char *tmp;
  TZ_INIT;
  COPYSTR(tmp , ppl_getWeekDayName(c, floor( fmod((t->real+offset)/3600/24+3 , 7))));
  pplObjStr(&OUTPUT,0,1,tmp);
 }

static void pplmethod_dateToDayWeekNum(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toDayWeekNum(<timezone>)";
  pplObj *t = in[-1].self_this;
  TZ_INIT;
  pplObjNum(&OUTPUT,0,floor( fmod((t->real+offset)/3600/24+4 , 7))+1,0);
 }

static void pplmethod_dateToHour(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toHour(<timezone>)";
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  TZ_INIT;
  ppl_fromUnixTime(c,t->real+offset,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,hour,0);
 }

static void pplmethod_dateToJD(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  pplObjNum(&OUTPUT,0, t->real / 86400.0 + 2440587.5 ,0);
 }

static void pplmethod_dateToMinute(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toMinute(<timezone>)";
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  TZ_INIT;
  ppl_fromUnixTime(c,t->real+offset,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,day,0);
 }

static void pplmethod_dateToMJD(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  pplObjNum(&OUTPUT,0, t->real / 86400.0 + 40587.0 ,0);
 }

static void pplmethod_dateToMonthName(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toMonthName(<timezone>)";
  int year, month, day, hour, minute; double second;
  char *tmp;
  pplObj *t = in[-1].self_this;
  TZ_INIT;
  ppl_fromUnixTime(c,t->real+offset,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  COPYSTR(tmp, ppl_getMonthName(c,month));
  pplObjStr(&OUTPUT,0,1,tmp);
 }

static void pplmethod_dateToMonthNum(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toMonthNum(<timezone>)";
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  TZ_INIT;
  ppl_fromUnixTime(c,t->real+offset,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,month,0);
 }

static void pplmethod_dateToSecond(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toSecond(<timezone>)";
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  TZ_INIT;
  ppl_fromUnixTime(c,t->real+offset,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,second,0);
 }

static void pplmethod_dateToStr(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char  *FunctionDescription = "str(<s>,<timezone>)";
  pplObj *t = in[-1].self_this;
  char  *format=NULL, *out=NULL, timezone[FNAME_LENGTH];
  double offset;
  if ((nArgs>0)&&(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a string as its first argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  if ((nArgs>1)&&(in[1].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a string as its second argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[1].objType]); return; }
  out = (char *)malloc(8192);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }

  if (nArgs>0) format = (char *)in[0].auxil; // Format specified
  else         format = NULL;                // Format not specified
  if (nArgs>1) ppl_calendarTimezoneSet(c, 1, (char*)in[1].auxil);
  else         ppl_calendarTimezoneSet(c, 0, NULL              );
  ppl_calendarTimezoneOffset(c, t->real, timezone, &offset);
  ppl_dateString(c, out, t->real+offset, format, timezone, status, errText);
  ppl_calendarTimezoneUnset(c);
  if (*status) { *errType=ERR_NUMERICAL; return; }
  pplObjStr(&OUTPUT,0,1,out);
 }

static void pplmethod_dateToUnix(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *t = in[-1].self_this;
  pplObjNum(&OUTPUT,0,t->real,0);
 }

static void pplmethod_dateToYear(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "toYear(<timezone>)";
  int year, month, day, hour, minute; double second;
  pplObj *t = in[-1].self_this;
  TZ_INIT;
  ppl_fromUnixTime(c,t->real+offset,&year,&month,&day,&hour,&minute,&second,status,errText);
  if (*status) { *errType=ERR_RANGE; return; }
  pplObjNum(&OUTPUT,0,year,0);
 }

// Color methods

static void pplmethod_colCompRGB(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_colCompCMYK(ppl_context *dummy, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_colCompHSB(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_colToRGB(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *a = in[-1].self_this;
  double r,g,b;
  if      (round(a->exponent[0])==SW_COLSPACE_RGB ) { r=a->exponent[8]; g=a->exponent[9]; b=a->exponent[10]; }
  else if (round(a->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(a->exponent[8],a->exponent[9],a->exponent[10],a->exponent[11],&r,&g,&b);
  else                                              pplcol_HSBtoRGB(a->exponent[8],a->exponent[9],a->exponent[10],&r,&g,&b);
  pplObjColor(&OUTPUT,0,SW_COLSPACE_RGB,r,g,b,0);
 }

static void pplmethod_colToCMYK(ppl_context *dummy, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *a = in[-1].self_this;
  double c,m,y,k;
  if      (round(a->exponent[0])==SW_COLSPACE_RGB ) pplcol_RGBtoCMYK(a->exponent[8],a->exponent[9],a->exponent[10],&c,&m,&y,&k);
  else if (round(a->exponent[0])==SW_COLSPACE_CMYK) { c=a->exponent[8]; m=a->exponent[9]; y=a->exponent[10]; k=a->exponent[11]; }
  else                                              pplcol_HSBtoCMYK(a->exponent[8],a->exponent[9],a->exponent[10],&c,&m,&y,&k);
  pplObjColor(&OUTPUT,0,SW_COLSPACE_CMYK,c,m,y,k);
 }

static void pplmethod_colToHSB(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *a = in[-1].self_this;
  double h,s,b;
  if      (round(a->exponent[0])==SW_COLSPACE_RGB ) pplcol_RGBtoHSB (a->exponent[8],a->exponent[9],a->exponent[10],&h,&s,&b);
  else if (round(a->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoHSB(a->exponent[8],a->exponent[9],a->exponent[10],a->exponent[11],&h,&s,&b);
  else                                              { h=a->exponent[8]; s=a->exponent[9]; b=a->exponent[10]; }
  pplObjColor(&OUTPUT,0,SW_COLSPACE_HSB,h,s,b,0);
 }

// Vector methods

static void pplmethod_vectorAppend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_vectorExtend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_vectorFilter(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  ppl_context *context = c;
  char        *FunctionDescription = "filter(f)";
  pplObj      *st = in[-1].self_this;
  gsl_vector  *va = ((pplVector *)st->auxil)->v;
  gsl_vector  *vo;
  double      *ovec;
  int          ovlen=0, i=0;
  pplObj       v;
  pplFunc     *fi;
  pplExpr      dummy;

  STACK_MUSTHAVE(c,4);
  if (c->stackFull) { *status=1; *errType=ERR_TYPE; strcpy(errText,"Stack overflow."); return; }

  if (in[0].objType != PPLOBJ_FUNC) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Supplied object is of type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  fi = (pplFunc *)in[0].auxil;
  if ((fi==NULL)||(fi->functionType==PPL_FUNC_MAGIC)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Integration and differentiation operators are not suitable functions.", FunctionDescription); return; }

  ovec = (double *)malloc(va->size * sizeof(double));
  if (ovec==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }

  // Dummy expression object with dummy line number information
  dummy.srcLineN = 0;
  dummy.srcId    = 0;
  dummy.srcFname = "<dummy>";
  dummy.ascii    = "<filter(f) function>";

  pplObjNum(&v,0,0,0);
  v.refCount = 1;
  ppl_unitsDimCpy(&v, st);

  for (i=0; i<va->size; i++)
   {
    const int stkLevelOld = c->stackPtr;

    // Push function object
    pplObjCpy(&c->stack[c->stackPtr], &in[0], 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push a
    v.real = gsl_vector_get(va,i);
    pplObjCpy(&c->stack[c->stackPtr], &v, 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Call function
    c->errStat.errMsgExpr[0]='\0';
    ppl_fnCall(c, &dummy, 0, 1, 1, 1);

    // Propagate error if function failed
    if (c->errStat.status)
     {
      *status=1;
      *errType=ERR_TYPE;
      sprintf(errText, "Error inside function supplied to the %s function: %s", FunctionDescription, c->errStat.errMsgExpr);
      free(ovec);
      return;
     }

    CAST_TO_BOOL(&c->stack[c->stackPtr-1]);
    if (c->stack[c->stackPtr-1].real) ovec[ovlen++] = v.real;
    while (c->stackPtr>stkLevelOld) { STACK_POP_LISTMETHOD; }
   }

  if (pplObjVector(&OUTPUT,0,1,ovlen)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); free(ovec); return; }
  vo = ((pplVector *)(OUTPUT.auxil))->v;
  for (i=0; i<ovlen; i++) gsl_vector_set(vo , i , ovec[i]);
  ppl_unitsDimCpy(&OUTPUT,st);
  return;
 }

static void pplmethod_vectorInsert(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_vectorLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  gsl_vector *v = ((pplVector *)in[-1].self_this->auxil)->v;
  pplObjNum(&OUTPUT,0,v->size,0);
 }

static void pplmethod_vectorList(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj     *st  = in[-1].self_this;
  gsl_vector *vec = ((pplVector *)st->auxil)->v;
  int         i;
  const int   l = vec->size;
  pplObj      v;
  list       *lo;
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { strcpy(errText, "Out of memory."); *status=1; *errType=ERR_MEMORY; return; }
  lo = (list *)OUTPUT.auxil;
  v.refCount = 1;
  pplObjNum(&v,1,0,0);
  ppl_unitsDimCpy(&v, st);
  for (i=0; i<l; i++) { v.real = gsl_vector_get(vec,i); ppl_listAppendCpy(lo, &v, sizeof(pplObj)); }
 }

static void pplmethod_vectorMap(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char       *FunctionDescription = "map(f)";
  pplObj     *st = in[-1].self_this;
  gsl_vector *vec = ((pplVector *)st->auxil)->v;
  int         i;
  gsl_vector *ovec;
  pplObj      v, val2;
  pplFunc    *fi;
  pplExpr     dummy;

  STACK_MUSTHAVE(c,4);
  if (c->stackFull) { *status=1; *errType=ERR_TYPE; strcpy(errText,"Stack overflow."); return; }

  if (in[0].objType != PPLOBJ_FUNC) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Supplied object is of type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  fi = (pplFunc *)in[0].auxil;
  if ((fi==NULL)||(fi->functionType==PPL_FUNC_MAGIC)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Integration and differentiation operators are not suitable functions.", FunctionDescription); return; }
  if (pplObjVector(&OUTPUT,0,1,vec->size)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  ovec = ((pplVector *)OUTPUT.auxil)->v;

  // Dummy expression object with dummy line number information
  dummy.srcLineN = 0;
  dummy.srcId    = 0;
  dummy.srcFname = "<dummy>";
  dummy.ascii    = "<filter(f) function>";

  pplObjNum(&v,0,0,0);
  v.refCount = 1;
  ppl_unitsDimCpy(&v, st);

  for (i=0; i<vec->size; i++)
   {
    const int stkLevelOld = c->stackPtr;

    // Push function object
    pplObjCpy(&c->stack[c->stackPtr], &in[0], 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push a
    v.real = gsl_vector_get(vec, i);
    pplObjCpy(&c->stack[c->stackPtr], &v, 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Call function
    c->errStat.errMsgExpr[0]='\0';
    ppl_fnCall(c, &dummy, 0, 1, 1, 1);

    // Propagate error if function failed
    if (c->errStat.status)
     {
      *status=1;
      *errType=ERR_TYPE;
      sprintf(errText, "Error inside function supplied to the %s function: %s", FunctionDescription, c->errStat.errMsgExpr);
      return;
     }

    if (c->stack[c->stackPtr-1].objType != PPLOBJ_NUM)
     {
      *status=1;
      *errType=ERR_TYPE;
      sprintf(errText, "Error inside function supplied to the %s function: function should have returned number, but returned object of type <%s> for element %d.", FunctionDescription, pplObjTypeNames[c->stack[c->stackPtr-1].objType], i);
      return;
     }

    if (i==0) memcpy(&val2, &c->stack[c->stackPtr-1], sizeof(pplObj));
    else if (!ppl_unitsDimEqual(&val2, &c->stack[c->stackPtr-1]))
     {
      *status=1;
      *errType=ERR_UNIT;
      sprintf(errText, "Error inside function supplied to the %s function: function returned values with inconsistent units of <%s> and <%s>. All of the elements of a vector must have matching dimensions.", FunctionDescription, ppl_printUnit(c, &val2, NULL, NULL, 0, 1, 0), ppl_printUnit(c, &c->stack[c->stackPtr-1], NULL, NULL, 1, 1, 0));
      return;
     }

    if (c->stack[c->stackPtr-1].flagComplex)
     {
      *status=1;
      *errType=ERR_NUMERICAL;
      sprintf(errText, "Error inside function supplied to the %s function: function returned a complex number for element %d; vectors can only hold real numbers.", FunctionDescription, i);
      return;
     }

    gsl_vector_set(ovec, i, c->stack[c->stackPtr-1].real);
    while (c->stackPtr>stkLevelOld) { STACK_POP_LISTMETHOD; }
   }

  return;
 }

static void pplmethod_vectorNorm(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st = in[-1].self_this;
  gsl_vector *v = ((pplVector *)st->auxil)->v;
  pplObjNum(&OUTPUT,0,gsl_blas_dnrm2(v),0);
  ppl_unitsDimCpy(&OUTPUT, st);
 }

static void pplmethod_vectorReduce(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char       *FunctionDescription = "reduce(f)";
  const int   stkLevelOld = c->stackPtr;
  int         i;
  pplExpr     dummy;
  pplObj     *st = in[-1].self_this;
  gsl_vector *v = ((pplVector *)st->auxil)->v;
  pplObj      val, val2;
  pplFunc    *fi;

  STACK_MUSTHAVE(c,4);
  if (c->stackFull) { *status=1; *errType=ERR_TYPE; strcpy(errText,"Stack overflow."); return; }

  if (in[0].objType != PPLOBJ_FUNC) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Supplied object is of type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  fi = (pplFunc *)in[0].auxil;
  if ((fi==NULL)||(fi->functionType==PPL_FUNC_MAGIC)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Integration and differentiation operators are not suitable functions.", FunctionDescription); return; }
  if (v->size < 2) { *status=1; *errType=ERR_RANGE; sprintf(errText, "The %s method cannot be called on vectors containing fewer than two elements. Supplied vector has %d element.", FunctionDescription, (int)v->size); return; }

  // Dummy expression object with dummy line number information
  dummy.srcLineN = 0;
  dummy.srcId    = 0;
  dummy.srcFname = "<dummy>";
  dummy.ascii    = "<reduce(f) function>";

  // Fetch first item from vector
  pplObjNum(&val,0,gsl_vector_get(v,0),0);
  val.refCount = 1;
  ppl_unitsDimCpy(&val, st);
  memcpy(&val2, &val, sizeof(pplObj));

  for (i=1; i<v->size; i++)
   {
    // Push function object
    pplObjCpy(&c->stack[c->stackPtr], &in[0], 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push old value
    pplObjCpy(&c->stack[c->stackPtr], &val, 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push next value
    val2.real = gsl_vector_get(v,i);
    pplObjCpy(&c->stack[c->stackPtr], &val2, 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Call function
    c->errStat.errMsgExpr[0]='\0';
    ppl_fnCall(c, &dummy, 0, 2, 1, 1);

    // Garbage collect val
    ppl_garbageObject(&val);

    // Propagate error if function failed
    if (c->errStat.status)
     {
      *status=1;
      *errType=ERR_TYPE;
      sprintf(errText, "Error inside function supplied to the %s function: %s", FunctionDescription, c->errStat.errMsgExpr);
      return;
     }

    pplObjCpy(&val, &c->stack[c->stackPtr-1], 0, 0, 1);
    val.refCount=1;
    while (c->stackPtr>stkLevelOld) { STACK_POP_LISTMETHOD; }
   }
  pplObjCpy(&OUTPUT, &val, 0, 0, 1);
  ppl_garbageObject(&val);
  return;
 }

static void pplmethod_vectorReverse(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st = in[-1].self_this;
  gsl_vector *v = ((pplVector *)st->auxil)->v;
  long i,j;
  long n = v->size;
  pplObjCpy(&OUTPUT,st,0,0,1);
  if (v->size<2) return;
  for ( i=0 , j=n-1 ; i<j ; i++ , j-- ) { double tmp = gsl_vector_get(v,i); gsl_vector_set(v,i,gsl_vector_get(v,j)); gsl_vector_set(v,j,tmp); }
 }

static void pplmethod_vectorSort(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st = in[-1].self_this;
  gsl_vector *v = ((pplVector *)st->auxil)->v;
  pplObjCpy(&OUTPUT,st,0,0,1);
  if (v->size<2) return;
  qsort((void *)v->data , v->size , sizeof(double)*v->stride , ppl_dblSort);
 }

// List methods

static void pplmethod_listAppend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj  v;
  pplObj *st = in[-1].self_this;
  list   *l  = (list *)st->auxil;
  v.refCount=1;
  pplObjCpy(&v,&in[0],0,1,1);
  ppl_listAppendCpy(l, &v, sizeof(pplObj));
  pplObjCpy(&OUTPUT,st,0,0,1);
 }

static void pplmethod_listCount(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st    = in[-1].self_this;
  list   *l     = (list *)st->auxil;
  int     count = 0;
  listIterator *li = ppl_listIterateInit(l);
  pplObj *item;
  while ((item = (pplObj*)ppl_listIterate(&li))!=NULL)
   {
    pplObj *a = item;
    pplObj *b = &in[0];
    if (pplObjCmpQuiet(&a,&b)==0) count++;
   }
  OUTPUT.real = count;
  return;
 }

static void pplmethod_listExtend(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_listFilter(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  ppl_context *context = c;
  char    *FunctionDescription = "filter(f)";
  pplObj  *st = in[-1].self_this;
  list    *l  = (list *)st->auxil;
  list    *ol;
  pplObj  *item;
  pplFunc *fi;
  listIterator *li = ppl_listIterateInit(l);
  pplExpr  dummy;

  STACK_MUSTHAVE(c,4);
  if (c->stackFull) { *status=1; *errType=ERR_TYPE; strcpy(errText,"Stack overflow."); return; }

  if (in[0].objType != PPLOBJ_FUNC) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Supplied object is of type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  fi = (pplFunc *)in[0].auxil;
  if ((fi==NULL)||(fi->functionType==PPL_FUNC_MAGIC)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Integration and differentiation operators are not suitable functions.", FunctionDescription); return; }
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  ol = (list *)OUTPUT.auxil;

  // Dummy expression object with dummy line number information
  dummy.srcLineN = 0;
  dummy.srcId    = 0;
  dummy.srcFname = "<dummy>";
  dummy.ascii    = "<filter(f) function>";

  while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
   {
    const int stkLevelOld = c->stackPtr;

    // Push function object
    pplObjCpy(&c->stack[c->stackPtr], &in[0], 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push a
    pplObjCpy(&c->stack[c->stackPtr], item, 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Call function
    c->errStat.errMsgExpr[0]='\0';
    ppl_fnCall(c, &dummy, 0, 1, 1, 1);

    // Propagate error if function failed
    if (c->errStat.status)
     {
      *status=1;
      *errType=ERR_TYPE;
      sprintf(errText, "Error inside function supplied to the %s function: %s", FunctionDescription, c->errStat.errMsgExpr);
      return;
     }

    CAST_TO_BOOL(&c->stack[c->stackPtr-1]);
    if (c->stack[c->stackPtr-1].real)
     {
      pplObj v;
      pplObjCpy(&v, item, 0, 1, 1);
      v.refCount=1;
      ppl_listAppendCpy(ol, &v, sizeof(v));
     }
    while (c->stackPtr>stkLevelOld) { STACK_POP_LISTMETHOD; }
   }

  return;
 }

static void pplmethod_listIndex(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st    = in[-1].self_this;
  list   *l     = (list *)st->auxil;
  int     i     = 0;
  listIterator *li = ppl_listIterateInit(l);
  pplObj *item;
  while ((item = (pplObj*)ppl_listIterate(&li))!=NULL)
   {
    pplObj *a = item;
    pplObj *b = &in[0];
    if (pplObjCmpQuiet(&a,&b)==0) { OUTPUT.real=i; return; }
    i++;
   }
  OUTPUT.real = -1;
  return;
 }

static void pplmethod_listInsert(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_listLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  list *l = (list *)in[-1].self_this->auxil;
  pplObjNum(&OUTPUT,0,l->length,0);
 }

static void pplmethod_listMap(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char    *FunctionDescription = "map(f)";
  pplObj  *st = in[-1].self_this;
  list    *l  = (list *)st->auxil;
  list    *ol;
  pplObj  *item;
  pplFunc *fi;
  pplExpr  dummy;
  listIterator *li = ppl_listIterateInit(l);

  STACK_MUSTHAVE(c,4);
  if (c->stackFull) { *status=1; *errType=ERR_TYPE; strcpy(errText,"Stack overflow."); return; }

  if (in[0].objType != PPLOBJ_FUNC) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Supplied object is of type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  fi = (pplFunc *)in[0].auxil;
  if ((fi==NULL)||(fi->functionType==PPL_FUNC_MAGIC)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Integration and differentiation operators are not suitable functions.", FunctionDescription); return; }
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  ol = (list *)OUTPUT.auxil;

  // Dummy expression object with dummy line number information
  dummy.srcLineN = 0;
  dummy.srcId    = 0;
  dummy.srcFname = "<dummy>";
  dummy.ascii    = "<map(f) function>";

  while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
   {
    const int stkLevelOld = c->stackPtr;

    // Push function object
    pplObjCpy(&c->stack[c->stackPtr], &in[0], 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push a
    pplObjCpy(&c->stack[c->stackPtr], item, 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Call function
    c->errStat.errMsgExpr[0]='\0';
    ppl_fnCall(c, &dummy, 0, 1, 1, 1);

    // Propagate error if function failed
    if (c->errStat.status)
     {
      *status=1;
      *errType=ERR_TYPE;
      sprintf(errText, "Error inside function supplied to the %s function: %s", FunctionDescription, c->errStat.errMsgExpr);
      return;
     }

    {
     pplObj v;
     pplObjCpy(&v, &c->stack[c->stackPtr-1], 0, 1, 1);
     v.refCount=1;
     ppl_listAppendCpy(ol, &v, sizeof(v));
    }
    while (c->stackPtr>stkLevelOld) { STACK_POP_LISTMETHOD; }
   }

  return;
 }

static void pplmethod_listMax(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st = in[-1].self_this;
  list   *l  = (list *)st->auxil;
  pplObj *item, *best=NULL;
  listIterator *li = ppl_listIterateInit(l);
  while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
   {
    if ((best==NULL)||(pplObjCmpQuiet((void*)&item, (void*)&best)==1)) best=item;
   }
  if (best==NULL) pplObjNull(&OUTPUT,0);
  else            pplObjCpy (&OUTPUT,best,0,0,1);
 }

static void pplmethod_listMin(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *st = in[-1].self_this;
  list   *l  = (list *)st->auxil;
  pplObj *item, *best=NULL;
  listIterator *li = ppl_listIterateInit(l);
  while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
   {
    if ((best==NULL)||(pplObjCmpQuiet((void*)&item, (void*)&best)==-1)) best=item;
   }
  if (best==NULL) pplObjNull(&OUTPUT,0);
  else            pplObjCpy (&OUTPUT,best,0,0,1);
 }

static void pplmethod_listPop(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_listReduce(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char     *FunctionDescription = "reduce(f)";
  const int stkLevelOld = c->stackPtr;
  pplExpr   dummy;
  pplObj   *st = in[-1].self_this;
  list     *l  = (list *)st->auxil;
  pplObj   *item, v;
  pplFunc  *fi;
  listIterator *li = ppl_listIterateInit(l);

  STACK_MUSTHAVE(c,4);
  if (c->stackFull) { *status=1; *errType=ERR_TYPE; strcpy(errText,"Stack overflow."); return; }

  if (in[0].objType != PPLOBJ_FUNC) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Supplied object is of type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  fi = (pplFunc *)in[0].auxil;
  if ((fi==NULL)||(fi->functionType==PPL_FUNC_MAGIC)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Integration and differentiation operators are not suitable functions.", FunctionDescription); return; }
  if (l->length < 2) { *status=1; *errType=ERR_RANGE; sprintf(errText, "The %s method cannot be called on lists containing fewer than two items. Supplied list has length %d.", FunctionDescription, l->length); return; }

  // Dummy expression object with dummy line number information
  dummy.srcLineN = 0;
  dummy.srcId    = 0;
  dummy.srcFname = "<dummy>";
  dummy.ascii    = "<reduce(f) function>";

  // Fetch first item from list
  item = (pplObj *)ppl_listIterate(&li);
  pplObjCpy(&v, item, 0, 0, 1);
  v.refCount = 1;

  while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
   {
    // Push function object
    pplObjCpy(&c->stack[c->stackPtr], &in[0], 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push old value
    pplObjCpy(&c->stack[c->stackPtr], &v, 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push next value
    pplObjCpy(&c->stack[c->stackPtr], item, 0, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Call function
    c->errStat.errMsgExpr[0]='\0';
    ppl_fnCall(c, &dummy, 0, 2, 1, 1);

    // Garbage collect v
    ppl_garbageObject(&v);

    // Propagate error if function failed
    if (c->errStat.status)
     {
      *status=1;
      *errType=ERR_TYPE;
      sprintf(errText, "Error inside function supplied to the %s function: %s", FunctionDescription, c->errStat.errMsgExpr);
      return;
     }

    pplObjCpy(&v, &c->stack[c->stackPtr-1], 0, 0, 1);
    v.refCount=1;
    while (c->stackPtr>stkLevelOld) { STACK_POP_LISTMETHOD; }
   }
  pplObjCpy(&OUTPUT, &v, 0, 0, 1);
  ppl_garbageObject(&v);
  return;
 }

static void pplmethod_listReverse(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long      i;
  pplObj   *st    = in[-1].self_this;
  list     *l     = (list *)st->auxil;
  listItem *l1    = l->first;
  listItem *l2    = l->last;
  long      n     = l->length;
  pplObjCpy(&OUTPUT,st,0,0,1);
  if (n<2) return;

  for (i=0; i<n/2; i++)
   {
    void *tmp = l1->data; l1->data=l2->data; l2->data=tmp;
    l1=l1->next; l2=l2->prev;
   }
 }

static void pplmethod_listSort(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long     i;
  pplObj  *st = in[-1].self_this;
  list    *l  = (list *)st->auxil;
  long     n  = l->length;
  pplObj **items;
  listIterator *li = ppl_listIterateInit(l);
  pplObjCpy(&OUTPUT,st,0,0,1);
  if (n<2) return;
  items = (pplObj **)malloc(n * sizeof(pplObj *));
  if (items==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }
  i=0; while (li!=NULL) { items[i++]=(pplObj*)li->data; ppl_listIterate(&li); }
  qsort(items, n, sizeof(pplObj*), pplObjCmpQuiet);
  li = ppl_listIterateInit(l);
  i=0; while (li!=NULL) { li->data=(void*)items[i++]; ppl_listIterate(&li); }
  free(items);
 }

static pplObj      *pplmethod_listSortOnCustom_fn      = NULL;
static ppl_context *pplmethod_listSortOnCustom_context = NULL;
static int          pplmethod_listSortOnCustom_errFlag = 0;

static int pplmethod_listSortOnCustom_slave(const void *a, const void *b)
 {
  ppl_context *c = pplmethod_listSortOnCustom_context;
  pplExpr      dummy;
  int          stkLevelOld;
  int          out = 0;
  pplObj     **ao  = (pplObj **)a;
  pplObj     **bo  = (pplObj **)b;
  if (pplmethod_listSortOnCustom_errFlag) return 0;
  if ((c==NULL)||(a==NULL)||(b==NULL)) return 0;
  stkLevelOld = c->stackPtr;

  // Dummy expression object with dummy line number information
  dummy.srcLineN = 0;
  dummy.srcId    = 0;
  dummy.srcFname = "<dummy>";
  dummy.ascii    = "<sortOn(f) method>";

  // Push function object
  pplObjCpy(&c->stack[c->stackPtr], pplmethod_listSortOnCustom_fn, 1, 0, 1);
  c->stack[c->stackPtr].refCount=1;
  c->stackPtr++;

  // Push a
  pplObjCpy(&c->stack[c->stackPtr], *ao, 0, 0, 1);
  c->stack[c->stackPtr].refCount=1;
  c->stackPtr++;

  // Push b
  pplObjCpy(&c->stack[c->stackPtr], *bo, 0, 0, 1);
  c->stack[c->stackPtr].refCount=1;
  c->stackPtr++;

  // Call function
  c->errStat.errMsgExpr[0]='\0';
  ppl_fnCall(c, &dummy, 0, 2, 1, 1);

  // Propagate error if function failed
  if (c->errStat.status) { pplmethod_listSortOnCustom_errFlag=1; return 0; }

  // Return error if function didn't return a number
  if (c->stack[c->stackPtr-1].objType!=PPLOBJ_NUM) { sprintf(c->errStat.errBuff, "The sortOn(f) function requires a comparison function that returns a number. Supplied function returned an object of type <%s>.", pplObjTypeNames[c->stack[c->stackPtr-1].objType]); TBADD_LISTMETHOD(ERR_TYPE); pplmethod_listSortOnCustom_errFlag=1; return 0; }

  // Get number back and clean stack
  if      (c->stack[c->stackPtr-1].real < 0) out=-1;
  else if (c->stack[c->stackPtr-1].real > 0) out= 1;
  else                                       out= 0;
  while (c->stackPtr>stkLevelOld) { STACK_POP_LISTMETHOD; }

  return out;
 }

static void pplmethod_listSortOnCustom(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char    *FunctionDescription = "sortOn(f)";
  long     i;
  pplObj  *st = in[-1].self_this;
  list    *l  = (list *)st->auxil;
  long     n  = l->length;
  pplObj **items;
  pplFunc *fi;
  int      fail;
  listIterator *li = ppl_listIterateInit(l);

  STACK_MUSTHAVE(c,4);
  if (c->stackFull) { *status=1; *errType=ERR_TYPE; strcpy(errText,"Stack overflow."); return; }

  if (in[0].objType != PPLOBJ_FUNC) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Supplied object is of type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  fi = (pplFunc *)in[0].auxil;
  if ((fi==NULL)||(fi->functionType==PPL_FUNC_MAGIC)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s method requires a function object as its first argument. Integration and differentiation operators are not suitable functions.", FunctionDescription); return; }
  pplObjCpy(&OUTPUT,st,0,0,1);
  if (n<2) return;
  items = (pplObj **)malloc(n * sizeof(pplObj *));
  if (items==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }
  i=0; while (li!=NULL) { items[i++]=(pplObj*)li->data; ppl_listIterate(&li); }
  pplmethod_listSortOnCustom_fn = &in[0]; pplmethod_listSortOnCustom_errFlag = 0; pplmethod_listSortOnCustom_context = c;
  qsort(items, n, sizeof(pplObj*), pplmethod_listSortOnCustom_slave);
  fail = pplmethod_listSortOnCustom_errFlag;
  pplmethod_listSortOnCustom_fn = NULL; pplmethod_listSortOnCustom_errFlag = 0; pplmethod_listSortOnCustom_context = NULL;
  if (fail) { *status=1; *errType=ERR_GENERIC; strcpy(errText, "Failure of user-supplied comparison function."); return; }
  li = ppl_listIterateInit(l);
  i=0; while (li!=NULL) { li->data=(void*)items[i++]; ppl_listIterate(&li); }
  free(items);
 }

static void pplmethod_listSortOnElement(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long     i;
  pplObj  *st = in[-1].self_this;
  list    *l  = (list *)st->auxil;
  long     n  = l->length;
  pplObj **items;
  int      eNum = (int)floor(in[0].real);
  listIterator *li = ppl_listIterateInit(l);
  i=0; while (li!=NULL) { pplObj *o=(pplObj*)li->data; ppl_listIterate(&li); i++; if (o->objType!=PPLOBJ_LIST) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The sortOnElement() method expects to be sorting a list of lists. Element %ld of list is not a list, but has type <%s>.",i,pplObjTypeNames[o->objType]); return; } }
  li = ppl_listIterateInit(l);
  i=0; while (li!=NULL) { list *o=(list *)((pplObj*)li->data)->auxil; ppl_listIterate(&li); i++; if ((eNum>=o->length)||(eNum<-o->length)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"The sortOnElement() method is sorting on element number %d of each sublist. However, sublist %ld only has elements 0-%d.", eNum, i, o->length-1); return; } }
  pplObjCpy(&OUTPUT,st,0,0,1);
  if (n<2) return;
  items = (pplObj **)malloc(n * 2 * sizeof(pplObj *));
  if (items==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }
  li = ppl_listIterateInit(l);
  i=0;
  while (li!=NULL)
   {
    pplObj *o=(pplObj*)li->data;
    list *l=(list *)o->auxil;
    items[i++]=ppl_listGetItem(l , (eNum>=0) ? eNum : (l->length+eNum));
    items[i++]=(pplObj*)li->data;
    ppl_listIterate(&li);
   }
  qsort(items, n, 2*sizeof(pplObj*), pplObjCmpQuiet);
  li = ppl_listIterateInit(l);
  i=0; while (li!=NULL) { i++; li->data=(void*)items[i++]; ppl_listIterate(&li); }
  free(items);
 }

static void pplmethod_listVector(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long     i       = 0;
  pplObj  *st      = in[-1].self_this;
  list    *listin  = (list *)st->auxil;
  listIterator *li = ppl_listIterateInit(listin);
  const long len   = listin->length;
  gsl_vector *v;
  if (len==0) { *errType = ERR_MEMORY; sprintf(errText,"Cannot create a vector of length zero."); *status=1; return; }
  if (pplObjVector(&OUTPUT,0,1,len)==NULL) { *status=1; sprintf(errText,"Out of memory."); *errType=ERR_MEMORY; return; }
  v = ((pplVector *)OUTPUT.auxil)->v;
  if (len>0)
   {
    pplObj *item = (pplObj*)ppl_listIterate(&li);
    if (item->objType!=PPLOBJ_NUM) { *status=1; sprintf(errText,"Vectors can only hold numeric values. Attempt to add object of type <%s> to vector.", pplObjTypeNames[item->objType]); *errType=ERR_TYPE; return; }
    if (item->flagComplex) { *status=1; sprintf(errText,"Vectors can only hold real numeric values. Attempt to add a complex number."); *errType=ERR_TYPE; return; }
    ppl_unitsDimCpy(&OUTPUT,item);
    gsl_vector_set(v,i,item->real);
   }
  for (i=1; i<len; i++)
   {
    pplObj *item = (pplObj*)ppl_listIterate(&li);
    if (item->objType!=PPLOBJ_NUM) { *status=1; sprintf(errText,"Vectors can only hold numeric values. Attempt to add object of type <%s> to vector.", pplObjTypeNames[item->objType]); *errType=ERR_TYPE; return; }
    if (item->flagComplex) { *status=1; sprintf(errText,"Vectors can only hold real numeric values. Attempt to add a complex number."); *errType=ERR_TYPE; return; }
    if (!ppl_unitsDimEqual(item, &OUTPUT))
     {
      if (OUTPUT.dimensionless)
       { sprintf(errText, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector is dimensionless, but number has units of <%s>.", i+1, ppl_printUnit(c, item, NULL, NULL, 1, 1, 0) ); }
      else if (item->dimensionless)
       { sprintf(errText, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector has units of <%s>, while number is dimensionless.", i+1, ppl_printUnit(c, &OUTPUT, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector has units of <%s>, while number has units of <%s>.", i+1, ppl_printUnit(c, &OUTPUT, NULL, NULL, 0, 1, 0), ppl_printUnit(c, item, NULL, NULL, 1, 1, 0) ); }
      *status  = 1;
      *errType = ERR_UNIT;
      return;
     }
    gsl_vector_set(v,i,item->real);
   }
  return;
 }


// Dictionary methods

static void pplmethod_dictDelete(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj *item;
  pplObj *st = in[-1].self_this;
  dict   *di = (dict *)st->auxil;
  char   *instr;
  if ((nArgs!=1)&&(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The method delete(x) requires a string as its argument; supplied argument had type <%s>.", pplObjTypeNames[in[0].objType]); return; }
  if (in[-1].self_this->immutable) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The method delete(x) cannot be called on an immutable object."); return; }
  instr = (char *)in[0].auxil;
  item  = ppl_dictLookup(di, instr);
  if (item==NULL) return;
  if (item->immutable) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The method delete(x) cannot be called on an immutable object."); return; }
  item->amMalloced = 0;
  ppl_garbageObject(item);
  ppl_dictRemoveKey(di, instr);
  pplObjCpy(&OUTPUT,st,0,0,1);
  return;
 }

static void pplmethod_dictHasKey(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj  *st = in[-1].self_this;
  dict    *di = (dict *)st->auxil;
  char    *instr;
  if ((nArgs!=1)&&(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The method hasKey() requires a string as its argument; supplied argument had type <%s>.", pplObjTypeNames[in[0].objType]); return; }
  instr = (char *)in[0].auxil;
  pplObjBool(&OUTPUT,0,ppl_dictContains(di, instr));
  return;
 }

static void pplmethod_dictItems(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_dictKeys(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_dictLen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  dict *d = (dict *)in[-1].self_this->auxil;
  pplObjNum(&OUTPUT,0,d->length,0);
 }

static void pplmethod_dictValues(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_matrixDet(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj          *st  = in[-1].self_this;
  gsl_matrix      *m   = ((pplMatrix *)in[-1].self_this->auxil)->m;
  double           d   = 0;
  int              n   = m->size1, i;
  int              s;
  gsl_matrix      *tmp = NULL;
  gsl_permutation *p   = NULL;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "The determinant is only defined for square matrices."); return; }
  if ((tmp=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp,m);
  if ((p = gsl_permutation_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if (gsl_linalg_LU_decomp(tmp,p,&s)!=0) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "LU decomposition failed whilst computing matrix determinant."); return; }
  d = gsl_linalg_LU_det(tmp,s);
  gsl_permutation_free(p);
  gsl_matrix_free(tmp);
  pplObjNum(&OUTPUT,0,d,0);
  ppl_unitsDimCpy(&OUTPUT, st);
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) OUTPUT.exponent[i] *= n;
 }

static void pplmethod_matrixDiagonal(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int i,j;
  gsl_matrix *m = ((pplMatrix *)in[-1].self_this->auxil)->m;
  if (m->size1 != m->size2) { pplObjBool(&OUTPUT,0,0); return; }
  for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) if ((i!=j) && (gsl_matrix_get(m,i,j)!=0)) { pplObjBool(&OUTPUT,0,0); return; }
  pplObjBool(&OUTPUT,0,1);
 }

static void pplmethod_matrixEigenvalues(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int         i,j;
  pplObj     *st  = in[-1].self_this;
  gsl_matrix *m   = ((pplMatrix *)st->auxil)->m;
  gsl_matrix *tmp = NULL;
  gsl_vector *vo;
  int         n   = m->size1;
  gsl_eigen_symm_workspace *w;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "Eigenvalues are only defined for square matrices."); return; }
  for (i=0; i<m->size1; i++) for (j=0; j<i; j++) if (gsl_matrix_get(m,i,j) != gsl_matrix_get(m,j,i)) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "Eigenvalues can only be computed for symmetric matrices; supplied matrix is not symmetric."); return; }
  if ((w=gsl_eigen_symm_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if ((tmp=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp,m);
  if (pplObjVector(&OUTPUT,0,1,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplVector *)OUTPUT.auxil)->v;
  if (gsl_eigen_symm(tmp, vo, w)!=0) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "Numerical failure whilst trying to compute eigenvalues."); return; }
  gsl_matrix_free(tmp);
  gsl_eigen_symm_free(w);
  ppl_unitsDimCpy(&OUTPUT, st);
 }

static void pplmethod_matrixEigenvectors(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "Eigenvectors are only defined for square matrices."); return; }
  for (i=0; i<m->size1; i++) for (j=0; j<i; j++) if (gsl_matrix_get(m,i,j) != gsl_matrix_get(m,j,i)) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "Eigenvectors can only be computed for symmetric matrices; supplied matrix is not symmetric."); return; }
  if ((w=gsl_eigen_symmv_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if ((tmp1=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if ((tmp2=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if ((vtmp=gsl_vector_alloc(n)  )==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp1,m);
  if (gsl_eigen_symmv(tmp1, vtmp, tmp2, w)!=0) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "Numerical failure whilst trying to compute eigenvectors."); return; }
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

static void pplmethod_matrixInv(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj          *st  = in[-1].self_this;
  gsl_matrix      *m   = ((pplMatrix *)st->auxil)->m;
  int              n   = m->size1;
  int              s;
  gsl_matrix      *tmp = NULL;
  gsl_matrix      *mo  = NULL;
  gsl_permutation *p   = NULL;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "The inverse is only defined for square matrices."); return; }
  if ((tmp=gsl_matrix_alloc(n,n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  gsl_matrix_memcpy(tmp,m);
  if ((p = gsl_permutation_alloc(n))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  if (gsl_linalg_LU_decomp(tmp,p,&s)!=0) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "LU decomposition failed whilst computing matrix determinant."); return; }
  if (pplObjMatrix(&OUTPUT,0,1,n,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  mo = ((pplMatrix *)OUTPUT.auxil)->m;
  if (gsl_linalg_LU_invert(tmp,p,mo)) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "Numerical failure while computing matrix inverse."); return; }
  gsl_permutation_free(p);
  gsl_matrix_free(tmp);
  ppl_unitsDimInverse(&OUTPUT,st);
 }

static void pplmethod_matrixSize(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  gsl_matrix *m = ((pplMatrix *)in[-1].self_this->auxil)->m;
  gsl_vector *vo;
  if (pplObjVector(&OUTPUT,0,1,2)==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }
  vo = ((pplVector *)OUTPUT.auxil)->v;
  gsl_vector_set(vo,0,m->size1);
  gsl_vector_set(vo,1,m->size2);
 }

static void pplmethod_matrixSymmetric(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int i,j;
  gsl_matrix *m = ((pplMatrix *)in[-1].self_this->auxil)->m;
  if (m->size1 != m->size2) { pplObjBool(&OUTPUT,0,0); return; }
  for (i=0; i<m->size1; i++) for (j=0; j<i; j++) if (gsl_matrix_get(m,i,j) != gsl_matrix_get(m,j,i)) { pplObjBool(&OUTPUT,0,0); return; }
  pplObjBool(&OUTPUT,0,1);
 }

static void pplmethod_matrixTrans(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj          *st  = in[-1].self_this;
  gsl_matrix      *m = ((pplMatrix *)st->auxil)->m;
  int              n = m->size1, i, j;
  gsl_matrix      *vo = NULL;
  if (m->size1 != m->size2) { *status=1; *errType=ERR_NUMERICAL; strcpy(errText, "The transpose is only defined for square matrices."); return; }
  if (pplObjMatrix(&OUTPUT,0,1,n,n)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  vo = ((pplMatrix *)OUTPUT.auxil)->m;
  for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) gsl_matrix_set(vo,i,j,gsl_matrix_get(m,j,i));
  ppl_unitsDimCpy(&OUTPUT,st);
 }

// File methods

static void pplmethod_fileClose(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (f->open)
   {
    if (f->pipe==2) { *status=1; *errType=ERR_TYPE; strcpy(errText, "It is not permitted to close this file handle."); return; }
    f->open=0;
    if (!f->pipe) fclose(f->file);
    else          pplObjNum(&OUTPUT,0,pclose(f->file),0);
    f->file=NULL;
   }
 }

static void pplmethod_fileDump(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  pplObjDump(c, &in[0], f->file);
  return;
 }

static void pplmethod_fileEof(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  pplObjBool(&OUTPUT,0,feof(f->file)!=0);
 }

static void pplmethod_fileFlush(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  if (fflush(f->file)<0) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }
 }

static void pplmethod_fileGetpos(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  long int fp;
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  if ((fp = ftell(f->file))<0) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }
  pplObjNum(&OUTPUT,0,fp*8,0);
  CLEANUP_APPLYUNIT(UNIT_BIT);
 }

static void pplmethod_fileIsOpen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  pplObjBool(&OUTPUT,0,f->open!=0);
 }

static void pplmethod_fileRead(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_fileReadline(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_fileReadlines(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_fileSetpos(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char FunctionDescription[] = "setPos(x)";
  int i;
  long int fp = (long int)in[0].real;
  pplFile *f = (pplFile *)in[-1].self_this->auxil;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "a number of bytes", UNIT_BIT, 1);
  if (!f->open) { pplObjNull(&OUTPUT,0); return; }
  if (!in[0].dimensionless) fp/=8;
  if (fseek(f->file,fp,SEEK_SET)!=0) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }

  if ((fp = ftell(f->file))<0) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }
  pplObjNum(&OUTPUT,0,fp*8,0);
  CLEANUP_APPLYUNIT(UNIT_BIT);
 }

static void pplmethod_fileWrite(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

static void pplmethod_excRaise(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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
    dict *d = pplObjMethods[i] = ppl_dictInit(1);
    if (d==NULL) ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Out of memory.");
    ppl_addSystemMethod(d,"class"   ,0,0,1,1,1,1,(void *)&pplmethod_class   , "class()", "\\mathrm{class}@<@>", "class() returns the class prototype of an object");
    ppl_addSystemMethod(d,"contents",0,0,1,1,1,1,(void *)&pplmethod_contents, "contents()", "\\mathrm{contents}@<@>", "contents() returns a list of all the methods and internal variables of an object");
    ppl_addSystemMethod(d,"data"    ,0,0,1,1,1,1,(void *)&pplmethod_data    , "data()", "\\mathrm{data}@<@>", "data() returns a list of all the internal variables (not methods) of an object");
    ppl_addSystemMethod(d,"methods" ,0,0,1,1,1,1,(void *)&pplmethod_methods , "methods()", "\\mathrm{methods}@<@>", "methods() returns a list of the methods of an object");
    ppl_addSystemMethod(d,"str"     ,0,0,1,1,1,1,(void *)&pplmethod_str     , "str()", "\\mathrm{str}@<@>", "str() returns a string representation of an object");
    ppl_addSystemMethod(d,"type"    ,0,0,1,1,1,1,(void *)&pplmethod_type    , "type()", "\\mathrm{type}@<@>", "type() returns the type of an object");
   }

  // String methods
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"append"    ,1,1,0,0,0,0,(void *)&pplmethod_strAppend    , "append(x)", "\\mathrm{append}@<@1@>", "append(x) appends the string x to the end of a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"beginsWith",1,1,0,0,0,0,(void *)&pplmethod_strBeginsWith, "beginsWith(x)", "\\mathrm{beginsWith}@<@1@>", "beginsWith(x) returns a boolean indicating whether a string begins with the substring x");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"endsWith"  ,1,1,0,0,0,0,(void *)&pplmethod_strEndsWith  , "endsWith(x)", "\\mathrm{endsWith}@<@1@>", "endsWith(x) returns a boolean indicating whether a string ends with the substring x");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"find"      ,1,1,0,0,0,0,(void *)&pplmethod_strFind      , "find(x)", "\\mathrm{find}@<@1@>", "find(x) returns the position of the first occurrence of x in a string, or -1 if it is not found");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"findAll"   ,1,1,0,0,0,0,(void *)&pplmethod_strFindAll   , "findAll(x)", "\\mathrm{findAll}@<@1@>", "findAll(x) returns a list of the positions where the substring x occurs in a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"isalnum"   ,0,0,1,1,1,1,(void *)&pplmethod_strisalnum   , "isalnum()", "\\mathrm{isalnum}@<@>", "isalnum() returns a boolean indicating whether all of the characters of a string are alphanumeric");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"isalpha"   ,0,0,1,1,1,1,(void *)&pplmethod_strisalpha   , "isalpha()", "\\mathrm{isalpha}@<@>", "isalpha() returns a boolean indicating whether all of the characters of a string are alphabetic");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"isdigit"   ,0,0,1,1,1,1,(void *)&pplmethod_strisdigit   , "isdigit()", "\\mathrm{isdigit}@<@>", "isdigit() returns a boolean indicating whether all of the characters of a string are numeric");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"len"       ,0,0,1,1,1,1,(void *)&pplmethod_strLen       , "len()", "\\mathrm{len}@<@>", "len() returns the length of a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"lower"     ,0,0,1,1,1,1,(void *)&pplmethod_strLower     , "lower()", "\\mathrm{lower}@<@>", "lower() converts a string to lowercase");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"split"     ,0,0,1,1,1,1,(void *)&pplmethod_strSplit     , "split()", "\\mathrm{split}@<@>", "split() returns a list of all the whitespace-separated words in a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"splitOn"   ,1,1e3,0,0,0,0,(void *)&pplmethod_strSplitOn , "splitOn(x,...)", "\\mathrm{splitOn}@<@0@>", "splitOn(x,...) splits a string whenever it encounters any of the substrings supplied as arguments, and returns a list of the split string segments");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"lstrip"    ,0,0,1,1,1,1,(void *)&pplmethod_strLStrip    , "lstrip()", "\\mathrm{lstrip}@<@>", "lstrip() strips whitespace off the beginning of a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"strip"     ,0,0,1,1,1,1,(void *)&pplmethod_strStrip     , "strip()", "\\mathrm{strip}@<@>", "strip() strips whitespace off the beginning and end of a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"rstrip"    ,0,0,1,1,1,1,(void *)&pplmethod_strRStrip    , "rstrip()", "\\mathrm{rstrip}@<@>", "rstrip() strips whitespace off the end of a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_STR],"upper"     ,0,0,1,1,1,1,(void *)&pplmethod_strUpper     , "upper()", "\\mathrm{upper}@<@>", "upper() converts a string to uppercase");

  // Date methods
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toDayOfMonth",0,1,0,0,0,0,(void *)pplmethod_dateToDayOfMonth, "toDayOfMonth(<timezone>)", "\\mathrm{toDayOfMonth}@<@0@>", "toDayOfMonth(<timezone>) returns the day of the month of a date object in the current calendar");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toDayWeekName",0,1,0,0,0,0,(void *)pplmethod_dateToDayWeekName, "toDayWeekName(<timezone>)", "\\mathrm{toDayWeekName}@<@0@>", "toDayWeekName(<timezone>) returns the name of the day of the week of a date object");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toDayWeekNum",0,1,0,0,0,0,(void *)pplmethod_dateToDayWeekNum, "toDayWeekNum(<timezone>)", "\\mathrm{toDayWeekNum}@<@0@>", "toDayWeekNum(<timezone>) returns the day of the week (1-7) of a date object");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toHour",0,1,0,0,0,0,(void *)pplmethod_dateToHour, "toHour(<timezone>)", "\\mathrm{toHour}@<@0@>", "toHour(<timezone>) returns the integer hour component (0-23) of a date object");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toJD",0,0,0,0,0,0,(void *)pplmethod_dateToJD, "toJD()", "\\mathrm{toJD}@<@>", "toJD() converts a date object to a numerical Julian date");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toMinute",0,1,0,0,0,0,(void *)pplmethod_dateToMinute, "toMinute(<timezone>)", "\\mathrm{toMinute}@<@0@>", "toMinute(<timezone>) returns the integer minute component (0-59) of a date object");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toMJD",0,0,0,0,0,0,(void *)pplmethod_dateToMJD, "toMJD()", "\\mathrm{toMJD}@<@>", "toMJD() converts a date object to a modified Julian date");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toMonthName",0,1,0,0,0,0,(void *)pplmethod_dateToMonthName, "toMonthName(<timezone>)", "\\mathrm{toMonthName}@<@0@>", "toMonthName(<timezone>) returns the name of the month in which a date object falls");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toMonthNum",0,1,0,0,0,0,(void *)pplmethod_dateToMonthNum, "toMonthNum(<timezone>)", "\\mathrm{toMonthNum}@<@0@>", "toMonthNum(<timezone>) returns the number (1-12) of the month in which a date object falls");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toSecond",0,1,0,0,0,0,(void *)pplmethod_dateToSecond, "toSecond(<timezone>)", "\\mathrm{toSecond}@<@0@>", "toSecond(<timezone>) returns the seconds component (0.0-60.0) of a date object");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"str",0,2,0,0,0,0,(void *)pplmethod_dateToStr, "str(<s>,<timezone>)", "\\mathrm{str}@<@0@>", "str(<s>,<timezone>) converts a date object to a string with an optional format string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toUnix",0,0,0,0,0,0,(void *)pplmethod_dateToUnix, "toUnix()", "\\mathrm{toUnix}@<@>", "toUnix() converts a date object to a Unix time");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_DATE],"toYear",0,1,0,0,0,0,(void *)pplmethod_dateToYear, "toYear(<timezone>)", "\\mathrm{toYear}@<@0@>", "toYear(<timezone>) returns the year in which a date object falls in the current calendar");

  // Color methods
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_COL],"componentsCMYK",0,0,1,1,1,1,(void *)pplmethod_colCompCMYK, "componentsCMYK()", "\\mathrm{componentsCMYK}@<@>", "componentsCMYK() returns a vector CMYK representation of a color");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_COL],"componentsHSB",0,0,1,1,1,1,(void *)pplmethod_colCompHSB, "componentsHSB()", "\\mathrm{componentsHSB}@<@>", "componentsHSB() returns a vector HSB representation of a color");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_COL],"componentsRGB",0,0,1,1,1,1,(void *)pplmethod_colCompRGB, "componentsRGB()", "\\mathrm{componentsRGB}@<@>", "componentsRGB() returns a vector RGB representation of a color");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_COL],"toCMYK",0,0,1,1,1,1,(void *)pplmethod_colToCMYK, "toCMYK()", "\\mathrm{toCMYK}@<@>", "toCMYK() returns a CMYK representation of a color");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_COL],"toHSB",0,0,1,1,1,1,(void *)pplmethod_colToHSB, "toHSB()", "\\mathrm{toHSB}@<@>", "toHSB() returns an HSB representation of a color");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_COL],"toRGB",0,0,1,1,1,1,(void *)pplmethod_colToRGB, "toRGB()", "\\mathrm{toRGB}@<@>", "toRGB() returns an RGB representation of a color");

  // Vector methods
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"append",1,1,1,1,1,0,(void *)pplmethod_vectorAppend, "append(x)", "\\mathrm{append}@<@0@>", "append(x) appends the object x to a vector");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"extend",1,1,0,0,0,0,(void *)pplmethod_vectorExtend, "extend(x)", "\\mathrm{extend}@<@0@>", "extend(x) appends the members of the list x to the end of a vector");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"filter",1,1,0,0,0,0,(void *)pplmethod_vectorFilter, "filter(f)", "\\mathrm{filter}@<@0}@>", "filter(f) takes a pointer to a function of one argument, f(a). It calls the function for every element of the vector, and returns a new vector of those elements for which f(a) tests true");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"insert",2,2,0,0,0,0,(void *)pplmethod_vectorInsert, "insert(n,x)", "\\mathrm{insert}@<@0@>", "insert(n,x) inserts the object x into a vector at position n");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"len",0,0,1,1,1,1,(void *)pplmethod_vectorLen, "len()", "\\mathrm{len}@<@>", "len() returns the number of dimensions of a vector");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"list",0,0,1,1,1,1,(void *)pplmethod_vectorList, "list()", "\\mathrm{list}@<@>", "list() returns a list representation of a vector");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"map",1,1,0,0,0,0,(void *)pplmethod_vectorMap, "map(f)", "\\mathrm{map}@<@0}@>", "map(f) takes a pointer to a function of one argument, f(a). It calls the function for every element of the vector, and returns a vector of the results");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"norm",0,0,1,1,1,1,(void *)pplmethod_vectorNorm, "norm()", "\\mathrm{norm}@<@>", "norm() returns the norm (quadrature sum) of a vector's elements");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"reduce",1,1,0,0,0,0,(void *)pplmethod_vectorReduce, "reduce(f)", "\\mathrm{reduce}@<@0}@>", "reduce(f) takes a pointer to a function of two arguments. It first calls f(a,b) on the first two elements of the vector, and then continues through the vector calling f(a,b) on the result and the next item in the vector. The final result is returned");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"reverse",0,0,1,1,1,1,(void *)pplmethod_vectorReverse, "reverse()", "\\mathrm{reverse}@<@>", "reverse() reverses the order of the elements of a vector");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_VEC],"sort",0,0,1,1,1,1,(void *)pplmethod_vectorSort, "sort()", "\\mathrm{sort}@<@>", "sort() sorts the members of a vector");

  // List methods
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"append",1,1,0,0,0,0,(void *)pplmethod_listAppend, "append(x)", "\\mathrm{append}@<@0}@>", "append(x) appends the object x to a list");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"count",1,1,0,0,0,0,(void *)pplmethod_listCount, "count(x)", "\\mathrm{count}@<@0}@>", "count(x) returns the number of items in a list that equal x");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"extend",1,1,0,0,0,0,(void *)pplmethod_listExtend, "extend(x)", "\\mathrm{extend}@<@0}@>", "extend(x) appends the members of the list x to the end of a list");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"filter",1,1,0,0,0,0,(void *)pplmethod_listFilter, "filter(f)", "\\mathrm{filter}@<@0}@>", "filter(f) takes a pointer to a function of one argument, f(a). It calls the function for every element of the list, and returns a new list of those elements for which f(a) tests true");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"index",1,1,0,0,0,0,(void *)pplmethod_listIndex, "index(x)", "\\mathrm{index}@<@0}@>", "index(x) returns the index of the first element of a list that equals x, or -1 if no elements match");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"insert",2,2,0,0,0,0,(void *)pplmethod_listInsert, "insert(n,x)", "\\mathrm{insert}@<@0@>", "insert(n,x) inserts the object x into a list at position n");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"len",0,0,1,1,1,1,(void *)pplmethod_listLen, "len()", "\\mathrm{len}@<@>", "len() returns the length of a list");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"map",1,1,0,0,0,0,(void *)pplmethod_listMap, "map(f)", "\\mathrm{map}@<@0}@>", "map(f) takes a pointer to a function of one argument, f(a). It calls the function for every element of the list, and returns a list of the results");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"max",0,0,1,1,1,1,(void *)pplmethod_listMax, "max()", "\\mathrm{max}@<@>", "max() returns the highest-valued item in a list");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"min",0,0,1,1,1,1,(void *)pplmethod_listMin, "min()", "\\mathrm{min}@<@>", "min() returns the lowest-valued item in a list");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"pop",0,1,1,1,1,1,(void *)pplmethod_listPop, "pop()", "\\mathrm{pop}@<@0@>", "pop(n) removes the nth item from a list and returns it. If n is not specified, the last list item is popped");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"reduce",1,1,0,0,0,0,(void *)pplmethod_listReduce, "reduce(f)", "\\mathrm{reduce}@<@0}@>", "reduce(f) takes a pointer to a function of two arguments. It first calls f(a,b) on the first two elements of the list, and then continues through the list calling f(a,b) on the result and the next item in the list. The final result is returned");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"reverse",0,0,1,1,1,1,(void *)pplmethod_listReverse, "reverse()", "\\mathrm{reverse}@<@>", "reverse() reverses the order of the members of a list");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"sort",0,0,1,1,1,1,(void *)pplmethod_listSort, "sort()", "\\mathrm{sort}@<@>", "sort() sorts the members of a list");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"sortOn",1,1,0,0,0,0,(void *)pplmethod_listSortOnCustom, "sortOn(f)", "\\mathrm{sortOn}@<@0@>", "sortOn(f) sorts the members of a list using the user-supplied function f(a,b) to determine the sort order. f should return 1, 0 or -1 depending whether a>b, a==b or a<b");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"sortOnElement",1,1,1,1,1,1,(void *)pplmethod_listSortOnElement, "sortOnElement(n)", "\\mathrm{sortOnElement}@<@1@>", "sortOnElement(n) sorts a list of lists on the nth element of each sublist");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_LIST],"vector",0,0,1,1,1,1,(void *)pplmethod_listVector, "vector()", "\\mathrm{vector}@<@>", "vector() turns a list into a vector, providing all the list elements are numbers with the same physical dimensions");

  // Dictionary methods
  for (i=0 ;i<3; i++)
   {
    int pplobj;
    if      (i==0) pplobj = PPLOBJ_DICT;
    else if (i==1) pplobj = PPLOBJ_MOD;
    else           pplobj = PPLOBJ_USER;

    ppl_addSystemMethod(pplObjMethods[pplobj],"delete",1,1,0,0,0,0,(void *)pplmethod_dictDelete, "delete(x)", "\\mathrm{delete}@<@1@>", "delete(x) deletes the key x from the dictionary, if it exists");
    ppl_addSystemMethod(pplObjMethods[pplobj],"hasKey",1,1,0,0,0,0,(void *)pplmethod_dictHasKey, "hasKey(x)", "\\mathrm{hasKey}@<@1@>", "hasKey(x) returns a boolean indicating whether the key x exists in the dictionary");
    ppl_addSystemMethod(pplObjMethods[pplobj],"items" ,0,0,1,1,1,1,(void *)pplmethod_dictItems , "items()", "\\mathrm{items}@<@>", "items() returns a list of the [key,value] pairs in a dictionary");
    ppl_addSystemMethod(pplObjMethods[pplobj],"keys"  ,0,0,1,1,1,1,(void *)pplmethod_dictKeys  , "keys()", "\\mathrm{keys}@<@>", "keys() returns a list of the keys defined in a dictionary");
    ppl_addSystemMethod(pplObjMethods[pplobj],"len"   ,0,0,1,1,1,1,(void *)pplmethod_dictLen   , "len()", "\\mathrm{len}@<@>", "len() returns the number of entries in a dictionary");
    ppl_addSystemMethod(pplObjMethods[pplobj],"values",0,0,1,1,1,1,(void *)pplmethod_dictValues, "values()", "\\mathrm{values}@<@>", "values() returns a list of the values in a dictionary");
   }

  // Matrix methods
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_MAT],"det" ,0,0,1,1,1,1,(void *)pplmethod_matrixDet,"det()", "\\mathrm{det}@<@>", "det() returns the determinant of a square matrix");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_MAT],"diagonal",0,0,1,1,1,1,(void *)pplmethod_matrixDiagonal,"diagonal()", "\\mathrm{diagonal}@<@>", "diagonal() returns a boolean indicating whether a matrix is diagonal");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_MAT],"eigenvalues",0,0,1,1,1,1,(void *)pplmethod_matrixEigenvalues,"eigenvalues()", "\\mathrm{eigenvalues}@<@>", "eigenvalues() returns a vector containing the eigenvalues of a square symmetric matrix");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_MAT],"eigenvectors",0,0,1,1,1,1,(void *)pplmethod_matrixEigenvectors,"eigenvectors()", "\\mathrm{eigenvectors}@<@>", "eigenvectors() returns a list of the eigenvectors of a square symmetric matrix");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_MAT],"inv" ,0,0,1,1,1,1,(void *)pplmethod_matrixInv,"inv()", "\\mathrm{inv}@<@>", "inv() returns the inverse of a square matrix");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_MAT],"size",0,0,1,1,1,1,(void *)pplmethod_matrixSize, "size()", "\\mathrm{size}@<@>", "size() returns the dimensions of a matrix");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_MAT],"symmetric",0,0,1,1,1,1,(void *)pplmethod_matrixSymmetric, "symmetric()", "\\mathrm{symmetric}@<@>", "symmetric() returns a boolean indicating whether a matrix is symmetric");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_MAT],"transpose",0,0,1,1,1,1,(void *)pplmethod_matrixTrans, "transpose()", "\\mathrm{transpose}@<@>", "transpose() returns the transpose of a matrix");

  // File methods
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"close",0,0,1,1,1,1,(void *)pplmethod_fileClose,"close()", "\\mathrm{close}@<@>", "close() closes a file handle");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"dump",1,1,0,0,0,0,(void *)pplmethod_fileDump,"dump()", "\\mathrm{dump@<@0@>", "dump(x) writes a typeable ASCII representation of the object x to the file handle");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"eof",0,0,1,1,1,1,(void *)pplmethod_fileEof,"eof()", "\\mathrm{eof}@<@>", "eof() returns a boolean flag to indicate whether the end of a file has been reached");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"flush",0,0,1,1,1,1,(void *)pplmethod_fileFlush,"flush()", "\\mathrm{flush}@<@>", "flush() flushes any buffered data which has not yet physically been written to a file");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"getPos",0,0,1,1,1,1,(void *)pplmethod_fileGetpos,"getPos()", "\\mathrm{getPos}@<@>", "getPos() returns a file handle's current position in a file");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"isOpen",0,0,1,1,1,1,(void *)pplmethod_fileIsOpen,"isOpen()", "\\mathrm{isOpen}@<@>", "isOpen() returns a boolean flag indicating whether a file is open");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"read",0,0,1,1,1,1,(void *)pplmethod_fileRead,"read()", "\\mathrm{read}@<@>", "read() returns the contents of a file as a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"readline",0,0,1,1,1,1,(void *)pplmethod_fileReadline,"readline()", "\\mathrm{readline}@<@>", "readline() returns a single line of a file as a string");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"readlines",0,0,1,1,1,1,(void *)pplmethod_fileReadlines,"readlines()", "\\mathrm{readlines}@<@>", "readlines() returns the lines of a file as a list of strings");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"setPos",1,1,1,1,1,1,(void *)pplmethod_fileSetpos,"setPos(x)", "\\mathrm{setPos}@<@0@>", "setPos(x) sets a file handle's current position in a file");
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_FILE],"write",1,1,0,0,0,0,(void *)pplmethod_fileWrite,"write(x)", "\\mathrm{write}@<@0@>", "write(x) writes the string x to a file");

  // Exception methods
  ppl_addSystemMethod(pplObjMethods[PPLOBJ_EXC],"raise",1,1,0,0,0,0,(void *)pplmethod_excRaise,"raise(s)", "\\mathrm{raise}@<@0@>", "raise(s) raises an exception with error string s");

  return;
 }

