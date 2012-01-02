// contextVarDef.c
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

#define _CONTEXT_C 1

#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"

#include "settings/settings_fns.h"

#include "stringTools/strConstants.h"

#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjUnits.h"

#include "defaultObjs/defaultUnits.h"
#include "defaultObjs/defaultVars.h"

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

