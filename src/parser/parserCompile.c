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

static void parse_descend(ppl_context *c, parserStatus *s, char *line, int *linepos, parserNode *node, int *tabCompNo, char *tabCompTxt,  int topLevel, int *match)
 {
  if      (node->type == PN_TYPE_CONFIRM)
   {
   }
  else if (node->type == PN_TYPE_DATABLK)
   {
   }
  else if (node->type == PN_TYPE_CODEBLK)
   {
   }
  else if (node->type == PN_TYPE_ITEM)
   {
   }
  else if (node->type == PN_TYPE_SEQ)
   {
   }
  else if ((node->type == PN_TYPE_REP) || (node->type == PN_TYPE_REP2))
   {
   }
  else if (node->type == PN_TYPE_OPT)
   {
   }
  else if (node->type == PN_TYPE_PER)
   {
   }
  else if (node->type == PN_TYPE_ORA)
   {
   }
  else
   {
    sprintf(c->errcontext.tempErrStr, "Hit an unexpected node type %d.", node->type);
    ppl_fatal(&c->errcontext,__FILE__,__LINE__,NULL);
   }
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

void ppl_parserStatInit(parserStatus **in, parserLine **pl)
 {
  *in = (parserStatus *)malloc(sizeof(parserStatus));
  if (*in==NULL) return;
  *pl = NULL;
  (*in)->rootpl = pl;
  ppl_parserStatReInit(*in);
  return;
 }

void ppl_parserStatReInit(parserStatus *in)
 {
  parserLine **pl = in->rootpl;
  if (*pl != NULL) ppl_parserLineFree(*pl);
  *pl = NULL;
  in->stk[0][0] = NULL;
  in->stk[0][1] = (void *)pl;
  strcpy(in->prompt, "pyxplot");
  in->blockDepth = 0;
  return;
 }

void ppl_parserStatAdd(parserStatus *in, parserLine *pl)
 {
  int i=0, j=in->blockDepth;
  for (i=0; in->stk[j][i]!=NULL; i++);
  i++;
  *(parserLine **)(in->stk[j][i]) = pl;
  in->stk[j][i] = (void *)&pl->next;
  return;
 }

void ppl_parserStatFree(parserStatus **in)
 {
  parserLine **pl = (*in)->stk[0][1];
  if (*pl != NULL) ppl_parserLineFree(*pl);
  *pl = NULL;
  free(*in);
  *in = NULL;
 }

void ppl_parserLineInit(parserLine **in, int srcLineN, long srcId, char *srcFname, char *line)
 {
  parserLine *output=NULL;
  *in = NULL;

  // Initialise output structure
  output = (parserLine *)malloc(sizeof(parserLine));
  if (output==NULL) return;
  output->linetxt = (char *)malloc(strlen(line)+1);
  if (output->linetxt==NULL) { free(output); return; }
  strcpy(output->linetxt, line);
  output->srcFname = (char *)malloc(strlen(srcFname)+1);
  if (output->srcFname==NULL) { free(output->linetxt); free(output); return; }
  strcpy(output->srcFname, srcFname);
  output->firstAtom = NULL;
  output->lastAtom  = NULL;
  output->next      = NULL;
  output->srcLineN  = srcLineN;
  output->srcId     = srcId;
  output->stackLen  = 16;

  *in = output;
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

#define LOOP_END(X) \
    else if (X) \
     { \
      if (obPos > obLen-16) { obLen += LSTR_LENGTH; outbuff = (char *)realloc(outbuff, obLen); if (outbuff==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); fail=1; break; } } \
      outbuff[obPos++] = line[i]; \
     } \
   } \
 }

#define TEST_FOR_MACROS \
  containsMacros=0; \
  LOOP_OVER_LINE \
  else if ((quoteChar=='\0') && (line[i]=='`')) { containsMacros=1; break; } \
  else if ((quoteChar=='\0') && (line[i]=='@')) { containsMacros=1; break; } \
  LOOP_END(0)

