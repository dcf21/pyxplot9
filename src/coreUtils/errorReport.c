// errorReport.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "coreUtils/errorReport.h"
#include "settings/settingTypes.h"

static char temp_stringA[LSTR_LENGTH], temp_stringB[LSTR_LENGTH], temp_stringC[LSTR_LENGTH], temp_stringD[LSTR_LENGTH], temp_stringE[LSTR_LENGTH];

void ppl_error_setstreaminfo(pplerr_context *context, int linenumber,char *filename)
 {
  context->error_input_linenumber = linenumber;
  if (filename != NULL) strcpy(context->error_input_filename, filename);
  return;
 }

void ppl_error(pplerr_context *context, int ErrType, int HighlightPos1, int HighlightPos2, char *msg)
 {
  unsigned char ApplyHighlighting, reverse=0;
  int i=0, j;

  if (msg==NULL) msg=context->tempErrStr;
  ApplyHighlighting = ((context->session_default.colour == SW_ONOFF_ON) && (isatty(STDERR_FILENO) == 1));
  if (msg!=temp_stringA) { strcpy(temp_stringA, msg); msg = temp_stringA; }

  temp_stringB[i]='\0';

  if (ErrType != ERR_PREFORMED) // Do not prepend anything to pre-formed errors
   {
    // When processing scripts, print error location
    if ((context->error_input_linenumber != -1) && (strcmp(context->error_input_filename, "") !=  0))
     {
      sprintf(temp_stringB+i, "%s:%d:", context->error_input_filename, context->error_input_linenumber);
      i += strlen(temp_stringB+i);
      if (ErrType != ERR_STACKED) { temp_stringB[i++] = ' '; temp_stringB[i] = '\0'; }
     }

    // Prepend error type
    switch (ErrType)
     {
      case ERR_INTERNAL: sprintf(temp_stringB+i, "Internal Error: ");  break;
      case ERR_MEMORY  :
      case ERR_GENERAL : sprintf(temp_stringB+i, "Error: ");           break;
      case ERR_SYNTAX  : sprintf(temp_stringB+i, "Syntax Error: ");    break;
      case ERR_NUMERIC : sprintf(temp_stringB+i, "Numerical Error: "); break;
      case ERR_FILE    : sprintf(temp_stringB+i, "File Error: ");      break;
     }
    i += strlen(temp_stringB+i);
   }

  for (j=0; msg[j]!='\0'; j++)
   {
    if (ApplyHighlighting && ((j==HighlightPos1-1) || (j==HighlightPos2-1))) { if (reverse==0) { strcpy(temp_stringB+i, "\x1b[7m"); i+=strlen(temp_stringB+i); } reverse=3; }
    else if (ApplyHighlighting && (reverse==1)) { strcpy(temp_stringB+i, "\x1b[27m"); i+=strlen(temp_stringB+i); reverse=0; }
    else if (ApplyHighlighting && (reverse> 1)) reverse--;
    temp_stringB[i++] = msg[j];
   }
  if (ApplyHighlighting && (reverse!=0)) { strcpy(temp_stringB+i, "\x1b[27m"); i+=strlen(temp_stringB+i); reverse=0; }
  temp_stringB[i] = '\0';

  if (DEBUG) { ppl_log(context, temp_stringB); }

  // Print message in colour or monochrome
  if (ApplyHighlighting)
   sprintf(temp_stringC, "%s%s%s\n", *(char **)FetchSettingName( context , context->session_default.colour_err , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)),
                                     temp_stringB,
                                     *(char **)FetchSettingName( context , SW_TERMCOL_NOR                      , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)) );
  else
   sprintf(temp_stringC, "%s\n", temp_stringB);
  fputs(temp_stringC, stderr);
  return; 
 }

void ppl_fatal(pplerr_context *context, char *file, int line, char *msg)
 {
  char introline[FNAME_LENGTH];
  if (msg==NULL) msg=context->tempErrStr;
  if (msg!=temp_stringE) strcpy(temp_stringE, msg);
  sprintf(introline, "Fatal Error encountered in %s at line %d: %s", file, line, temp_stringE);
  ppl_error(context, ERR_PREFORMED, -1, -1, introline);
  if (DEBUG) ppl_log(context, "Terminating with error condition 1.");
  exit(1);
 }

