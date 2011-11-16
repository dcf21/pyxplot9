// children.h
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

#ifndef _CHILDREN_H
#define _CHILDREN_H 1

#include "coreUtils/list.h"
#include "userspace/context.h"

// Functions to be called from main PyXPlot process

void  pplcsp_init                  (ppl_context *context);
void  pplcsp_checkForGvOutput      (ppl_context *context);
void  pplcsp_sendCommand           (ppl_context *context, char *cmd);

// Functions to be called from the Child Support Process

void  pplcsp_main                  (ppl_context *context);
void  pplcsp_checkForChildExits    (int signo);
void  pplcsp_checkForNewCommands   (ppl_context *context);
void  pplcsp_processCommand        (ppl_context *context, char *in);
int   pplcsp_forkNewGv             (ppl_context *context, char *fname, list *gv_list);
void  pplcsp_killAllGvs            (ppl_context *context);
void  pplcsp_killLatestSinglewindow(ppl_context *context);

// Functions for spawning helper processes

void  pplcsp_checkForHelperExits   (int signo);
void  pplcsp_killAllHelpers        (ppl_context *context);
void  pplcsp_forkSed               (ppl_context *context, char *cmd, int *fstdin, int *fstdout);
void  pplcsp_forkLaTeX             (ppl_context *context, char *filename, int *PidOut, int *fstdin, int *fstdout);
void  pplcsp_forkInputFilter       (ppl_context *context, char **cmd, int *fstdout);
void  pplcsp_forkKpseWhich         (ppl_context *context, const char *ftype, int *fstdout);

#endif

