// help.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2013 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2013 Ross Church
//
// $Id$
//
// Pyxplot is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// You should have received a copy of the GNU General Public License along with
// Pyxplot; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA  02110-1301, USA

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "coreUtils/dict.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/stringList.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "userspace/context.h"

#include "pplConstants.h"

#define MAX_HELP_DEPTH 16 // The maximum possible depth of a help hierarchy
#define MAX_HELP_HITS  12 // The maximum possible number of help topics which match the same autocomplete strings

static void help_PagerDisplay(pplerr_context *c, char *PageName, xmlNode *node, int interactive)
 {
  xmlNode *cur_node;
  char     textBuffer      [LSTR_LENGTH];
  char     textBuffer2     [LSTR_LENGTH];
  char     defaultpagerName[] = "less";
  char    *pagerName;
  char    *Ncolumns_text;
  int      i, j, Ncolumns, Nchildren, strlen_version, strlen_date;
  FILE    *pagerHandle;

  sprintf(textBuffer,"\\\\**** Help Topic: %s****\\\\", PageName);
  i = strlen(textBuffer);
  for (cur_node = node->children; cur_node; cur_node = cur_node->next) // Loop over children
   if (cur_node->type == XML_TEXT_NODE)
    {
     strlen_version = strlen("$VERSION");
     strlen_date    = strlen("$DATE"   );
     for (j=0; cur_node->content[j]!='\0'; )
      {
       if      (strncmp((char *)cur_node->content+j,"$VERSION",strlen_version)==0)
        { j+=strlen_version; strcpy(textBuffer+i,VERSION); i += strlen(textBuffer+i); }
       else if (strncmp((char *)cur_node->content+j,"$DATE"   ,strlen_date   )==0)
        { j+=strlen_date   ; strcpy(textBuffer+i,DATE   ); i += strlen(textBuffer+i); }
       else
        { textBuffer[i++] = cur_node->content[j++]; }
      }
    } // NB, textBuffer is not null terminated at this point, but we're about to add stuff to it...

  sprintf(textBuffer+i, "\\\\\\\\\\\\");
  // Insert information about children
  Nchildren = 0;
  for (cur_node = node->children; cur_node; cur_node = cur_node->next) // Loop over children
   if (cur_node->type == XML_ELEMENT_NODE)
    {
     if (Nchildren == 0) { sprintf(textBuffer+i,"This help page has the following subtopics:\\\\\\\\"); i += strlen(textBuffer+i); }
     else                { sprintf(textBuffer+i,", ");                                                  i += strlen(textBuffer+i); }
     sprintf(textBuffer+i,"%s",cur_node->name); i += strlen(textBuffer+i);
     Nchildren++;
    }
  if (Nchildren == 0) { sprintf(textBuffer+i,"This help page has no subtopics.\\\\\\\\"); i += strlen(textBuffer+i); }
  else                { sprintf(textBuffer+i,".\\\\\\\\");                                i += strlen(textBuffer+i); }

  // If interactive, tell user how to quit, and also work out column width of display
  if (interactive!=0)
   {
    sprintf(textBuffer+i,"Press the 'Q' key to exit this help page.\\\\");
    Ncolumns_text = getenv("COLUMNS");
    if (!((Ncolumns_text != NULL) && (Ncolumns = ppl_getFloat(Ncolumns_text, &i), i==strlen(Ncolumns_text)))) Ncolumns = 80;
    pagerName = getenv("PAGER");
    if (pagerName == NULL) pagerName = defaultpagerName;
   } else {
    Ncolumns = 80;
    pagerName = defaultpagerName;
   }

  // Word wrap the text we have, and send it to a pager
  ppl_strWordWrap(textBuffer, textBuffer2, Ncolumns);
  if (interactive == 0)
   {
    ppl_report(c,textBuffer2); // In non-interactive sessions, we just send text to stdout
   }
  else
   {
    if ((pagerHandle = popen(pagerName,"w"))==NULL) { ppl_error(c, ERR_INTERNAL, -1, -1, "Cannot open pipe to pager application."); }
    else
     {
      fprintf(pagerHandle, "%s", textBuffer2);
      pclose(pagerHandle);
     }
   }
  return;
 }

static void help_TopicPrint(char *out, char **words, int Nwords)
 {
  int i,j;
  for (i=0,j=0; i<=Nwords; i++)
   {
    strcpy(out+j, words[i]);
    j += strlen(out+j);
    out[j++]=' ';
   }
  out[j]='\0';
 }

