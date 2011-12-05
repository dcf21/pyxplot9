// moduleOs.c
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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "coreUtils/dict.h"
#include "coreUtils/getPasswd.h"
#include "settings/settings.h"
#include "stringTools/asciidouble.h"

#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"

#include "defaultObjs/moduleOs.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

void pplfunc_osGetEgid (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjNum(&OUTPUT,0,getegid(),0);
 }

void pplfunc_osGetEuid (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjNum(&OUTPUT,0,geteuid(),0);
 }

void pplfunc_osGetGid  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjNum(&OUTPUT,0,getgid(),0);
 }

void pplfunc_osGetPid  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjNum(&OUTPUT,0,getpid(),0);
 }

void pplfunc_osGetPgrp (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjNum(&OUTPUT,0,getpgrp(),0);
 }

void pplfunc_osGetPpid (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjNum(&OUTPUT,0,getppid(),0);
 }

void pplfunc_osGetUid  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjNum(&OUTPUT,0,getuid(),0);
 }

void pplfunc_osGetHome(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *out,*tmp;
  tmp = ppl_unixGetHomeDir(&c->errcontext);
  out = (char *)malloc(strlen(tmp)+1);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); }
  strcpy(out, tmp);
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

void pplfunc_osGetHost(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  const int outlen=FNAME_LENGTH;
  char *out;
  out = (char *)malloc(outlen);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); }
  gethostname(out,outlen);
  out[outlen-1]='\0';
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

void pplfunc_osGetLogin(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {   
  char *out,*tmp;
  tmp = getlogin();
  out = (char *)malloc(strlen(tmp)+1);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); }
  strcpy(out, tmp);
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

void pplfunc_osGetRealName(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *out,*tmp;
  tmp = ppl_unixGetIRLName(&c->errcontext);
  out = (char *)malloc(strlen(tmp)+1);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); }
  strcpy(out, tmp);
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

void pplfunc_osSystem(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.system() function requires a single string argument."); return; }
  pplObjNum(&OUTPUT,0,  system((char*)in[0].auxil)  ,0);
 }

// Path operations

void pplfunc_osPathExists(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.exists() function requires a single string argument."); return; }
  pplObjBool(&OUTPUT,0,!access((char*)in[0].auxil , F_OK));
 }

void pplfunc_osPathFilesize(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  struct stat buffer;
  int         st;

  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.filesize() function requires a single string argument."); return; }
  st = stat((char*)in[0].auxil, &buffer);
  if (st) { pplObjNull(&OUTPUT,0); return; }
  pplObjNum(&OUTPUT,0,buffer.st_size*8,0);
  CLEANUP_APPLYUNIT(UNIT_BIT);
 }

void pplfunc_osPathATime(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  struct stat buffer;
  int         st;

  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.atime() function requires a single string argument."); return; }
  st = stat((char*)in[0].auxil, &buffer);
  if (st) { pplObjNull(&OUTPUT,0); return; }
  pplObjDate(&OUTPUT,0,buffer.st_atime);
 }

void pplfunc_osPathCTime(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  struct stat buffer;
  int         st;

  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.ctime() function requires a single string argument."); return; }
  st = stat((char*)in[0].auxil, &buffer);
  if (st) { pplObjNull(&OUTPUT,0); return; }
  pplObjDate(&OUTPUT,0,buffer.st_ctime);
 }

void pplfunc_osPathMTime(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  struct stat buffer;
  int         st;

  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.mtime() function requires a single string argument."); return; }
  st = stat((char*)in[0].auxil, &buffer);
  if (st) { pplObjNull(&OUTPUT,0); return; }
  pplObjDate(&OUTPUT,0,buffer.st_mtime);
 }

void pplfunc_osPathExpandUser(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  const int outlen=LSTR_LENGTH;
  char *out;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.expanduser() function requires a single string argument."); return; }
  out = (char *)malloc(outlen);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); }
  ppl_unixExpandUserHomeDir(&c->errcontext, (char*)in[0].auxil , c->errcontext.session_default.cwd , out);
  out[outlen-1]='\0';
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

void pplfunc_osPathJoin(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  const int outstep=FNAME_LENGTH;
  int outlen=outstep, newlen, j=0;
  char *out;
  if (nArgs<1) return;
  out = (char *)malloc(outlen);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); }
  if (in[0].objType==PPLOBJ_LIST)
   {
    listIterator *iter = ppl_listIterateInit((list *)in[0].auxil);
    pplObj       *item;
    int           i=0;
    if (nArgs>1) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.join() function must be called with a single list of strings."); return; }
    out[0]='\0';
    while ((item = (pplObj *)ppl_listIterate(&iter))!=NULL)
     {
      if (item->objType!=PPLOBJ_STR) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.join() function must be passed a list of string; input %d has type <%s>.",i+1,pplObjTypeNames[item->objType]); return; }
      if (i>0) { strcpy(out+j,PATHLINK); j+=strlen(out+j); }
      if (strncmp((char*)item->auxil,PATHLINK,strlen(PATHLINK))==0) j=0;
      newlen=j+item->auxilLen;
      if (newlen > outlen-10) { char *newout=(char *)realloc((void *)out,outlen+=outstep); if (newout==NULL) { free(out); *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); } out=newout; }
      strcpy(out+j, (char *)item->auxil);
      j+=strlen(out+j);
      while ((j>0)&&(out[j-1]==PATHLINK[0])) out[--j]='\0';
      i++;
     }
   }
  else
   {
    int i;
    for (i=0; i<nArgs; i++)
     {
      if (in[i].objType!=PPLOBJ_STR) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.path.join() function must be passed a list of string; input %d has type <%s>.",i+1,pplObjTypeNames[in[i].objType]); return; }
      if (i>0) { strcpy(out+j,PATHLINK); j+=strlen(out+j); }
      if (strncmp((char*)in[i].auxil,PATHLINK,strlen(PATHLINK))==0) j=0;
      newlen=j+in[i].auxilLen;
      if (newlen > outlen-10) { char *newout=(char *)realloc((void *)out,outlen+=outstep); if (newout==NULL) { free(out); *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); } out=newout; }
      strcpy(out+j, (char *)in[i].auxil);
      j+=strlen(out+j);
      while ((j>0)&&(out[j-1]==PATHLINK[0])) out[--j]='\0';
     }
   }
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

