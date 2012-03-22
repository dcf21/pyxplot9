// asciidouble.c
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include <gsl/gsl_math.h>

/* ppl_getFloat(): This gets a float from a string */

double ppl_getFloat(const char *str, int *Nchars)
 {
  double accumulator = 0;
  int decimals = 0;
  unsigned char past_decimal_point = 0;
  unsigned char negative = 0;
  int pos = 0;
  int pos2= 0;

  if ((str[pos+0]=='n') && (str[pos+1]=='a') && (str[pos+2]=='n') && (str[pos+3]>='\0') && (str[pos+3]<=' ')) { if (Nchars!=NULL) *Nchars=3; return GSL_NAN; }
  if ((str[pos+0]=='N') && (str[pos+1]=='A') && (str[pos+2]=='N') && (str[pos+3]>='\0') && (str[pos+3]<=' ')) { if (Nchars!=NULL) *Nchars=3; return GSL_NAN; }

  if      (str[pos] == '-') { negative = 1; pos++; }  // Deal with negatives
  else if (str[pos] == '+') {               pos++; }  // Deal with e.g. 1E+09

  if ((str[pos+0]=='0') && (str[pos+1]=='x')) // Hexadecimal
   {
    pos+=2;
    while (1)
     {
      if      ((str[pos]>='0') && (str[pos]<='9')) accumulator = ((16 * accumulator) + (((int)str[pos])-48));
      else if ((str[pos]>='A') && (str[pos]<='F')) accumulator = ((16 * accumulator) + (((int)str[pos])-55));
      else if ((str[pos]>='a') && (str[pos]<='f')) accumulator = ((16 * accumulator) + (((int)str[pos])-87));
      else                                         break;
      pos++;
     }
    if (negative == 1) accumulator *= -1; // Deal with negatives
   }
  else
   {
    while (((str[pos]>='0') && (str[pos]<='9')) || (str[pos] == '.'))
     {
      if (str[pos] == '.')
       {
        past_decimal_point = 1;
       } else {
        accumulator = ((10 * accumulator) + (((int)str[pos])-48));
        if (past_decimal_point == 1) decimals++;
       }
      pos++;
     }

    while (decimals != 0) // Deals with decimals
     {
      decimals--;
      accumulator /= 10;
     }

    if (negative == 1) accumulator *= -1; // Deals with negatives
    if ((str[pos] == 'e') || (str[pos] == 'E')) accumulator *= pow(10.0,ppl_getFloat(str+pos+1 , &pos2)); /* Deals with exponents */
    if (pos2   >     0) pos += (1+pos2); // Add on characters taken up by exponent, including one for the 'e' character.
   }
  if (pos    ==    0) pos = -1; // Alert the user that this was a blank string!
  if (Nchars != NULL) *Nchars = pos;
  return(accumulator);
 }

/* ppl_validFloat(): Sees whether candidate string is a valid float */

