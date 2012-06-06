// moduleOs.c
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
#include <errno.h>
#include <glob.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>

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

#define COPYSTR(X,Y) \
 { \
  X = (char *)malloc(strlen(Y)+1); \
  if (X==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; } \
  strcpy(X, Y); \
 }

void pplfunc_osChdir  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.chdir() function requires a single string argument."); return; }
  if (chdir((char*)in[0].auxil)!=0) { *status=1; *errType=ERR_FILE; sprintf(errText,"The os.chdir() encountered an error: %s",strerror(errno)); return; }
  strncpy(c->errcontext.session_default.cwd , (char*)in[0].auxil , FNAME_LENGTH);
  c->errcontext.session_default.cwd[FNAME_LENGTH-1]='\0';
 }

void pplfunc_osGetCwd  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *tmp;
  COPYSTR(tmp, c->errcontext.session_default.cwd);
  pplObjStr(&OUTPUT,0,1,tmp);
 }

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
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  strcpy(out, tmp);
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

void pplfunc_osGetHost(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  const int outlen=FNAME_LENGTH;
  char *out;
  out = (char *)malloc(outlen);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
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
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  strcpy(out, tmp);
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

void pplfunc_osGetRealName(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *out,*tmp;
  tmp = ppl_unixGetIRLName(&c->errcontext);
  out = (char *)malloc(strlen(tmp)+1);
  if (out==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  strcpy(out, tmp);
  pplObjStr(&OUTPUT,0,1,out);
  return;
 }

void pplfunc_osGlob(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  wordexp_t  w;
  glob_t     g;
  pplObj     v;
  list      *l;
  char      *tmp;
  int        i,j;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.glob() function requires a single string argument."); return; }
  if (pplObjList(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  v.refCount=1;
  l = (list *)OUTPUT.auxil;
  if (wordexp((char*)in[0].auxil, &w, 0) != 0) return; // No matches; return empty list
  for (i=0; i<w.we_wordc; i++)
   {
    if (glob(w.we_wordv[i], 0, NULL, &g) != 0) continue;
    for (j=0; j<g.gl_pathc; j++)
     {
      COPYSTR(tmp,g.gl_pathv[j]);
      pplObjStr(&v,1,1,tmp);
      ppl_listAppendCpy(l, &v, sizeof(v));
     }
    globfree(&g);
   }
  wordfree(&w);
  return;
 }

void pplfunc_osPopen(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  FILE *f;
  char *mode;
  if ((nArgs<1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The open() function requires string arguments."); return; }
  if ((nArgs>1)&&(in[1].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The open() function requires string arguments."); return; }
  if (nArgs>1) mode=(char*)in[1].auxil;
  else         mode="r";
  f = popen((char*)in[0].auxil,mode);
  if (f==NULL) { *status=1; *errType=ERR_FILE; strcpy(errText, strerror(errno)); return; }
  pplObjFile(&OUTPUT,0,1,f,1);
 }

void pplfunc_osStat(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *tmp, permissions[]="---------";
  dict *d;
  pplObj v;
  struct stat s;
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.stat() function requires a single string argument."); return; }
  if (stat((char*)in[0].auxil,&s)!=0) { pplObjNull(&OUTPUT,0); return; }
  v.refCount=1;
  tmp = (char *)malloc(LSTR_LENGTH);
  if (pplObjDict(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  d = (dict *)OUTPUT.auxil;
  pplObjDate(&v,1,s.st_atime   ); ppl_dictAppendCpy(d, "atime", &v, sizeof(v));
  pplObjNum (&v,1,s.st_gid   ,0); ppl_dictAppendCpy(d, "gid"  , &v, sizeof(v));
  pplObjDate(&v,1,s.st_ctime   ); ppl_dictAppendCpy(d, "ctime", &v, sizeof(v));
  pplObjNum (&v,1,s.st_ino   ,0); ppl_dictAppendCpy(d, "ino"  , &v, sizeof(v));
  pplObjNum (&v,1,s.st_mode  ,0); ppl_dictAppendCpy(d, "mode" , &v, sizeof(v));
  pplObjDate(&v,1,s.st_mtime   ); ppl_dictAppendCpy(d, "mtime", &v, sizeof(v));
  pplObjNum (&v,1,s.st_nlink ,0); ppl_dictAppendCpy(d, "nlink", &v, sizeof(v));
  pplObjNum (&v,1,s.st_size*8,0); v.exponent[UNIT_BIT]=1; v.dimensionless=0; ppl_dictAppendCpy(d, "size", &v, sizeof(v));
  pplObjNum (&v,1,s.st_uid   ,0); ppl_dictAppendCpy(d, "uid"  , &v, sizeof(v));
  if      (S_ISREG (s.st_mode)) { COPYSTR(tmp,"regular file"); }
  else if (S_ISDIR (s.st_mode)) { COPYSTR(tmp,"directory"); }
  else if (S_ISCHR (s.st_mode)) { COPYSTR(tmp,"character special"); }
  else if (S_ISBLK (s.st_mode)) { COPYSTR(tmp,"block special"); }
  else if (S_ISFIFO(s.st_mode)) { COPYSTR(tmp,"fifo"); }
  else if (S_ISLNK (s.st_mode)) { COPYSTR(tmp,"symbolic link"); }
  else if (S_ISSOCK(s.st_mode)) { COPYSTR(tmp,"socket"); }
  else                          { COPYSTR(tmp,"other"); }
  pplObjStr(&v,1,1,tmp); ppl_dictAppendCpy(d, "type", &v, sizeof(v));
  if (s.st_mode & S_IRUSR) permissions[0] = 'r';
  if (s.st_mode & S_IWUSR) permissions[1] = 'w';
  if (s.st_mode & S_IXUSR) permissions[2] = 'x';
  if (s.st_mode & S_IRGRP) permissions[3] = 'r';
  if (s.st_mode & S_IWGRP) permissions[4] = 'w';
  if (s.st_mode & S_IXGRP) permissions[5] = 'x';
  if (s.st_mode & S_IROTH) permissions[6] = 'r';
  if (s.st_mode & S_IWOTH) permissions[7] = 'w';
  if (s.st_mode & S_IXOTH) permissions[8] = 'x';
  COPYSTR(tmp,permissions); pplObjStr(&v,1,1,tmp); ppl_dictAppendCpy(d, "permissions", &v, sizeof(v));
  return;
 }

void pplfunc_osSystem(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if ((nArgs!=1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The os.system() function requires a single string argument."); return; }
  pplObjNum(&OUTPUT,0,  system((char*)in[0].auxil)  ,0);
 }

void pplfunc_osTmpfile(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjFile(&OUTPUT,0,1,tmpfile(),0);
 }

void pplfunc_osUname(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *tmp;
  dict *d;
  pplObj v;
  struct utsname u;
  if (uname(&u)) { *status=1; *errType=ERR_INTERNAL; sprintf(errText,"The uname() function failed."); return; }
  v.refCount=1;
  tmp = (char *)malloc(LSTR_LENGTH);
  if (pplObjDict(&OUTPUT,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
  d = (dict *)OUTPUT.auxil;
  COPYSTR(tmp,u.sysname ); pplObjStr(&v,1,1,tmp); ppl_dictAppendCpy(d, "sysname" , &v, sizeof(v));
  COPYSTR(tmp,u.nodename); pplObjStr(&v,1,1,tmp); ppl_dictAppendCpy(d, "nodename", &v, sizeof(v));
  COPYSTR(tmp,u.release ); pplObjStr(&v,1,1,tmp); ppl_dictAppendCpy(d, "release" , &v, sizeof(v));
  COPYSTR(tmp,u.version ); pplObjStr(&v,1,1,tmp); ppl_dictAppendCpy(d, "version" , &v, sizeof(v));
  COPYSTR(tmp,u.machine ); pplObjStr(&v,1,1,tmp); ppl_dictAppendCpy(d, "machine" , &v, sizeof(v));
  return;
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

