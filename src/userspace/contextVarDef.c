// contextVarDef.c
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

#define _CONTEXT_C 1

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"

#include "expressions/traceback_fns.h"

#include "settings/settings_fns.h"

#include "stringTools/strConstants.h"

#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjUnits.h"

#include "defaultObjs/defaultUnits.h"
#include "defaultObjs/defaultVars.h"

#define TBADD(et,pos) ppl_tbAdd(c,srcLineN,srcId,srcFname,0,et,pos,linetxt,"")

void ppl_contextVarHierLookup(ppl_context *c, int srcLineN, int srcId, char *srcFname, char *linetxt, pplObj *stk, int *stkPos, pplObj **out, int base, int offset)
 {
  int     first=1;
  int     pos=base;

  while (stk[pos].objType == PPLOBJ_NUM)
   {
    int   last;
    char *name;
    pos  = (int)round(stk[pos].real);
    if (pos<=0) break;
    last = (stk[pos].objType!=PPLOBJ_NUM);
    name = (char *)stk[pos+offset].auxil;
    if (first && !last)
     {
      ppl_contextVarLookup(c, name, out, 0);
      if (*out==NULL) { sprintf(c->errStat.errBuff,"No such variable, '%s'.",name); TBADD(ERR_NAMESPACE,stkPos[pos+offset]); return; }
      if (((*out)->objType!=PPLOBJ_DICT)&&((*out)->objType!=PPLOBJ_MOD)&&((*out)->objType!=PPLOBJ_USER)) { sprintf(c->errStat.errBuff,"Cannot reference members of object of type '%s'.",pplObjTypeNames[(*out)->objType]); TBADD(ERR_TYPE,stkPos[pos+offset]); return; }
     }
    else if (first)
     {
      pplObj tmp;
      ppl_contextGetVarPointer(c, name, out, &tmp);
      ppl_contextRestoreVarPointer(c, name, &tmp);
      if ((*out)->objType==PPLOBJ_GLOB)
       {
        int ns_ptr = c->ns_ptr;
        c->ns_ptr = 1;
        ppl_contextGetVarPointer(c, name, out, &tmp);
        ppl_contextRestoreVarPointer(c, name, &tmp);
        c->ns_ptr = ns_ptr;
       }
     }
    else if (!last)
     {
      *out = (pplObj *)ppl_dictLookup((dict *)(*out)->auxil, name);
      if ((*out)==NULL) { sprintf(c->errStat.errBuff,"No such variable, '%s'.",name); TBADD(ERR_NAMESPACE,stkPos[pos+offset]); return; }
      if (((*out)->objType!=PPLOBJ_DICT)&&((*out)->objType!=PPLOBJ_MOD)&&((*out)->objType!=PPLOBJ_USER)) { sprintf(c->errStat.errBuff,"Cannot reference members of object of type '%s'.",pplObjTypeNames[(*out)->objType]); TBADD(ERR_TYPE,stkPos[pos+offset]); return; }
     }
    else
     {
      dict *d = (dict *)(*out)->auxil;
      *out = (pplObj *)ppl_dictLookup((dict *)(*out)->auxil, name);
      if ((*out)==NULL)
       {
        pplObj tmp;
        tmp.refCount=1;
        pplObjNum(&tmp,1,1,0);
        ppl_dictAppendCpy(d, name, &tmp, sizeof(pplObj));
        *out = (pplObj *)ppl_dictLookup(d, name);
       }
     }
    if (((*out)!=NULL)&&((*out)->immutable)) { sprintf(c->errStat.errBuff,"Cannot %s '%s'.",last?"modify the immutable variable":"set variables in the immutable namespace",name); TBADD(ERR_NAMESPACE,stkPos[pos+offset]); return; }
    first=0;
   }
  if ((*out)==NULL) { sprintf(c->errStat.errBuff,"No such variable."); TBADD(ERR_NAMESPACE,0); return; }

  return;
 }

void ppl_contextVarLookup(ppl_context *c, char *name, pplObj **output, int returnGlobObjs)
 {
  int i;
  for (i=c->ns_ptr ; i>=0 ; i=(i>1)?1:i-1)
   {
    pplObj *obj = (pplObj *)ppl_dictLookup(c->namespaces[i] , name);
    if (obj==NULL) continue;
    if ((!returnGlobObjs)&&((obj->objType==PPLOBJ_GLOB)||(obj->objType==PPLOBJ_ZOM))) continue;
    *output = obj;
    return;
   }
  *output = NULL;
  return;
 }

void ppl_contextGetVarPointer(ppl_context *c, char *name, pplObj **output, pplObj *temp)
 {
  dict *d=c->namespaces[c->ns_ptr];
  *output = (pplObj *)ppl_dictLookup(d, name);
  if (*output!=NULL)
   {
    int om = (*output)->amMalloced;
    *temp = **output;
    pplObjNum(*output,om,1,0);
   }
  else // If variable is not defined, create it now
   {
    temp->refCount=1;
    pplObjNum(temp,1,1,0);
    ppl_dictAppendCpy(d, name, temp, sizeof(pplObj));
    *output = (pplObj *)ppl_dictLookup(d, name);
    pplObjZom(temp,1);
   }
 }

void ppl_contextRestoreVarPointer(ppl_context *c, char *name, pplObj *temp)
 {
  dict   *d   = c->namespaces[c->ns_ptr];
  pplObj *obj = (pplObj *)ppl_dictLookup(d, name);
  if (obj!=NULL)
   {
    int om = obj->amMalloced;
    int tm = temp->amMalloced;
    int rc = obj->refCount;
    obj->amMalloced = 0;
    ppl_garbageObject(obj);
    memcpy(obj,temp,sizeof(pplObj));
    obj->amMalloced = om;
    obj->refCount = rc;
    pplObjZom(temp,tm);
   }
  else
   {
    int tm = temp->amMalloced;
    temp->amMalloced = 1;
    temp->refCount = 1;
    ppl_dictAppendCpy(d, name, temp, sizeof(pplObj));
    pplObjZom(temp,tm);
   }
 }

