// pyxplot_watch.c
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
#include <time.h>
#include <glob.h>
#include <sys/stat.h>

#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/errorReport.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "settings/settingTypes.h"
#include "userspace/context.h"

void RunPyXPlotOnFile(pplerr_context *context, char *fname)
 {
  char         LineBuffer[LSTR_LENGTH];
  int          interactive, status;

  interactive = ((isatty(STDIN_FILENO) == 1) && (context->session_default.splash == SW_ONOFF_ON));

  if (interactive)
   {
    sprintf(LineBuffer, "[%s] Running %s.", ppl_strStrip(ppl_friendlyTimestring(), context->tempErrStr), fname);
    ppl_report(context,LineBuffer);
   }

  sprintf(LineBuffer, "%s -q %s %s", PPLBINARY, (context->session_default.color == SW_ONOFF_ON) ? "-c" : "-m", fname);
  status = system(LineBuffer);

  if (status && !interactive)
   {
    sprintf(LineBuffer, "[%s] Encountered problem in script file %s.", ppl_strStrip(ppl_friendlyTimestring(), context->tempErrStr), fname);
    ppl_error(context,ERR_PREFORMED, -1, -1, LineBuffer);
   }

  if (interactive)
   {
    sprintf(LineBuffer, "[%s] Finished %s.", ppl_strStrip(ppl_friendlyTimestring(), context->tempErrStr), fname);
    ppl_report(context,LineBuffer);
   }
  return;
 }

