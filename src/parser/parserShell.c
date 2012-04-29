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
#include "commands/eqnsolve.h"
#include "commands/fft.h"
#include "commands/flowctrl.h"
#include "commands/funcset.h"
#include "commands/histogram.h"
#include "commands/interpolate.h"
#include "commands/set.h"
#include "commands/show.h"
#include "commands/help.h"

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"

#include "epsMaker/canvasDraw.h"

#include "settings/arrows_fns.h"
#include "settings/axes_fns.h"
#include "settings/labels_fns.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"

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

#include "canvasItems.h"
#include "children.h"
#include "pplConstants.h"

#define TBADD(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

#define STACK_POP \
   { \
    c->stackPtr--; \
    if (c->stack[c->stackPtr].objType!=PPLOBJ_NUM) /* optimisation: Don't waste time garbage collecting numbers */ \
     { \
      ppl_garbageObject(&c->stack[c->stackPtr]); \
      if (c->stack[c->stackPtr].refCount != 0) { strcpy(c->errStat.errBuff,"Stack forward reference detected."); TBADD(ERR_INTERNAL,0); return; } \
    } \
   } \

void ppl_parserShell(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj *stk = in->stk;
  char *d = (char *)stk[PARSE_arc_directive].auxil;

  // If directive is an expression, evaluate it and print result, unless it's an assignment
  if (stk[PARSE_arc_directive].objType == PPLOBJ_EXP)
   {
    int       lastOpAssign=0;
    const int stkLevelOld = c->stackPtr;
    pplExpr *expr = (pplExpr *)stk[PARSE_arc_directive].auxil;
    pplObj  *val  = ppl_expEval(c, expr, &lastOpAssign, 1, iterDepth+1);
    if (c->errStat.status) return; // On error, stop
    if (!lastOpAssign) { pplObjPrint(c,val,NULL,c->errcontext.tempErrStr,LSTR_LENGTH,0,0); ppl_report(&c->errcontext, NULL); }
    while (c->stackPtr>stkLevelOld) { STACK_POP; }
    return;
   }

  // If directive is not a string, something has gone wrong
  if (stk[PARSE_arc_directive].objType != PPLOBJ_STR) { ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, "directive type not a string"); return; }

  // Directive is a string
  if      (strcmp(d, "var_set")==0)
    ppl_directive_varset(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "func_set")==0)
    ppl_directive_funcset(c,pl,in,interactive);
  else if ((strcmp(d, "arrow")==0)||(strcmp(d,"line")==0))
    ppl_directive_arrow(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "assert")==0)
    ppl_directive_assert(c,pl,in);
  else if (strcmp(d, "box")==0)
    ppl_directive_box(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "break")==0)
    ppl_directive_break(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "call")==0)
    { } // Work already done in evaluating the called object!
  else if (strcmp(d, "cd")==0)
    ppl_directive_cd(c,pl,in);
  else if ((strcmp(d, "circle")==0)||(strcmp(d, "arc")==0))
    ppl_directive_circle(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "clear")==0)
    { ppl_directive_clear(c,pl,in,interactive); pplcsp_sendCommand(c,"A\n"); }
  else if (strcmp(d, "continue")==0)
    ppl_directive_continue(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "delete")==0)
    ppl_directive_delete(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "do")==0)
    ppl_directive_do(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "ellipse")==0)
    ppl_directive_ellipse(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "eps")==0)
    ppl_directive_eps(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "exec")==0)
    ppl_directive_load(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "for")==0)
    ppl_directive_for(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "foreach")==0)
    ppl_directive_foreach(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "foreachdatum")==0)
    ppl_directive_fordata(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "global")==0)
    ppl_directive_global(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "help")==0)
    ppl_directive_help(c,pl,in,interactive);
  else if (strcmp(d, "history")==0)
    ppl_directive_history(c,pl,in);
  else if (strcmp(d, "if")==0)
    ppl_directive_if(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "list")==0)
    ppl_directive_list(c,pl,in,interactive);
  else if (strcmp(d, "load")==0)
    ppl_directive_load(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "local")==0)
    ppl_directive_local(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "maximise")==0)
    ppl_directive_maximise(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "minimise")==0)
    ppl_directive_minimise(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "move")==0)
    ppl_directive_move(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "pling")==0)
   {
    if (system((char *)stk[PARSE_pling_cmd].auxil)) { if (DEBUG) ppl_log(&c->errcontext, "Pling command received non-zero return value."); }
    return;
   }
  else if (strcmp(d, "point")==0)
    ppl_directive_point(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "polygon")==0)
    ppl_directive_polygon(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "print")==0)
    ppl_directive_print(c,pl,in);
  else if (strcmp(d, "pwd")==0)
    ppl_report(&c->errcontext, c->errcontext.session_default.cwd);
  else if (strcmp(d, "quit")==0)
    c->shellExiting = 1;
  else if (strcmp(d, "refresh")==0)
   {
    if (c->set->term_current.display == SW_ONOFF_ON)
     {
      unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
      ppl_canvas_draw(c, unsuccessful_ops, iterDepth);
     }
   }
  else if (strcmp(d, "reset")==0)
   {
    int i;
    c->set->term_current  = c->set->term_default;
    c->set->graph_current = c->set->graph_default;

    for (i=0; i<PALETTE_LENGTH; i++)
     {
      c->set->palette_current [i] = c->set->palette_default [i];
      c->set->paletteS_current[i] = c->set->paletteS_default[i];
      c->set->palette1_current[i] = c->set->palette1_default[i];
      c->set->palette2_current[i] = c->set->palette2_default[i];
      c->set->palette3_current[i] = c->set->palette3_default[i];
      c->set->palette4_current[i] = c->set->palette4_default[i];
     }
    for (i=0; i<MAX_AXES; i++) { pplaxis_destroy(c, &(c->set->XAxes[i]) ); pplaxis_copy(c, &(c->set->XAxes[i]), &(c->set->XAxesDefault[i]));
                                 pplaxis_destroy(c, &(c->set->YAxes[i]) ); pplaxis_copy(c, &(c->set->YAxes[i]), &(c->set->YAxesDefault[i]));
                                 pplaxis_destroy(c, &(c->set->ZAxes[i]) ); pplaxis_copy(c, &(c->set->ZAxes[i]), &(c->set->ZAxesDefault[i]));
                               }
    for (i=0; i<MAX_PLOTSTYLES; i++) { ppl_withWordsDestroy(c, &(c->set->plot_styles[i])); ppl_withWordsCpy(c, &(c->set->plot_styles[i]) , &(c->set->plot_styles_default[i])); }
    pplarrow_list_destroy(c, &c->set->pplarrow_list);
    pplarrow_list_copy   (c, &c->set->pplarrow_list, &c->set->pplarrow_list_default);
    ppllabel_list_destroy(c, &c->set->ppllabel_list);
    ppllabel_list_copy   (c, &c->set->ppllabel_list, &c->set->ppllabel_list_default);
    ppl_directive_clear(c, NULL, NULL, 0);
    pplcsp_sendCommand(c,"A\n");
   }
  else if (strcmp(d, "return")==0)
    ppl_directive_return(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "save")==0)
    ppl_directive_save(c,pl,in);
  else if (strcmp(d, "show")==0)
    ppl_directive_show(c,pl,in,interactive);
  else if (strcmp(d, "solve")==0)
    ppl_directive_solve(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "set")==0)
    ppl_directive_set(c,pl,in,interactive);
  else if (strcmp(d, "set_error")==0)
    ppl_directive_seterror(c,pl,in,interactive);
  else if (strcmp(d, "subroutine")==0)
    ppl_directive_subrt(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "swap")==0)
    ppl_directive_swap(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "text")==0)
    ppl_directive_text(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "undelete")==0)
    ppl_directive_undelete(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "unset")==0)
    ppl_directive_set(c,pl,in,interactive);
  else if (strcmp(d, "unset_error")==0)
    ppl_directive_unseterror(c,pl,in,interactive);
  else if (strcmp(d, "while")==0)
    ppl_directive_while(c,pl,in,interactive,iterDepth);
  else if (strcmp(d, "with")==0)
    ppl_directive_with(c,pl,in,interactive,iterDepth);
  else
   {
    snprintf(c->errStat.errBuff, LSTR_LENGTH, "Unimplemented command: %s", d);
    TBADD(ERR_INTERNAL,0);
   }

  return;
 }