int ppl_validFloat(const char *str, int *end)
 {
  unsigned char past_decimal_point=0, had_number=0, expvalid=1;
  int pos = 0;
  int pos2= 0;

  if ((str[pos+0]=='n') && (str[pos+1]=='a') && (str[pos+2]=='n') && (str[pos+3]>='\0') && (str[pos+3]<=' ')) { pos+=3; had_number=1; goto VALID_FLOAT_ENDED; }
  if ((str[pos+0]=='N') && (str[pos+1]=='A') && (str[pos+2]=='N') && (str[pos+3]>='\0') && (str[pos+3]<=' ')) { pos+=3; had_number=1; goto VALID_FLOAT_ENDED; }

  if      (str[pos] == '-') { pos++; }  /* Deal with negatives */
  else if (str[pos] == '+') { pos++; }  /* Deal with e.g. 1E+09 */

  if ((str[pos+0]=='0') && (str[pos+1]=='x')) // Hexadecimal
   {
    pos+=2;
    while (1)
     {
      if      ((str[pos]>='0') && (str[pos]<='9')) pos++;
      else if ((str[pos]>='A') && (str[pos]<='F')) pos++;
      else if ((str[pos]>='a') && (str[pos]<='f')) pos++;
      else                                         break;
      had_number = 1;
     }
    if (!had_number) return 0;
   }
  else
   {
    while (((str[pos]>='0') && (str[pos]<='9')) || (str[pos] == '.'))
     {
      if (str[pos] == '.')
       {
        if (past_decimal_point) goto VALID_FLOAT_ENDED;
        else                    past_decimal_point = 1;
       }
      else
       { had_number = 1; }
      pos++;
     }

    if (!had_number) return 0;

    if ((str[pos] == 'e') || (str[pos] == 'E')) /* Deals with exponents */
     {
      expvalid = ppl_validFloat(str+pos+1 , &pos2);
      pos += pos2+1;
     }

   }
  while ((str[pos]!='\0')&&(str[pos]<=' ')) pos++; /* Fast-forward over spaces at end */

VALID_FLOAT_ENDED:
  if ((!had_number)||(!expvalid)) return 0;
  if (end==NULL) return 1;
  if ((*end>=0)&&(pos<*end)) return 0;
  *end = pos;
  return 1;
 }

/* ppl_numericDisplay(): Displays a double in either %f or %e formats */

char *ppl_numericDisplay(double in, char *output, int SigFig, int latex)
 {
  char   format[16];
  double x, AccLevel;
  int    DecimalLevel, DPmax, i, j, k, l;
  if ((fabs(in) < 1e10) && (fabs(in) > 1e-3))
   {
    x = fabs(in);
    AccLevel = x*(1.0+pow(10,-SigFig));
    DPmax    = SigFig-log10(x);
    for (DecimalLevel=0; DecimalLevel<DPmax; DecimalLevel++) if ((x - ((floor(x*pow(10,DecimalLevel))/pow(10,DecimalLevel)) - x))<AccLevel) break;
    sprintf(format,"%%.%df",DecimalLevel);
    sprintf(output,format,in);
   }
  else
   {
    if (in==0)
     { sprintf(output,"0"); }
    else
     {
      x  = fabs(in);
      x /= pow(10,(int)log10(x));
      AccLevel = x*(1.0+pow(10,-SigFig));
      for (DecimalLevel=0; DecimalLevel<SigFig; DecimalLevel++) if ((x - ((floor(x*pow(10,DecimalLevel))/pow(10,DecimalLevel)) - x))<AccLevel) break;
      sprintf(format,"%%.%de",DecimalLevel);
      sprintf(output,format,in);
      if (latex) // Turn 1e10 into nice latex
       {
        for (i=0;((output[i]!='\0')&&(output[i]!='e')&&(output[i]!='E'));i++);
        if (output[i]!='\0')
         {
          for (j=i,k=i+32;output[j]!='\0';j++) output[j+32]=output[j];
          output[j+32]='\0';
          if ((i==1)&&(output[0]=='1')) { strcpy(output  ,       "10^{"); i =strlen(output  ); } // Don't output 1 times 10^3
          else                          { strcpy(output+i,"\\times10^{"); i+=strlen(output+i); } // Replace e with times ten to the...
          k++; // FFW over the E
          if (output[k]=='+') k++; // We don't need to say +8... 8 will do
          for (l=0,j=k;output[j]!='\0';j++) { if ((output[j]>'0')&&(output[j]<='9')) l=1; if ((l==1)||(output[j]!='0')) output[i++]=output[j]; } // Turn -08 into -8
          output[i++]='}';
          output[i++]='\0';
         }
       }
     }
   }
  for (i=0; ((output[i]!='\0')&&(output[i]!='.')); i++); // If we have trailing decimal zeros, get rid of them
  if (output[i]!='.') return output;
  for (j=i+1; isdigit(output[j]); j++);
  if (i==j) return output;
  for (k=j-1; output[k]=='0'; k--);
  if (k==i) k--;
  k++;
  if (k==j) return output;
  for (l=0; output[j+l]!='\0'; l++) output[k+l] = output[j+l];
  output[k+l]='\0';
  return output;
 }

