// children.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "coreUtils/memAlloc.h"
#include "coreUtils/errorReport.h"

#include "settings/settingTypes.h"

#include "children.h"

// Pipes for communication between the main PyXPlot process and the CSP

int PipeCSP2MAIN[2]={0,0};
int PipeMAIN2CSP[2]={0,0};

char PipeOutputBuffer[LSTR_LENGTH] = "";

static char SIGTERM_NAME[16]; // The name of SIGTERM, which we filter out from GV's output, as it tends to whinge about being killed.

static ppl_context *static_context;

// Flags used to keep track of all of the open GhostView processes

#define PTABLE_LISTLEN 4096

static int  *GhostViews;                    // A list of X11_multiwindow and X11_singlewindow sessions which we kill on PyXPlot exit
static int  *GhostView_Persists;            // A list of X11_persist sessions for which we leave our temporary directory until they quit
static int  *HelperPIDs;                    // A list of helper processes forked by the main PyXPlot process
static int   GhostView_pid = 0;             // pid of any running gv process launched under X11_singlewindow
static int   PyXPlotRunning = 1;            // Flag which we drop in the CSP when the main process stops running

// Functions to be called from main PyXPlot process

void  pplcsp_init(ppl_context *context)
 {
  int pid, fail, i;
  struct stat statinfo;

  // Create empty lists for storing lists of GhostView processes
  GhostViews         = (int *)malloc(PTABLE_LISTLEN*sizeof(int));
  GhostView_Persists = (int *)malloc(PTABLE_LISTLEN*sizeof(int));
  HelperPIDs         = (int *)malloc(PTABLE_LISTLEN*sizeof(int));
  static_context     = context;

  if ((GhostViews==NULL)||(GhostView_Persists==NULL)||(HelperPIDs==NULL)) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Out of memory.");
  for (i=0; i<PTABLE_LISTLEN; i++) GhostViews        [i]=0;
  for (i=0; i<PTABLE_LISTLEN; i++) GhostView_Persists[i]=0;
  for (i=0; i<PTABLE_LISTLEN; i++) HelperPIDs        [i]=0;

  // The string "signal 15" we filter out of GhostView's output
  sprintf(SIGTERM_NAME, "signal %d", SIGTERM);

  // Create pipes for communication between main process and the CSP
  if (pipe(PipeCSP2MAIN) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not create a pipe.");
  if (pipe(PipeMAIN2CSP) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not create a pipe.");

  if      ((pid=fork()) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not fork a child process for the CSP.");
  else if ( pid        != 0)
   {
    close(PipeMAIN2CSP[0]); // Parent process; close CSP's ends of pipes
    //close(PipeCSP2MAIN[1]); // Leave this pipe open so that sed can return error messages down it
    if (signal(SIGCHLD, pplcsp_checkForHelperExits) == SIG_ERR) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Main process could not set up a signal handler for SIGCHLD.");
    return; // Parent process returns
   }

  // Close main process's ends of the pipes
  close(PipeCSP2MAIN[0]);
  close(PipeMAIN2CSP[1]);

  // Make all log messages appear to come from the CSP
  sprintf(context->errcontext.error_source, "CSP%6d", getpid());
  signal(SIGINT, SIG_IGN); // Ignore SIGINT
  if (setpgid( getpid() , getpid() ) < 0) if (DEBUG) ppl_log(&context->errcontext,"Failed to set process group ID."); // Make into a process group leader so that we won't catch SIGINT

  // Make temporary working directory
  fail=0;
  if ((mkdir(context->errcontext.session_default.tempdir , 0700) != 0) ||             // Create temporary working directory
      (access(context->errcontext.session_default.tempdir, F_OK) != 0) )  { fail=1; } // If temporary directory does not exist, fail.
  else
   {
    if (stat(context->errcontext.session_default.tempdir, &statinfo) <0) fail=1; // Otherwise stat it and make sure it's a directory we own
    if (!S_ISDIR(statinfo.st_mode)) fail=1;
    if (statinfo.st_uid != getuid()) fail=1;
   }
  if (fail==1)                                     { ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Failed to create temporary directory." ); }
  if (chdir(context->errcontext.session_default.tempdir) < 0) { ppl_fatal(&context->errcontext,__FILE__,__LINE__,"chdir into temporary directory failed."); } // chdir into temporary directory

  // Enter CSP execution loop
  if (signal(SIGCHLD, pplcsp_checkForChildExits) == SIG_ERR) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"CSP could not set up a signal handler for SIGCHLD.");
  pplcsp_main(context);
  close(PipeCSP2MAIN[1]); // Close pipes
  close(PipeMAIN2CSP[0]);
  if (chdir("/") < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"chdir out of temporary directory failed."); // chdir out of temporary directory
  sprintf(PipeOutputBuffer, "rm -Rf %s", context->errcontext.session_default.tempdir);
  if (system(PipeOutputBuffer) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Removal of temporary directory failed."); // Remove temporary directory
  ppl_memAlloc_FreeAll(0);
  if (DEBUG) ppl_log(&context->errcontext,"CSP terminating normally.");
  exit(0);
 }

void pplcsp_checkForGvOutput(ppl_context *context)
 {
  struct timespec waitperiod; // A time.h timespec specifier for a wait of zero seconds
  fd_set          readable;
  int             pos, TrialNumber;
  char            linebuffer[SSTR_LENGTH];

  TrialNumber=1;
  while (1)
   {
    waitperiod.tv_sec  = waitperiod.tv_nsec = 0;
    FD_ZERO(&readable); FD_SET(PipeCSP2MAIN[0], &readable);
    if (pselect(PipeCSP2MAIN[0]+1, &readable, NULL, NULL, &waitperiod, NULL) == -1)
     {
      if ((errno==EINTR) && (TrialNumber<3)) { TrialNumber++; continue; }
      if (DEBUG) ppl_log(&context->errcontext,"pselect failure whilst checking for GV output");
      return;
     }
    break;
   }
  if (!FD_ISSET(PipeCSP2MAIN[0] , &readable)) return; // select tells us that pipe from CSP is not readable

  pos = strlen(PipeOutputBuffer);
  if (read(PipeCSP2MAIN[0], PipeOutputBuffer+pos, LSTR_LENGTH-pos-5) > 0)
   while (PipeOutputBuffer[0]!='\0')
    {
     ppl_strRemoveCompleteLine(PipeOutputBuffer, linebuffer);
     if (linebuffer[0]=='\0') continue;
     if (strstr(linebuffer, SIGTERM_NAME)!=NULL) continue;
     if (strncmp(linebuffer, SED_COMMAND, strlen(SED_COMMAND))==0) ppl_error(&context->errcontext,ERR_GENERAL, -1, -1, "A problem was encounter with the supplied regular expression.");
     else ppl_error(&context->errcontext,ERR_GENERAL, -1, -1, linebuffer);
    }
  return;
 }

void pplcsp_sendCommand(ppl_context *context, char *cmd)
 {
  if (PipeMAIN2CSP[1]==0) return; // Pipe has not yet been opened (Ooops).
  if (write(PipeMAIN2CSP[1], cmd, strlen(cmd)) != strlen(cmd)) ppl_error(&context->errcontext,ERR_INTERNAL, -1, -1, "Attempt to send a message to the CSP failed.");
  return;
 }

// Functions to be called from the Child Support Process

void  pplcsp_main(ppl_context *context)
 {
  int gotPersists=1;

  while (gotPersists)
   {
    struct timespec waitperiod, waitedperiod;
    waitperiod.tv_sec  = 0;
    waitperiod.tv_nsec = PyXPlotRunning ? 100000000 : 1000000000; // After pyxplot has quit, reduce polling rate to once a second. Why not?
    nanosleep(&waitperiod,&waitedperiod); // Wake up every 100ms
    pplcsp_checkForNewCommands(context); // Check for orders from PyXPlot
    if (PyXPlotRunning && (getppid()==1)) // We've been orphaned and adopted by init
     {
      PyXPlotRunning=0;
      if (DEBUG) ppl_log(&context->errcontext,"CSP has detected that it has been orphaned.");
     }
    gotPersists=0;
    if (PyXPlotRunning)   gotPersists=1;
    else                { int i; for (i=0; i<PTABLE_LISTLEN; i++) if (GhostView_Persists[i]!=0) { gotPersists=1; break; } }
   }
  return;
 }

void pplcsp_checkForChildExits(int signo)
 {
  int           i;
  char          errtext[FNAME_LENGTH];
  ppl_context  *context = static_context;

  for (i=0; i<PTABLE_LISTLEN; i++)
   {
    const int gv_pid = GhostViews[i];
    if (gv_pid==0) continue;
    if (waitpid(gv_pid,NULL,WNOHANG) != 0)
     {
      if (DEBUG) { sprintf(errtext, "A ghostview process with pid %d has terminated.", gv_pid); ppl_log(&context->errcontext,errtext); }
      GhostViews[i]=0; // Stabat mater dolorosa
      if (GhostView_pid == gv_pid) GhostView_pid = 0;
     }
   }

  for (i=0; i<PTABLE_LISTLEN; i++)
   {
    const int gv_pid = GhostView_Persists[i];
    if (gv_pid==0) continue;
    if (waitpid(gv_pid,NULL,WNOHANG) != 0)
     {
      if (DEBUG) { sprintf(errtext, "A persistent ghostview process with pid %d has terminated.", gv_pid); ppl_log(&context->errcontext,errtext); }
      GhostView_Persists[i]=0; // Stabat mater dolorosa
      if (GhostView_pid == gv_pid) GhostView_pid = 0;
     }
   }
  return;
 }

void pplcsp_checkForNewCommands(ppl_context *context)
 {
  struct timespec waitperiod; // A time.h timespec specifier for a wait of zero seconds
  fd_set          readable;
  int             pos, TrialNumber;
  char linebuffer[SSTR_LENGTH];

  TrialNumber=1;
  while (1)
   {
    waitperiod.tv_sec  = waitperiod.tv_nsec = 0;
    FD_ZERO(&readable); FD_SET(PipeMAIN2CSP[0], &readable);
    if (pselect(PipeMAIN2CSP[0]+1, &readable, NULL, NULL, &waitperiod, NULL) == -1)
     {
      if ((errno==EINTR) && (TrialNumber<3)) { TrialNumber++; continue; }
      if (DEBUG) ppl_log(&context->errcontext,"pselect failure whilst CSP checking for new commands");
      return;
     }
    break;
   }
  if (!FD_ISSET(PipeMAIN2CSP[0] , &readable)) return; // select tells us that pipe from main process is not readable

  pos = strlen(PipeOutputBuffer);
  if (read(PipeMAIN2CSP[0], PipeOutputBuffer+pos, LSTR_LENGTH-pos-5) > 0)
   while (1)
    {
     ppl_strRemoveCompleteLine(PipeOutputBuffer, linebuffer);
     if (linebuffer[0]=='\0') break;
     pplcsp_processCommand(context, linebuffer);
    }
  return;
 }

void pplcsp_processCommand(ppl_context *context, char *in)
 {
  char cmd[FNAME_LENGTH];

  if      (in[0]=='\0') return;
  else if (in[0]=='A')                                 // clear command executed
   {
    if (DEBUG) ppl_log(&context->errcontext,"Received request to clear display.");
    pplcsp_killLatestSinglewindow(context);
   }
  else if (in[0]=='B')                                 // PyXPlot quit
   {
    if (DEBUG) ppl_log(&context->errcontext,"Received notice that PyXPlot is quitting.");
    pplcsp_killAllGvs(context);
    PyXPlotRunning=0;
   }
  else if (in[0]=='0')                                // gv_singlewindow
   {
    // Pick out filename of eps file to display, which is last commandline argument
    int state=0,i; char *filename=in+1;
    for (i=1; in[i]!='\0'; i++)
      if ( ((in[i]>' ')||(in[i]<'\0')) || ((i>1)&&(in[i-1]=='\\')) )
       {
        if (!state) { filename = in+i; state=1; } // New word
       }
      else state=0;

    if (GhostView_pid > 1)
     {
      if (DEBUG) { sprintf(context->errcontext.tempErrStr, "Received gv_singlewindow request. Putting into existing window with pid %d.", GhostView_pid); ppl_log(&context->errcontext,NULL); }
      sprintf(cmd, "cp -f %s %s", filename, context->pplcsp_ghostView_fname);
      if (system(cmd) != 0) if (DEBUG) { ppl_log(&context->errcontext,"Failed to copy postscript document into existing gv_singlewindow session.");}
     }
    else
     {
      if (DEBUG) ppl_log(&context->errcontext,"Received gv_singlewindow request. Making a new window for it.");
      strcpy(context->pplcsp_ghostView_fname, filename);
      GhostView_pid = pplcsp_forkNewGv(context, in+1, GhostViews);
     }
   }
  else if (in[0]=='1')                                // gv_multiwindow
   {
    if (DEBUG) ppl_log(&context->errcontext,"Received gv_multiwindow request.");
    pplcsp_forkNewGv(context, in+1, GhostViews);
   }
  else if (in[0]=='2')                                // gv_persist
   {
    if (DEBUG) ppl_log(&context->errcontext,"Received gv_persist request.");
    pplcsp_forkNewGv(context, in+1, GhostView_Persists);
   }
  return;
 }

int pplcsp_forkNewGv(ppl_context *context, char *fname, int *gv_list)
 {

#define MAX_CMDARGS 64

  int pid, i, state=0, NArgs=0;
  char *Args[MAX_CMDARGS], ViewerApp[FNAME_LENGTH];
  sigset_t sigs;

  // Split up commandline into words
  for (i=0; fname[i]!='\0'; i++)
   {
    if ( ((fname[i]>' ')||(fname[i]<'\0')) || ((i>1)&&(fname[i-1]=='\\')) )
     {
      if (state) {} // In the middle of a word... keep going
      else       { Args[NArgs++] = fname+i; state=1; } // New word
     }
    else
     {
      if (state) { fname[i]='\0'; state=0; } // End of word
      else       {} // Lots of whitespace
     }

    if (NArgs==MAX_CMDARGS)
     {
      FILE *f = fdopen(PipeCSP2MAIN[1], "w");
      if (f==NULL) return 0;
      fprintf(f, "Command for launching postscript viewer contains too many commandline switches.\n");
      fclose(f);
      return 0;
     }
   }
  Args[NArgs]=NULL;

  snprintf(ViewerApp, FNAME_LENGTH, "%s", Args[0]);
  ViewerApp[FNAME_LENGTH-1]='\0';

  sigemptyset(&sigs);
  sigaddset(&sigs,SIGCHLD);
  sigprocmask(SIG_BLOCK, &sigs, NULL);

  if      ((pid=fork()) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not fork a child process for the CSP.");
  else if ( pid        != 0)
   {
    // Parent process (i.e. the CSP)
    int i;
    for (i=0; i<PTABLE_LISTLEN; i++) if (gv_list[i]==0) { gv_list[i]=pid; break; }
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    return pid;
   }
  else
   {
    // Child process; about to run postscript viewer
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    sprintf(context->errcontext.error_source, "GV%7d", getpid());
    context->errcontext.session_default.color = SW_ONOFF_OFF;
    if (DEBUG) { sprintf(context->errcontext.tempErrStr, "New postscript viewer process alive; going to view %s.", fname); ppl_log(&context->errcontext,NULL); }
    if (PipeCSP2MAIN[1] != STDERR_FILENO) // Redirect stderr to pipe, so that GhostView doesn't spam terminal
     {
      if (dup2(PipeCSP2MAIN[1], STDERR_FILENO) != STDERR_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stderr to pipe.");
      close(PipeCSP2MAIN[1]);
     }
    if (setpgid( getpid() , getpid() )) if (DEBUG) ppl_log(&context->errcontext,"Failed to set process group ID."); // Make into a process group leader so that we won't catch SIGINT

    // Run postscript viewer
    if (execvp(Args[0],Args)!=0) { if (DEBUG) ppl_log(&context->errcontext,"Attempt to execute postscript viewer returned error code."); } // Execute postscript viewer
    fprintf(stderr, "Execution of postscript viewer '%s' failed.\n", ViewerApp); fflush(stderr); // execvp call should not return
    exit(1);
   }
  return 0;
 }

void pplcsp_killAllGvs(ppl_context *context)
 {
  int i;
  if (DEBUG) ppl_log(&context->errcontext,"Killing all ghostview processes.");
  for (i=0; i<PTABLE_LISTLEN; i++)
   {
    const int gv_pid = GhostViews[i];
    if (gv_pid==0) continue;
    kill(gv_pid, SIGTERM); // Dulce et decorum est pro patria mori
    if (GhostView_pid == gv_pid) GhostView_pid = 0;
   }
  return;
 }

void pplcsp_killLatestSinglewindow(ppl_context *context)
 {
  if (GhostView_pid > 1)
   {
    if (DEBUG) { sprintf(context->errcontext.tempErrStr, "Killing latest ghostview singlewindow process with pid %d.", GhostView_pid); ppl_log(&context->errcontext,NULL); }
    kill(GhostView_pid, SIGTERM);
   }
  else
   {
    if (DEBUG) { sprintf(context->errcontext.tempErrStr, "No ghostview singlewindow process to kill."); ppl_log(&context->errcontext,NULL); }
   }
  GhostView_pid = 0;
  return;
 }

// Facilities for forking helper processes with pipes from the main PyXPlot process

void pplcsp_checkForHelperExits(int signo)
 {
  int           i;
  char          errtext[256];
  ppl_context  *context = static_context;

  for (i=0; i<PTABLE_LISTLEN; i++)
   {
    const int pid = HelperPIDs[i];
    if (pid==0) continue;
    if (waitpid(pid,NULL,WNOHANG) != 0)
     {
      if (DEBUG) { sprintf(errtext, "A helper process with pid %d has terminated.", pid); ppl_log(&context->errcontext,errtext); }
      HelperPIDs[i]=0; // Stabat mater dolorosa
     }
   }
  return;
 }

void pplcsp_killAllHelpers(ppl_context *context)
 {
  int      i;
  sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs,SIGCHLD);
  sigprocmask(SIG_BLOCK, &sigs, NULL);
  for (i=0; i<PTABLE_LISTLEN; i++)
   {
    const int pid = HelperPIDs[i];
    if (pid==0) continue;
    kill(pid, SIGTERM); // Dulce et decorum est pro patria mori
   }
  sigprocmask(SIG_UNBLOCK, &sigs, NULL);
  return;
 }

// NB: Leaves SIGCHLD blocked
void pplcsp_forkSed(ppl_context *context, char *cmd, int *fstdin, int *fstdout)
 {
  int fd0[2], fd1[2];
  int i,pid;
  sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs,SIGCHLD);

  if ((pipe(fd0)<0) || (pipe(fd1)<0)) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not open required pipes.");

  sigprocmask(SIG_BLOCK, &sigs, NULL);

  if      ((pid=fork()) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not fork a child process for sed process.");
  else if ( pid        != 0)
   {
    // Parent process
    close(fd0[0]); *fstdin  = fd0[1];
    close(fd1[1]); *fstdout = fd1[0];
    for (i=0; i<PTABLE_LISTLEN; i++) if (HelperPIDs[i]==0) { HelperPIDs[i]=pid; break; }
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    return;
   }
  else
   {
    // Child process
    close(fd0[1]); close(fd1[0]);
    close(PipeCSP2MAIN[0]);
    close(PipeMAIN2CSP[1]);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    sprintf(context->errcontext.error_source, "SED%6d", getpid());
    context->errcontext.session_default.color = SW_ONOFF_OFF;
    if (DEBUG) { sprintf(context->errcontext.tempErrStr, "New sed process alive; going to run command \"%s\".", cmd); ppl_log(&context->errcontext,NULL); }
    if (fd0[0] != STDIN_FILENO) // Redirect stdin to pipe
     {
      if (dup2(fd0[0], STDIN_FILENO) != STDIN_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stdin to pipe.");
      close(fd0[0]);
     }
    if (fd1[1] != STDOUT_FILENO) // Redirect stdout to pipe
     {
      if (dup2(fd1[1], STDOUT_FILENO) != STDOUT_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stdout to pipe.");
      close(fd1[1]);
     }
    if (PipeCSP2MAIN[1] != STDERR_FILENO) // Redirect stderr to pipe
     {
      if (dup2(PipeCSP2MAIN[1], STDERR_FILENO) != STDERR_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stderr to pipe.");
      close(PipeCSP2MAIN[1]);
     }
    if (execl(SED_COMMAND, SED_COMMAND, cmd, NULL)!=0) if (DEBUG) ppl_log(&context->errcontext,"Attempt to execute sed returned error code."); // Execute sed
    ppl_error(&context->errcontext,ERR_GENERAL, -1, -1, "Execution of helper process 'sed' failed."); // execlp call should not return
    exit(1);
   }
  return;
 }

// NB: Leaves SIGCHLD blocked
void pplcsp_forkLaTeX(ppl_context *context, char *filename, int *PidOut, int *fstdin, int *fstdout)
 {
  int fd0[2], fd1[2];
  int i,pid;
  sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs,SIGCHLD);

  if ((pipe(fd0)<0) || (pipe(fd1)<0)) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not open required pipes.");

  sigprocmask(SIG_BLOCK, &sigs, NULL);

  if      ((pid=fork()) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not fork a child process for latex process.");
  else if ( pid        != 0)
   {
    // Parent process
    close(fd0[0]); *fstdin  = fd0[1];
    close(fd1[1]); *fstdout = fd1[0];
    *PidOut = pid;
    for (i=0; i<PTABLE_LISTLEN; i++) if (HelperPIDs[i]==0) { HelperPIDs[i]=pid; break; }
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    return;
   }
  else
   {
    // Child process
    close(fd0[1]); close(fd1[0]);
    close(PipeCSP2MAIN[0]);
    close(PipeMAIN2CSP[1]);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    sprintf(context->errcontext.error_source, "TEX%6d", getpid());
    context->errcontext.session_default.color = SW_ONOFF_OFF;
    if (DEBUG) { sprintf(context->errcontext.tempErrStr, "New latex process alive; going to latex file \"%s\".", filename); ppl_log(&context->errcontext,NULL); }
    if (fd0[0] != STDIN_FILENO) // Redirect stdin to pipe
     {
      if (dup2(fd0[0], STDIN_FILENO) != STDIN_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stdin to pipe.");
      close(fd0[0]);
     }
    if (fd1[1] != STDOUT_FILENO) // Redirect stdout to pipe
     {
      if (dup2(fd1[1], STDOUT_FILENO) != STDOUT_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stdout to pipe.");
     }
    if (PipeCSP2MAIN[1] != STDERR_FILENO) // Redirect stderr to pipe
     {
      if (dup2(fd1[1], STDERR_FILENO) != STDERR_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stderr to pipe.");
      close(fd1[1]);
     }
    if (execl(LATEX_COMMAND, LATEX_COMMAND, "-file-line-error", filename, NULL)!=0) if (DEBUG) ppl_log(&context->errcontext,"Attempt to execute latex returned error code."); // Execute latex
    ppl_error(&context->errcontext,ERR_GENERAL, -1, -1, "Execution of helper process 'latex' failed."); // execlp call should not return
    exit(1);
   }
  return;
 }

// NB: Leaves SIGCHLD blocked
void pplcsp_forkInputFilter(ppl_context *context, char **cmd, int *fstdout)
 {
  int fd0[2];
  int i,pid;
  sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs,SIGCHLD);

  if (pipe(fd0)<0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not open required pipes.");

  sigprocmask(SIG_BLOCK, &sigs, NULL);

  if      ((pid=fork()) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not fork a child process for input filter process.");
  else if ( pid        != 0)
   {
    // Parent process
    close(fd0[1]); *fstdout = fd0[0];
    for (i=0; i<PTABLE_LISTLEN; i++) if (HelperPIDs[i]==0) { HelperPIDs[i]=pid; break; }
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    return;
   }
  else
   {
    // Child process
    close(fd0[0]);
    close(PipeCSP2MAIN[0]);
    close(PipeMAIN2CSP[1]);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    sprintf(context->errcontext.error_source, "IF %6d", getpid());
    context->errcontext.session_default.color = SW_ONOFF_OFF;
    if (DEBUG) { sprintf(context->errcontext.tempErrStr, "New input filter process alive; going to run command \"%s\".", cmd[0]); ppl_log(&context->errcontext,NULL); }
    if (fd0[1] != STDOUT_FILENO) // Redirect stdout to pipe
     {
      if (dup2(fd0[1], STDOUT_FILENO) != STDOUT_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stdout to pipe.");
      close(fd0[1]);
     }
    if (PipeCSP2MAIN[1] != STDERR_FILENO) // Redirect stderr to pipe
     {
      if (dup2(PipeCSP2MAIN[1], STDERR_FILENO) != STDERR_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stderr to pipe.");
      close(PipeCSP2MAIN[1]);
     }
    if (execvp(cmd[0], cmd)!=0) if (DEBUG) ppl_log(&context->errcontext,"Attempt to execute input filter returned error code."); // Execute input filter
    sprintf(context->errcontext.tempErrStr, "Execution of input filter '%s' failed.", cmd[0]);
    ppl_error(&context->errcontext,ERR_GENERAL, -1, -1, NULL); // execvp call should not return
    exit(1);
   }
  return;
 }

// NB: Leaves SIGCHLD blocked
void pplcsp_forkKpseWhich(ppl_context *context, const char *ftype, int *fstdout)
 {
  char CmdLineOpt[128];
  int fd0[2];
  int i,pid;
  sigset_t sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs,SIGCHLD);

  if (pipe(fd0)<0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not open required pipes.");

  sigprocmask(SIG_BLOCK, &sigs, NULL);

  if      ((pid=fork()) < 0) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not fork a child process for kpsewhich process.");
  else if ( pid        != 0)
   {
    // Parent process
    close(fd0[1]); *fstdout = fd0[0];
    for (i=0; i<PTABLE_LISTLEN; i++) if (HelperPIDs[i]==0) { HelperPIDs[i]=pid; break; }
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    return;
   }
  else
   {
    // Child process
    close(fd0[0]);
    close(PipeCSP2MAIN[0]);
    close(PipeMAIN2CSP[1]);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
    sprintf(context->errcontext.error_source, "KPS%6d", getpid());
    context->errcontext.session_default.color = SW_ONOFF_OFF;
    if (DEBUG) { sprintf(context->errcontext.tempErrStr, "New kpsewhich process alive; going to get paths for filetype \"%s\".", ftype); ppl_log(&context->errcontext,NULL); }
    if (fd0[1] != STDOUT_FILENO) // Redirect stdout to pipe
     {
      if (dup2(fd0[1], STDOUT_FILENO) != STDOUT_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stdout to pipe.");
      close(fd0[1]);
     }
    if (PipeCSP2MAIN[1] != STDERR_FILENO) // Redirect stderr to pipe
     {
      if (dup2(PipeCSP2MAIN[1], STDERR_FILENO) != STDERR_FILENO) ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Could not redirect stderr to pipe.");
      close(PipeCSP2MAIN[1]);
     }
    sprintf(CmdLineOpt, "-show-path=.%s", ftype);
    if (execl(KPSE_COMMAND, KPSE_COMMAND, CmdLineOpt, NULL)!=0) if (DEBUG) ppl_log(&context->errcontext,"Attempt to execute kpsewhich returned error code."); // Execute kpsewhich
    ppl_error(&context->errcontext,ERR_GENERAL, -1, -1, "Execution of helper process 'kpsewhich' failed."); // execlp call should not return
    exit(1);
   }
  return;
 }

