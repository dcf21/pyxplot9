// errorReport.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "coreUtils/errorReport.h"
#include "settings/settingTypes.h"

#define BLEN LSTR_LENGTH-1

static char temp_stringA[BLEN+1], temp_stringB[BLEN+1], temp_stringC[BLEN+1], temp_stringD[BLEN+1], temp_stringE[BLEN+1];

void ppl_error_setstreaminfo(pplerr_context *context, int linenumber,char *filename)
 {
  static long sourceId=0;
  context->error_input_sourceId   = sourceId++;
  context->error_input_linenumber = linenumber;
  if (filename != NULL)
   {
    strncpy(context->error_input_filename, filename, FNAME_LENGTH);
    context->error_input_filename[FNAME_LENGTH-1]='\0';
   }
  return;
 }

void ppl_error(pplerr_context *context, int ErrType, int HighlightPos1, int HighlightPos2, char *msg)
 {
  unsigned char ApplyHighlighting, reverse=0;
  int i=0, j;

  if (msg==NULL) msg=context->tempErrStr;
  ApplyHighlighting = ((context->session_default.color == SW_ONOFF_ON) && (isatty(STDERR_FILENO) == 1));
  if (msg!=temp_stringA) { snprintf(temp_stringA, BLEN, "%s", msg); temp_stringA[BLEN]='\0'; msg = temp_stringA; }

  temp_stringB[i] = temp_stringB[BLEN] = '\0';

  if (ErrType != ERR_PREFORMED) // Do not prepend anything to pre-formed errors
   {
    // When processing scripts, print error location
    if ((context->error_input_linenumber != -1) && (strcmp(context->error_input_filename, "") !=  0))
     {
      snprintf(temp_stringB+i, BLEN-i, "%s:%d:", context->error_input_filename, context->error_input_linenumber);
      i += strlen(temp_stringB+i);
      if ((ErrType != ERR_STACKED)&&(i<BLEN-2)) { temp_stringB[i++] = ' '; temp_stringB[i] = '\0'; }
     }

    // Prepend error type
    switch (ErrType)
     {
      case ERR_INTERNAL : snprintf(temp_stringB+i, BLEN-i, "Internal Error: ");  break;
      case ERR_MEMORY   :
      case ERR_GENERAL  : snprintf(temp_stringB+i, BLEN-i, "Error: ");           break;
      case ERR_SYNTAX   : snprintf(temp_stringB+i, BLEN-i, "Syntax Error: ");    break;
      case ERR_NUMERIC  : snprintf(temp_stringB+i, BLEN-i, "Numerical Error: "); break;
      case ERR_FILE     : snprintf(temp_stringB+i, BLEN-i, "File Error: ");      break;
      case ERR_RANGE    : snprintf(temp_stringB+i, BLEN-i, "Range Error: ");     break;
      case ERR_UNIT     : snprintf(temp_stringB+i, BLEN-i, "Unit Error: ");      break;
      case ERR_OVERFLOW : snprintf(temp_stringB+i, BLEN-i, "Overflow Error: ");  break;
      case ERR_NAMESPACE: snprintf(temp_stringB+i, BLEN-i, "Namespace Error: "); break;
      case ERR_TYPE     : snprintf(temp_stringB+i, BLEN-i, "Type Error: ");      break;
      case ERR_INTERRUPT: snprintf(temp_stringB+i, BLEN-i, "Interrupt Error: "); break;
      case ERR_DICTKEY  : snprintf(temp_stringB+i, BLEN-i, "Key Error: ");       break;
     }
    i += strlen(temp_stringB+i);
   }

  for (j=0; msg[j]!='\0'; j++)
   {
    if (ApplyHighlighting && ((j==HighlightPos1-1) || (j==HighlightPos2-1))) { if (reverse==0) { snprintf(temp_stringB+i, BLEN-i, "\x1b[7m"); i+=strlen(temp_stringB+i); } reverse=3; }
    else if (ApplyHighlighting && (reverse==1)) { snprintf(temp_stringB+i, BLEN-i, "\x1b[27m"); i+=strlen(temp_stringB+i); reverse=0; }
    else if (ApplyHighlighting && (reverse> 1)) reverse--;
    if (i<BLEN) temp_stringB[i++] = msg[j];
   }
  if (ApplyHighlighting && (reverse!=0)) { snprintf(temp_stringB+i, BLEN-i, "\x1b[27m"); i+=strlen(temp_stringB+i); reverse=0; }
  if (i<BLEN) temp_stringB[i] = '\0';

  if (DEBUG) { ppl_log(context, temp_stringB); }

  // Print message in color or monochrome
  if (ApplyHighlighting)
   snprintf(temp_stringC, BLEN, "%s%s%s\n", *(char **)ppl_fetchSettingName( context , context->session_default.color_err , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)),
                                           temp_stringB,
                                            *(char **)ppl_fetchSettingName( context , SW_TERMCOL_NOR                      , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)) );
  else
   snprintf(temp_stringC, BLEN, "%s\n", temp_stringB);
  temp_stringC[BLEN]='\0';
  fputs(temp_stringC, stderr);
  return; 
 }

void ppl_fatal(pplerr_context *context, char *file, int line, char *msg)
 {
  char introline[FNAME_LENGTH];
  if (msg==NULL) msg=context->tempErrStr;
  if (msg!=temp_stringE) { snprintf(temp_stringE, BLEN, "%s", msg); temp_stringE[BLEN-1]='\0'; msg = temp_stringE; }
  snprintf(introline, FNAME_LENGTH, "Fatal Error encountered in %s at line %d: %s", file, line, temp_stringE);
  introline[FNAME_LENGTH-1]='\0';
  ppl_error(context, ERR_PREFORMED, -1, -1, introline);
  if (DEBUG) ppl_log(context, "Terminating with error condition 1.");
  exit(1);
 }