void ppl_warning(pplerr_context *context, int ErrType, char *msg)
 {
  int i=0;

  if (msg==NULL) msg=context->tempErrStr;
  if (msg!=temp_stringA) { strcpy(temp_stringA, msg); msg = temp_stringA; }

  temp_stringB[i]='\0';

  if (ErrType != ERR_PREFORMED) // Do not prepend anything to pre-formed errors
   {
    // When processing scripts, print error location
    if ((context->error_input_linenumber != -1) && (strcmp(context->error_input_filename, "") !=  0))
     {
      sprintf(temp_stringB+i, "%s:%d:", context->error_input_filename, context->error_input_linenumber);
      i += strlen(temp_stringB+i);
      if (ErrType != ERR_STACKED) { temp_stringB[i++] = ' '; temp_stringB[i] = '\0'; }
     }

    // Prepend error type
    switch (ErrType)
     {
      case ERR_INTERNAL: sprintf(temp_stringB+i, "Internal Warning: ");  break;
      case ERR_MEMORY  :
      case ERR_GENERAL : sprintf(temp_stringB+i, "Warning: ");           break;
      case ERR_SYNTAX  : sprintf(temp_stringB+i, "Syntax Warning: ");    break;
      case ERR_NUMERIC : sprintf(temp_stringB+i, "Numerical Warning: "); break;
      case ERR_FILE    : sprintf(temp_stringB+i, "File Warning: ");      break;
     }
    i += strlen(temp_stringB+i);
   }

  strcpy(temp_stringB+i, msg);

  if (DEBUG) { ppl_log(context, temp_stringB); }

  // Print message in colour or monochrome
  if ((context->session_default.colour == SW_ONOFF_ON) && (isatty(STDERR_FILENO) == 1))
   sprintf(temp_stringC, "%s%s%s\n", *(char **)FetchSettingName( context , context->session_default.colour_wrn , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)),
                                     temp_stringB,
                                     *(char **)FetchSettingName( context , SW_TERMCOL_NOR                      , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)) );
  else
   sprintf(temp_stringC, "%s\n", temp_stringB);
  fputs(temp_stringC, stderr);
  return;
 }

void ppl_report(pplerr_context *context, char *msg)
 {
  if (msg==NULL) msg=context->tempErrStr;
  if (msg!=temp_stringA) strcpy(temp_stringA, msg);
  if (DEBUG) { sprintf(temp_stringC, "%s%s", "Reporting:\n", temp_stringA); ppl_log(context, temp_stringC); }
  if ((context->session_default.colour == SW_ONOFF_ON) && (isatty(STDOUT_FILENO) == 1))
   sprintf(temp_stringC, "%s%s%s\n", *(char **)FetchSettingName( context , context->session_default.colour_rep , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)),
                                     temp_stringA,
                                     *(char **)FetchSettingName( context , SW_TERMCOL_NOR                      , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)) );
  else
   sprintf(temp_stringC, "%s\n", temp_stringA);
  fputs(temp_stringC, stdout);
  return;
 }

void ppl_log(pplerr_context *context, char *msg)
 {
  static FILE *logfile = NULL;
  static int   latch = 0;
  char         linebuffer[LSTR_LENGTH];

  if (latch) return; // Do not allow recursive calls, which might be generated by the call to ppl_fatal below
  latch = 1;
  if (logfile==NULL)
   {
    char LogFName[128];
    sprintf(LogFName,"pyxplot.%d.log",getpid());
    if ((logfile=fopen(LogFName,"w")) == NULL) { ppl_fatal(context,__FILE__,__LINE__,"Could not open log file to write."); exit(1); }
    setvbuf(logfile, NULL, _IOLBF, 0); // Set log file to be line-buffered, so that log file is always up-to-date
   }

  if (msg==NULL) msg=context->tempErrStr;
  if (msg!=temp_stringD) strcpy(temp_stringD, msg);
  fprintf(logfile, "[%s] [%s] %s\n", ppl_strStrip(ppl_friendlyTimestring(), linebuffer), context->error_source, temp_stringD);
  latch = 0;
  return;
 }