static void help_MatchFound(pplerr_context *c, xmlNode *node, xmlNode **matchNode, list *topicWords, int *matchTextPos, int *Nmatches, int *ambiguous, char **helpPosition, char **helpTexts)
 {
  int   abbreviation,i;
  list *prevMatchPosition;

  if ((*Nmatches)==MAX_HELP_HITS) { ppl_error(c, ERR_INTERNAL, -1, -1, "Cannot parse ppl_help.xml. Need to increase MAX_HELP_HITS."); return; }
  help_TopicPrint(helpTexts[*Nmatches], helpPosition, ppl_listLen(topicWords));
  if (*ambiguous == 0)
   {
    if (*matchNode == NULL)
     {
      *matchTextPos = 0;
      *matchNode    = node; // This is the first match we've found
     } else {
      prevMatchPosition = ppl_strSplit( helpTexts[*matchTextPos] );

      abbreviation = 1; // If previous match is an autocomplete shortening of current match, let it stand
      for (i=0; i<=ppl_listLen(topicWords); i++) if (ppl_strAutocomplete( (char *)ppl_listGetItem(prevMatchPosition,i) , helpPosition[i] , 1)==-1) {abbreviation=0; break;}
      if (abbreviation!=1)
       {
        abbreviation = 1; // If current match is an autocomplete shortening of previous match, the current match wins
        for (i=0; i<=ppl_listLen(topicWords); i++) if (ppl_strAutocomplete( helpPosition[i] , (char *)ppl_listGetItem(prevMatchPosition,i) , 1)==-1) {abbreviation=0; break;}
        if (abbreviation==1)
         {
          *matchTextPos = *Nmatches;
          *matchNode    = node;
         } else {
          *ambiguous = 1; // We have multiple ambiguous matches
         }
       }
     }
   }
  (*Nmatches)++;
  return;
 }

void help_explore(pplerr_context *c, xmlNode *node, xmlNode **matchNode, list *topicWords, int *matchTextPos, int *Nmatches, int *ambiguous, char **helpPosition, char **helpTexts, int depth)
 {
  xmlNode *cur_node = NULL;
  int match,i;

  if (depth>ppl_listLen(topicWords)) return;
  if (depth==MAX_HELP_DEPTH) { ppl_error(c, ERR_INTERNAL, -1, -1, "Cannot parse ppl_help.xml. Need to increase MAX_HELP_DEPTH."); return; }

  for (cur_node = node; cur_node; cur_node = cur_node->next) // Loop over siblings
   {
    match=1;
    if (cur_node->type == XML_ELEMENT_NODE)
     {
      sprintf(helpPosition[depth], "%s", cur_node->name); // Converted signedness of chars
      for (i=1;i<=depth;i++) if (ppl_strAutocomplete( (char *)ppl_listGetItem(topicWords,i-1) , helpPosition[i], 1)==-1) {match=0; break;}
      if ((match==1) && (depth==ppl_listLen(topicWords))) help_MatchFound(c,cur_node,matchNode,topicWords,matchTextPos,Nmatches,ambiguous,helpPosition,helpTexts);
     }
    if (match==1) help_explore(c,cur_node->children,matchNode,topicWords,matchTextPos,Nmatches,ambiguous,helpPosition,helpTexts,depth+1);
   }
  return;
 }

void ppl_directive_help(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplerr_context *e     = &c->errcontext;
  list    *topicWords   = NULL; // A list of the help topic words supplied by the user
  char    *topicString  = NULL; // The string of help topic words supplied by the user
  char     filename[FNAME_LENGTH]; // The filename of ppl_help.xml
  char    *helpPosition[MAX_HELP_DEPTH]; // A list of xml tags, used to keep track of our position as we traverse the xml hierarchy
  char    *helpTexts   [MAX_HELP_HITS ]; // A list of all of the help topics which have matched the user's request
  xmlDoc  *doc          = NULL; // The XML document ppl_help.xml
  xmlNode *root_element = NULL; // The root element of the above
  xmlNode *matchNode    = NULL; // The XML node which best fits the user's request
  int      matchTextPos = -1; // The position within helpTexts of the node pointed to by matchNode
  int      ambiguous    = 0; // Becomes one if we find that the user's request matches multiple help pages
  int      Nmatches     = 0; // Counts the number of pages the user's request matches
  int      i;

  topicString = (char *)in->stk[PARSE_help_topic].auxil;
  topicWords = ppl_strSplit( topicString );

  sprintf(filename, "%s%s%s", SRCDIR, PATHLINK, "help.xml"); // Find help.xml
  LIBXML_TEST_VERSION

  doc = xmlReadFile(filename, NULL, 0);

  if (doc==NULL)
   {
    sprintf(e->tempErrStr, "Help command cannot find help data in expected location of '%s'.", filename);
    ppl_error(e, ERR_INTERNAL, -1, -1, NULL);
    return;
   }

  for (i=0;i<MAX_HELP_DEPTH;i++) if ((helpPosition[i]=(char *)ppl_memAlloc(SSTR_LENGTH*sizeof(char)) )==NULL) return;
  for (i=0;i<MAX_HELP_HITS ;i++) if ((helpTexts   [i]=(char *)ppl_memAlloc(SSTR_LENGTH*sizeof(char)) )==NULL) return;

  root_element = xmlDocGetRootElement(doc);
  help_explore(e, root_element, &matchNode, topicWords, &matchTextPos, &Nmatches, &ambiguous, helpPosition, helpTexts, 0);

  if (ambiguous == 1)
   {
    ppl_report(e,"Ambiguous help request. The following help topics were matched:");
    for(i=0;i<Nmatches;i++) ppl_report(e, helpTexts[i] );
    ppl_report(e,"Please make your help request more specific, and try again.");
   }
  else if (matchNode == NULL)
   {
    ppl_report(e,"Please make your help request more specific, and try again.");
   }
  else
   {
    help_PagerDisplay(e , helpTexts[matchTextPos] , matchNode , interactive );
   }

  xmlFreeDoc(doc); // Tidy up
  xmlCleanupParser();
  return;
 }

