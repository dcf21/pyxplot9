// parserShell.c
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

#define _PARSERSHELL_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands/core.h"
#include "commands/show.h"
#include "commands/help.h"

#include "coreUtils/errorReport.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjPrint.h"

#include "pplConstants.h"

#define TBADD(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt)

void ppl_parserShell(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj *stk = in->stk;
  char *d = (char *)stk[PARSE_arc_directive].auxil;

  // If directive is an expression, evaluate it and print result, unless it's an assignment
  if (stk[PARSE_arc_directive].objType == PPLOBJ_EXP)
   {
    int lastOpAssign=0;
    pplExpr *expr = (pplExpr *)stk[PARSE_arc_directive].auxil;
    pplObj *val = ppl_expEval(c, expr, &lastOpAssign, 1, iterDepth+1);
    if (c->errStat.status) return; // On error, stop
    if (!lastOpAssign) { pplObjPrint(c,val,NULL,c->errcontext.tempErrStr,LSTR_LENGTH,0,0); ppl_report(&c->errcontext, NULL); }
    ppl_garbageObject(val);
    return;
   }

  // If directive is not a string, something has gone wrong
  if (stk[PARSE_arc_directive].objType != PPLOBJ_STR) { ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, "directive type not a string"); return; }

  // Directive is a string
  if      (strcmp(d, "var_set")==0)
   {
    int i;
    char *varname = (char *)stk[PARSE_var_set_varname].auxil;
    pplObj *val = &stk[PARSE_var_set_value];
    pplObj  out; pplObjZom(&out,1); out.refCount=1; // Create a temporary zombie for now

    for (i=c->ns_ptr ; ; i=1)
     {
      int om, rc;
      pplObj *obj = (pplObj *)ppl_dictLookup(c->namespaces[i] , varname);
      if ((obj==NULL)&&(!c->namespaces[i]->immutable))
       {
        ppl_dictAppendCpy(c->namespaces[i] , varname , (void *)&out , sizeof(pplObj));
        obj = (pplObj *)ppl_dictLookup(c->namespaces[i] , varname);
        if (obj==NULL) { sprintf(c->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0); break; }
       }
      if ((obj==NULL)||((c->namespaces[i]->immutable)&&(obj->objType!=PPLOBJ_GLOB))) { sprintf(c->errStat.errBuff,"Cannot modify variable in immutable namespace."); TBADD(ERR_NAMESPACE,0); break; }
      if (obj->objType==PPLOBJ_GLOB) { if (i<2) { sprintf(c->errStat.errBuff,"Variable declared global in global namespace."); TBADD(ERR_NAMESPACE,0); break; } continue; }
      om = obj->amMalloced;
      rc = obj->refCount;
      obj->amMalloced = 0;
      obj->refCount = 1;
      ppl_garbageObject(obj);
      pplObjCpy(obj, val, 0, om, 1);
      obj->refCount = rc;
      break;
     }
   }
  else if (strcmp(d, "assert")==0)
    directive_assert(c,pl,in);
  else if (strcmp(d, "cd")==0)
    directive_cd(c,pl,in);
  else if (strcmp(d, "help")==0)
    directive_help(c,pl,in,interactive);
  else if (strcmp(d, "history")==0)
    directive_history(c,pl,in);
  else if (strcmp(d, "pling")==0)
   {
    if (system((char *)stk[PARSE_pling_cmd].auxil)) { if (DEBUG) ppl_log(&c->errcontext, "Pling command received non-zero return value."); }
    return;
   }
  else if (strcmp(d, "print")==0)
    directive_print(c,pl,in);
  else if (strcmp(d, "save")==0)
    directive_save(c,pl,in);
  else if (strcmp(d, "show")==0)
    directive_show(c,pl,in,interactive);
  else if (strcmp(d, "set_error")==0)
   directive_seterror(c,pl,in,interactive);
  else if (strcmp(d, "unset_error")==0)
   directive_unseterror(c,pl,in,interactive);
  else
   {
    snprintf(c->errStat.errBuff, LSTR_LENGTH, "Unimplemented command: %s", d);
    TBADD(ERR_INTERNAL,0);
   }

  return;
 }

