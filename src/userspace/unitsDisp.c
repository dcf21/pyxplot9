// unitsDisp.c
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

#define _PPL_UNITSDISP_C 1

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjUnits.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

const char *SIprefixes_full  [] = {"yocto","zepto","atto","femto","pico","nano","micro","milli","","kilo","mega","giga","tera","peta","exa","zetta","yotta"};
const char *SIprefixes_abbrev[] = {"y","z","a","f","p","n","u","m","","k","M","G","T","P","E","Z","Y"};
const char *SIprefixes_latex [] = {"y","z","a","f","p","n","\\upmu ","m","","k","M","G","T","P","E","Z","Y"};

// NB: The LaTeX upmu character is defined by the {upgreek} package

// Display a value with units
char *ppl_unitsNumericDisplay(ppl_context *c, pplObj *in, int N, int typeable, int NSigFigs)
 {
  double numberOutReal, numberOutImag, OoM;
  char *output, *unitstr;
  int i=0;
  if (N==0) output = c->udNumDispA;
  else      output = c->udNumDispB;

  if (NSigFigs <= 0) NSigFigs = c->set->term_current.SignificantFigures; // If number of significant figures not specified, use user-selected number

  if ((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) && (in->flagComplex!=0)) return ppl_numericDisplay(GSL_NAN, c->numdispBuff[N], NSigFigs, (typeable==SW_DISPLAY_L));

  if (typeable==0) typeable = c->set->term_current.NumDisplay;
  unitstr = ppl_printUnit(c, in, &numberOutReal, &numberOutImag, N, 1, typeable);

  if (((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) && (in->flagComplex!=0)) || (!gsl_finite(numberOutReal)) || (!gsl_finite(numberOutImag)))
   {
    if (typeable == SW_DISPLAY_L) output[i++] = '$';
    strcpy(output+i, ppl_numericDisplay(GSL_NAN, c->numdispBuff[N], NSigFigs, (typeable==SW_DISPLAY_L)));
    i+=strlen(output+i);
   }
  else
   {
    OoM = hypot(numberOutReal , numberOutImag) * pow(10 , -NSigFigs);

    if (typeable == SW_DISPLAY_L) output[i++] = '$';
    if ((fabs(numberOutReal) >= OoM) && (fabs(numberOutImag) > OoM)) output[i++] = '('; // open brackets on complex number
    if (fabs(numberOutReal) >= OoM) { strcpy(output+i, ppl_numericDisplay(numberOutReal, c->numdispBuff[N], NSigFigs, (typeable==SW_DISPLAY_L))); i+=strlen(output+i); }
    if ((fabs(numberOutReal) >= OoM) && (fabs(numberOutImag) > OoM) && (numberOutImag > 0)) output[i++] = '+';
    if (fabs(numberOutImag) > OoM)
     {
      if (fabs(numberOutImag-1.0)>=OoM) // If this is false, imaginary part is 1, so print +i, not +1i
       {
        if (fabs(numberOutImag+1.0)>=OoM)
         {
          strcpy(output+i, ppl_numericDisplay(numberOutImag, c->numdispBuff[N], NSigFigs, (typeable==SW_DISPLAY_L)));
          i+=strlen(output+i);
         }
        else
         { output[i++]='-'; } // Just print -i, not -1i
       }
      if      (typeable != SW_DISPLAY_T)                                       { output[i++] = 'i'; }
      else if ((fabs(numberOutImag-1.0)>=OoM)&&(fabs(numberOutImag+1.0)>=OoM)) { strcpy(output+i, "*sqrt(-1)"); i+=strlen(output+i); }
      else                                                                     { strcpy(output+i,  "sqrt(-1)"); i+=strlen(output+i); } // We've not printed 1 or -1, so nothing to multiply with
     }
    if ((fabs(numberOutReal) >= OoM) && (fabs(numberOutImag) > OoM)) output[i++] = ')'; // close brackets on complex number
   }

  if (unitstr[0]!='\0')
   {
    if      (typeable == SW_DISPLAY_N) output[i++] = ' ';
    else if (typeable == SW_DISPLAY_L) { output[i++] = '\\'; output[i++] = ','; }
    sprintf(output+i, "%s", unitstr);
    i+=strlen(output+i); // Add unit string as required
   }

  if (typeable == SW_DISPLAY_L) output[i++] = '$';
  output[i++] = '\0'; // null terminate string
  return output;
 }