int ppl_parserCompile(ppl_context *c, parserStatus *s, int srcLineN, long srcId, char *srcFname, char *line, int expandMacros, int iterDepth)
 {
  listIterator *cmdIter=NULL;
  int           i, cln, containsMacros=0, fail=0;
  int           obLen = LSTR_LENGTH, obPos;
  char         *outbuff = NULL;

  if (iterDepth+s->blockDepth > MAX_RECURSION_DEPTH) { strcpy(c->errStat.errBuff,"Maximum recursion depth exceeded."); ppl_tbAdd(c,srcLineN,srcId,srcFname,1,ERR_OVERFLOW,0,line); ppl_parserStatReInit(s); return 1; }

  // Deal with macros and ` ` substitutions
  if (!expandMacros)
   {
    TEST_FOR_MACROS;
    if (containsMacros) // If we're not expanding macros at this stage, flag this parserLine as containing macros, and exit
     {
      parserLine *output=NULL;
      ppl_parserLineInit(&output, srcLineN, srcId, srcFname, line);
      output->containsMacros = 1;
      if (output==NULL) { sprintf(c->errStat.errBuff,"Out of memory."); ppl_tbAdd(c,srcLineN,srcId,srcFname,1,ERR_MEMORY,0,line); ppl_parserStatReInit(s); return 1; }
      ppl_parserStatAdd(s, output);
      return 0;
     }
   }
  else
   {
    char *lineOriginal = line;
    int   l=0;
    for (l=0; l<16; l++) // Repeatedly test for and substitute macros; nested macros are allowed
     {
      TEST_FOR_MACROS;
      if (!containsMacros) break;
       {
        int fail=0;
        outbuff = (char *)malloc(obLen);
        obPos   = 0;
        if (outbuff!=NULL) LOOP_OVER_LINE

        // First, substitute for ` ` expressions
        else if ((quoteChar=='\0') && (line[i]=='`'))
         {
          int   is=++i, status, fail=0;
          char *key=NULL;
          FILE *substPipe;
          for ( ; ((line[i]!='\0')&&(line[i]!='`')) ; i++); // Find end of ` ` expression
          if (line[i]!='`') { sprintf(c->errStat.errBuff, "Mismatched `"); fail=1; break; }
          key = (char *)malloc(i-is+1); // Put macro name into a string
          if (key==NULL) { sprintf(c->errStat.errBuff,"Out of memory."); fail=1; break; }
          strncpy(key, line+is, i-is);
          key[i-is]='\0';
          if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Shell substitution with command '%s'.", key); ppl_log(&c->errcontext,NULL); }
          if ((substPipe = popen(key,"r"))==NULL)
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
          i--;
         }

        // Second, substitute for macros
        else if ((quoteChar=='\0') && (line[i]=='@'))
         {
          int   is=++i, j, got=0;
          char *key=NULL;
          for ( ; (isalnum(line[i])||(line[i]=='_')) ; i++); // Find end of macro name
          key = (char *)malloc(i-is+1); // Put macro name into a string
          if (key==NULL) { sprintf(c->errStat.errBuff,"Out of memory."); fail=1; break; }
          strncpy(key, line+is, i-is);
          key[i-is]='\0';
          for (j=1; j>=0 ; j--)
           {
            pplObj *obj = (pplObj *)ppl_dictLookup(c->namespaces[j] , key);
            if (obj==NULL) continue;
            if ((obj->objType==PPLOBJ_GLOB)||(obj->objType==PPLOBJ_ZOM)) continue;
            if (obj->objType!=PPLOBJ_STR)
             {
              sprintf(c->errStat.errBuff,"Attempt to expand a macro, \"%s\", which is not a string variable.", key);
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
          i--;
         }
        LOOP_END(1);
        outbuff[obPos++]='\0';

        // Clean up after macro substitution
        if ((!fail)&&(outbuff!=NULL)) { if (line!=lineOriginal) free(line); line = outbuff; }
        else if (outbuff!=NULL) { free(outbuff); outbuff=NULL; }
        if (fail) { ppl_tbAdd(c,srcLineN,srcId,srcFname,1,ERR_SYNTAX,0,line); ppl_parserStatReInit(s); return 1; }
       }
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
    int match=0, linepos=0;
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
    parse_descend(c, s, line, &linepos, item, NULL, NULL, 1, &match);

    // If this command did not match as far as the '=' token, clear stack and traceback
    if (match==0) { ppl_tbClear(c); continue; }

    // If we did match as far as '=', but got an error traceback, free the stack and return to the user
    if (c->errStat.status) { if (outbuff!=NULL) free(outbuff); ppl_parserStatReInit(s); return 1; }

    // Success!
    if (outbuff!=NULL) free(outbuff);
    return 0;
   }

  // We have not found a match to this command
  sprintf(c->errStat.errBuff, "Unrecognised command.");
  ppl_tbAdd(c,srcLineN,srcId,srcFname,1,ERR_SYNTAX,0,line);
  if (outbuff!=NULL) free(outbuff);
  ppl_parserStatReInit(s);
  return 1;
 }

#ifdef HAVE_READLINE
static ppl_context *rootContext = NULL;

void ppl_parseAutocompleteSetContext(ppl_context *c)
 {
  rootContext = c;
  return;
 }

char *ppl_parseAutocomplete(const char *dummy, int status)
 {
  static int   matchNum=0, start=0;
  static char *linebuff = NULL, *line = NULL;
  char        *output = NULL;

  // We are called once with negative status to set up static varaibles, before readline calls us with status>=0
  if (status<0)
   {
    start    = -status-1;
    matchNum = -1;
    if (line != NULL) { free(linebuff); linebuff=NULL; }
    if (rootContext->inputLineAddBuffer == NULL) // If we're on the second line of a continued line, add InputLineAddBuffer to beginning of line
     { line = rl_line_buffer; }
    else
     {
      int i = strlen(rl_line_buffer) + strlen(rootContext->inputLineAddBuffer);
      if ((linebuff = (char *)malloc(i+1))==NULL) return NULL;
      strcpy(linebuff, rootContext->inputLineAddBuffer);
      strcpy(linebuff+strlen(linebuff) , rl_line_buffer);
      linebuff[i] = '\0';
      line = linebuff;
      start += strlen(rootContext->inputLineAddBuffer);
     }
   }

  // Look for possible tab completions. Do this in a while loop, since if we get a %s filename completion, we search again
  while (1)
   {
    int           cln=26;
    listIterator *cmdIter=NULL;
    int           number = matchNum++;
    char          tabCompTxt[FNAME_LENGTH] = "\0";
    if (number<0) number=0; // Return first item twice

    // Loop over all of the possible commands that could be intended
    while (1)
     {
      int         match=0, linepos=0;
      parserNode *item;

      if (cmdIter == NULL) // Once we've finished cycling through commands that start with punctuation, cycle through ones that start with first letter
       {
        if (cln==-1) break;
        cmdIter = ppl_listIterateInit(pplParserCmdList[cln--]);
        if (cmdIter==NULL) continue;
       }

      // Fetch next item from list of possible commands and try to parse it
      item  = (parserNode *)ppl_listIterate(&cmdIter);
      match = linepos = 0;
      parse_descend(rootContext, NULL, line, &linepos, item, &number, tabCompTxt, 1, &match);

      if (tabCompTxt[0] == '\n') // Special case: use Readline's filename tab completion
       {
        if (status < 0) // On initial pass through, it's not too late to tell wrapper to use readline's own completer
         {
          if ((output = (char *)malloc(strlen(tabCompTxt)+1))==NULL) return NULL;
          strcpy(output, tabCompTxt);
          return output;
         } else { // On subsequent pass-throughs, it's too late...
          break;
         }
       }
      if (tabCompTxt[0] != '\0') // We have a new completion option; do not iterate through more commands
       {
        if ((output = (char *)malloc(strlen(tabCompTxt)+1))==NULL) return NULL;
        strcpy(output, tabCompTxt);
        return output;
       }
     }
    if (tabCompTxt[0] == '\n') continue; // We've been asked to match filenames, but have already made other PyXPlot syntax suggestions... skip on and return next match
    else                       break;
   }
  return NULL; // Tell readline we have no more matches to offer
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