// Useful function for checking whether two doubles are roughly equal to one another
unsigned char ppl_dblEqual(double a, double b)
 {
  if ( (fabs(a) < 1e-100) && (fabs(b) < 1e-100) ) return 1;
  if ( (fabs(a-b) > fabs(1e-7*a)) || (fabs(a-b) > fabs(1e-7*b)) ) return 0;
  return 1;
 }

unsigned char ppl_dblApprox(double a, double b, double err)
 {
  if ( fabs(a-b) > (fabs(err)+1e-50) ) return 0;
  return 1;
 }

/* ppl_file_readline(): This remarkably useful function forwards a file to the next newline */

void ppl_file_readline(FILE *file, char *output, int MaxLength)
{
 char c = '\x07';
 char *outputscan = output;
 int i=0;

 while (((int)c != '\n') && (!feof(file)) && (!ferror(file)))
   if ((fscanf(file,"%c",&c)>=0) && ((c>31)||(c==9)) && (i<MaxLength-2)) { i++; *(outputscan++) = c; } // ASCII 9 is a tab
  *(outputscan++) = '\0';
}

/* ppl_getWord(): This returns the first word (terminated by any whitespace). Maximum <max> characters. */

void ppl_getWord(char *out, const char *in, int max)
 {
  int count = 0;
  while  ((*in <= ' ') && (*in > '\0')) in++; /* Fastforward over preceeding whitespace */
  while (((*in >  ' ') || (*in < '\0')) && (count < (max-1)))
   {
    *(out++) = *(in++);
    count++;
   }
  *out = '\0'; /* Terminate output */
 }

/* ppl_nextWord(): Fast forward over word, and return pointer to next word */

char *ppl_nextWord(char *in)
 {
  while ((*in <= ' ') && (*in > '\0')) in++; /* Fastforward over preceeding whitespace */
  while ((*in >  ' ') || (*in < '\0')) in++; /* Fastforward over one word */
  while ((*in <= ' ') && (*in > '\0')) in++; /* Fastforward over whitespace before next word */
  return(in); /* Return pointer to next word */
 }

/* ppl_friendlyTimestring(): Returns pointer to time string */

char *ppl_friendlyTimestring()
 {
  time_t timenow;
  timenow = time(NULL);
  return( ctime(&timenow) );
 }

/* ppl_strStrip(): Strip whitespace from both ends of a string */

char *ppl_strStrip(const char *in, char *out)
 {
  char *scan = out;
  while ((*in <= ' ') && (*in > '\0')) in++;
  while (*in != '\0') *(scan++)=*(in++);
  scan--;
  while ((scan>out) && (*scan >= '\0') && (*scan <= ' ')) scan--;
  *++scan = '\0';
  return out;
 }

/* ppl_strUpper(): Capitalise a string */

char *ppl_strUpper(const char *in, char *out)
 {
  char *scan = out;
  while (*in != '\0')
   if ((*in >='a') && (*in <='z')) *scan++ = *in++ +'A'-'a';
   else                            *scan++ = *in++;
  *scan = '\0';
  return out;
 }

/* ppl_strLower(): Lowercase a string */

char *ppl_strLower(const char *in, char *out)
 {
  char *scan = out;
  while (*in != '\0')
   if ((*in >='A') && (*in <='Z')) *scan++ = *in++ +'a'-'A';
   else                            *scan++ = *in++;
  *scan = '\0';
  return out;
 }

/* ppl_strUnderline(): Underline a string */

