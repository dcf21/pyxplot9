// parserInit.c
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

#define _PARSERINIT_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "userspace/context.h"

list *pplParserCmdList[27];

static void ppl_parserStartNode(pplerr_context *c, parserNode **stk, int *i, int type)
 {
  parserNode  *newNode = (parserNode *)malloc(sizeof(parserNode));
  if (newNode==NULL) { ppl_fatal(c,__FILE__,__LINE__,"Out of memory."); exit(1); }
  newNode->type        = type;
  newNode->listLen     = -1;
  newNode->acLevel     = -1;
  newNode->outStackPos = -1;
  newNode->matchString = NULL;
  newNode->outString   = NULL;
  newNode->varName     = NULL;
  newNode->firstChild  = newNode->nextSibling = NULL;

  if ((*i)>0) // If this new node is not the root context, check whether we need to insert an implicit sequence between *list* *seq* STUFF stuff stuff *endseq* *endlist*
   {
    parserNode **target = &(stk[(*i)-1]->firstChild); // Pointer to where first child of parent is linked
    if ( (stk[(*i)-1]->type != PN_TYPE_SEQ) && (type != PN_TYPE_SEQ) ) ppl_parserStartNode(c, stk, i, PN_TYPE_SEQ);
    while (*target != NULL) target = &((*target)->nextSibling); // If not first child of parent, traverse linked list to end
    *target = newNode; // Add new node into hierarchy, either as first child of parent, or into linked list of siblings
   }
  stk[(*i)++] = newNode; // Add new node structure to context stack
  return;
 }

static void ppl_parserRollBack(pplerr_context *c, parserNode **stk, int *i, int type, char *cmdText, int *NcharsTaken)
 {
  int j=1; // Count characters taken; start at 1 for closing bracket
  int varNameStart=-1, varNameEnd=-1;

  while (((*i)>0) && (stk[(*i)-1]->type == PN_TYPE_SEQ)) (*i)--; // Automatically close SEQs; they don't have closing grammar

  if ((*i)<=0)
   { sprintf(c->tempErrStr, "Incorrect nesting of types in command specification -- attempt to close an unopened bracket."); ppl_fatal(c,__FILE__,__LINE__, NULL); }
  else if (stk[(*i)-1]->type != type)
   { sprintf(c->tempErrStr, "Incorrect nesting of types in command specification -- expected %d, but got %d.", type, stk[(*i)-1]->type); ppl_fatal(c,__FILE__,__LINE__, NULL); }

  if (cmdText[j]=='@')
   {
    int k,l;
    if (type!=PN_TYPE_REP) { sprintf(c->tempErrStr, "Unexpected storage information after non [ ] data structure."); ppl_fatal(c,__FILE__,__LINE__, NULL); }
    j++;
    if (cmdText[j]=='0') { type=PN_TYPE_REP2; stk[(*i)-1]->type=type; j++; } // If storage variable name begins '0', zero repeats are allowed; a REP2 item, not a REP item
    if ((cmdText[j]>' ')&&(cmdText[j]!='@'))
     {
      varNameStart = j;
      while ((cmdText[j]>' ')&&(cmdText[j]!='@')) j++; // FFW over variable name
      varNameEnd   = j;
     }
    if (cmdText[j++]!='@') { sprintf(c->tempErrStr, "Syntax error: expecting @ after ]@varname..."); ppl_fatal(c,__FILE__,__LINE__, NULL); }
    while ((cmdText[j]>' ')&&(cmdText[j]!='@')) j++;
    if (cmdText[j++]!='@') { sprintf(c->tempErrStr, "Syntax error: expecting @ after ]@varname@value..."); ppl_fatal(c,__FILE__,__LINE__, NULL); }
    while ((cmdText[j]>' ')&&(cmdText[j]!='@')) j++;
    if (cmdText[j++]!='@') { sprintf(c->tempErrStr, "Syntax error: expecting @ after ]@varname@value@ACL..."); ppl_fatal(c,__FILE__,__LINE__, NULL); }
    k=(int)ppl_getFloat(cmdText+j,NULL); while ((cmdText[j]>' ')&&(cmdText[j]!='@')) j++;
    stk[(*i)-1]->outStackPos = k;
    if (cmdText[j++]!='@') { sprintf(c->tempErrStr, "Syntax error: expecting @ after ]@varname@value@ACL@stackpos..."); ppl_fatal(c,__FILE__,__LINE__, NULL); }
    l=(int)ppl_getFloat(cmdText+j,NULL); while ((cmdText[j]>' ')&&(cmdText[j]!='@')) j++;
    stk[(*i)-1]->listLen     = l;
    while ((cmdText[j]!='\0') && (cmdText[j]!='\n') && (cmdText[j]<' ')) j++; // FFW over trailing whitespace

    if (varNameStart > -1)
     {
      stk[(*i)-1]->varName = (char *)malloc((varNameEnd-varNameStart+1)*sizeof(char));
      if (stk[(*i)-1]->varName == NULL) { ppl_fatal(c,__FILE__,__LINE__,"Out of memory whilst setting up PyXPlot's command line parser."); exit(1); }
      strncpy( stk[(*i)-1]->varName , cmdText+varNameStart , varNameEnd-varNameStart );
      stk[(*i)-1]->varName[varNameEnd - varNameStart] = '\0';
     }
   }
  (*i)--; // Close structure of type "type" and roll back stack.
  *NcharsTaken = j;
  return;
 }

