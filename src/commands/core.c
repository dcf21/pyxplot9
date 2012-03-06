// core.c
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

#define _CORE_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <glob.h>
#include <math.h>
#include <unistd.h>

#ifdef HAVE_READLINE
#include <readline/history.h>
#endif

#include <gsl/gsl_math.h>

#include "commands/core.h"

#include "coreUtils/backup.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/getPasswd.h"

#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"

#include "settings/settingTypes.h"
#include "settings/textConstants.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "expressions/expEval.h"
#include "expressions/traceback.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjPrint.h"

#include "pplConstants.h"

#define TBADD(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

void directive_assert(ppl_context *c, parserLine *pl, parserOutput *in)
 {
  pplObj *stk = in->stk;
  int     lt, gtreq;
  char   *txt, *version;
  pplObj *val;

  txt     = (stk[PARSE_assert_message].objType==PPLOBJ_STR) ? (char *)stk[PARSE_assert_message].auxil : NULL;
  version = (stk[PARSE_assert_version].objType==PPLOBJ_STR) ? (char *)stk[PARSE_assert_version].auxil : NULL;
  val     = &stk[PARSE_assert_expr];
  lt      = (stk[PARSE_assert_lt     ].objType==PPLOBJ_STR);
  gtreq   = (stk[PARSE_assert_gtreq  ].objType==PPLOBJ_STR);

  if (val->objType != PPLOBJ_ZOM)
   {
    ppl_context *context = c;
    CAST_TO_BOOL(val);
    if (txt==NULL) txt="Assertion was not true.";
    if (val->real<0.5)
     {
      snprintf(c->errStat.errBuff, LSTR_LENGTH, "%s", txt);
      TBADD(ERR_ASSERT,in->stkCharPos[PARSE_assert_expr]);
      return;
     }
   }

  if (version != NULL)
   {
    int i=0,j=0,sgn,pass=0;
    char txtauto[64];
    sprintf(txtauto, "This script requires a%s version of PyXPlot (%s %s)", (!lt)?" newer":"n older", (!lt)?">=":"<", version);
#define VERSION_FAIL { snprintf(c->errStat.errBuff, LSTR_LENGTH, "%s", txt); TBADD(ERR_ASSERT,in->stkCharPos[PARSE_assert_version]); return; }
#define VERSION_MALF { snprintf(c->errStat.errBuff, LSTR_LENGTH, "Malformed version string."); TBADD(ERR_SYNTAX,in->stkCharPos[PARSE_assert_version]); return; }
    if (txt==NULL) txt=txtauto;
    sgn = (!lt)?1:-1;
    while ((version[i]>'\0')&&(version[i]<=' ')) i++;
    if (version[i]=='\0') return;
    for (j=0; ((version[i]>='0')&&(version[i]<='9')); i++) { j=10*j+(version[i]-'0'); if (j>1e6)j=1e6; }
    if ((sgn*VERSION_MAJ < sgn*j)&&(!pass)) VERSION_FAIL;
    if  (sgn*VERSION_MAJ > sgn*j) pass=1;
    while ((version[i]>'\0')&&(version[i]<=' ')) i++;
    if      (version[i]=='.') i++;
    else if (version[i]!='\0') { VERSION_MALF; }
    while ((version[i]>'\0')&&(version[i]<=' ')) i++;
    if ((version[i]=='\0')&&(sgn==-1)&&(!pass)) VERSION_FAIL;
    if (version[i]=='\0') return;
    for (j=0; ((version[i]>='0')&&(version[i]<='9')); i++) { j=10*j+(version[i]-'0'); if (j>1e6)j=1e6; }
    if ((sgn*VERSION_MIN < sgn*j)&&(!pass)) VERSION_FAIL;
    if  (sgn*VERSION_MIN > sgn*j) pass=1;
    while ((version[i]>'\0')&&(version[i]<=' ')) i++;
    if      (version[i]=='.') i++;
    else if (version[i]!='\0') { VERSION_MALF; }
    while ((version[i]>'\0')&&(version[i]<=' ')) i++;
    if ((version[i]=='\0')&&(sgn==-1)&&(!pass)) VERSION_FAIL;
    if (version[i]=='\0') return;
    for (j=0; ((version[i]>='0')&&(version[i]<='9')); i++) { j=10*j+(version[i]-'0'); if (j>1e6)j=1e6; }
    if ((sgn*VERSION_REV < sgn*j)&&(!pass)) VERSION_FAIL;
    if  (sgn*VERSION_REV > sgn*j) pass=1;
    while ((version[i]>'\0')&&(version[i]<=' ')) i++;
    if (version[i]!='\0') { VERSION_MALF; }
    if ((version[i]=='\0')&&(sgn==-1)&&(!pass)) VERSION_FAIL;
   }

  return;
 }