char *ppl_strUnderline(const char *in, char *out)
 {
  char *scan = out;
  while (*in != '\0') { if ((*in>=' ')||(*in<'\0')) *scan++='-'; in++; }
  *scan = '\0';
  return out;
 }

/* ppl_strRemoveCompleteLine(): Removes a single complete line from a text buffer, if there is one */

char  *ppl_strRemoveCompleteLine(char *in, char *out)
 {
  char *scan, *scan2, *scanout;
  scan = scan2 = in;
  scanout      = out;
  while ((*scan != '\0') && (*scan != '\n')) scan++; // Find first carriage-return in text buffer
  if (*scan != '\0') while (scan2<scan) *scanout++=*scan2++; // If one is found, copy up to this point into temporary string processing buffer
  *scanout = '\0';
  ppl_strStrip(out, out); // Strip it, and then this is what we will return

  if (*scan != '\0') // If we've taken a line out of the buffer, delete it from buffer
   {
    scan2 = in;
    while (*scan == '\n') scan++; // Forward over carriage return
    while (*scan != '\0') *scan2++ = *scan++;
    *scan2 = '\0';
   }
  return out;
 }

/* ppl_strSlice(): Take a slice out of a string */

char *ppl_strSlice(const char *in, char *out, int start, int end)
 {
  char *scan = out;
  int   pos  = 0;
  while ((pos<start) && (in[pos]!='\0')) pos++;
  while ((pos<end  ) && (in[pos]!='\0')) *(scan++) = in[pos++];
  *scan = '\0';
  return out;
 }

/* StrCommaSeparatedListScan(): Split up a comma-separated list into individual values */

char *ppl_strCommaSeparatedListScan(char **inscan, char *out)
 {
  char *outscan = out;
  while ((**inscan != '\0') && (**inscan != ',')) *(outscan++) = *((*inscan)++);
  if (**inscan == ',') (*inscan)++; // Fastforward over comma character
  *outscan = '\0';
  ppl_strStrip(out,out);
  return out;
 }

/* ppl_strAutocomplete(): Test whether a candidate string matches the beginning of test string, and is a least N characters long */

int ppl_strAutocomplete(const char *candidate, const char *test, int Nmin)
 {
  int i,j;

  if ((candidate==NULL) || (test==NULL)) return -1;

  for (j=0; ((candidate[j]>'\0') && (candidate[j]<=' ')); j++); // Fastforward over whitespace at beginning of candidate

  for (i=0; 1; i++,j++)
   if (test[i]!='\0')
    {
     if (toupper(test[i]) != toupper(candidate[j])) // Candidate string has deviated from test string
      {
       if (i<Nmin) return -1; // We've not passed enough characters yet

       if (candidate[j]<=' ') // Word teminated with space...
        {
         return j;
        }
       else if ((isalnum(candidate[j])==0) && (candidate[j]!='_')) // Word teminated with punctuation...
        {
         // Alphanumeric test strings can be terminated by punctuation; others must have spaces after them
         for (i=0; test[i]!='\0'; i++) if ((test[i]>' ') && (isalnum(test[i])==0) && (test[i]!='_')) return -1;
         return j;
        }
       else // Word terminated with more letters
        { return -1; }
      }
    } else { // We've hit the end of the test string
     if (candidate[j]<=' ') // Word teminated with space...
      {
       return j;
      }
     else if ((isalnum(candidate[j])==0) && (candidate[j]!='_')) // Word teminated with punctuation...
      {
       // Alphanumeric test strings can be terminated by punctuation; others must have spaces after them
       for (i=0; test[i]!='\0'; i++) if ((test[i]>' ') && (isalnum(test[i])==0) && (test[i]!='_')) return -1;
       return j;
      }
     else // Word terminated with more letters
      { return -1; }
    }
 }

/* ppl_strWordWrap(): Word wrap a piece of text to a certain width */

