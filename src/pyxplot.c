// pyxplot.c
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

#define _PYXPLOT_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include <sys/stat.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <gsl/gsl_errno.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "coreUtils/list.h"
#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/errorReport.h"

#include "children.h"
#include "input.h"
#include "pplConstants.h"
#include "settings/papersizes.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/textConstants.h"

#include "userspace/context.h"

#include "pyxplot.h"

// SIGINT Handling information

sigjmp_buf ppl_sigjmpToMain;
sigjmp_buf ppl_sigjmpToInteractive;
sigjmp_buf ppl_sigjmpToDirective;
sigjmp_buf *ppl_sigjmpFromSigInt = NULL;

int main(int argc, char **argv)
 {
  int          i,fail=0;
  int          tempdirnumber = 1;
  char         tempdirpath[FNAME_LENGTH];
  char        *EnvDisplay;
  ppl_context *context;
  struct stat  statinfo;

  sigset_t sigs;

  struct timespec waitperiod, waitedperiod; // A time.h timespec specifier for a 50ms nanosleep wait
  waitperiod.tv_sec  = 0;
  waitperiod.tv_nsec = 50000000;

  // Initialise sub-modules
  context = ppl_contextInit();
  if (DEBUG) ppl_log(&context->errcontext,"Initialising PyXPlot.");
  ppl_memAlloc_MemoryInit(&context->errcontext, &ppl_error, &ppl_log);
  ppl_PaperSizeInit();
  ppl_inputInit(context);
  pplset_makedefault(context);
  ppltxt_init();

  // Turn off GSL's automatic error handler
  gsl_set_error_handler_off();

  // Initialise GNU Readline
#ifdef HAVE_READLINE
  rl_readline_name = "PyXPlot";                          /* Allow conditional parsing of the ~/.inputrc file. */
  //rl_attempted_completion_function = ppl_rl_completion;  /* Tell the completer that we want a crack first. */
#endif

  // Set default terminal
  EnvDisplay = getenv("DISPLAY"); // Check whether the environment variable DISPLAY is set
  if      (strcmp(GHOSTVIEW_COMMAND, "/bin/false")!=0) pplset_term_current.viewer = pplset_term_default.viewer = SW_VIEWER_GV;
  else if (strcmp(GGV_COMMAND      , "/bin/false")!=0) pplset_term_current.viewer = pplset_term_default.viewer = SW_VIEWER_GGV;
  else                                                 pplset_term_current.viewer = pplset_term_default.viewer = SW_VIEWER_NULL;
  if ( /* (ppl_termtype_set_in_configfile == 0) && */ ((context->willBeInteractive==0) ||
                                                (EnvDisplay==NULL) ||
                                                (EnvDisplay[0]=='\0') ||
                                                (pplset_term_default.viewer == SW_VIEWER_NULL) ||
                                                (isatty(STDIN_FILENO) != 1)))
   {
    if (DEBUG) ppl_log(&context->errcontext,"Detected that we are running a non-interactive session; defaulting to the EPS terminal.");
    pplset_term_current.TermType = pplset_term_default.TermType = SW_TERMTYPE_EPS;
   }

  // Scan commandline options for any switches
  for (i=1; i<argc; i++)
   {
    if (strlen(argv[i])==0) continue;
    if (argv[i][0]!='-')
     {
      if (context->willBeInteractive==1) context->willBeInteractive=0;
      continue;
     }
    if      (strcmp(argv[i], "-q"          )==0) pplset_session_default.splash = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-quiet"      )==0) pplset_session_default.splash = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "--quiet"     )==0) pplset_session_default.splash = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-V"          )==0) pplset_session_default.splash = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-verbose"    )==0) pplset_session_default.splash = SW_ONOFF_ON;
    else if (strcmp(argv[i], "--verbose"   )==0) pplset_session_default.splash = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-c"          )==0) pplset_session_default.colour = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-colour"     )==0) pplset_session_default.colour = SW_ONOFF_ON;
    else if (strcmp(argv[i], "--colour"    )==0) pplset_session_default.colour = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-color"      )==0) pplset_session_default.colour = SW_ONOFF_ON;
    else if (strcmp(argv[i], "--color"     )==0) pplset_session_default.colour = SW_ONOFF_ON;
    else if (strcmp(argv[i], "-m"          )==0) pplset_session_default.colour = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-mono"       )==0) pplset_session_default.colour = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "--mono"      )==0) pplset_session_default.colour = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-monochrome" )==0) pplset_session_default.colour = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "--monochrome")==0) pplset_session_default.colour = SW_ONOFF_OFF;
    else if (strcmp(argv[i], "-"           )==0) context->willBeInteractive=2;
    else if ((strcmp(argv[i], "-v")==0) || (strcmp(argv[i], "-version")==0) || (strcmp(argv[i], "--version")==0))
     {
      printf("%s\n",ppltxt_version);
      if (DEBUG) ppl_log(&context->errcontext,"Reported version number as requested.");
      ppl_memAlloc_FreeAll(0); ppl_memAlloc_MemoryStop();
      return 0;
     }
    else if ((strcmp(argv[i], "-h")==0) || (strcmp(argv[i], "-help")==0) || (strcmp(argv[i], "--help")==0))
     {
      printf("%s\n",ppltxt_help);
      if (DEBUG) ppl_log(&context->errcontext,"Reported help text as requested.");
      ppl_memAlloc_FreeAll(0); ppl_memAlloc_MemoryStop();
      return 0;
     }
    else
    {
     sprintf(context->errcontext.tempErrStr, "Received switch '%s' which was not recognised. Type 'pyxplot -help' for a list of available commandline options.", argv[i]);
     ppl_error(&context->errcontext,ERR_PREFORMED, -1, -1, context->errcontext.tempErrStr);
     if (DEBUG) ppl_log(&context->errcontext,"Received unexpected commandline switch.");
     ppl_memAlloc_FreeAll(0); ppl_memAlloc_MemoryStop();
     return 1;
    }
   }

  // Decide upon a path for a temporary directory for us to live in
  if (DEBUG) ppl_log(&context->errcontext,"Finding a filepath for a temporary directory.");
  if (getcwd( pplset_session_default.cwd , FNAME_LENGTH ) == NULL) { ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not read current working directory."); } // Store cwd
  while (1) { sprintf(tempdirpath, "/tmp/pyxplot_%d_%d", getpid(), tempdirnumber); if (access(tempdirpath, F_OK) != 0) break; tempdirnumber++; } // Find an unused dir path
  strcpy(pplset_session_default.tempdir, tempdirpath); // Store our chosen temporary directory path

  // Launch child process
  if (DEBUG) ppl_log(&context->errcontext,"Launching the Child Support Process.");
  pplcsp_init(context);

  // Set up SIGINT handler
  if (sigsetjmp(ppl_sigjmpToMain, 1) == 0)
   {
    ppl_sigjmpFromSigInt = &ppl_sigjmpToMain;
    if (signal(SIGINT, SIG_IGN)!=SIG_IGN) signal(SIGINT, ppl_sigIntHandle);
    signal(SIGPIPE, SIG_IGN);

    // Wait for temporary directory to appear, and change directory into it
    if (DEBUG) ppl_log(&context->errcontext,"Waiting for temporary directory to appear.");
    strcpy(tempdirpath, pplset_session_default.tempdir);
    for (i=0; i<100; i++) { if (access(tempdirpath, F_OK) == 0) break; nanosleep(&waitperiod,&waitedperiod); } // Wait for temp dir to be created by child process
    if (access(tempdirpath, F_OK) != 0) { fail=1; } // If it never turns up, fail.
    else
     {
      stat(tempdirpath, &statinfo); // Otherwise stat it and make sure it's a directory we own
      if (!S_ISDIR(statinfo.st_mode)) fail=1;
      if (statinfo.st_uid != getuid()) fail=1;
     }
    if (fail==1)                { ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Failed to create temporary directory." ); }

    // Read GNU Readline history
#ifdef HAVE_READLINE
    if (DEBUG) ppl_log(&context->errcontext,"Reading GNU Readline history.");
    sprintf(tempdirpath, "%s%s%s", pplset_session_default.homedir, PATHLINK, ".pyxplot_history");
    read_history(tempdirpath);
    stifle_history(1000);
#endif

    // Scan commandline and process all script files we have been given
    for (i=1; i<argc; i++)
     {
      if (strlen(argv[i])==0) continue;
      if (argv[i][0]=='-')
       {
        if (argv[i][1]=='\0') ppl_interactiveSession(context);
        continue;
       }
      ppl_processScript(context, argv[i], 0);
     }
    if (context->willBeInteractive==1) ppl_interactiveSession(context);

   // SIGINT longjmps to main return here
   } else {
    sigemptyset(&sigs);
    sigaddset(&sigs,SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    ppl_error(&context->errcontext,ERR_PREFORMED, -1, -1, "\nReceived SIGINT. Terminating.");
   }

  // Notify the CSP that we are about to quit
  pplcsp_sendCommand(context,"B\n");

  // Free all of the plot styles which are set
  for (i=0; i<MAX_PLOTSTYLES; i++) ppl_withWordsDestroy(context, &(pplset_plot_styles        [i]));
  for (i=0; i<MAX_PLOTSTYLES; i++) ppl_withWordsDestroy(context, &(pplset_plot_styles_default[i]));

  // Save GNU Readline history
#ifdef HAVE_READLINE
  if (context->willBeInteractive>0)
   {
    if (DEBUG) ppl_log(&context->errcontext,"Saving GNU Readline history.");
    sprintf(tempdirpath, "%s%s%s", pplset_session_default.homedir, PATHLINK, ".pyxplot_history");
    write_history(tempdirpath);
   }
#endif

  // Terminate
  if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Main process could not unconfigure signal handler for SIGCHLD.");
  ppl_contextInit(context);
  ppl_memAlloc_FreeAll(0);
  ppl_memAlloc_MemoryStop();
  if (DEBUG) ppl_log(&context->errcontext,"Terminating normally.");
  return 0;
 }

void ppl_sigIntHandle(int signo)
 {
  sigjmp_buf *destination = ppl_sigjmpFromSigInt;
  ppl_sigjmpFromSigInt = NULL; // DO NOT recursively go back to the same sigint handler over and over again if it doesn't seem to work.
  if (destination != NULL) siglongjmp(*destination, 1);
  raise(SIGTERM);
  raise(SIGKILL);
 }