// -------------------------------------------------------------------
// Routines for printing units
// -------------------------------------------------------------------

#define UNIT_INSCHEME(X)  (  ((X).si       && (c->set->term_current.UnitScheme == SW_UNITSCH_SI  )) \
                          || ((X).cgs      && (c->set->term_current.UnitScheme == SW_UNITSCH_CGS )) \
                          || ((X).imperial && (c->set->term_current.UnitScheme == SW_UNITSCH_IMP )) \
                          || ((X).us       && (c->set->term_current.UnitScheme == SW_UNITSCH_US  )) \
                          || ((X).planck   && (c->set->term_current.UnitScheme == SW_UNITSCH_PLK )) \
                          || ((X).ancient  && (c->set->term_current.UnitScheme == SW_UNITSCH_ANC ))  )

void ppl_udFindOptimalNextUnit(ppl_context *c, pplObj *in, unsigned char first, unit **best, double *pow)
 {
  int i,j,k,score,found=0,BestScore=0;
  double power;

  for (i=0; i<c->unit_pos; i++)
   {
    if (!ppl_tempTypeMatch(in->tempType, c->unit_database[i].tempType)) continue; // Don't convert between different temperature units
    for (j=0; j<UNITS_MAX_BASEUNITS; j++)
     {
      if ( (c->unit_database[i].exponent[j] == 0) || (in->exponent[j]==0) ) continue;
      power = in->exponent[j] / c->unit_database[i].exponent[j];
      score = 0;
      for (k=0; k<UNITS_MAX_BASEUNITS; k++) if (ppl_dblEqual(in->exponent[k] , power*c->unit_database[i].exponent[k])) score++;

      if (c->unit_database[i].notToBeCompounded && ((!first) || (score<UNITS_MAX_BASEUNITS-1))) continue;

      if (found == 0) // This is first possible unit we've found, and we have nothing to compare it to.
       {
        *best     = c->unit_database+i;
        *pow      = power;
        BestScore = score;
        found     = 1;
       }

      // A user-preferred unit always beats a non-user-preferred unit
      if ( (score >= BestScore) && ( (c->unit_database[i].userSel)) && (!((*best)->userSel)) )
       { *best = c->unit_database+i; *pow = power; BestScore = score; continue; }
      if ( (score <= BestScore ) &&(!(c->unit_database[i].userSel)) && ( ((*best)->userSel)) )
       continue;

      // A unit in the current scheme always beats one not in current scheme
      if (( UNIT_INSCHEME(c->unit_database[i])) && (!UNIT_INSCHEME(**best)))
        { *best = c->unit_database+i; *pow = power; BestScore = score; continue; }
      if ((!UNIT_INSCHEME(c->unit_database[i])) && ( UNIT_INSCHEME(**best)))
        continue;

      // A unit which matches more dimensions wins
      if (score > BestScore)
        { *best = c->unit_database+i; *pow = power; BestScore = score; continue; }
      if ((score == BestScore) && (ppl_dblEqual(fabs(power), 1.0) && (!ppl_dblEqual(fabs(*pow), 1.0))) )
        { *best = c->unit_database+i; *pow = power; continue; }
      if ((score == BestScore) && (ppl_dblEqual(power, 1.0) && (!ppl_dblEqual(*pow, 1.0))) )
        { *best = c->unit_database+i; *pow = power; continue; }
      if (score < BestScore)
        continue;
     }
   }
  if (found==0) {*pow = 0; *best = NULL; return;}
  for (j=0; j<UNITS_MAX_BASEUNITS; j++)
   {
    in->exponent[j] -= (*pow) * (*best)->exponent[j];
   }
  return;
 }

