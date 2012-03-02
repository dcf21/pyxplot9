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

#include "settings/epsColors.h"
#include "settings/settingTypes.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"

static char *strDup(const char *in, int N)
 {
  char *out = (char *)malloc(N+1);
  if (out==NULL) return NULL;
  strncpy(out, in, N+1);
  out[N]='\0';
  return out;
 }

void ppl_parserAtomAdd(parserLine *in, int stackOutPos, int linePos, char *options, pplExpr *expr, pplObj *literal)
 {
  parserAtom *output = (parserAtom *)malloc(sizeof(parserAtom));
  if (output==NULL) return;
  output->stackOutPos = stackOutPos;
  output->linePos     = linePos;
  output->expr        = expr;
  output->next        = NULL;
  strncpy(output->options, options, 8);
  output->options[7]='\0';
  if (literal == NULL) output->literal = NULL;
  else
   {
    output->literal = (pplObj *)malloc(sizeof(pplObj));
    if (output->literal==NULL) { free(output); return; }
    memcpy(output->literal, literal, sizeof(pplObj));
    output->literal->amMalloced = 1;
   }
  if (in->lastAtom==NULL) { in->firstAtom = output; }
  else                    { in->lastAtom->next = output; }
  in->lastAtom = output;
 }

static int parse_descend(ppl_context *c, parserStatus *s, int writeOut, int srcLineN, long srcId, char *srcFname, char *line, int *linepos, parserNode *node, int *tabCompNo, int tabCompStart, char *tabCompTxt, int level, int blockDepth, int *match)
 {
  int status=0; // 0=fail; 1=pass; 2=tab completion returned; 3=want another line.

  // If node is NULL, this means we're putting back together a hierarchy of SEQs that we earlier descended out of to look for more input statements
  if (node==NULL)
   {
    parserNode *nodeNext = s->stk[blockDepth][level+1];
    node = s->stk[blockDepth][level];
    if (nodeNext != NULL) // We need to descend still further...
     {
      parserNode *nodeIter = node->firstChild;
      status = parse_descend(c,s,writeOut,srcLineN,srcId,srcFname,line,linepos,NULL,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
      if (status!=1) goto cleanup;

      // We came out of child node with a success return code. This means we carry on chomping along the SEQ
      while ((nodeIter!=nodeNext) && (nodeIter!=NULL)) nodeIter = nodeIter->nextSibling; // Work out where we were...
      if (nodeIter==nodeNext) nodeIter = nodeIter->nextSibling;
      while (nodeIter != NULL)
       {
        status = parse_descend(c,s,writeOut,srcLineN,srcId,srcFname,line,linepos,nodeIter,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
        if (status!=1) goto cleanup;
        nodeIter = nodeIter->nextSibling;
       }
      goto cleanup;
     }
    // We don't need to descend any further... we let ourselves loose on the code below!
   }

  // If this is a top-level attempt to parse a statement, make a new parserLine structure to hold the output of our attempt
  if ((s!=NULL) && writeOut && (s->pl[blockDepth]==NULL))
   {
    parserLine *output=NULL;
    ppl_parserLineInit(&output,srcLineN,srcId,srcFname,line);
    output->stackLen = node->listLen;
    if (output==NULL) { sprintf(c->errStat.errBuff,"Out of memory."); ppl_tbAdd(c,srcLineN,srcId,srcFname,1,ERR_MEMORY,0,line); return 0; }
    ppl_parserStatAdd(s, blockDepth, output);
   }

  // FFW over spaces
  while ((line[*linepos]!='\0') && (line[*linepos]<=' ')) (*linepos)++;

  // Deal with the node we've been given
  switch (node->type)
   {
    case PN_TYPE_CONFIRM:
     {
      *match=1;
      status=1;
      goto cleanup;
     }
    case PN_TYPE_DATABLK:
     {
      status=1;
      goto cleanup;
     }
    case PN_TYPE_CODEBLK:
     {
      status=1;
      goto cleanup;
     }
    case PN_TYPE_ITEM:
     {
      int i;
      static int atNLastPos=-1; // Position of the last @n item matched
      if (*linepos<atNLastPos) atNLastPos=-1;

      // See if this item straddles the point from which we've been asked to make tab-completion suggestions; if so, return one.
      if ((tabCompTxt != NULL) && (tabCompStart <= *linepos))
       {
        if (node->matchString[0]=='%')
         {
          if ((node->varName != NULL) && ((strcmp(node->varName,"filename")==0)||(strcmp(node->varName,"directory")==0)))
           { // Expecting a filename
            if ((*tabCompNo)!=0) { status=0; (*tabCompNo)--; goto item_cleanup; }
            status=2; strcpy(tabCompTxt, "\n"); (*tabCompNo)--; goto item_cleanup;
           }
          goto finished_looking_for_tabcomp; // Expecting a float or string, which we don't tab complete... move along and look for something else
         }

        if ((*linepos>0) && (isalnum(line[(*linepos)-1])) && (atNLastPos!=*linepos)) goto finished_looking_for_tabcomp; // 'plot a' cannot tab complete 'using' without space

        for (i=0; ((line[*linepos+i]>' ') && (node->matchString[i]>' ')); i++)
         if (toupper(line[*linepos+i])!=toupper(node->matchString[i]))
          {
           status=0; goto item_cleanup; // We don't match the beginning of this string
          }
        if ((node->acLevel == -1) && (node->matchString[i]<=' ')) goto finished_looking_for_tabcomp; // We've matched an @n string right to the end... move on
        if ((*tabCompNo)!=0) { status=0; (*tabCompNo)--; goto item_cleanup; }
        status=2;
        for (i=0; i<((*linepos)-tabCompStart); i++) tabCompTxt[i] = line[tabCompStart+i];
        strcpy(tabCompTxt+i, node->matchString); // Matchstring should match itself
        (*tabCompNo)--;
        goto item_cleanup;
       }
finished_looking_for_tabcomp:

      // Treat items which are @n
      if (node->acLevel == -1)
       {
        int i;
        status=1;
        for (i=0; (node->matchString[i]>' '); i++)
         if (toupper(line[*linepos+i])!=toupper(node->matchString[i]))
          {
           status=0; break; // We don't match this string
          }
        if (status==1)
         {
          *linepos += i; // We do match this string: advance by i spaces
          atNLastPos = *linepos;
          if ((s!=NULL) && writeOut && (strcmp(node->varName,"X")!=0))
           {
            pplObj val;
            val.refCount=1;
            pplObjStr(&val,0,0,node->outString);
            ppl_parserAtomAdd(s->pl[blockDepth], s->pl[blockDepth]->stackOffset + node->outStackPos, *linepos, "", NULL, &val);
           }
         }
       }

      // Treat items which are not @n
      else if (node->matchString[0]!='%') // Items which do not begin % match themselves
       {
        int i = ppl_strAutocomplete(line+*linepos , node->matchString , node->acLevel);
        if (i<0)
         { status=0; }
        else
         {
          status=1;
          *linepos += i; // We match this string: advance by i spaces
          atNLastPos = *linepos;
          if ((s!=NULL) && writeOut && (strcmp(node->varName,"X")!=0))
           {
            pplObj val;
            val.refCount=1;
            pplObjStr(&val,0,0,node->outString);
            ppl_parserAtomAdd(s->pl[blockDepth], s->pl[blockDepth]->stackOffset + node->outStackPos, *linepos, "", NULL, &val);
           }
         }
       }
      else // Item begins with a %, so substitute a value for this variable
       {
        pplExpr *expr;
        int      explen=0, errPos=-1, errType;
        int      dollarAllowed = (node->matchString[1]=='E');

        switch (node->matchString[1])
         {
          case 's': case 'S': case 'r': case 'v':
           {
            if      (node->matchString[1]=='r')
              explen = strlen(line+*linepos);
            else if (node->matchString[1]=='s')
             {
              for (explen=0; ((isalpha(line[*linepos+explen]))||(line[*linepos+explen]=='_')); explen++);
              if (explen==0) { status=0; goto item_cleanup; }
             }
            else if (node->matchString[1]=='S')
             {
              for (explen=0; ((line[*linepos+explen]>' ')&&(line[*linepos+explen]!='\'')&&(line[*linepos+explen]!='\"')); explen++);
              if (explen==0) { status=0; goto item_cleanup; }
             }
            else if (node->matchString[1]=='v')
             {
              if (!isalpha(line[*linepos])) { status=0; goto item_cleanup; } // variable name must start with a letter
              for (explen=1; ((isalnum(line[*linepos+explen]))||(line[*linepos+explen]=='_')); explen++); // afterwards, numbers and _ allowed
             }
            if ((s!=NULL) && writeOut)
             {
              char   *str = strDup(line+*linepos,explen);
              pplObj  val;
              if (str==NULL) { status=0; goto item_cleanup; }
              val.refCount=1;
              pplObjStr(&val,0,1,str);
              ppl_parserAtomAdd(s->pl[blockDepth], s->pl[blockDepth]->stackOffset + node->outStackPos, *linepos, "", NULL, &val);
             }
            break;
           }
          case 'a':
           {
            int xyz;
            if      ((line[*linepos]=='x')||(line[*linepos]=='X')) xyz=0;
            else if ((line[*linepos]=='y')||(line[*linepos]=='Y')) xyz=1;
            else if ((line[*linepos]=='z')||(line[*linepos]=='Z')) xyz=2;
            else                                                   { status=0; goto item_cleanup; }
            ppl_expCompile(c,srcLineN,srcId,srcFname,line+*linepos+1,&explen,dollarAllowed,0,&expr,&errPos,&errType,c->errStat.errBuff);
            if (errPos>=0)
             {
              pplExpr_free(expr);
              ppl_tbAdd(c,srcLineN,srcId,srcFname,0,errType,errPos+*linepos+1,line);
              status=0;
              goto item_cleanup;
             }
            if ((s==NULL)||(!writeOut)) pplExpr_free(expr);
            else
             {
              char opt[8];
              sprintf(opt,"a%d",xyz);
              ppl_parserAtomAdd(s->pl[blockDepth], s->pl[blockDepth]->stackOffset + node->outStackPos, *linepos+1, opt, expr, NULL);
             }
            explen++;
            break;
           }
          case 'c': case 'C':
           {
            char nambuff[64];
            int id, i;
            for (i=0; ((i<63)&&(line[*linepos+i]>' ')); i++) nambuff[i] = line[*linepos+i];
            nambuff[i] = '\0';
            id = ppl_fetchSettingByName(&c->errcontext, nambuff, SW_COLOR_INT, SW_COLOR_STR);
            if (id > 0)
             {
              pplObj val;
              val.refCount=1;
              pplObjNum(&val,0,id,0);
              ppl_parserAtomAdd(s->pl[blockDepth], s->pl[blockDepth]->stackOffset + node->outStackPos, *linepos, "", NULL, &val);
              explen=i;
              break;
             }
            // If supplied string did not match a named colour, treat it as an expression
           }
          default:
           {
            ppl_expCompile(c,srcLineN,srcId,srcFname,line+*linepos,&explen,dollarAllowed,0,&expr,&errPos,&errType,c->errStat.errBuff);
            if (errPos>=0)
             {
              pplExpr_free(expr);
              ppl_tbAdd(c,srcLineN,srcId,srcFname,0,errType,errPos+*linepos,line);
              status=0;
              goto item_cleanup;
             }
            if ((s==NULL)||(!writeOut)) pplExpr_free(expr);
            else if ((node->matchString[1]!='C') && (node->matchString[1]!='e') && (node->matchString[1]!='E'))
             {
              ppl_parserAtomAdd(s->pl[blockDepth], s->pl[blockDepth]->stackOffset + node->outStackPos, *linepos, node->matchString+1, expr, NULL);
             }
            else
             {
              pplObj val;
              val.refCount=1;
              pplObjExpression(&val,0,(void *)expr);
              ppl_parserAtomAdd(s->pl[blockDepth], s->pl[blockDepth]->stackOffset + node->outStackPos, *linepos, "", NULL, &val);
             }
           }
         }
        *linepos += explen;
        status=1;
       }

      // If we're not running in tab-completion mode and have failed, return an "expecting" hint
item_cleanup:
      if ((s!=NULL) && (status!=1) && (s->eLlinePos <= *linepos))
       {
        int i,j,k,l;
        char varname[FNAME_LENGTH];
        if (s->eLlinePos < *linepos) { s->eLlinePos = *linepos; s->eLPos = 0; s->expectingList[0]='\0'; }
        if ((node->varName != NULL) && (node->varName[0] != '\0'))  sprintf(varname, " (%s)", node->varName);
        else                                                        varname[0]='\0';
        for (i=1; 1; i++)
         {
          if (s->eLPos != 0) { strcpy(s->expectingList+s->eLPos, ", or "); s->eLPos+=strlen(s->expectingList+s->eLPos); }

          if (node->matchString[0]!='%')
           { sprintf(s->expectingList+s->eLPos, "\"%s\"", node->matchString); }
          else
           {
            if (node->matchString[i]=='\0') { if (s->eLPos!=0) { s->eLPos-=5; s->expectingList[s->eLPos]='\0'; } break; }
            switch(node->matchString[i])
             {
              case 'a': sprintf(s->expectingList+s->eLPos, "an axis name%s", varname); break;
              case 'A': sprintf(s->expectingList+s->eLPos, "an angle%s", varname); break;
              case 'b': sprintf(s->expectingList+s->eLPos, "a boolean%s", varname); break;
              case 'c': sprintf(s->expectingList+s->eLPos, "a color%s", varname); break;
              case 'C': sprintf(s->expectingList+s->eLPos, "a color%s", varname); break;
              case 'd': sprintf(s->expectingList+s->eLPos, "an integer%s", varname); break;
              case 'D': sprintf(s->expectingList+s->eLPos, "a distance%s", varname); break;
              case 'e': sprintf(s->expectingList+s->eLPos, "an algebraic expression%s", varname); break;
              case 'E': sprintf(s->expectingList+s->eLPos, "an algebraic expression%s", varname); break;
              case 'f': sprintf(s->expectingList+s->eLPos, "a real, dimensionless number%s", varname); break;
              case 'o': sprintf(s->expectingList+s->eLPos, "an expression%s", varname); break;
              case 'p': sprintf(s->expectingList+s->eLPos, "a position vector%s", varname); break;
              case 'P': sprintf(s->expectingList+s->eLPos, "a position vector%s", varname); break;
              case 'q': sprintf(s->expectingList+s->eLPos, "a string%s", varname); break;
              case 'r': sprintf(s->expectingList+s->eLPos, "a string of text%s", varname); break;
              case 's': sprintf(s->expectingList+s->eLPos, "an alphabetical word%s", varname); break;
              case 'S': sprintf(s->expectingList+s->eLPos, "a word%s", varname); break;
              case 'u': sprintf(s->expectingList+s->eLPos, "a physical quantity%s", varname); break;
             }
           }
          l = strlen(s->expectingList+s->eLPos);
          for (j=k=0; ((j<s->eLPos)&&!k); j++) k = (strncmp(s->expectingList+j,s->expectingList+s->eLPos,l)==0); // Don't repeat ourselves
          if (!k) s->eLPos+=strlen(s->expectingList+s->eLPos);
          else    { if (s->eLPos!=0) { s->eLPos-=5; } s->expectingList[s->eLPos]='\0'; }
          if (node->matchString[0]!='%') break;
         }
       }
      goto cleanup;
     }
    case PN_TYPE_SEQ:
     {
      parserNode *nodeIter = node->firstChild;
      while (nodeIter != NULL)
       {
        status = parse_descend(c,s,writeOut,srcLineN,srcId,srcFname,line,linepos,nodeIter,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
        if (status!=1) goto cleanup;
        nodeIter = nodeIter->nextSibling;
       }
      goto cleanup;
     }
    case PN_TYPE_REP: case PN_TYPE_REP2:
     {
      int        first=1, separator=1;
      char       sepStr[4];
      parserNode sepNode;
      int        oldStackOffset = s->pl[blockDepth]->stackOffset;
      sepStr[0] = node->varName[strlen(node->varName)-1];
      sepStr[1] = '\0';
      if (isalnum(sepStr[0])) separator=0;
      sepNode.type        = PN_TYPE_ITEM;
      sepNode.listLen     = 0;
      sepNode.acLevel     = -1;
      sepNode.matchString = sepStr;
      sepNode.outString   = sepStr;
      sepNode.varName     = "X";
      sepNode.outStackPos = PARSE_arc_X;
      sepNode.firstChild  = NULL;
      sepNode.nextSibling = NULL;

      while (1)
       {
        int         lineposOld = *linepos;
        int         matchOld   = *match;
        int         lineposOld2;
        parserNode *nodeIter   = node->firstChild;

        if (separator && !first)
         {
          status = parse_descend(c,s,0,srcLineN,srcId,srcFname,line,linepos,&sepNode,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
          if (status==3) { status=0; }
          if (status==0) { *linepos=lineposOld; *match=matchOld; status=1; goto repCleanup; }
          if (status==2) goto repCleanup;
         }
        lineposOld2 = *linepos;
        while (nodeIter != NULL)
         {
          status = parse_descend(c,s,0,srcLineN,srcId,srcFname,line,linepos,nodeIter,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
          if (status==3) { status=0; goto repCleanup; }
          if (status==0) { if ((!separator)&&((!first)||(node->type==PN_TYPE_REP2))) { status=1; *linepos=lineposOld; *match=matchOld; } goto repCleanup; }
          if (status==2) goto repCleanup;
          nodeIter = nodeIter->nextSibling;
         }
        if ((s!=NULL) && writeOut)
         {
          int    oldStackLen = s->pl[blockDepth]->stackLen;
          pplObj val;
          val.refCount=1;
          pplObjNum(&val,1,oldStackLen,0);
          ppl_parserAtomAdd(s->pl[blockDepth], s->pl[blockDepth]->stackOffset + ((s->pl[blockDepth]->stackOffset == oldStackOffset) ? node->outStackPos : 0), *linepos, "", NULL, &val);
          s->pl[blockDepth]->stackLen += node->listLen;
          s->pl[blockDepth]->stackOffset = oldStackLen;
          nodeIter=node->firstChild;
          *linepos=lineposOld2;
          *match=matchOld;
          while (nodeIter != NULL)
           {
            // Add an atom to the parserLine to point to the beginning of structure
            // This is written to either stack location (offset + stackPos) if first iteration, or (offset + 0) subsequently
            // In any repeating structure, first stack position is pointer to next item
            status = parse_descend(c,s,writeOut,srcLineN,srcId,srcFname,line,linepos,nodeIter,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
            if (status==3) status=0;
            if (status!=1) goto repCleanup;
            nodeIter = nodeIter->nextSibling;
           }
         }
        first=0;
       }
repCleanup:
      s->pl[blockDepth]->stackOffset = oldStackOffset;
      goto cleanup;
     }
    case PN_TYPE_OPT:
     {
      int         lineposOld = *linepos;
      int         matchOld   = *match;
      parserNode *nodeIter   = node->firstChild;
      int         wo         = 0;
      while (1)
       {
        while (nodeIter != NULL)
         {
          status = parse_descend(c,s,wo,srcLineN,srcId,srcFname,line,linepos,nodeIter,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
          if (status==3) { status=0; goto cleanup; }
          if (status==0) { *linepos=lineposOld; *match=matchOld; status=1; goto cleanup; }
          if (status==2) goto cleanup;
          nodeIter = nodeIter->nextSibling;
         }

        // The dry run succeeded, so do it again, for real
        if (wo==writeOut) goto cleanup;
        *linepos=lineposOld; *match=matchOld;
        nodeIter=node->firstChild;
        wo=writeOut;
       }
     }
    case PN_TYPE_PER:
     {
#define PER_MAXSIZE 64
      int repeating=1;
      unsigned char excluded[PER_MAXSIZE];
      memset((void *)excluded,0,PER_MAXSIZE);
      while (repeating)
       {
        int         count      = 0;
        int         lineposOld = *linepos;
        int         matchOld   = *match;
        parserNode *nodeIter   = node->firstChild;
        int         wo         = 0, secondPass=0;
        repeating=0;
        while (nodeIter != NULL)
         {
          if (excluded[count]) { nodeIter=nodeIter->nextSibling; count++; continue; } 

          status = parse_descend(c,s,wo,srcLineN,srcId,srcFname,line,linepos,nodeIter,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
          if (secondPass) { excluded[count]=1; repeating=1; break; }
          if (status==3) { status=0; }
          if (status==0) { *linepos=lineposOld; *match=matchOld; nodeIter=nodeIter->nextSibling; count++; continue; }
          if (status==2) goto cleanup;
          *linepos=lineposOld; *match=matchOld;
          wo=writeOut; secondPass=1;
         }
       }
      status=1;
      goto cleanup;
     }
    case PN_TYPE_ORA:
     {
      int         lineposOld = *linepos;
      int         matchOld   = *match;
      parserNode *nodeIter   = node->firstChild;
      int         wo         = 0, secondPass=0;
      while (nodeIter != NULL)
       {
        status = parse_descend(c,s,wo,srcLineN,srcId,srcFname,line,linepos,nodeIter,tabCompNo,tabCompStart,tabCompTxt,level+1,blockDepth,match);
        if (secondPass) goto cleanup;
        if (status==3) { status=0; goto cleanup; }
        if (status==0) { *linepos=lineposOld; *match=matchOld; nodeIter=nodeIter->nextSibling; continue; }
        if (status==2) goto cleanup;
        *linepos=lineposOld; *match=matchOld;
        wo=writeOut; secondPass=1;
       }
      goto cleanup;
     }
    default:
     {
      sprintf(c->errcontext.tempErrStr, "Hit an unexpected node type %d.", node->type);
      ppl_fatal(&c->errcontext,__FILE__,__LINE__,NULL);
     }
   }

cleanup:
  // If we are not going back for another line in a CODEBLOCK, clear the parserNode stack
  if (s!=NULL)
   {
    if (status!=3) s->stk[blockDepth][level] = NULL;
    else           s->stk[blockDepth][level] = node;
   }

  // If we are at the top level of the hierarchy and we have failed to parse, throw a traceback now.
  // If we have succeeded in parsing, we should by now have consumed a whole line; if there is trailig text, that's an error
  if ((level==0) && (blockDepth==0) && (status<2) && (s!=NULL))
   {
    if (status==1)
     {
      while ((line[*linepos]>'\0') && (line[*linepos]<=' ')) (*linepos)++;
      if (line[*linepos]!='\0')
       {
        if (s->eLPos>0) { snprintf(s->expectingList+s->eLPos,LSTR_LENGTH-s->eLPos,", or "); s->eLPos+=strlen(s->expectingList+s->eLPos); }
        snprintf(s->expectingList+s->eLPos,LSTR_LENGTH-s->eLPos,"the end of the command");
        s->eLPos+=strlen(s->expectingList+s->eLPos);
        status=0;
       }
     }
    if (status==0)
     {
      snprintf(c->errStat.errBuff,LSTR_LENGTH,"At this point, was expecting %s.",s->expectingList);
      c->errStat.errBuff[LSTR_LENGTH-1] = '\0';
      ppl_tbAdd(c,srcLineN,srcId,srcFname,1,ERR_SYNTAX,*linepos,line);
     }
   }

  if (status==1) ppl_tbClear(c);
  return status;
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
  int i,j;
  parserLine **pl = in->rootpl;
  if (*pl != NULL) ppl_parserLineFree(*pl);
  *pl = NULL;
  for (i=0; i<MAX_RECURSION_DEPTH; i++) in->pl[i]=NULL;
  for (i=0; i<MAX_RECURSION_DEPTH; i++) for (j=0; j<16; j++) in->stk[i][j]=NULL;
  strcpy(in->prompt, "pyxplot");
  in->blockDepth = 0;
  in->eLPos      = 0;
  in->eLlinePos  = 0;
  in->expectingList[0] = '\0';
  return;
 }

void ppl_parserStatAdd(parserStatus *in, int bd, parserLine *pl)
 {
  if (in->pl[bd] != NULL)
   {
    in->pl[bd] = in->pl[bd]->next = pl;
   }
  else
   {
    if (bd==0) *(in->rootpl) = pl;
    in->pl[bd] = pl;
   }
  return;
 }

void ppl_parserStatFree(parserStatus **in)
 {
  parserLine **pl = (*in)->rootpl;
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
  output->firstAtom      = NULL;
  output->lastAtom       = NULL;
  output->next           = NULL;
  output->containsMacros = 0;
  output->srcLineN       = srcLineN;
  output->srcId          = srcId;
  output->stackLen       = 16;
  output->stackOffset    = 0;

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

int ppl_parserCompile(ppl_context *c, parserStatus *s, int srcLineN, long srcId, char *srcFname, char *line, int expandMacros, int blockDepth)
 {
  listIterator *cmdIter=NULL;
  int           i, cln, containsMacros=0, fail=0;
  int           obLen = LSTR_LENGTH, obPos;
  char         *outbuff = NULL;

  if (blockDepth > MAX_RECURSION_DEPTH) { strcpy(c->errStat.errBuff,"Maximum recursion depth exceeded."); ppl_tbAdd(c,srcLineN,srcId,srcFname,1,ERR_OVERFLOW,0,line); ppl_parserStatReInit(s); return 1; }

  // Deal with macros and ` ` substitutions
  if (!expandMacros)
   {
    TEST_FOR_MACROS;
    if (containsMacros) // If we're not expanding macros at this stage, flag this parserLine as containing macros, and exit
     {
      parserLine *output=NULL;
      ppl_parserLineInit(&output, srcLineN, srcId, srcFname, line);
      if (output==NULL) { sprintf(c->errStat.errBuff,"Out of memory."); ppl_tbAdd(c,srcLineN,srcId,srcFname,1,ERR_MEMORY,0,line); ppl_parserStatReInit(s); return 1; }
      output->stackLen       = 0;
      output->containsMacros = 1;
      ppl_parserStatAdd(s, blockDepth, output);
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
  cmdIter = ppl_listIterateInit(pplParserCmdList[cln]);
  while (1)
   {
    int match=0, linepos=0;
    parserNode *item;

    if (cmdIter == NULL) // Once we've finished cycling through commands that start with punctuation, cycle through ones that start with first letter
     {
      if (cln==26) break;
      cmdIter = ppl_listIterateInit(pplParserCmdList[cln=26]);
      if (cmdIter==NULL) break;
     }

    // Fetch next item from list of possible commands and try to parse it
    item  = (parserNode *)ppl_listIterate(&cmdIter);
    match = linepos = 0;
    parse_descend(c, s, 1, srcLineN, srcId, srcFname, line, &linepos, item, NULL, -1, NULL, 0, blockDepth, &match);

    // If this command did not match as far as the '=' token, clear stack and traceback
    if (match==0)
     {
      ppl_tbClear(c);
      ppl_parserLineFree(s->pl[blockDepth]);
      s->pl[blockDepth]=NULL;
      if (blockDepth==0) *(s->rootpl) = NULL;
      s->eLPos = s->eLlinePos = 0;
      s->expectingList[0] = '\0';
      continue;
     }

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
    int           cln=0;
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
        if (cln==26) break;
        cmdIter = ppl_listIterateInit(pplParserCmdList[cln++]);
        if (cmdIter==NULL) continue;
       }

      // Fetch next item from list of possible commands and try to parse it
      item  = (parserNode *)ppl_listIterate(&cmdIter);
      match = linepos = 0;
      parse_descend(rootContext, NULL, 0, -1, -1, "", line, &linepos, item, &number, start, tabCompTxt, 0, 0, &match);

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
  char        **matches;
  char         *firstItem;

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

