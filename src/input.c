// input.c
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

#define _INPUT_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "coreUtils/errorReport.h"
#include "coreUtils/getPasswd.h"

#include "expressions/traceback_fns.h"

#include "parser/parser.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/textConstants.h"

#include "userspace/context.h"

#include "children.h"
#include "input.h"
#include "pplConstants.h"
#include "pyxplot.h"

int ppl_inputInit(ppl_context *context)
 {
  context->inputLineBufferLen = LSTR_LENGTH;
  context->inputLineBuffer    = (char *)malloc( context->inputLineBufferLen );
  return (context->inputLineBuffer!=NULL);
 }

sigjmp_buf sigint_destination;
int        sigint_longjmp=0;

void ppl_interactiveSigInt(int signo)
 {
  if (sigint_longjmp) siglongjmp(sigint_destination, 1);
  fprintf(stdout,"\n");
  cancellationFlag = 1;
  return;
 }

void ppl_interactiveSession(ppl_context *context)
 {
  int           linenumber = 1;
  void        (*sigint_old)(int) = NULL;
  parserLine   *pl = NULL;
  parserStatus *ps = NULL;

  if (DEBUG) ppl_log(&context->errcontext,"Starting an interactive session.");

  ppl_parserStatInit(&ps,&pl);
  if ( (ps==NULL) || (context->inputLineBuffer == NULL) ) { ppl_error(&context->errcontext,ERR_MEMORY,-1,-1,"Out of memory."); return; }

  if ((isatty(STDIN_FILENO) == 1) && (context->errcontext.session_default.splash == SW_ONOFF_ON)) ppl_report(&context->errcontext,ppltxt_welcome);

  context->shellExiting = 0;
  sigint_old = signal(SIGINT, ppl_interactiveSigInt); // Set up SIGINT handler
  while ((!context->shellExiting)&&(!context->shellBroken)&&(!context->shellContinued)&&(!context->shellReturned))
   {
    pplcsp_checkForGvOutput(context);
    cancellationFlag = 0;
    if (isatty(STDIN_FILENO) == 1)
     {
      ppl_error_setstreaminfo(&context->errcontext,-1, "");
#ifdef HAVE_READLINE
       {
        int len;
        char *line_ptr, prompt[32];
        if (context->inputLineAddBuffer!=NULL) { strcpy(prompt,".......> "); }
        else                                   { snprintf(prompt,16,"%s.......",ps->prompt); strcpy(prompt+7, "> "); }

        if (sigsetjmp(sigint_destination,1)!=0) { printf("\n"); }
        sigint_longjmp=1;
        line_ptr = readline(prompt);
        sigint_longjmp=0;
        if (line_ptr==NULL) break;
        add_history(line_ptr);
        context->historyNLinesWritten++;
        len = strlen(line_ptr)+1;
        if (len > context->inputLineBufferLen) { context->inputLineBuffer = (char *)realloc(context->inputLineBuffer, len); context->inputLineBufferLen=len; }
        if (context->inputLineBuffer == NULL) break;
        strcpy(context->inputLineBuffer, line_ptr);
        free(line_ptr);
       }
#else
       {
        char prompt[32];
        if (sigsetjmp(sigint_destination,1)!=0) { printf("\n"); }
        sigint_longjmp=1;
        if (context->inputLineAddBuffer!=NULL) { strcpy(prompt,".......> "); }
        else                                   { snprintf(prompt,16,"%s.......",ps->prompt); strcpy(prompt+7, "> "); }
        printf("%s",prompt);
        if (fgets(context->inputLineBuffer,LSTR_LENGTH-1,stdin)==NULL) { break; }
        sigint_longjmp=0;
        context->inputLineBuffer[LSTR_LENGTH-1]='\0';
       }
#endif
     }
    else
     {
      ppl_error_setstreaminfo(&context->errcontext, linenumber, "piped input");
      if ((feof(stdin)) || (ferror(stdin))) break;
      ppl_file_readline(stdin, context->inputLineBuffer, LSTR_LENGTH-1);
      context->inputLineBuffer[LSTR_LENGTH-1]='\0';
      linenumber++;
     }
    if ((!cancellationFlag)&&(!context->shellExiting)&&(!context->shellBroken)&&(!context->shellContinued)&&(!context->shellReturned))
      { ppl_processLine(context, ps, context->inputLineBuffer, isatty(STDIN_FILENO), 0); }
    cancellationFlag = 0;
    ppl_error_setstreaminfo(&context->errcontext, -1, "");
    pplcsp_killAllHelpers(context);
   }

  if (context->inputLineAddBuffer != NULL) { free(context->inputLineAddBuffer); context->inputLineAddBuffer=NULL; }
  context->shellExiting = 0;
  signal(SIGINT, sigint_old); // Restore old SIGINT handler
  if (isatty(STDIN_FILENO) == 1)
   {
    if (context->errcontext.session_default.splash == SW_ONOFF_ON) ppl_report(&context->errcontext,"\nGoodbye. Have a nice day.");
    else                                              ppl_report(&context->errcontext,""); // Make a new line
   }
  ppl_parserStatFree(&ps);
  return;
 }

