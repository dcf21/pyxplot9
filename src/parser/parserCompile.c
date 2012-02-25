// parserCompile.c
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

#define _PARSERCOMPILE_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#endif

#include "coreUtils/errorReport.h"

#include "expressions/expCompile.h"
#include "expressions/traceback_fns.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/parser.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"

static void parse_descend(ppl_context *c, char *line, int *linepos, parserNode *node, int tabCompNo, char *tabCompTxt,  int topLevel, int *match, parserLine *out)
 {
  return;
 }

void ppl_parserAtomFree(parserAtom **in)
 {
  parserAtom *i;
  if (in==NULL) return;
  i = *in;
  while (i!=NULL)
   {
    parserAtom *next = i->next;
    if (i->literal!=NULL) ppl_garbageObject(i->literal);
    if (i->expr   !=NULL) pplExpr_free(i->expr);
    free(i);
    i = next;
   }
  *in=NULL;
  return;
 }

void ppl_parserLineFree(parserLine *in)
 {
  parserLine *item = in;
  while (item != NULL)
   {
    parserLine *next = item->next;
    ppl_parserAtomFree(&item->firstAtom);
    if (item->linetxt !=NULL) free(item->linetxt);
    if (item->srcFname!=NULL) free(item->srcFname);
    free(item);
    item = next;
   }
  return;
 }

#define LOOP_OVER_LINE \
 { \
  int i; char quoteChar; \
  for (i=0 , quoteChar='\0'; line[i]!='\0'; i++) \
   { \
    if      ((quoteChar=='\0') && (line[i]=='\'')                     ) quoteChar = '\''; \
    else if ((quoteChar=='\0') && (line[i]=='\"')                     ) quoteChar = '\"'; \
    else if ((quoteChar=='\'') && (line[i]=='\'') && (line[i-1]!='\\')) quoteChar = '\0'; \
    else if ((quoteChar=='\"') && (line[i]=='\"') && (line[i-1]!='\\')) quoteChar = '\0'; \
    else if ((quoteChar=='\0') && (line[i]=='`' )                     ) quoteChar = '`';  \
    else if ((quoteChar=='`' ) && (line[i]=='`' ) && (line[i-1]!='\\')) quoteChar = '\0';

#define LOOP_END(X) \
    else if (X) \
     { \
      if (obPos > obLen-16) { obLen += LSTR_LENGTH; outbuff = (char *)realloc(outbuff, obLen); if (outbuff==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); fail=1; break; } } \
      outbuff[obPos++] = line[i]; \
     } \
   } \
 }