void directive_cd(ppl_context *c, parserLine *pl, parserOutput *in)
 {
  pplObj *stk = in->stk;
  int     pos = PARSE_cd_path;
  wordexp_t     wordExp;
  glob_t        globData;

  while (stk[pos].objType == PPLOBJ_NUM)
   {
    char *dirName;
    pos = (int)round(stk[pos].real);
    if (pos<=0) break;
    dirName = (char *)stk[pos+PARSE_cd_directory_path].auxil;

    if ((wordexp(dirName, &wordExp, 0) != 0) || (wordExp.we_wordc <= 0)) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "Could not enter directory '%s'.", dirName); TBADD(ERR_FILE,in->stkCharPos[pos+PARSE_cd_directory_path]); return; }
    if ((glob(wordExp.we_wordv[0], 0, NULL, &globData) != 0) || (globData.gl_pathc <= 0)) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "Could not enter directory '%s'.", dirName); TBADD(ERR_FILE,in->stkCharPos[pos+PARSE_cd_directory_path]); wordfree(&wordExp); return; }
    wordfree(&wordExp);
    if (chdir(globData.gl_pathv[0]) < 0)
     {
      snprintf(c->errStat.errBuff, LSTR_LENGTH, "Could not change into directory '%s'.", globData.gl_pathv[0]);
      TBADD(ERR_FILE,in->stkCharPos[pos+PARSE_cd_directory_path]);
      globfree(&globData);
      break;
     }
    globfree(&globData);
   }
  if (getcwd( c->errcontext.session_default.cwd , FNAME_LENGTH ) == NULL) { ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Could not read current working directory."); } // Store cwd
  return;
 }

void directive_history(ppl_context *c, parserLine *pl, parserOutput *in)
 {
#ifdef HAVE_READLINE
  pplObj *stk = in->stk;
  int start=0,endpos,k;
  HIST_ENTRY **history_data;

  endpos       = where_history();
  history_data = history_list();

  if (stk[PARSE_history_number_lines].objType == PPLOBJ_NUM)
   {
    start = endpos - (int)round(stk[PARSE_history_number_lines].real);
    if (start < 0) start=0;
   }

  for (k=start; k<endpos; k++) ppl_report(&c->errcontext,history_data[k]->line);
  return;
#else
  snprintf(c->errStat.errBuff, LSTR_LENGTH, "The 'history' command is not available as the GNU readline library was not linked to when PyXPlot was installed.");
  TBADD(ERR_GENERAL,0);
  return;
#endif
 }

void directive_print(ppl_context *c, parserLine *pl, parserOutput *in)
 {
  pplObj *stk = in->stk;
  int     pos = PARSE_print_0print_list;
  int     i   = 0;
  c->errcontext.tempErrStr[i] = '\0';

  while (stk[pos].objType == PPLOBJ_NUM)
   {
    pos = (int)round(stk[pos].real);
    if (pos<=0) break;
    pplObjPrint(c,stk+pos+PARSE_print_expression_0print_list,NULL,c->errcontext.tempErrStr+i,LSTR_LENGTH-i,0,0);
    c->errcontext.tempErrStr[LSTR_LENGTH-1] = '\0';
    i += strlen(c->errcontext.tempErrStr+i);
   }
  ppl_report(&c->errcontext, NULL);
  return;
 }

