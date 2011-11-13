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

#include "children.h"
#include "input.h"
#include "pplConstants.h"
#include "pyxplot.h"

char    *pplinLineBuffer    = NULL;
char    *pplinLineAddBuffer = NULL;

int      ppl_shellExiting      = 0;
long int history_NLinesWritten = 0;

int ppl_inputInit()
 {
  pplinLineBuffer = (char *)malloc(LSTR_LENGTH);
  return (pplinLineBuffer!=NULL);
 }

void ppl_interactiveSession()
 {
  int   linenumber = 1;
  sigset_t sigs;

  if (DEBUG) ppl_log("Starting an interactive session.");

  if ((isatty(STDIN_FILENO) == 1) && (pplset_session_default.splash == SW_ONOFF_ON)) ppl_report(ppltxt_welcome);

  ppl_shellExiting = 0;
  while (!ppl_shellExiting)
   {
    // Set up SIGINT handler
    if (sigsetjmp(ppl_sigjmpToInteractive, 1) == 0)
     {
      ppl_sigjmpFromSigInt = &ppl_sigjmpToInteractive;

      pplcsp_checkForGvOutput();
      if (isatty(STDIN_FILENO) == 1)
       {
        ppl_error_setstreaminfo(-1, "");
#ifdef HAVE_READLINE
         {
          char *line_ptr;
          line_ptr = readline( (pplinLineAddBuffer == NULL) ? "pyxplot> " : ".......> ");
          if (line_ptr==NULL) break;
          add_history(line_ptr);
          history_NLinesWritten++;
          strncpy(pplinLineBuffer, line_ptr, LSTR_LENGTH-1);
          pplinLineBuffer[LSTR_LENGTH-1]='\0';
          free(line_ptr);
         }
#else
        printf("%s",prompt);
        if (fgets(pplinLineBuffer,LSTR_LENGTH-1,stdin)==NULL) break;
        pplinLineBuffer[LSTR_LENGTH-1]='\0';
#endif
       }
      else
       {
        ppl_error_setstreaminfo(linenumber, "piped input");
        if ((feof(stdin)) || (ferror(stdin))) break;
        ppl_file_readline(stdin, pplinLineBuffer, LSTR_LENGTH-1);
        pplinLineBuffer[LSTR_LENGTH-1]='\0';
        linenumber++;
       }
      ppl_processLine(pplinLineBuffer, isatty(STDIN_FILENO), 0, 0);
      ppl_error_setstreaminfo(-1, "");
      pplcsp_killAllHelpers();
     } else {
      sigemptyset(&sigs); // SIGINT longjmps return here
      sigaddset(&sigs,SIGCHLD);
      sigprocmask(SIG_UNBLOCK, &sigs, NULL);
      fprintf(stdout,"\n");
      if (pplinLineAddBuffer != NULL) { free(pplinLineAddBuffer); pplinLineAddBuffer=NULL; }
      if (chdir(pplset_session_default.cwd) < 0) { ppl_fatal(__FILE__,__LINE__,"chdir into cwd failed."); } // chdir out of temporary directory
     }
   }

  ppl_sigjmpFromSigInt = &ppl_sigjmpToMain; // SIGINT now drops back through to main().
  if (pplinLineAddBuffer != NULL) { free(pplinLineAddBuffer); pplinLineAddBuffer=NULL; }
  ppl_shellExiting = 0;
  if (isatty(STDIN_FILENO) == 1)
   {
    if (pplset_session_default.splash == SW_ONOFF_ON) ppl_report("\nGoodbye. Have a nice day.");
    else                                              ppl_report(""); // Make a new line
   }
  return;
 }