int ppl_parserInit(ppl_context *c)
 {
  pplerr_context *e = &c->errcontext;
  int i;
  int inPos=0;

  for (i=0; i<27; i++)
   {
    pplParserCmdList[i] = ppl_listInit(1);
    if (pplParserCmdList[i]==NULL) { ppl_fatal(e,__FILE__,__LINE__,"Out of memory."); exit(1); }
   }

  // Loop over instructions in command list
  while (1)
   {
    int stackPos=0, cmdLetter, rootStackLen, nc=0;
    parserNode *defnStack[16];

    while ((ppl_cmdList[inPos] != '\0') && (ppl_cmdList[inPos] <= ' ')) inPos++; // Ignore whitespace
    rootStackLen = (int)ppl_getFloat(ppl_cmdList+inPos, &nc); // Each line of command list begins with the length of the root stack namespace
    inPos+=nc;
    while ((ppl_cmdList[inPos] != '\0') && (ppl_cmdList[inPos] <= ' ')) inPos++; // Ignore whitespace
    if (ppl_cmdList[inPos] == '\0') break; // End of cmdList -- all instructions have now been read
    if ((ppl_cmdList[inPos]>='a')&&(ppl_cmdList[inPos]<='z')) cmdLetter = (int)(ppl_cmdList[inPos]-'a');
    else                                                      cmdLetter = 26; // Begins with punctuation
    ppl_parserStartNode(e, defnStack, &stackPos, PN_TYPE_SEQ);
    defnStack[0]->listLen = rootStackLen;

    // Loop over words in instruction definition
    while (1)
     {
      while ((ppl_cmdList[inPos] != '\0') && (ppl_cmdList[inPos] != '\n') && (ppl_cmdList[inPos] <= ' ')) inPos++; // Ignore whitespace
      if ((ppl_cmdList[inPos] == '\0') || (ppl_cmdList[inPos] == '\n')) break; // Newline means end of instruction

      // Punctuation which descends into a new hierarchical data structure
      if      (ppl_cmdList[inPos] == '{') // {} grammar indicates an optional series of items, which go into a new structure.
       { ppl_parserStartNode(e, defnStack, &stackPos, PN_TYPE_OPT); inPos++; }
      else if (ppl_cmdList[inPos] == '[') // [] grammar indicates a series of items which repeat 0 or more times. These go into a new structure.
       { ppl_parserStartNode(e, defnStack, &stackPos, PN_TYPE_REP); inPos++; }
      else if (ppl_cmdList[inPos] == '(') // () grammar indicates items which can appear in any order, but each one not more than once. New structure.
       { ppl_parserStartNode(e, defnStack, &stackPos, PN_TYPE_PER); inPos++; }
      else if (ppl_cmdList[inPos] == '<') // <> grammar indicates a list of items of which only one should be matched. New structure.
       { ppl_parserStartNode(e, defnStack, &stackPos, PN_TYPE_ORA); inPos++; }

      // Punctuation which preceeds a new item in a sequence
      else if (ppl_cmdList[inPos] == '~') // ~ is used inside () to separate items
       {
        while ((stackPos>=0) && (defnStack[stackPos-1]->type == PN_TYPE_SEQ)) stackPos--; // Automatically close SEQs; they don't have closing grammar
        if ((stackPos<0) || (defnStack[stackPos-1]->type != PN_TYPE_PER)) { sprintf(e->tempErrStr, "Tilda should be used only in permutation structures (char pos %d).",inPos); ppl_fatal(e,__FILE__,__LINE__, NULL); }
        inPos++;
       }
      else if (ppl_cmdList[inPos] == '|') // | is used inside <> for either/or items
       {
        while ((stackPos>=0) && (defnStack[stackPos-1]->type == PN_TYPE_SEQ)) stackPos--; // Automatically close SEQs; they don't have closing grammar
        if ((stackPos<0) || (defnStack[stackPos-1]->type != PN_TYPE_ORA)) { sprintf(e->tempErrStr, "Pipe alternatives should only be used inside ORA structures (char pos %d).",inPos); ppl_fatal(e,__FILE__,__LINE__, NULL); }
        inPos++;
       }

      // Punctuation which closes a hierarchical data structure
      else if (ppl_cmdList[inPos] == '>') // Match closing brackets for the above types
       { int j=0; ppl_parserRollBack(e, defnStack, &stackPos, PN_TYPE_ORA, ppl_cmdList+inPos, &j) ; inPos+=j; }
      else if (ppl_cmdList[inPos] == ')')
       { int j=0; ppl_parserRollBack(e, defnStack, &stackPos, PN_TYPE_PER, ppl_cmdList+inPos, &j) ; inPos+=j; }
      else if (ppl_cmdList[inPos] == ']')
       { int j=0; ppl_parserRollBack(e, defnStack, &stackPos, PN_TYPE_REP, ppl_cmdList+inPos, &j) ; inPos+=j; }
      else if (ppl_cmdList[inPos] == '}')
       { int j=0; ppl_parserRollBack(e, defnStack, &stackPos, PN_TYPE_OPT, ppl_cmdList+inPos, &j) ; inPos+=j; }

      // Non-punctuation items which match strings and expressions
      else if (ppl_cmdList[inPos] == '@')
       {
        int strStart, k, l, m;
        parserNode *newNode, **target;
        inPos++;
        if (defnStack[stackPos-1]->type != PN_TYPE_SEQ) ppl_parserStartNode(e, defnStack, &stackPos, PN_TYPE_SEQ); // Match words have to go in sequences, not, e.g. "ora" structures

        if ((newNode = (parserNode *)malloc(sizeof(parserNode)))==NULL) { ppl_fatal(e,__FILE__,__LINE__,"Out of memory whilst setting up PyXPlot's command line parser."); exit(1); }
        newNode->type = PN_TYPE_ITEM;
        newNode->firstChild  = newNode->nextSibling = NULL;

        strStart = inPos;
        while ((ppl_cmdList[inPos]>' ')&&(ppl_cmdList[inPos]!='@')) inPos++; // FFW over match string
        newNode->matchString = (char *)malloc((inPos-strStart+1)*sizeof(char));
        if (newNode->matchString == NULL) { ppl_fatal(e,__FILE__,__LINE__,"Out of memory whilst setting up PyXPlot's command line parser."); exit(1); }
        strncpy( newNode->matchString , ppl_cmdList+strStart , inPos-strStart );
        newNode->matchString[inPos - strStart] = '\0';
        if (strcmp(newNode->matchString, "DATABLOCK")==0) newNode->type = PN_TYPE_DATABLK;
        if (strcmp(newNode->matchString, "CODEBLOCK")==0) newNode->type = PN_TYPE_CODEBLK;
        if (ppl_cmdList[inPos++]!='@') { sprintf(e->tempErrStr, "Syntax error: expecting @ after @matchstr..."); ppl_fatal(e,__FILE__,__LINE__, NULL); }

        strStart = inPos;
        while ((ppl_cmdList[inPos]>' ')&&(ppl_cmdList[inPos]!='@')) inPos++; // FFW over variable name
        newNode->varName = (char *)malloc((inPos-strStart+1)*sizeof(char));
        if (newNode->varName == NULL) { ppl_fatal(e,__FILE__,__LINE__,"Out of memory whilst setting up PyXPlot's command line parser."); exit(1); }
        strncpy( newNode->varName , ppl_cmdList+strStart , inPos-strStart );
        newNode->varName[inPos - strStart] = '\0';
        if (ppl_cmdList[inPos++]!='@') { sprintf(e->tempErrStr, "Syntax error: expecting @ after @matchstr@varname..."); ppl_fatal(e,__FILE__,__LINE__, NULL); }

        strStart = inPos;
        while ((ppl_cmdList[inPos]>' ')&&(ppl_cmdList[inPos]!='@')) inPos++; // FFW over output string literal
        newNode->outString = (char *)malloc((inPos-strStart+1)*sizeof(char));
        if (newNode->outString == NULL) { ppl_fatal(e,__FILE__,__LINE__,"Out of memory whilst setting up PyXPlot's command line parser."); exit(1); }
        strncpy( newNode->outString , ppl_cmdList+strStart , inPos-strStart );
        newNode->outString[inPos - strStart] = '\0';
        if (ppl_cmdList[inPos++]!='@') { sprintf(e->tempErrStr, "Syntax error: expecting @ after @matchstr@varname@str..."); ppl_fatal(e,__FILE__,__LINE__, NULL); }

        k=(int)ppl_getFloat(ppl_cmdList+inPos,NULL); while ((ppl_cmdList[inPos]>' ')&&(ppl_cmdList[inPos]!='@')) inPos++;
        newNode->acLevel     = k;
        if (ppl_cmdList[inPos++]!='@') { sprintf(e->tempErrStr, "Syntax error: expecting @ after ]@varname@value@ACL..."); ppl_fatal(e,__FILE__,__LINE__, NULL); }
        l=(int)ppl_getFloat(ppl_cmdList+inPos,NULL); while ((ppl_cmdList[inPos]>' ')&&(ppl_cmdList[inPos]!='@')) inPos++;
        newNode->outStackPos = l;
        if (ppl_cmdList[inPos++]!='@') { sprintf(e->tempErrStr, "Syntax error: expecting @ after ]@varname@value@ACL@stackpos..."); ppl_fatal(e,__FILE__,__LINE__, NULL); }
        m=(int)ppl_getFloat(ppl_cmdList+inPos,NULL); while ((ppl_cmdList[inPos]>' ')&&(ppl_cmdList[inPos]!='@')) inPos++;
        newNode->listLen     = m;
        while ((ppl_cmdList[inPos]!='\0') && (ppl_cmdList[inPos]!='\n') && (ppl_cmdList[inPos]<' ')) inPos++; // FFW over trailing whitespace

        target = &(defnStack[stackPos-1]->firstChild); // Pointer to where first child of parent is linked
        while (*target != NULL) target = &((*target)->nextSibling); // If not first child of parent, traverse linked list to end
        *target = newNode; // Add new node into hierarchy, either as first child of parent, or into linked list of siblings
       }
      else if (ppl_cmdList[inPos]=='=')
       {
        parserNode *newNode, **target;
        if ((newNode = (parserNode *)calloc(1,sizeof(parserNode)))==NULL) { ppl_fatal(e,__FILE__,__LINE__,"Out of memory whilst setting up PyXPlot's command line parser."); exit(1); }
        newNode->type = PN_TYPE_CONFIRM;
        target = &(defnStack[stackPos-1]->firstChild); // Pointer to where first child of parent is linked
        while (*target != NULL) target = &((*target)->nextSibling); // If not first child of parent, traverse linked list to end
        *target = newNode; // Add new node into hierarchy, either as first child of parent, or into linked list of siblings
        inPos++;
       }
      else
       {
        sprintf(e->tempErrStr, "Syntax error: unexpected character '%c'", ppl_cmdList[inPos]);
        ppl_fatal(e,__FILE__,__LINE__, NULL);
       }
     }
    ppl_listAppend(pplParserCmdList[cmdLetter], defnStack[0]);
   }
  return 0;
 }