void ppl_udPrefixFix(ppl_context *c, pplObj *in, unit **UnitList, double *UnitPow, int *UnitPref, int Nunits)
 {
  int     i,j;
  double  NewValueReal, NewValueImag, PrefixBestVal, NewMagnitude, OldMagnitude;
  int     PrefixBestPos, BestPrefix=0;

  // Apply unit multipliers to the value we're going to display
  for (i=0; i<Nunits; i++)
   {
    in->real /= pow(UnitList[i]->multiplier , UnitPow[i]);
    in->imag /= pow(UnitList[i]->multiplier , UnitPow[i]);
    UnitPref[i]=0;
   }

  // Search for alternative dimensionally-equivalent units which give a smaller value
  for (i=0; i<Nunits; i++)
   for (j=0; j<c->unit_pos; j++)
    if (ppl_tempTypeMatch(UnitList[i]->tempType, c->unit_database[j].tempType) && (ppl_unitsDimEqual3(UnitList[i] , c->unit_database + j)))
     {
      OldMagnitude = hypot(in->real , in->imag);
      NewValueReal = in->real * pow(UnitList[i]->multiplier / c->unit_database[j].multiplier , UnitPow[i]);
      NewValueImag = in->imag * pow(UnitList[i]->multiplier / c->unit_database[j].multiplier , UnitPow[i]);
      NewMagnitude = hypot(NewValueReal , NewValueImag);

      // A user-preferred unit always beats a non-user-preferred unit
      if ( ( (c->unit_database[j].userSel)) && (!(UnitList[i]->userSel)) )
       { UnitList[i] = c->unit_database+j; in->real = NewValueReal; in->imag = NewValueImag; continue; }
      if ( (!(c->unit_database[j].userSel)) && ( (UnitList[i]->userSel)) )
       continue;

      // A unit in the current scheme always beats one which is not
      if (( UNIT_INSCHEME(c->unit_database[j])) && (!UNIT_INSCHEME(*(UnitList[i]))))
        { UnitList[i] = c->unit_database+j; in->real = NewValueReal; in->imag = NewValueImag; continue; }
      if ((!UNIT_INSCHEME(c->unit_database[j])) && ( UNIT_INSCHEME(*(UnitList[i]))))
        continue;

      // Otherwise, a unit with a smaller display value wins
      if ((NewMagnitude < OldMagnitude) && (NewMagnitude >= 1))
        { UnitList[i] = c->unit_database+j; in->real = NewValueReal; in->imag = NewValueImag; }
     }

  // Apply unit multiplier which arise from user-preferred SI prefixes, for example, millimetres
  for (i=0; i<Nunits; i++)
   if (UnitList[i]->userSel)
    {
     in->real /= pow(10,(UnitList[i]->userSelPrefix-8)*3 * UnitPow[i]);
     in->imag /= pow(10,(UnitList[i]->userSelPrefix-8)*3 * UnitPow[i]);
     UnitPref[i] = UnitList[i]->userSelPrefix-8;
    }

  // Search for an SI prefix we can use to reduce the size of this number
  if (c->set->term_current.UnitDisplayPrefix == SW_ONOFF_ON)
   {
    OldMagnitude = hypot(in->real , in->imag);
    PrefixBestPos = -1;
    PrefixBestVal = OldMagnitude;
    for (i=0; i<Nunits; i++) if (ppl_dblEqual(UnitPow[i] , 1))
     if (UnitList[i]->userSel == 0)
      for (j=UnitList[i]->minPrefix; j<=UnitList[i]->maxPrefix; j+=3)
       {
        NewMagnitude = OldMagnitude / pow(10,j);
        if ( (NewMagnitude >= 1) && ((NewMagnitude < PrefixBestVal) || (PrefixBestVal<1)) )
         { PrefixBestPos = i; BestPrefix = j; PrefixBestVal = NewMagnitude; }
       }
    if (PrefixBestPos>=0)
     {
      in->real /= pow(10,BestPrefix);
      in->imag /= pow(10,BestPrefix);
      UnitPref[PrefixBestPos] = BestPrefix/3;
     }
   }
  return;
 }