void ppl_processScript(ppl_context *context, char *input, int iterDepth)
 {
  int           linenumber = 1;
  int           status;
  char          full_filename[FNAME_LENGTH];
  char          filename_description[FNAME_LENGTH];
  FILE         *infile;
  parserLine   *pl = NULL;
  parserStatus *ps = NULL;
  int           shellBreakableOld  = context->shellBreakable;
  int           shellReturnableOld = context->shellReturnable;

  if (DEBUG) { sprintf(context->errcontext.tempErrStr, "Processing input from the script file '%s'.", input); ppl_log(&context->errcontext, NULL); }
  ppl_unixExpandUserHomeDir(&context->errcontext, input, context->errcontext.session_default.cwd, full_filename);
  sprintf(filename_description, "file '%s'", input);
  if ((infile=fopen(full_filename,"r")) == NULL)
   {
    sprintf(context->errcontext.tempErrStr, "Could not find command file '%s'. Skipping on to next command file.", full_filename); ppl_error(&context->errcontext, ERR_FILE, -1, -1, NULL);
    return;
   }

  ppl_parserStatInit(&ps,&pl);
  if ( (ps==NULL) || (context->inputLineBuffer == NULL) ) { ppl_error(&context->errcontext,ERR_MEMORY,-1,-1,"Out of memory."); return; }

  context->shellExiting    = 0;
  context->shellBreakable  = 0;
  context->shellReturnable = 0;
  while ((!context->shellExiting)&&(!context->shellBroken)&&(!context->shellContinued)&&(!context->shellReturned)&&(!cancellationFlag))
   {
    ppl_error_setstreaminfo(&context->errcontext, linenumber, filename_description);
    if ((feof(infile)) || (ferror(infile))) break;
    ppl_file_readline(infile, context->inputLineBuffer, context->inputLineBufferLen);
    linenumber++;
    status = ppl_processLine(context, ps, context->inputLineBuffer, 0, iterDepth);
    ppl_error_setstreaminfo(&context->errcontext, -1, "");
    pplcsp_killAllHelpers(context);
    if ( (status>0) || context->shellExiting || cancellationFlag )// If an error occurs in a script, aborted processing it
     {
      if (!context->shellExiting) ppl_error(&context->errcontext, ERR_FILE, -1, -1, "Aborting.");
      if (context->inputLineAddBuffer != NULL) { free(context->inputLineAddBuffer); context->inputLineAddBuffer=NULL; }
      break;
     }
   }
  if ((!context->shellExiting)&&(!context->shellBroken)&&(!context->shellContinued)&&(!context->shellReturned)&&(!cancellationFlag))
   if (context->inputLineAddBuffer != NULL) // Process last line of file if there is still text buffered
    {
     ppl_warning(&context->errcontext,ERR_SYNTAX,"Line continuation character (\\) at the end of command script, with no line following it.");
     ppl_error_setstreaminfo(&context->errcontext, linenumber, filename_description);
     status = ppl_processLine(context, ps, "", 0, iterDepth);
     ppl_error_setstreaminfo(&context->errcontext, -1, "");
     if (status>0) ppl_error(&context->errcontext, ERR_FILE, -1, -1, "Aborting.");
     if (context->inputLineAddBuffer != NULL) { free(context->inputLineAddBuffer); context->inputLineAddBuffer=NULL; }
    }
  context->shellExiting       = 0;
  context->shellBreakable     = shellBreakableOld;
  context->shellReturnable    = shellReturnableOld;
  context->inputLineAddBuffer = NULL;
  fclose(infile);
  ppl_parserStatFree(&ps);
  pplcsp_checkForGvOutput(context);
  return;
 }

