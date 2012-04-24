// texify.c
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
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "coreUtils/memAlloc.h"
#include "expressions/expCompile_fns.h"
#include "settings/settingTypes.h"
#include "stringTools/strConstants.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"
#include "pplConstants.h"
#include "texify.h"

#define ENTERMATHMODE "$\\displaystyle "

static char *LatexVarNamesFr[] = {"alpha"  ,"beta"   ,"gamma"  ,"delta"  ,"epsilon","zeta"   ,"eta"    ,"theta"  ,"iota"   ,"kappa"  ,"lambda" ,"mu"     ,"nu"     ,"xi"     ,"pi"     ,"rho"    ,"sigma"  ,"tau"    ,"upsilon","phi"    ,"chi"    ,"psi"    ,"omega"  ,"Gamma"  ,"Delta"  ,"Theta"  ,"Lambda" ,"Xi"     ,"Pi"     ,"Sigma"  ,"Upsilon","Phi"    ,"Psi"    ,"Omega"  ,"aleph"  , NULL};

static char *LatexVarNamesTo[] = {"\\alpha","\\beta","\\gamma","\\delta","\\epsilon","\\zeta","\\eta","\\theta","\\iota","\\kappa","\\lambda","\\mu","\\nu","\\xi","\\pi","\\rho","\\sigma","\\tau","\\upsilon","\\phi","\\chi","\\psi","\\omega","\\Gamma","\\Delta","\\Theta","\\Lambda","\\Xi","\\Pi","\\Sigma","\\Upsilon","\\Phi","\\Psi","\\Omega","\\aleph",NULL};

void ppl_texify_MakeGreek(const char *in, char *out)
 {
  int i,ji=0,jg,k,l,m;
  char *outno_=out;

  while ((isalnum(in[ji]))||(in[ji]=='_'))
   {
    k=0;
    for (i=0; LatexVarNamesFr[i]!=NULL; i++)
     {
      for (jg=0; LatexVarNamesFr[i][jg]!='\0'; jg++) if (LatexVarNamesFr[i][jg]!=in[ji+jg]) break;
      if (LatexVarNamesFr[i][jg]!='\0') continue;
      if (!isalnum(in[ji+jg])) // We have a Greek letter
       {
        strcpy(out, LatexVarNamesTo[i]);
        out += strlen(out);
        ji += jg;
        k=1;
        break;
       }
      else if (isdigit(in[ji+jg]))
       {
        l=ji+jg;
        while (isdigit(in[ji+jg])) jg++;
        if (isalnum(in[ji+jg])) continue;
        sprintf(out, "%s_{", LatexVarNamesTo[i]); // We have Greek letter underscore number
        out += strlen(out);
        for (m=l;m<ji+jg;m++) *(out++)=in[m];
        *(out++) = '}';
        ji += jg;
        k=1;
        break;
       }
     }
    if ((k==0)&&(ji>1))
     {
      for (jg=0; isdigit(in[ji+jg]); jg++);
      if ( (jg>0) && (!isalnum(in[ji+jg])) && (in[ji+jg]!='_') )
       {
        out=outno_;
        sprintf(out, "_{"); // We have an underscore number at the end of variable name
        out += strlen(out);
        for (m=ji;m<ji+jg;m++) *(out++)=in[m];
        *(out++) = '}';
        ji += jg;
        k=1;
       }
     }
    if (k==0)
     {
      for (jg=0; (isalnum(in[ji+jg])); jg++) *(out++)=in[ji+jg];
      ji+=jg;
     }
    outno_=out;
    if (in[ji]=='_')
     {
      *(out++)='\\'; *(out++)='_';
      ji++;
     }
   }
  *(out++)='\0';
  return;
 }

void ppl_texify_string(char *in, char *out, int outlen)
 {
  int i,j,DoubleQuoteLevel=0,QuoteLevel=0;

  for (i=j=0; in[i]!='\0'; i++)
   {
    if (j>outlen-16) { strcpy(out+j, "..."); j+=strlen(out+j); break; }
    if      (in[i]=='\\') { strcpy(out+j, "$\\backslash$"); j+=strlen(out+j); }
    else if (in[i]=='_' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='&' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='%' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='$' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='{' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='}' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='#' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='^' ) { strcpy(out+j, "\\verb|^|"); j+=strlen(out+j); }
    else if (in[i]=='~' ) { strcpy(out+j, "$\\sim$"); j+=strlen(out+j); }
    else if (in[i]=='<' ) { strcpy(out+j, "$<$"); j+=strlen(out+j); }
    else if (in[i]=='>' ) { strcpy(out+j, "$>$"); j+=strlen(out+j); }
    else if ((in[i]=='\"')&&((i==0)||(in[i-1]!='\\'))) { out[j++]=DoubleQuoteLevel?'\'':'`'; out[j++]=DoubleQuoteLevel?'\'':'`'; DoubleQuoteLevel=!DoubleQuoteLevel; }
    else if ((in[i]=='\'')&&((i==0)||(in[i-1]!='\\'))) { out[j++]=QuoteLevel?'\'':'`'; QuoteLevel=!QuoteLevel; }
    else                  { out[j++]=in[i]; }
   }
  out[j++]='\0';
  return;
 }

void ppl_texify_generic(ppl_context *c, char *in, int *end, char *out, int outlen)
 {
  const int  allowCommaOperator = 1;
  const int  equalsAllowed      = 1;
  const int  dollarAllowed      = 1;
  int        errPos=-1, errType, tlen;
  char       errbuff[LSTR_LENGTH];

  // First tokenise expression
  ppl_expTokenise(c, in, end, dollarAllowed, equalsAllowed, allowCommaOperator, 0, 0, 0, &tlen, &errPos, &errType, errbuff);
  if (errPos>=0) { *end=0; return; }

  strcpy(out, "texify output");
  return;
 }