// Main entry point for printing units
char *ppl_printUnit(ppl_context *c, const pplObj *in, double *numberOutReal, double *numberOutImag, int N, int DivAllowed, int typeable)
 {
  char         *output,*temp;
  pplObj        residual=*in;
  unit         *UnitList[UNITS_MAX_BASEUNITS];
  double        UnitPow [UNITS_MAX_BASEUNITS];
  int           UnitPref[UNITS_MAX_BASEUNITS];
  unsigned char UnitDisp[UNITS_MAX_BASEUNITS];
  double        ExpMax=0;
  int           pos=0, OutputPos=0;
  int           i, j=0, k, l, found, first;
  listIterator *listiter;
  PreferredUnit *pu;

  if (typeable==0) typeable = c->set->term_current.NumDisplay;

  if      (N==0) output = c->udBuffA;
  else if (N==1) output = c->udBuffB;
  else           output = c->udBuffC;

  // Check whether input value is dimensionally equal to any preferred units
  listiter = ppl_listIterateInit(c->unit_PreferredUnits);
  while (listiter != NULL)
   {
    pu = (PreferredUnit *)listiter->data;
    if (ppl_unitsDimEqual(&pu->value , in) && (in->tempType == pu->value.tempType)) break; // Preferred unit matches dimensions
    ppl_listIterate(&listiter);
   }

  if (listiter != NULL) // Found a preferred unit to use
   {
    for (i=0; i<pu->NUnits; i++)
     {
      UnitList[i] = c->unit_database + pu->UnitID[i];
      UnitPow [i] = pu->exponent[i];
      UnitPref[i] = (pu->prefix[i]>=0) ? pu->prefix[i]-8 : 0;
      UnitDisp[i] = 0;
     }
    pos = pu->NUnits;
    residual.real /= pu->value.real;
    residual.imag /= pu->value.real;
    for (i=0; i<UNITS_MAX_BASEUNITS; i++) residual.exponent[i]=0;
    residual.dimensionless = 1;
   }
  else // Not using a preferred unit
   {
    if (in->dimensionless != 0)
     {
      output[0]='\0';
      if (numberOutReal != NULL) *numberOutReal = in->real;
      if (numberOutImag != NULL) *numberOutImag = in->imag;
      return output;
     }

    // Find a list of units which multiply together to match dimensions of quantity to display
    while (1)
     {
      if (pos>=UNITS_MAX_BASEUNITS) { ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, "Overflow whilst trying to display a unit."); break; }
      ppl_udFindOptimalNextUnit(c, &residual, pos==0, UnitList + pos, UnitPow + pos);
      UnitDisp[pos] = 0;
      if (ppl_dblEqual(UnitPow[pos],0)!=0) break;
      pos++;
     }

    // Go through list of units and fix prefixes / unit choice to minimise displayed number
    ppl_udPrefixFix(c, &residual, UnitList, UnitPow, UnitPref, pos);
   }

  // Display units one by one
  first = 1;
  while (1)
   {
    found = 0;
    for (i=0; i<pos; i++) if ((UnitDisp[i]==0) && ((found==0) || (UnitPow[i]>ExpMax))) { ExpMax=UnitPow[i]; found=1; j=i; }
    if (found==0) break;
    if ((typeable==SW_DISPLAY_T) && first) { strcpy(output+OutputPos, "*unit("); OutputPos+=strlen(output+OutputPos); }
    if (!first) // Print * or /
     {
      if ((!DivAllowed) || (UnitPow[j] > 0) || (ppl_dblEqual(UnitPow[j],0)))
          { if (typeable==SW_DISPLAY_L) { output[OutputPos++] = '\\'; output[OutputPos++] = ','; }  else output[OutputPos++] = '*'; }
      else
          { output[OutputPos++] = '/'; }
     }
    if (typeable==SW_DISPLAY_L) { strcpy(output+OutputPos, "\\mathrm{"); OutputPos+=strlen(output+OutputPos); }
    if (UnitPref[j] != 0) // Print SI prefix
     {
      if (c->set->term_current.UnitDisplayAbbrev == SW_ONOFF_ON)
       {
        if (typeable!=SW_DISPLAY_L) strcpy(output+OutputPos, SIprefixes_abbrev[UnitPref[j]+8]);
        else                        strcpy(output+OutputPos, SIprefixes_latex [UnitPref[j]+8]);
       }
      else
       { strcpy(output+OutputPos, SIprefixes_full  [UnitPref[j]+8]); }
      OutputPos+=strlen(output+OutputPos);
     }
    if (c->set->term_current.UnitDisplayAbbrev == SW_ONOFF_ON)
     {
      if (typeable!=SW_DISPLAY_L)
       {
        if (UnitPow[j] >= 0) strcpy(output+OutputPos, UnitList[j]->nameAp); // Use abbreviated name for unit
        else                 strcpy(output+OutputPos, UnitList[j]->nameAs);
       }
      else
       {
        if (UnitPow[j] >= 0) strcpy(output+OutputPos, UnitList[j]->nameLp); // Use abbreviated LaTeX name for unit
        else                 strcpy(output+OutputPos, UnitList[j]->nameLs);
       }
     } else {
      if (UnitPow[j] >= 0) temp = UnitList[j]->nameFp; // Use full name for unit...
      else                 temp = UnitList[j]->nameFs;

      if (typeable!=SW_DISPLAY_L)
       { strcpy(output+OutputPos, temp); } // ... either as it comes
      else
       {
        for (k=l=0;temp[k]!='\0';k++)
         {
          if (temp[k]=='_') output[OutputPos+(l++)]='\\'; // ... or with escaped underscores for LaTeX
          output[OutputPos+(l++)]=temp[k];
         }
        output[OutputPos+(l++)]='\0';
       }
     }
    OutputPos+=strlen(output+OutputPos);
    if (typeable==SW_DISPLAY_L) { output[OutputPos++]='}'; }
    if ( ((first||(!DivAllowed)) && (!ppl_dblEqual(UnitPow[j],1))) || (((!first)||DivAllowed) && (!ppl_dblEqual(fabs(UnitPow[j]),1))) ) // Print power
     {
      if (typeable==SW_DISPLAY_L) { output[OutputPos++]='^'; output[OutputPos++]='{'; }
      else                        { output[OutputPos++]='*'; output[OutputPos++]='*'; }
      if ((first)||(!DivAllowed)) sprintf(output+OutputPos, "%s", ppl_numericDisplay(     UnitPow[j] , c->numdispBuff[N], c->set->term_current.SignificantFigures, (typeable==SW_DISPLAY_L)));
      else                        sprintf(output+OutputPos, "%s", ppl_numericDisplay(fabs(UnitPow[j]), c->numdispBuff[N], c->set->term_current.SignificantFigures, (typeable==SW_DISPLAY_L)));
      OutputPos+=strlen(output+OutputPos);
      if (typeable==SW_DISPLAY_L) { output[OutputPos++]='}'; }
     }
    UnitDisp[j] = 1;
    first = 0;
   }

  // Clean up and return
  if (typeable==SW_DISPLAY_T) output[OutputPos++] = ')';
  output[OutputPos] = '\0';
  if (numberOutReal != NULL) *numberOutReal = residual.real;
  if (numberOutImag != NULL) *numberOutImag = residual.imag;
  return output;
 }

