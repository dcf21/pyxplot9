// input.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
//               2010-2011 Zoltan Voros
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
  context->inputLineBuffer = (char *)malloc(LSTR_LENGTH);
  return (context->inputLineBuffer!=NULL);
 }

void ppl_interactiveSession(ppl_context *context)
 {
  int   linenumber = 1;
  sigset_t sigs;

  if (DEBUG) ppl_log(&context->errcontext,"Starting an interactive session.");

  if ((isatty(STDIN_FILENO) == 1) && (pplset_session_default.splash == SW_ONOFF_ON)) ppl_report(&context->errcontext,ppltxt_welcome);

  context->shellExiting = 0;
  while (!context->shellExiting)
   {
    // Set up SIGINT handler
    if (sigsetjmp(ppl_sigjmpToInteractive, 1) == 0)
     {
      ppl_sigjmpFromSigInt = &ppl_sigjmpToInteractive;

      pplcsp_checkForGvOutput(context);
      if (isatty(STDIN_FILENO) == 1)
       {
        ppl_error_setstreaminfo(&context->errcontext,-1, "");
#ifdef HAVE_READLINE
         {
          char *line_ptr;
          line_ptr = readline( (context->inputLineAddBuffer == NULL) ? "pyxplot> " : ".......> ");
          if (line_ptr==NULL) break;
          add_history(line_ptr);
          context->historyNLinesWritten++;
          strncpy(context->inputLineBuffer, line_ptr, LSTR_LENGTH-1);
          context->inputLineBuffer[LSTR_LENGTH-1]='\0';
          free(line_ptr);
         }
#else
        printf("%s",prompt);
        if (fgets(context->inputLineBuffer,LSTR_LENGTH-1,stdin)==NULL) break;
        context->inputLineBuffer[LSTR_LENGTH-1]='\0';
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
      ppl_processLine(context, context->inputLineBuffer, isatty(STDIN_FILENO), 0, 0);
      ppl_error_setstreaminfo(&context->errcontext, -1, "");
      pplcsp_killAllHelpers(context);
     } else {
      sigemptyset(&sigs); // SIGINT longjmps return here
      sigaddset(&sigs,SIGCHLD);
      sigprocmask(SIG_UNBLOCK, &sigs, NULL);
      fprintf(stdout,"\n");
      if (context->inputLineAddBuffer != NULL) { free(context->inputLineAddBuffer); context->inputLineAddBuffer=NULL; }
      if (chdir(pplset_session_default.cwd) < 0) { ppl_fatal(&context->errcontext, __FILE__,__LINE__,"chdir into cwd failed."); } // chdir out of temporary directory
     }
   }

  ppl_sigjmpFromSigInt = &ppl_sigjmpToMain; // SIGINT now drops back through to main().
  if (context->inputLineAddBuffer != NULL) { free(context->inputLineAddBuffer); context->inputLineAddBuffer=NULL; }
  context->shellExiting = 0;
  if (isatty(STDIN_FILENO) == 1)
   {
    if (pplset_session_default.splash == SW_ONOFF_ON) ppl_report(&context->errcontext,"\nGoodbye. Have a nice day.");
    else                                              ppl_report(&context->errcontext,""); // Make a new line
   }
  return;
 }

void ppl_processScript(ppl_context *context, char *input, int iterLevel)
 {
  int  linenumber = 1;
  int  status;
  int  ProcessedALine = 0;
  char full_filename[FNAME_LENGTH];
  char filename_description[FNAME_LENGTH];
  FILE *infile;

  if (DEBUG) { sprintf(context->errcontext.tempErrStr, "Processing input from the script file '%s'.", input); ppl_log(&context->errcontext, context->errcontext.tempErrStr); }
  ppl_unixExpandUserHomeDir(&context->errcontext, input, pplset_session_default.cwd, full_filename);
  sprintf(filename_description, "file '%s'", input);
  if ((infile=fopen(full_filename,"r")) == NULL)
   {
    sprintf(context->errcontext.tempErrStr, "Could not find command file '%s'. Skipping on to next command file.", full_filename); ppl_error(&context->errcontext, ERR_FILE, -1, -1, context->errcontext.tempErrStr);
    return;
   }

  context->shellExiting = 0;
  while (!context->shellExiting)
   {
    ppl_error_setstreaminfo(&context->errcontext, linenumber, filename_description);
    if ((feof(infile)) || (ferror(infile))) break;
    ppl_file_readline(infile, context->inputLineBuffer, LSTR_LENGTH);
    linenumber++;
    status = ppl_processLine(context, context->inputLineBuffer, 0, iterLevel, !ProcessedALine);
    ppl_error_setstreaminfo(&context->errcontext, -1, "");
    pplcsp_killAllHelpers(context);
    if (status>0) // If an error occurs on the first line of a script, aborted processing it
     {
      ppl_error(&context->errcontext, ERR_FILE, -1, -1, "Error on first line of command file.  Is this a valid script?  Aborting.");
      if (context->inputLineAddBuffer != NULL) { free(context->inputLineAddBuffer); context->inputLineAddBuffer=NULL; }
      break;
     }
    if (status>=0) ProcessedALine = 1;
   }
  context->shellExiting = 0;
  if (context->inputLineAddBuffer != NULL) { free(context->inputLineAddBuffer); context->inputLineAddBuffer=NULL; }
  fclose(infile);
  pplcsp_checkForGvOutput(context);
  return;
 }

int ppl_processLine(ppl_context *context, char *in, int interactive, int iterLevel, int exitOnError)
 {
  int   i, status=0;
  char *inputLineBuffer = NULL;
  char  quoteChar;

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
  for (i=0 , quoteChar='\0'; inputLineBuffer[i]!='\0'; i++) \
   { \
    if      ((quoteChar=='\0') && (inputLineBuffer[i]=='\'')                                ) quoteChar = '\''; \
    else if ((quoteChar=='\0') && (inputLineBuffer[i]=='\"')                                ) quoteChar = '\"'; \
    else if ((quoteChar=='\'') && (inputLineBuffer[i]=='\'') && (inputLineBuffer[i-1]!='\\')) quoteChar = '\0'; \
    else if ((quoteChar=='\"') && (inputLineBuffer[i]=='\"') && (inputLineBuffer[i-1]!='\\')) quoteChar = '\0'; \
    else if ((quoteChar=='\0') && (inputLineBuffer[i]=='`' )                                ) quoteChar = '`';  \
    else if ((quoteChar=='`' ) && (inputLineBuffer[i]=='`' ) && (inputLineBuffer[i-1]!='\\')) quoteChar = '\0';

#define LOOP_END }

  // Cut comments off the ends of lines
LOOP_OVER_LINE
  else if ((quoteChar=='\0') && (inputLineBuffer[i]=='#' )) break;
LOOP_END
  inputLineBuffer[i] = '\0';

  // Loop over semicolon-separated line segments
LOOP_OVER_LINE
  else if ((quoteChar=='\0') && (inputLineBuffer[i]==';' )                     )
   {
    inputLineBuffer[i]='\0';
    status = ppl_ProcessStatement(context, inputLineBuffer);
    if ((status) && (exitOnError)) break;
    inputLineBuffer = inputLineBuffer+i+1;
    i=0;
   }
LOOP_END

  if ((!status) || (!exitOnError)) status = ppl_ProcessStatement(context, inputLineBuffer);
  if (context->inputLineAddBuffer != NULL) free(context->inputLineAddBuffer);
  context->inputLineAddBuffer = NULL;
  return (status!=0);
 }

#include "expressions/expCompile.h"

int ppl_ProcessStatement(ppl_context *context, char *line)
 {
  int end, errPos, bufflen=65536, tmplen=65536;
  char errText[LSTR_LENGTH];
  unsigned char buff[65536], tmp[65536];

  ppl_expCompile(context,line,&end,0,(void *)buff,&bufflen,(void *)tmp,tmplen,&errPos,errText);
  if (errPos>=0)
   {
    printf("%d:%s\n",errPos,errText);
    return 1;
   }

  // Print tokens
  ppl_tokenPrint(context, line, tmp, end);

  // Print bytecode
  ppl_reversePolishPrint(context, buff, context->errcontext.tempErrStr);
  ppl_report(&context->errcontext, context->errcontext.tempErrStr);

  return 0;
 }