void ppl_strWordWrap(const char *in, char *out, int width)
 {
  int WhiteSpace =  1;
  int LastSpace  = -1;
  int LineStart  =  0;
  int LineFeeds  =  0; // If we meet > 1 linefeed during a period of whitespace, it marks the beginning of a new paragraph
  int i,j;
  for (i=0,j=0 ; in[i]!='\0' ; i++)
   {
    if ((WhiteSpace==1) && (in[i]<=' ')) // Once we've encountered one space, ignore any further whitespaceness
     {
      if (j==0) j++; // If we open document with a new paragraph, we haven't already put down a space character to overwrite
      if ((in[i]=='\n') && (++LineFeeds==2)) { out[j-1]='\n'; out[j]='\n'; LineStart=j++; LastSpace=-1; } // Two linefeeds in a period of whitespace means a new paragraph
      continue;
     }
    if ((WhiteSpace==0) && (in[i]<=' ')) // First whitespace character after a string of letters
     {
      if (in[i]=='\n') LineFeeds=1;
      out[j]=' '; LastSpace=j++; WhiteSpace=1;
      continue;
     }
    if ((in[i]=='\\') && (in[i+1]=='\\')) {i++; out[j]='\n'; LineStart=j++; LastSpace=-1; WhiteSpace=1; continue;} // Double-backslash implies a hard linebreak.
    if (in[i]=='#') {out[j++]=' '; WhiteSpace=1; continue;} // A hash character implies a hard space character, used to tabulate data
    WhiteSpace=0; LineFeeds=0;
    if (((j-LineStart) > width) && (LastSpace != -1)) { out[LastSpace]='\n'; LineStart=LastSpace; LastSpace=-1; } // If line is too line, insert a linebreak
    if (strncmp(in+i, "\\lab"    , 4)==0) {i+=3; out[j++]='<'; continue;} // Macros for left-angle-brackets, etc.
    if (strncmp(in+i, "\\rab"    , 4)==0) {i+=3; out[j++]='>'; continue;}
    if (strncmp(in+i, "\\VERSION", 8)==0) {i+=7; strcpy(out+j,VERSION); j+=strlen(out+j); continue;}
    if (strncmp(in+i, "\\DATE"   , 5)==0) {i+=4; strcpy(out+j,DATE   ); j+=strlen(out+j); continue;}
    out[j++] = in[i];
   }
  out[j]='\0';
  return;
 }

/* ppl_strBacketMatch(): Find a closing bracket to match an opening bracket, and optionally return a list of all comma positions */
/*                       'in' should point to the opening bracket character for which we are looking for the closing partner */

void ppl_strBracketMatch(const char *in, char open, char close, int *CommaPositions, int *Nargs, int *ClosingBracketPos, int MaxCommaPoses)
 {
  int  BracketLevel = 0;
  int  inpos        = 0;
  int  commapos     = 0;
  char QuoteType    = '\0';

  for ( ; in[inpos] != '\0'; inpos++)
   {
    if (QuoteType != '\0') // Do not pay attention to brackets inside quoted strings
     {
      if ((in[inpos]==QuoteType) && (in[inpos-1]!='\\')) QuoteType='\0';
      continue;
     }
    else if ((in[inpos]=='\'') || (in[inpos]=='\"')) QuoteType     = in[inpos]; // Entering a quoted string
    else if  (in[inpos]==open)  // Entering a nested level of brackets
     {
      BracketLevel += 1;
      if (BracketLevel == 1)
       if ((CommaPositions != NULL) && ((MaxCommaPoses < 0) || (commapos < MaxCommaPoses))) *(CommaPositions+(commapos++)) = inpos; // Put ( on comma list
     }
    else if  (in[inpos]==close)  // Leaving a nested level of brackets
     {
      BracketLevel -= 1;
      if (BracketLevel == 0) break;
     }
    else if ((in[inpos]==',') && (BracketLevel==1)) // Found a new comma-separated item
     {
      if ((CommaPositions != NULL) && ((MaxCommaPoses < 0) || (commapos < MaxCommaPoses))) *(CommaPositions+(commapos++)) = inpos; // Put , on comma list
     }
   }

  if (in[inpos] == '\0')
   {
    if (Nargs             != NULL) *Nargs             = -1;
    if (ClosingBracketPos != NULL) *ClosingBracketPos = -1;
    return;
   } else {
    if ((CommaPositions   != NULL) && ((MaxCommaPoses < 0) || (commapos < MaxCommaPoses))) *(CommaPositions+(commapos++)) = inpos; // Put ) on comma list
    if (Nargs             != NULL) *Nargs             = commapos-1; // There are N+1 arguments between N commas, but we've also counted ( and ).
    if (ClosingBracketPos != NULL) *ClosingBracketPos = inpos;
    return;
   }
 }