// ------------------------------------------------
// Function to evaluate strings of the form "m/s"
// ------------------------------------------------

static int unitNameCmp(const char *in, const char *unit, const unsigned char caseSensitive)
 {
  int k;
  if (unit==NULL) return 0;
  if (caseSensitive) for (k=0; ((unit[k]!='\0') && (        unit[k] ==        in[k] )); k++);
  else               for (k=0; ((unit[k]!='\0') && (toupper(unit[k])==toupper(in[k]))); k++);
  if ((unit[k]=='\0') && (!(isalnum(in[k]) || (in[k]=='_')))) return k;
  return 0;
 }

void ppl_unitsStringEvaluate(ppl_context *c, char *in, pplObj *out, int *end, int *errpos, char *errText)
 {
  int i=0,j=0,k,l,m,p;
  double power=1.0, powerneg=1.0, multiplier;
  pplObjZero(out,out->amMalloced);

  while ((in[i]<=' ')&&(in[i]!='\0')) i++;
  out->real = ppl_getFloat(in+i , &j); // Unit strings can have numbers out the front
  if (j<0) j=0;
  i+=j;
  if (j==0)
   { out->real = 1.0; }
  else
   {
    while ((in[i]<=' ')&&(in[i]!='\0')) i++;
    if      (in[i]=='*')   i++;
    else if (in[i]=='/') { i++; powerneg=-1.0; }
   }

  while (powerneg!=0.0)
   {
    p=0;
    while ((in[i]<=' ')&&(in[i]!='\0')) i++;
    for (j=0; j<c->unit_pos; j++)
     {
      multiplier = 1.0;
      if      ((k = unitNameCmp(in+i, c->unit_database[j].nameAp,1))!=0) p=1;
      else if ((k = unitNameCmp(in+i, c->unit_database[j].nameAs,1))!=0) p=1;
      else if ((k = unitNameCmp(in+i, c->unit_database[j].nameFp,0))!=0) p=1;
      else if ((k = unitNameCmp(in+i, c->unit_database[j].nameFs,0))!=0) p=1;
      else if ((k = unitNameCmp(in+i, c->unit_database[j].nameFp,0))!=0) p=1;
      else if ((k = unitNameCmp(in+i, c->unit_database[j].alt1  ,0))!=0) p=1;
      else if ((k = unitNameCmp(in+i, c->unit_database[j].alt2  ,0))!=0) p=1;
      else if ((k = unitNameCmp(in+i, c->unit_database[j].alt3  ,0))!=0) p=1;
      else if ((k = unitNameCmp(in+i, c->unit_database[j].alt4  ,0))!=0) p=1;
      else
       {
        for (l=c->unit_database[j].minPrefix/3+8; l<=c->unit_database[j].maxPrefix/3+8; l++)
         {
          if (l==8) continue;
          for (k=0; ((SIprefixes_full[l][k]!='\0') && (toupper(SIprefixes_full[l][k])==toupper(in[i+k]))); k++);
          if (SIprefixes_full[l][k]=='\0')
           {
            if      ((m = unitNameCmp(in+i+k, c->unit_database[j].nameFp,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = unitNameCmp(in+i+k, c->unit_database[j].nameFs,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = unitNameCmp(in+i+k, c->unit_database[j].alt1  ,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = unitNameCmp(in+i+k, c->unit_database[j].alt2  ,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = unitNameCmp(in+i+k, c->unit_database[j].alt3  ,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = unitNameCmp(in+i+k, c->unit_database[j].alt4  ,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
           }
          for (k=0; ((SIprefixes_abbrev[l][k]!='\0') && (SIprefixes_abbrev[l][k]==in[i+k])); k++);
          if (SIprefixes_abbrev[l][k]=='\0')
           {
            if      ((m = unitNameCmp(in+i+k, c->unit_database[j].nameAp,1))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = unitNameCmp(in+i+k, c->unit_database[j].nameAs,1))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
           }
         }
       }
      if (p==0) continue;
      i+=k;
      while ((in[i]<=' ')&&(in[i]!='\0')) i++;
      if (((in[i]=='^') && (i++,1)) || (((in[i]=='*') && (in[i+1]=='*')) && (i+=2,1)))
       {
        power = ppl_getFloat(in+i,&k);
        if (k<=0) { *errpos=i; strcpy(errText, "Syntax Error: Was expecting a numerical constant here."); return; }
        i+=k;
        while ((in[i]<=' ')&&(in[i]!='\0')) i++;
       }

      if (c->unit_database[j].tempType != 0)
       {
        if ((out->tempType >0) && (out->tempType!=c->unit_database[j].tempType))
         { *errpos=i; strcpy(errText, "Unit Error: Attempt to mix Kelvin, oC and oF in the units of a single quantity. Try again with all quantities converted into Kelvin. Type 'help units temperatures' for more details."); return; }
        out->tempType = c->unit_database[j].tempType;
       }

      for (k=0; k<UNITS_MAX_BASEUNITS; k++) out->exponent[k] += c->unit_database[j].exponent[k] * power * powerneg;
      if (ppl_dblEqual(out->exponent[UNIT_TEMPERATURE], 0)) out->tempType = 0; // We've lost our temperature dependence
      out->real *= pow( multiplier * c->unit_database[j].multiplier , power*powerneg );
      power = 1.0;
      if      (in[i]=='*') { powerneg= 1.0; i++; }
      else if (in[i]=='/') { powerneg=-1.0; i++; }
      else                 { powerneg= 0.0;      }
      break;
     }
    if (p==0)
     {
      if ((in[i]==')') || (in[i]=='\0'))  { powerneg=0.0; }
      else                                { *errpos=i; strcpy(errText, "No such unit."); return; }
     }
   }
  j=1;
  for (k=0; k<UNITS_MAX_BASEUNITS; k++) if (ppl_dblEqual(out->exponent[k], 0) == 0) j=0;
  out->dimensionless = j;
  if (end != NULL) *end=i;
  return;
 }

// Function for making preferred unit structures
void ppl_newPreferredUnit(ppl_context *c, PreferredUnit **output, char *instr, int OutputContext, int *errpos, char *errText)
 {
  int end, outpos, PrefixOut, i, j, k, l, m, p;
  double power=1.0, powerneg=1.0;
  pplObj UnitVal;

  *output = NULL;
  *errpos = -1;
  end = strlen(instr);
  ppl_unitsStringEvaluate(c, instr, &UnitVal, &end, errpos, errText);
  if (*errpos>=0) return; // Error in parsing unit expression
  *output = (PreferredUnit *)malloc(sizeof(PreferredUnit));
  if (*output==NULL) { errpos=0; sprintf(errText, "Out of memory."); return; }
  (*output)->value = UnitVal;
  (*output)->modified = 1;

  // Read list of units from preferred unit string
  outpos=i=0;
  while ((powerneg!=0.0) && (outpos<UNITS_MAX_BASEUNITS))
   {
    PrefixOut=-1;
    p=0;
    while ((instr[i]<=' ')&&(instr[i]!='\0')) i++;
    for (j=0; j<c->unit_pos; j++)
     {
      if      ((k = unitNameCmp(instr+i, c->unit_database[j].nameAp,1))!=0) p=1;
      else if ((k = unitNameCmp(instr+i, c->unit_database[j].nameAs,1))!=0) p=1;
      else if ((k = unitNameCmp(instr+i, c->unit_database[j].nameFp,0))!=0) p=1;
      else if ((k = unitNameCmp(instr+i, c->unit_database[j].nameFs,0))!=0) p=1;
      else if ((k = unitNameCmp(instr+i, c->unit_database[j].nameFp,0))!=0) p=1;
      else if ((k = unitNameCmp(instr+i, c->unit_database[j].alt1  ,0))!=0) p=1;
      else if ((k = unitNameCmp(instr+i, c->unit_database[j].alt2  ,0))!=0) p=1;
      else if ((k = unitNameCmp(instr+i, c->unit_database[j].alt3  ,0))!=0) p=1;
      else if ((k = unitNameCmp(instr+i, c->unit_database[j].alt4  ,0))!=0) p=1;
      else
       {
        for (l=c->unit_database[j].minPrefix/3+8; l<=c->unit_database[j].maxPrefix/3+8; l++)
         {
          if (l==8) continue;
          for (k=0; ((SIprefixes_full[l][k]!='\0') && (toupper(SIprefixes_full[l][k])==toupper(instr[i+k]))); k++);
          if (SIprefixes_full[l][k]=='\0')
           {
            if      ((m = unitNameCmp(instr+i+k, c->unit_database[j].nameFp,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = unitNameCmp(instr+i+k, c->unit_database[j].nameFs,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = unitNameCmp(instr+i+k, c->unit_database[j].alt1  ,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = unitNameCmp(instr+i+k, c->unit_database[j].alt2  ,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = unitNameCmp(instr+i+k, c->unit_database[j].alt3  ,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = unitNameCmp(instr+i+k, c->unit_database[j].alt4  ,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
           }
          for (k=0; ((SIprefixes_abbrev[l][k]!='\0') && (SIprefixes_abbrev[l][k]==instr[i+k])); k++);
          if (SIprefixes_abbrev[l][k]=='\0')
           {
            if      ((m = unitNameCmp(instr+i+k, c->unit_database[j].nameAp,1))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = unitNameCmp(instr+i+k, c->unit_database[j].nameAs,1))!=0) { p=1; k+=m; PrefixOut=l; break; }
           }
         }
       }
      if (p==0) continue;
      i+=k;
      while ((instr[i]<=' ')&&(instr[i]!='\0')) i++;
      if (((instr[i]=='^') && (i++,1)) || (((instr[i]=='*') && (instr[i+1]=='*')) && (i+=2,1)))
       {
        power = ppl_getFloat(instr+i,&k);
        if (k<=0) { *errpos=i; strcpy(errText, "Syntax Error: Was expecting a numerical constant here."); return; }
        i+=k;
        while ((instr[i]<=' ')&&(instr[i]!='\0')) i++;
       }
      (*output)->UnitID  [outpos] = j;
      (*output)->prefix  [outpos] = PrefixOut;
      (*output)->exponent[outpos] = power*powerneg;
      outpos++;
      power = 1.0;
      if      (instr[i]=='*') { powerneg= 1.0; i++; }
      else if (instr[i]=='/') { powerneg=-1.0; i++; }
      else                    { powerneg= 0.0;      }
      break;
     }
    if (p==0)
     {
      if ((instr[i]==')') || (instr[i]=='\0'))  { powerneg=0.0; }
      else                                      { *errpos=i; strcpy(errText, "No such unit."); return; }
     }
   }

  (*output)->NUnits = outpos;
  return;
 }