int main(int argc, char **argv)
 {
  struct timespec waitperiod, waitedperiod; // A time.h timespec specifier; used for sleeping
  char init_string[LSTR_LENGTH], help_string[LSTR_LENGTH], version_string[FNAME_LENGTH], version_string_underline[FNAME_LENGTH];
  int i, j, HaveFilenames, DoneWork;
  glob_t GlobData;
  pplerr_context *context;

  dict *StatInfodict;
  struct stat StatInfo, *StatInfoPtr;

  // Initialise sub-modules
  context = malloc(sizeof(pplerr_context));
  if (context==NULL) { printf("malloc fail."); return 1; }
  context->error_input_linenumber = -1;
  context->error_input_filename[0] = '\0';
  strcpy(context->error_source,"main     ");

  if (DEBUG) ppl_log(context,"Initialising PyXPlot Watch.");
  ppl_memAlloc_MemoryInit(context, &ppl_error, &ppl_log);
  StatInfodict = ppl_dictInit(HASHSIZE_SMALL, 1);

  // Set up default session settings
  context->session_default.splash    = SW_ONOFF_ON;
  context->session_default.color    = SW_ONOFF_ON;
  context->session_default.color_rep= SW_TERMCOL_GRN;
  context->session_default.color_wrn= SW_TERMCOL_BRN;
  context->session_default.color_err= SW_TERMCOL_RED;

  // Make help and version strings
  sprintf(version_string, "\nPyXPlot Watch %s\n", VERSION);

  sprintf(help_string   , "%s\
%s\n\
\n\
Usage: pyxplot_watch <options> <filelist>\n\
  -h, --help:       Display this help.\n\
  -v, --version:    Display version number.\n\
  -q, --quiet:      Turn off initial welcome message.\n\
  -V, --verbose:    Turn on initial welcome message.\n\
  -c, --color:     Use colored highlighting of output.\n\
  -m, --monochrome: Turn off colored highlighting.\n\
\n\
A brief introduction to PyXPlot can be obtained by typing 'man pyxplot'; the\n\
full Users' Guide can be found in the file:\n\
%s%spyxplot.pdf\n\
\n\
For the latest information on PyXPlot development, see the project website:\n\
<http://www.pyxplot.org.uk>\n", version_string, ppl_strUnderline(version_string, version_string_underline), DOCDIR, PATHLINK);

sprintf(init_string, "\n\
 ____       __  ______  _       _      PYXPLOT WATCH\n\
|  _ \\ _   _\\ \\/ /  _ \\| | ___ | |_    Version %s\n\
| |_) | | | |\\  /| |_) | |/ _ \\| __|   %s\n\
|  __/| |_| |/  \\|  __/| | (_) | |_ \n\
|_|    \\__, /_/\\_\\_|   |_|\\___/ \\__|   Copyright (C) 2006-2012 Dominic Ford\n\
       |___/                                         2008-2011 Ross Church\n\
                                                     2010-2011 Zoltan Voros\n\
\n\
Send comments, bug reports, feature requests and coffee supplies to:\n\
<coders@pyxplot.org.uk>\n\
", VERSION, DATE);

  // Scan commandline options for any switches
  HaveFilenames=0;
  for (i=1; i<argc; i++)
   {
    if (strlen(argv[i])==0) continue;
    if (argv[i][0]!='-')
     {
      HaveFilenames=1;
      continue;
     }
    if      (strcmp(argv[i], "-q"          )==0) context->session_default.splash = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-quiet"      )==0) context->session_default.splash = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "--quiet"     )==0) context->session_default.splash = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-V"          )==0) context->session_default.splash = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-verbose"    )==0) context->session_default.splash = SW_ONOFF_ON;
    else if (strcmp(argv[i], "--verbose"   )==0) context->session_default.splash = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-c"          )==0) context->session_default.color  = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-colour"     )==0) context->session_default.color  = SW_ONOFF_ON;
    else if (strcmp(argv[i], "--colour"    )==0) context->session_default.color  = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-color"      )==0) context->session_default.color  = SW_ONOFF_ON;
    else if (strcmp(argv[i], "--color"     )==0) context->session_default.color  = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-m"          )==0) context->session_default.color  = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-mono"       )==0) context->session_default.color  = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "--mono"      )==0) context->session_default.color  = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-monochrome" )==0) context->session_default.color  = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "--monochrome")==0) context->session_default.color  = SW_ONOFF_OFF;
    else if ((strcmp(argv[i], "-v")==0) || (strcmp(argv[i], "-version")==0) || (strcmp(argv[i], "--version")==0))
     {
      ppl_report(context,version_string);
      if (DEBUG) ppl_log(context,"Reported version number as requested.");
      ppl_memAlloc_FreeAll(0); ppl_memAlloc_MemoryStop();
      return 0;
     }
    else if ((strcmp(argv[i], "-h")==0) || (strcmp(argv[i], "-help")==0) || (strcmp(argv[i], "--help")==0))
     {
      ppl_report(context,help_string);
      if (DEBUG) ppl_log(context,"Reported help text as requested.");
      ppl_memAlloc_FreeAll(0); ppl_memAlloc_MemoryStop();
      return 0;
     }
    else
    {
     sprintf(context->tempErrStr, "Received switch '%s' which was not recognised. Type 'pyxplot_watch -help' for a list of available commandline options.", argv[i]);
     ppl_error(context,ERR_PREFORMED, -1, -1, NULL);
     if (DEBUG) ppl_log(context,"Received unexpected commandline switch.");
     ppl_memAlloc_FreeAll(0); ppl_memAlloc_MemoryStop();
     return 1;
    }
   }

  // Produce splash text
  if ((isatty(STDIN_FILENO) == 1) && (context->session_default.splash == SW_ONOFF_ON)) ppl_report(context,init_string);

  // Check that we have some filenames to watch
  if (!HaveFilenames)
   {
    ppl_error(context,ERR_PREFORMED, -1, -1, "No filenames were supplied to watch. PyXPlot Watch's commandline syntax is:\n\npyxplot_watch [options] filename_list\n\nAs PyXPlot Watch has no work to do, it is exiting...");
    exit(1);
   }

  // Scan commandline options and glob all filenames
  while (1)
   {
    DoneWork = 0;
    for (i=1; i<argc; i++)
     {
      if ((argv[i][0]=='\0') || (argv[i][0]=='-')) continue;
      if (glob(argv[i], 0, NULL, &GlobData) != 0) continue;

      for (j=0; j<GlobData.gl_pathc; j++)
       {
        if (stat(GlobData.gl_pathv[j], &StatInfo) != 0) continue; // Stat didn't work...
        StatInfoPtr = (struct stat *)ppl_dictLookup(StatInfodict, GlobData.gl_pathv[j]);
        if ((StatInfoPtr != NULL) && (StatInfoPtr->st_mtime >= StatInfo.st_mtime)) continue; // This file has not been modified lately
        ppl_dictAppendCpy(StatInfodict, GlobData.gl_pathv[j], (void *)&StatInfo, sizeof(struct stat));
        DoneWork = 1;
        RunPyXPlotOnFile(context, GlobData.gl_pathv[j]);
        waitperiod.tv_sec  = 0;
        waitperiod.tv_nsec = 100000000;
        nanosleep(&waitperiod,&waitedperiod);
       }

      globfree(&GlobData);
     }
    if (!DoneWork)
     {
      waitperiod.tv_sec  = 1;
      waitperiod.tv_nsec = 0;
      nanosleep(&waitperiod,&waitedperiod);
     }
   }

  // Never get here...
  free(context);
  return 0;
 }

