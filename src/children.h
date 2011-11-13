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

extern char pplcsp_ghostView_fname[];

// Functions to be called from main PyXPlot process

void  pplcsp_init                  ();
void  pplcsp_checkForGvOutput      ();
void  pplcsp_sendCommand           (char *cmd);

// Functions to be called from the Child Support Process

void  pplcsp_main                  ();
void  pplcsp_checkForChildExits    (int signo);
void  pplcsp_checkForNewCommands   ();
void  pplcsp_processCommand        (char *in);
int   pplcsp_forkNewGv             (char *fname, list *gv_list);
void  pplcsp_killAllGvs            ();
void  pplcsp_killLatestSinglewindow();

// Functions for spawning helper processes

void  pplcsp_checkForHelperExits   (int signo);
void  pplcsp_killAllHelpers        ();
void  pplcsp_forkSed               (char *cmd, int *fstdin, int *fstdout);
void  pplcsp_forkLaTeX             (char *filename, int *PidOut, int *fstdin, int *fstdout);
void  pplcsp_forkInputFilter       (char **cmd, int *fstdout);
void  pplcsp_forkKpseWhich         (const char *ftype, int *fstdout);

#endif

