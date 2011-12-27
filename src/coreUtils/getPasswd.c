// ppl_passwd.c
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

// Functions for getting information out of /etc/passwd

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "coreUtils/errorReport.h"
#include "pplConstants.h"

struct passwd *UnixGetPwEntry(pplerr_context *context)
 {
  int uid;
  struct passwd *ptr;

  uid = getuid();
  setpwent(); // Memory leak which valgrind reveals here is probably okay; getpwent sets up a static variable with malloc
  while ((ptr = getpwent()) != NULL)
   if (ptr->pw_uid == uid) break;
  endpwent();
  return(ptr);
 }

char *ppl_unixGetHomeDir(pplerr_context *context)
 {
  struct passwd *ptr;
  ptr = UnixGetPwEntry(context);
  if (ptr==NULL) ppl_fatal(context,__FILE__,__LINE__,"Could not find user's entry in /etc/passwd file.");
  return ptr->pw_dir;
 }

char *ppl_unixGetUserHomeDir(pplerr_context *context,char *username)
 {
  struct passwd *ptr;
  setpwent(); // Ditto memory leak comment above
  while ((ptr = getpwent()) != NULL)
   if (strcmp(ptr->pw_name , username) == 0) break;
  endpwent();
  if (ptr==NULL) return NULL;
  return ptr->pw_dir;
 }

char *ppl_unixGetIRLName(pplerr_context *context)
 {
  struct passwd *ptr;
  int i;
  ptr = UnixGetPwEntry(context);
  if (ptr==NULL) ppl_fatal(context,__FILE__,__LINE__,"Could not find user's entry in /etc/passwd file.");
  strcpy(context->tempErrStr, ptr->pw_gecos);
  for (i=0; context->tempErrStr[i]!='\0'; i++) if (context->tempErrStr[i]==',') context->tempErrStr[i]='\0'; // Remove commas from in-real-life name
  return context->tempErrStr;
 }

// Expand out filenames line ~dcf21/script.ppl
char *ppl_unixExpandUserHomeDir(pplerr_context *context,char *in, char *cwd, char *out)
 {
  char UserName[1024];
  char *scan_in, *scan_out;
  if      (in[0] == '/') strcpy (out, in);
  else if (in[0] != '~') sprintf(out, "%s%s%s", cwd, PATHLINK, in);
  else if (in[1] == '/') sprintf(out, "%s%s%s", ppl_unixGetHomeDir(context), PATHLINK, in+2);
  else
   {
    for ( scan_in=in+1,scan_out=UserName ; ((*scan_in!='/') && (*scan_in!='\0')) ; *(scan_out++)=*(scan_in++) );
    *scan_out='\0';
    scan_out = ppl_unixGetUserHomeDir(context,UserName);
    if (scan_out != NULL) sprintf(out, "%s%s", scan_out, scan_in);
    else                  strcpy (out, in);
   }
  return out;
 }

