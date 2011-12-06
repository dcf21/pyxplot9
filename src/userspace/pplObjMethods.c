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

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjMethods.h"
#include "userspace/pplObjUnits.h"

dict **pplObjMethods;

void pplObjMethodsInit(ppl_context *c)
 {
  int i;
  const int n = PPLOBJ_USER+1;
  pplObjMethods = (dict **)malloc(n * sizeof(dict *));
  if (pplObjMethods==NULL) ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Out of memory.");
  for (i=0; i<n; i++)
   {
    pplObjMethods[i] = ppl_dictInit(HASHSIZE_LARGE,1);
    if (pplObjMethods[i]==NULL) ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Out of memory.");
   }
  return;
 }