void ppl_processScript(char *input, int iterLevel)
 {
  int  linenumber = 1;
  int  status;
  int  ProcessedALine = 0;
  char full_filename[FNAME_LENGTH];
  char filename_description[FNAME_LENGTH];
  FILE *infile;

  if (DEBUG) { sprintf(ppl_tempErrStr, "Processing input from the script file '%s'.", input); ppl_log(ppl_tempErrStr); }
  ppl_unixExpandUserHomeDir(input , pplset_session_default.cwd, full_filename);
  sprintf(filename_description, "file '%s'", input);
  if ((infile=fopen(full_filename,"r")) == NULL)
   {
    sprintf(ppl_tempErrStr, "Could not find command file '%s'. Skipping on to next command file.", full_filename); ppl_error(ERR_FILE, -1, -1, ppl_tempErrStr);
    return;
   }

  ppl_shellExiting = 0;
  while (!ppl_shellExiting)
   {
    ppl_error_setstreaminfo(linenumber, filename_description);
    if ((feof(infile)) || (ferror(infile))) break;
    ppl_file_readline(infile, pplinLineBuffer, LSTR_LENGTH);
    linenumber++;
    status = ppl_processLine(pplinLineBuffer, 0, iterLevel, !ProcessedALine);
    ppl_error_setstreaminfo(-1, "");
    pplcsp_killAllHelpers();
    if (status>0) // If an error occurs on the first line of a script, aborted processing it
     {
      ppl_error(ERR_FILE, -1, -1, "Error on first line of command file.  Is this a valid script?  Aborting.");
      if (pplinLineAddBuffer != NULL) { free(pplinLineAddBuffer); pplinLineAddBuffer=NULL; }
      break;
     }
    if (status>=0) ProcessedALine = 1;
   }
  ppl_shellExiting = 0;
  if (pplinLineAddBuffer != NULL) { free(pplinLineAddBuffer); pplinLineAddBuffer=NULL; }
  fclose(infile);
  pplcsp_checkForGvOutput();
  return;
 }

int ppl_processLine(char *in, int interactive, int iterLevel, int exitOnError)
 {
  int   i, status=0;
  char *inputLineBuffer = NULL;
  char  quoteChar;

  // Find last character of presently entered line. If it's a \, buffer line and collect another
  for (i=0; in[i]!='\0'; i++); for (; ((i>0)&&(in[i]<=' ')); i--);
  if (in[i]=='\\')
   {
    if (pplinLineAddBuffer==NULL)
     {
      pplinLineAddBuffer = (char *)malloc(i+1);
      if (pplinLineAddBuffer == NULL) { ppl_error(ERR_MEMORY, -1, -1, "Out of memory whilst trying to combine input lines."); return 1; }
      strncpy(pplinLineAddBuffer, in, i);
      pplinLineAddBuffer[i]='\0';
     } else {
      int j = strlen(pplinLineAddBuffer);
      pplinLineAddBuffer = (char *)realloc((void *)pplinLineAddBuffer, j+i+1);
      if (pplinLineAddBuffer == NULL) { ppl_error(ERR_MEMORY, -1, -1, "Out of memory whilst trying to combine input lines."); return 1; }
      strncpy(pplinLineAddBuffer+j, in, i);
      pplinLineAddBuffer[j+i]='\0';
     }
    return 0;
   }

  // Add previous backslashed lines to the beginning of this one
  if (pplinLineAddBuffer!=NULL)
   {
    int j = strlen(pplinLineAddBuffer);
    pplinLineAddBuffer = (char *)realloc((void *)pplinLineAddBuffer, j+i+2);
    if (pplinLineAddBuffer == NULL) { ppl_error(ERR_MEMORY, -1, -1, "Out of memory whilst trying to combine input lines."); return 1; }
    strncpy(pplinLineAddBuffer+j, in, i+1);
    pplinLineAddBuffer[j+i+1]='\0';
    inputLineBuffer = pplinLineAddBuffer;
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
    status = ppl_ProcessStatement(inputLineBuffer);
    if ((status) && (exitOnError)) break;
    inputLineBuffer = inputLineBuffer+i+1;
    i=0;
   }
LOOP_END

  if ((!status) || (!exitOnError)) status = ppl_ProcessStatement(inputLineBuffer);
  if (pplinLineAddBuffer != NULL) free(pplinLineAddBuffer);
  pplinLineAddBuffer = NULL;
  return (status!=0);
 }

#include "expressions/expCompile.h"

int ppl_ProcessStatement(char *line)
 {
  int end, errPos, i;
  char errText[LSTR_LENGTH];
  unsigned char buff[65536];
  expMarkup(line,&end,0,buff,&errPos,errText);
  if (errPos>=0)
   {
    printf("%d:%s\n",errPos,errText);
   }
  else
   {
    printf("%s\n",line);
    for (i=0; i<end; i++) printf("%c",'@'+buff[i]);
    printf("\n");
   }
  return 0;
 }