int ppl_processLine(ppl_context *context, parserStatus *ps, char *in, int interactive, int iterDepth)
 {
  int   i, status=0;
  char *inputLineBuffer = NULL;

  // Find last character of presently entered line. If it's a \, buffer line and collect another
  for (i=0; in[i]!='\0'; i++); for (; ((i>0)&&(in[i]<=' ')); i--);
  if (in[i]=='\\')
   {
    if (context->inputLineAddBuffer==NULL)
     {
      context->inputLineAddBuffer = (char *)malloc(i+1);
      if (context->inputLineAddBuffer == NULL) { ppl_error(&context->errcontext, ERR_MEMORY, -1, -1, "Out of memory whilst trying to combine input lines."); return 1; }
      strncpy(context->inputLineAddBuffer, in, i);
      context->inputLineAddBuffer[i]='\0';
     } else {
      int j = strlen(context->inputLineAddBuffer);
      context->inputLineAddBuffer = (char *)realloc((void *)context->inputLineAddBuffer, j+i+1);
      if (context->inputLineAddBuffer == NULL) { ppl_error(&context->errcontext, ERR_MEMORY, -1, -1, "Out of memory whilst trying to combine input lines."); return 1; }
      strncpy(context->inputLineAddBuffer+j, in, i);
      context->inputLineAddBuffer[j+i]='\0';
     }
    return 0;
   }

  // Add previous backslashed lines to the beginning of this one
  if (context->inputLineAddBuffer!=NULL)
   {
    int j = strlen(context->inputLineAddBuffer);
    context->inputLineAddBuffer = (char *)realloc((void *)context->inputLineAddBuffer, j+i+2);
    if (context->inputLineAddBuffer == NULL) { ppl_error(&context->errcontext, ERR_MEMORY, -1, -1, "Out of memory whilst trying to combine input lines."); return 1; }
    strncpy(context->inputLineAddBuffer+j, in, i+1);
    context->inputLineAddBuffer[j+i+1]='\0';
    inputLineBuffer = context->inputLineAddBuffer;
   }
  else
   {
    inputLineBuffer = in;
   }

#define LOOP_OVER_LINE \
 { \
  int   bracketLevel=0; \
  char  quoteChar='\0'; \
  for (i=0; inputLineBuffer[i]!='\0'; i++) \
   { \
    if      ((quoteChar=='\0') && (inputLineBuffer[i]=='(')                                 ) bracketLevel++; \
    else if ((quoteChar=='\0') && (inputLineBuffer[i]==')')                                 ) bracketLevel--; \
    else if ((quoteChar=='\0') && (inputLineBuffer[i]=='\'')                                ) quoteChar = '\''; \
    else if ((quoteChar=='\0') && (inputLineBuffer[i]=='\"')                                ) quoteChar = '\"'; \
    else if ((quoteChar=='\'') && (inputLineBuffer[i]=='\'') && (inputLineBuffer[i-1]!='\\')) quoteChar = '\0'; \
    else if ((quoteChar=='\"') && (inputLineBuffer[i]=='\"') && (inputLineBuffer[i-1]!='\\')) quoteChar = '\0'; \
    else if ((quoteChar=='\0') && (inputLineBuffer[i]=='`' )                                ) quoteChar = '`';  \
    else if ((quoteChar=='`' ) && (inputLineBuffer[i]=='`' ) && (inputLineBuffer[i-1]!='\\')) quoteChar = '\0';

#define LOOP_END } }

  // Cut comments off the ends of lines
LOOP_OVER_LINE
  else if ((quoteChar=='\0') && (inputLineBuffer[i]=='#')) break;
LOOP_END
  inputLineBuffer[i] = '\0';

  // Loop over semicolon-separated line segments
LOOP_OVER_LINE
  else if ((quoteChar=='\0') && (bracketLevel==0) && (inputLineBuffer[i]==';'))
   {
    inputLineBuffer[i]='\0';
    status = ppl_ProcessStatement(context, ps, inputLineBuffer, interactive, iterDepth);
    if (status) break;
    inputLineBuffer = inputLineBuffer+i+1;
    i=-1;
   }
LOOP_END

  if (!status) status = ppl_ProcessStatement(context, ps, inputLineBuffer, interactive, iterDepth);
  if (context->inputLineAddBuffer != NULL) free(context->inputLineAddBuffer);
  context->inputLineAddBuffer = NULL;
  return (status!=0);
 }

int ppl_ProcessStatement(ppl_context *context, parserStatus *ps, char *line, int interactive, int iterDepth)
 {
  int stat=0;

  ppl_tbClear(context);

  // If line is blank, ignore it
  { int i=0,j=0; for (i=0; line[i]!='\0'; i++) if (line[i]>' ') { j=1; break; } if (j==0) return 0; }

  stat = ppl_parserCompile(context, ps, context->errcontext.error_input_linenumber, context->errcontext.error_input_sourceId, context->errcontext.error_input_filename, line, 0, iterDepth);

  if ( (!stat) && (!context->errStat.status) && (ps->blockDepth==0) )
   {
    ppl_parserExecute(context, ps->pl[iterDepth], NULL, interactive, iterDepth);
   }

  if ((iterDepth>0)&&(ps->blockDepth==0))
   {
    ppl_parserLineFree(ps->pl[iterDepth]);
    ps->pl[iterDepth]=NULL;
   }

  if (stat || context->errStat.status)
   {
    if (iterDepth==0)
     {
      ppl_tbWrite(context);
      ppl_parserStatReInit(ps);
     }
    return 1;
   }

  return 0;
 }