void ppl_warning(pplerr_context *context, int ErrType, char *msg)
 {
  int i=0;

  if (msg==NULL) msg=context->tempErrStr;
  if (msg!=temp_stringA) { snprintf(temp_stringA, BLEN, "%s", msg); temp_stringA[BLEN-1]='\0'; msg = temp_stringA; }

  temp_stringB[i] = temp_stringB[BLEN] = '\0';

  if (ErrType != ERR_PREFORMED) // Do not prepend anything to pre-formed errors
   {
    // When processing scripts, print error location
    if ((context->error_input_linenumber != -1) && (strcmp(context->error_input_filename, "") !=  0))
     {
      snprintf(temp_stringB+i, BLEN+i, "%s:%d:", context->error_input_filename, context->error_input_linenumber);
      i += strlen(temp_stringB+i);
      if ((ErrType != ERR_STACKED)&&(i<BLEN-2)) { temp_stringB[i++] = ' '; temp_stringB[i] = '\0'; }
     }

    // Prepend error type
    switch (ErrType)
     {
      case ERR_INTERNAL : snprintf(temp_stringB+i, BLEN-i, "Internal Warning: ");  break;
      case ERR_MEMORY   :
      case ERR_GENERAL  : snprintf(temp_stringB+i, BLEN-i, "Warning: ");           break;
      case ERR_SYNTAX   : snprintf(temp_stringB+i, BLEN-i, "Syntax Warning: ");    break;
      case ERR_NUMERIC  : snprintf(temp_stringB+i, BLEN-i, "Numerical Warning: "); break;
      case ERR_FILE     : snprintf(temp_stringB+i, BLEN-i, "File Warning: ");      break;
      case ERR_RANGE    : snprintf(temp_stringB+i, BLEN-i, "Range Warning: ");     break;
      case ERR_UNIT     : snprintf(temp_stringB+i, BLEN-i, "Unit Warning: ");      break;
      case ERR_OVERFLOW : snprintf(temp_stringB+i, BLEN-i, "Overflow Warning: ");  break;
      case ERR_NAMESPACE: snprintf(temp_stringB+i, BLEN-i, "Namespace Warning: "); break;
      case ERR_TYPE     : snprintf(temp_stringB+i, BLEN-i, "Type Warning: ");      break;
      case ERR_INTERRUPT: snprintf(temp_stringB+i, BLEN-i, "Interrupt Warning: "); break;
      case ERR_DICTKEY  : snprintf(temp_stringB+i, BLEN-i, "Key Warning: ");       break;
     }
    i += strlen(temp_stringB+i);
   }

  snprintf(temp_stringB+i, BLEN-i, "%s", msg);

  if (DEBUG) { ppl_log(context, temp_stringB); }

  // Print message in color or monochrome
  if ((context->session_default.color == SW_ONOFF_ON) && (isatty(STDERR_FILENO) == 1))
   snprintf(temp_stringC, BLEN, "%s%s%s\n", *(char **)ppl_fetchSettingName( context , context->session_default.color_wrn , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)),
                                            temp_stringB,
                                            *(char **)ppl_fetchSettingName( context , SW_TERMCOL_NOR                      , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)) );
  else
   snprintf(temp_stringC, BLEN, "%s\n", temp_stringB);
  temp_stringC[BLEN]='\0';
  fputs(temp_stringC, stderr);
  return;
 }

void ppl_report(pplerr_context *context, char *msg)
 {
  if (msg==NULL) msg=context->tempErrStr;
  if (msg!=temp_stringA) { snprintf(temp_stringA, BLEN, "%s", msg); temp_stringA[BLEN-1]='\0'; }
  if (DEBUG) { snprintf(temp_stringC, BLEN, "%s%s", "Reporting:\n", temp_stringA); temp_stringC[BLEN]='\0'; ppl_log(context, temp_stringC); }
  if ((context->session_default.color == SW_ONOFF_ON) && (isatty(STDOUT_FILENO) == 1))
   snprintf(temp_stringC, BLEN, "%s%s%s\n", *(char **)ppl_fetchSettingName( context , context->session_default.color_rep , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)),
                                            temp_stringA,
                                            *(char **)ppl_fetchSettingName( context , SW_TERMCOL_NOR                      , SW_TERMCOL_INT , (void *)SW_TERMCOL_TXT, sizeof(char *)) );
  else
   snprintf(temp_stringC, BLEN, "%s\n", temp_stringA);
  temp_stringC[BLEN]='\0';
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
    snprintf(LogFName,127,"pyxplot.%d.log",getpid());
    LogFName[127]='\0';
    if ((logfile=fopen(LogFName,"w")) == NULL) { ppl_fatal(context,__FILE__,__LINE__,"Could not open log file to write."); exit(1); }
    setvbuf(logfile, NULL, _IOLBF, 0); // Set log file to be line-buffered, so that log file is always up-to-date
   }

  if (msg==NULL) msg=context->tempErrStr;
  if (msg!=temp_stringD) { snprintf(temp_stringD, BLEN, "%s", msg); temp_stringD[BLEN-1]='\0'; }
  fprintf(logfile, "[%s] [%s] %s\n", ppl_strStrip(ppl_friendlyTimestring(), linebuffer), context->error_source, temp_stringD);
  latch = 0;
  return;
 }