void directive_save(ppl_context *c, parserLine *pl, parserOutput *in)
 {
#ifdef HAVE_READLINE
  pplObj *stk = in->stk;
  int pos=PARSE_save_filename;
  int start=0,endpos,k;
  long x;
  char *outfname = "";
  FILE *outfile = NULL;
  HIST_ENTRY **history_data;

  if (stk[pos].objType == PPLOBJ_STR)
   {
    outfname = (char *)stk[pos].auxil;
    ppl_createBackupIfRequired(c, outfname);
    outfile = fopen(outfname , "w");
   }
  if (outfile == NULL) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "The save command could not open output file '%s' for writing.", outfname); TBADD(ERR_FILE,in->stkCharPos[pos]); return; }
  fprintf(outfile, "# Command script saved by PyXPlot %s\n# Timestamp: %s\n", VERSION, ppl_strStrip(ppl_friendlyTimestring(),c->errcontext.tempErrStr));
  fprintf(outfile, "# User: %s\n\n", ppl_unixGetIRLName(&c->errcontext));

  endpos       = history_length;
  history_data = history_list();

  x = endpos - c->historyNLinesWritten;
  if (x < history_base) x=history_base;
  start = (int)x;

  for (k=start; k<endpos-1; k++) { fprintf(outfile, "%s\n", (history_data[k]->line)); }
  fclose(outfile);
  return;
#else
  snprintf(c->errStat.errBuff, LSTR_LENGTH, "The 'save' command is not available as the GNU readline library was not linked to when PyXPlot was installed.");
  TBADD(ERR_GENERAL,0);
  return;
#endif
 }

void directive_seterror(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj *o = &in->stk[PARSE_set_error_set_option];
  char *tempstr = (o->objType==PPLOBJ_STR) ? (char *)in->stk[PARSE_set_error_set_option].auxil : NULL;
  if (tempstr != NULL)
   {
    if (!interactive) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "Unrecognised set option '%s'.", tempstr); TBADD(ERR_SYNTAX,in->stkCharPos[PARSE_set_error_set_option]); }
    else              { snprintf(c->errStat.errBuff, LSTR_LENGTH, ppltxt_set                     , tempstr); TBADD(ERR_SYNTAX,in->stkCharPos[PARSE_set_error_set_option]); }
   }
  else
   {
    if (!interactive) { snprintf(c->errStat.errBuff, LSTR_LENGTH,  "set command detected with no set option following it."); TBADD(ERR_SYNTAX,0); }
    else              { snprintf(c->errStat.errBuff, LSTR_LENGTH, "%s", ppltxt_set_noword); TBADD(ERR_SYNTAX,0); }
   }
  return;
 }

void directive_unseterror(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj *o = &in->stk[PARSE_set_error_set_option];
  char *tempstr = (o->objType==PPLOBJ_STR) ? (char *)in->stk[PARSE_set_error_set_option].auxil : NULL;
  if (tempstr != NULL)
   { 
    if (!interactive) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "Unrecognised set option '%s'.", tempstr); TBADD(ERR_SYNTAX,in->stkCharPos[PARSE_set_error_set_option]); }
    else              { snprintf(c->errStat.errBuff, LSTR_LENGTH, ppltxt_unset                   , tempstr); TBADD(ERR_SYNTAX,in->stkCharPos[PARSE_set_error_set_option]); }
   }
  else
   {
    if (!interactive) { snprintf(c->errStat.errBuff, LSTR_LENGTH,  "unset command detected with no set option following it."); TBADD(ERR_SYNTAX,0); }
    else              { snprintf(c->errStat.errBuff, LSTR_LENGTH, "%s", ppltxt_unset_noword); TBADD(ERR_SYNTAX,0); }
   }
  return;
 }