/* ppl_strCmpNoCase(): A case-insensitive version of the standard strcmp() function */

int ppl_strCmpNoCase(const char *a, const char *b)
 {
  char aU, bU;
  while (1)
   {
    if ((*a == '\0')&&(*b == '\0')) return 0;
    if (*a == *b) {a++; b++; continue;}
    if ((*a>='a')&&(*a<='z')) aU=*a-'a'+'A'; else aU=*a;
    if ((*b>='a')&&(*b<='z')) bU=*b-'a'+'A'; else bU=*b;
    if (aU==bU) {a++; b++; continue;}
    if (aU< bU) return -1;
    return 1;
   }
 }

/* ppl_strEscapify(): Inserts escape characters into strings before quote characters */

char *ppl_strEscapify(const char *in, char *out)
 {
  const char *scanin  = in;
  char       *scanout = out;
  *scanout++ = '\"';
  while (*scanin != '\0')
   {
    if (*scanin=='\?') { strcpy(scanout,"\\?"); scanout+=2; scanin++; continue; }
    if (*scanin=='\a') { strcpy(scanout,"\\a"); scanout+=2; scanin++; continue; }
    if (*scanin=='\b') { strcpy(scanout,"\\b"); scanout+=2; scanin++; continue; }
    if (*scanin=='\f') { strcpy(scanout,"\\f"); scanout+=2; scanin++; continue; }
    if (*scanin=='\n') { strcpy(scanout,"\\n"); scanout+=2; scanin++; continue; }
    if (*scanin=='\r') { strcpy(scanout,"\\r"); scanout+=2; scanin++; continue; }
    if (*scanin=='\t') { strcpy(scanout,"\\t"); scanout+=2; scanin++; continue; }
    if (*scanin=='\v') { strcpy(scanout,"\\v"); scanout+=2; scanin++; continue; }
    if ((*scanin=='\'')||(*scanin=='\"')||(*scanin=='\\')) *(scanout++) = '\\';
    *(scanout++) = *(scanin++);
   }
  *scanout++ = '\"';
  *scanout++ = '\0';
  return out;
 }

/* ppl_strWildcardTest(): Test whether test string matches wildcard string */

int ppl_strWildcardTest(const char *test, const char *wildcard)
 {
  int i=0, j, k, mineat=0, maxeat=0;

  while ((wildcard[i]!='\0') && (wildcard[i]!='?') && (wildcard[i]!='*')) if (test[i] != wildcard[i]) { return 0; } else { i++; }
  if (wildcard[i]=='\0') return (test[i]=='\0');

  j=i;
  while ((wildcard[j]=='?') || (wildcard[j]=='*'))
   {
    if (wildcard[j]=='?') { mineat++; maxeat++; }
    else                  { maxeat = 10000; }
    j++;
   }

  for (k=0; k<mineat; k++) if (test[i++]=='\0') return 0;

  for (k=0; k<maxeat-mineat; k++)
   {
    if (ppl_strWildcardTest(test+i,wildcard+j)) return 1;
    if (test[i++]=='\0') return 0;
   }
  return 0;
 }

