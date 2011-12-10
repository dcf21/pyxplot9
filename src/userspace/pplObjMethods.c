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

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
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
    ppl_addSystemFunc(d,"methods" ,0,0,1,1,1,1,(void *)&pplmethod_methods , "methods()", "\\mathrm{methods}@<@>", "methods() returns a list of the methods of an object");
    ppl_addSystemFunc(d,"str"     ,0,0,1,1,1,1,(void *)&pplmethod_str     , "str()", "\\mathrm{str}@<@>", "str() returns a string representation of an object");
    ppl_addSystemFunc(d,"type"    ,0,0,1,1,1,1,(void *)&pplmethod_type    , "type()", "\\mathrm{type}@<@>", "type() returns the type of an object");
   }
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"beginswith",1,1,0,0,0,0,(void *)&pplmethod_strBeginsWith, "beginswith(x)", "\\mathrm{beginswith}@<@1@>", "beginswith(x) returns a boolean indicating whether a string begins with the substring x");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"endswith"  ,1,1,0,0,0,0,(void *)&pplmethod_strEndsWith  , "endswith(x)", "\\mathrm{endswith}@<@1@>", "endswith(x) returns a boolean indicating whether a string ends with the substring x");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"len"       ,0,0,1,1,1,1,(void *)&pplmethod_strLen       , "len()", "\\mathrm{len}@<@>", "len() returns the length of a string");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"lower"     ,0,0,1,1,1,1,(void *)&pplmethod_strLower     , "lower()", "\\mathrm{lower}@<@>", "lower() converts a string to lowercase");
  ppl_addSystemFunc(pplObjMethods[PPLOBJ_STR],"upper"     ,0,0,1,1,1,1,(void *)&pplmethod_strUpper     , "upper()", "\\mathrm{upper}@<@>", "upper() converts a string to uppercase");
  return;
 }