parserLine *ppl_parserCompile(ppl_context *c, int srcLineN, long srcId, char *srcFname, char *line, int expandMacros)
 {
  parserLine   *output=NULL;
  listIterator *cmdIter=NULL;
  int           i, cln, containsMacros=0, fail=0;
  int           obLen = LSTR_LENGTH, obPos=0;
  char         *outbuff = NULL;

  // Initialise output structure
  output = (parserLine *)malloc(sizeof(parserLine));
  if (output==NULL) return NULL;
  output->linetxt = (char *)malloc(strlen(line)+1);
  if (output->linetxt==NULL) { free(output); return NULL; }
  strcpy(output->linetxt, line);
  output->srcFname = (char *)malloc(strlen(srcFname)+1);
  if (output->srcFname==NULL) { free(output->linetxt); free(output); return NULL; }
  strcpy(output->srcFname, srcFname);
  output->firstAtom = NULL;
  output->lastAtom  = NULL;
  output->next      = NULL;
  output->srcLineN  = srcLineN;
  output->srcId     = srcId;
  output->stackLen  = 16;

  // Test to see whether input expression contains macros and ` ` substitutions
  LOOP_OVER_LINE
  else if ((quoteChar=='\0') && (line[i]=='`')) { containsMacros=1; break; }
  else if ((quoteChar=='\0') && (line[i]=='@')) { containsMacros=1; break; }
  LOOP_END(0)

  // Deal with macros and ` ` substitutions
  if (!expandMacros)
   {
    output->containsMacros = containsMacros; // If we're not expanding macros at this stage, flag this parserLine as containing macros, and exit
    if (containsMacros) return output;
   }
  else
   {
    output->containsMacros = 0;

    if (containsMacros)
     {
      int fail=0;
      outbuff = (char *)malloc(obLen);

      // First, substitute for ` ` expressions
      if ((!fail)&&(outbuff!=NULL)) LOOP_OVER_LINE
      else if ((quoteChar=='\0') && (line[i]=='`'))
       {
        int   is=++i, status, fail=0;
        char *key=NULL;
        FILE *substPipe;
        for ( ; ((line[i]!='\0')&&(line[i]!='`')) ; i++); // Find end of ` ` expression
        if (line[i]!='`') { sprintf(c->errStat.errBuff, "Mismatched `"); fail=1; break; }
        key = (char *)malloc(is-i+1); // Put macro name into a string
        if (key==NULL) { sprintf(c->errStat.errBuff,"Out of memory."); fail=1; break; }
        strncpy(key, line+i, is-i);
        key[is-i]='\0';
        if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Shell substitution with command '%s'.", key); ppl_log(&c->errcontext,NULL); }
        if ((substPipe = popen(line+is,"r"))==NULL)
         {
          sprintf(c->errStat.errBuff, "Could not spawl shell substitution command '%s'.", key);
          fail=1; goto shellSubstErr;
         }
        while ((!feof(substPipe)) && (!ferror(substPipe)))
         {
          if (fscanf(substPipe,"%c",outbuff+obPos) == EOF) break;
          if (outbuff[obPos]=='\n') outbuff[obPos] = ' ';
          if (outbuff[obPos]!='\0') obPos++;
          if (obPos > obLen-16) { obLen += LSTR_LENGTH; outbuff = (char *)realloc(outbuff, obLen); if (outbuff==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); fail=1; break; } } \
         }
        status = pclose(substPipe);
        if (fail) goto shellSubstErr;
        if (status!=0) { sprintf(c->errStat.errBuff, "Command failure during ` ` substitution."); fail=1; goto shellSubstErr; }
shellSubstErr:
        if (key!=NULL) free(key);
        if (fail) break;
       }
      LOOP_END(1)

      // Second, substitute for macros
      if ((!fail)&&(outbuff!=NULL)) LOOP_OVER_LINE
      else if ((quoteChar=='\0') && (line[i]=='@'))
       {
        int   is=++i, j, got=0;
        char *key=NULL;
        for ( ; (isalnum(line[i])) ; i++); // Find end of macro name
        key = (char *)malloc(is-i+1); // Put macro name into a string
        if (key==NULL) { sprintf(c->errStat.errBuff,"Out of memory."); fail=1; break; }
        strncpy(key, line+i, is-i);
        key[is-i]='\0';
        for (j=1; j>=0 ; j--)
         {
          pplObj *obj = (pplObj *)ppl_dictLookup(c->namespaces[i] , key);
          if (obj==NULL) continue;
          if ((obj->objType==PPLOBJ_GLOB)||(obj->objType==PPLOBJ_ZOM)) continue;
          if (obj->objType!=PPLOBJ_STR)
           {
            sprintf(c->errStat.errBuff,"Attempt to expand a macro, \"%s\", which is a numerical variable not a string.", key);
            got=-1;
            break;
           }
          if (obPos+strlen((char *)obj->auxil) > obLen-16) { obLen+=strlen((char *)obj->auxil) + LSTR_LENGTH; outbuff = (char *)realloc(outbuff, obLen); if (outbuff==NULL) { got=-2; break; } }
          strcpy(outbuff+obPos,(char *)obj->auxil);
          obPos += strlen(outbuff+obPos);
          got=1;
          break;
         }
        if (got==0) { sprintf(c->errStat.errBuff,"Undefined macro, \"%s\".",key); }
        free(key);
        if (got<=0) { fail=1; break; }
       }
      LOOP_END(1)

      // Clean up after macro substitution
      if ((!fail)&&(outbuff!=NULL)) line = outbuff;
      else if (outbuff!=NULL) { free(outbuff); outbuff=NULL; }
      if (fail) { ppl_tbAdd(c,output->srcLineN,output->srcId,output->srcFname,1,ERR_SYNTAX,0,line); ppl_parserLineFree(output); return NULL; }
     }
   }

  // Fetch first non-whitespace character of command string
  for (i=0; ((line[i]>='\0')&&(line[i]<=' ')); i++);
  if      ((line[i]>='a')&&(line[i]<='z')) cln=(int)(line[i]-'a');
  else if ((line[i]>='A')&&(line[i]<='Z')) cln=(int)(line[i]-'A');
  else                                     cln=-1;

  // Cycle through possible command matches
  cmdIter = ppl_listIterateInit(pplParserCmdList[26]);
  while (1)
   {
    int match, linepos;
    parserNode *item;

    if (cmdIter == NULL) // Once we've finished cycling through commands that start with punctuation, cycle through ones that start with first letter
     {
      if (cln==-1) break;
      cmdIter = ppl_listIterateInit(pplParserCmdList[cln]);
      cln=-1;
      if (cmdIter==NULL) break;
     }

    // Fetch next item from list of possible commands and try to parse it
    item  = (parserNode *)ppl_listIterate(&cmdIter);
    match = linepos = 0;
    parse_descend(c, line, &linepos, item, -1, NULL, 1, &match, output);

    // If this command did not match as far as the '=' token, clear stack and traceback
    if (match==0) { ppl_parserAtomFree(&output->firstAtom); ppl_tbClear(c); continue; }

    // If we did match as far as '=', but got an error traceback, free the stack and return to the user
    if (c->errStat.status) { ppl_parserLineFree(output); if (outbuff!=NULL) free(outbuff); return NULL; }

    // Success!
    if (outbuff!=NULL) free(outbuff);
    return output;
   }

  // We have not found a match to this command
  sprintf(c->errStat.errBuff, "Unrecognised command.");
  ppl_tbAdd(c,output->srcLineN,output->srcId,output->srcFname,1,ERR_SYNTAX,0,line);
  ppl_parserLineFree(output);
  if (outbuff!=NULL) free(outbuff);
  return NULL;
 }

#ifdef HAVE_READLINE
static ppl_context *rootContext = NULL;

void ppl_parseAutocompleteSetContext(ppl_context *c)
 {
  rootContext = c;
  return;
 }

char *ppl_parseAutocomplete(const char *line, int status)
 {
  return NULL;
 }

char **ppl_rl_completion(const char *text, int start, int end)
 {
  char **matches;
  char  *firstItem;

  if ((start>0)&&((rl_line_buffer[start-1]=='\"')||(rl_line_buffer[start-1]=='\''))) return NULL; // Do filename completion

  firstItem = ppl_parseAutocomplete(text, -1-start); // Setup parse_autocomplete to know what string it's working on

  if (firstItem!=NULL)
   {
    if (firstItem[0]=='\n') // We are trying to match a %s:filename field, so turn on filename completion
     {
      free(firstItem);
      rl_attempted_completion_over = 1; // NULL means that readline's default filename completer is activated
      return NULL;
     }
    else
     {
      free(firstItem);
     }
   }

  matches = rl_completion_matches(text, ppl_parseAutocomplete);
  rl_attempted_completion_over = 1; // Make sure that filenames are not completion options
  return matches;
 }
#endif

