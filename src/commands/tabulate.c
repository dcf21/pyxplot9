// tabulate.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

#define _TABULATE_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glob.h>
#include <wordexp.h>

#include <gsl/gsl_math.h>

#include "commands/tabulate.h"
#include "coreUtils/backup.h"
#include "coreUtils/getPasswd.h"
#include "coreUtils/memAlloc.h"
#include "expressions/traceback_fns.h"
#include "mathsTools/dcfmath.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "datafile.h"

// Display data from a data block
static int ppl_tab_dataGridDisplay(ppl_context *c, FILE *output, dataTable *data, int *minSet, double *min, int *maxSet, double *max, pplObj *unit, char *format)
 {
  dataBlock *blk;
  char      *cptr, tmpchr='\0';
  double     tmpdbl, multiplier[USING_ITEMS_MAX];
  int        inRange, split, allInts[USING_ITEMS_MAX], allSmall[USING_ITEMS_MAX];
  long       i,k;
  int        j,l,pos;
  double     val;
  const int  Ncolumns = data->Ncolumns_real;

  // Check that the firstEntries above have the same units as any supplied ranges
  for (j=0; j<Ncolumns; j++)
   if (minSet[j]||maxSet[j])
    {
     if (!ppl_unitsDimEqual(&unit[j],data->firstEntries+j)) { sprintf(c->errcontext.tempErrStr, "The minimum and maximum limits specified in range %d in the tabulate command have conflicting physical dimensions with the data returned from the data file. The limits have units of <%s>, whilst the data have units of <%s>.", j+1, ppl_printUnit(c,&unit[j],NULL,NULL,0,1,0), ppl_printUnit(c,data->firstEntries+j,NULL,NULL,1,1,0)); ppl_error(&c->errcontext,ERR_NUMERICAL,-1,-1,NULL); return 1; }
    }

  // Output a column units line
  fprintf(output, "# ColumnUnits: ");
  for (j=0; j<Ncolumns; j++)
   {
    if (data->firstEntries[j].dimensionless)
     {
      fprintf(output, "1 "); // This column contains dimensionless data
      multiplier[j] = 1.0;
     }
    else
     {
      data->firstEntries[j].real = 1.0;
      data->firstEntries[j].imag = 0.0;
      data->firstEntries[j].flagComplex = 0;
      cptr = ppl_printUnit(c, data->firstEntries+j, multiplier+j, &tmpdbl, 0, 1, SW_DISPLAY_T);
      for (i=0; ((cptr[i]!='\0')&&(cptr[i]!='(')); i++);
      i++; // Fastforward over opening bracket
      for (   ; ((cptr[i]!='\0')&&(cptr[i]!=')')); i++) fprintf(output, "%c", cptr[i]);
      fprintf(output, " ");
     }
   }
  fprintf(output, "\n");

  // Iterate over columns of data in this data grid, working out which columns of data are all ints, and which are all %f-able data
  for (j=0; j<Ncolumns; j++) { allInts[j] = allSmall[j] = 1; }
  blk = data->first;
  while (blk != NULL)
   {
    for (i=0; i<blk->blockPosition; i++)
     {
      inRange=1;
      for (k=0; k<Ncolumns; k++)
       {
        val = blk->data_real[k + Ncolumns*i];
        if ( (minSet[k]&&(val<min[k])) || (maxSet[k]&&(val>max[k])) ) { inRange=0; break; } // Check that value is within range
       }
      if (inRange)
       for (k=0; k<Ncolumns; k++)
        {
         val = blk->data_real[k + Ncolumns*i] * multiplier[k];
         if (!gsl_finite(val)                                       ) allInts [k]=0; // don't try to represent NANs as integers!
         if ((fabs(val)>1000) || (!ppl_dblEqual(val,floor(val+0.5)))) allInts [k]=0;
         if ((fabs(val)>1000) || (fabs(val)<0.0099999999999)        ) allSmall[k]=0; // Columns containing only numbers in this range are fprintfed using %f, rather than %e
        }
     }
    blk=blk->next;
   }

  // Iterate over columns of data in this data grid, working out which columns of data are all ints, and which are all %f-able data
  blk = data->first;
  split = 0;
  while (blk != NULL)
   {
    for (i=0; i<blk->blockPosition; i++)
     {
      inRange=1;
      if (blk->split[i]) split=1;
      for (k=0; k<Ncolumns; k++)
       {
        val = blk->data_real[k + Ncolumns*i];
        if ( (minSet[k]&&(val<min[k])) || (maxSet[k]&&(val>max[k])) ) { inRange=0; break; } // Check that value is within range
       }
      if (inRange)
       {
        if (split) { fprintf(output, "\n"); split=0; }
        if (format == NULL) // User has not supplied a format string, and so we just list the contents of each column in turn using best-fit format style
         {
          for (k=0; k<Ncolumns; k++)
           {
            val = blk->data_real[k + Ncolumns*i] * multiplier[k];
            if      (allInts [k]) fprintf(output, "%10d "   , (int)val);
            else if (allSmall[k]) fprintf(output, "%17.10f ",      val);
            else                  fprintf(output, "%17.8e " ,      val);
           }
         } else { // The user has supplied a format string, which we now substitute column values into
          for (pos=l=0; ; pos++)
           {
            if (format[pos]=='\0')
             {
              if ((l>0)&&(l<Ncolumns)) pos=0;
              else                     break;
             }
            if (format[pos]!='%') fprintf(output, "%c", format[pos]); // Just copy text of format string until we hit a % character
            else
             {
              k=pos+1; // k looks ahead to see experimentally if syntax is right
              if (format[k]=='%') { fprintf(output, "%%"); pos++; continue; }
              if ((format[k]=='+')||(format[k]=='-')||(format[k]==' ')||(format[k]=='#')) k++; // optional flag can be <+- #>
              while ((format[k]>='0') && (format[k]<='9')) k++; // length can be some digits
              if (format[k]=='.') // precision starts with a . and is followed by more digits
               {
                k++; while ((format[k]>='0') && (format[k]<='9')) k++; // length can be some digits
               }
              // We do not allow user to specify optional length flag, which could potentially be <hlL>
              if (l>=Ncolumns) val = GSL_NAN;
              else             val = blk->data_real[l + Ncolumns*i] * multiplier[l]; // Set val to equal data from data table that we're about to print.
              l++;
              if (format[k]!='\0') { tmpchr = format[k+1]; format[k+1] = '\0'; } // NULL terminate format token before passing it to fprintf
              if      (format[k]=='d') // %d -- print quantity as an integer, but take care to avoid overflows
               {
                if ((!gsl_finite(val))||(val>INT_MAX-1)||(val<INT_MIN+1)) fprintf(output, "nan");
                else                                                      fprintf(output, format+pos, (int)floor(val));
                pos = k;
               }
              else if ((format[k]=='e') || (format[k]=='f')) // %f or %e -- print quantity as floating point number
               {
                fprintf(output, format+pos, val);
                pos = k;
               }
              else if (format[k]=='s') // %s -- print quantity in our best-fit format style
               {
                if      (l>=Ncolumns)   sprintf(c->errcontext.tempErrStr, "nan");
                else if (allInts [l-1]) sprintf(c->errcontext.tempErrStr, "%10d "   , (int)val);
                else if (allSmall[l-1]) sprintf(c->errcontext.tempErrStr, "%17.10f ",      val);
                else                    sprintf(c->errcontext.tempErrStr, "%17.8e " ,      val);
                fprintf(output, format+pos, c->errcontext.tempErrStr);
                pos = k;
               }
              else
               { fprintf(output, "%c", format[pos]); l--; } // l-- because this wasn't a valid format token, so we didn't print any data in the end
              if (format[k]!='\0') format[k+1] = tmpchr; // Remove temporary NULL termination at the end of the format token
             }
           }
         }
        fprintf(output, "\n");
       }
      else split=1;
     }
    blk=blk->next;
   }

  return 0;
 }

#define TBADD2(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

void ppl_directive_tabulate(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  wordexp_t   wordExp;
  int         pos;
  pplObj     *stk=in->stk;
  FILE       *output=NULL;
  long        j;
  char       *filename, filenameTemp[FNAME_LENGTH];
  parserLine *spool=NULL, **dataSpool = &spool;
  pplObj      unit  [USING_ITEMS_MAX];
  int         minSet[USING_ITEMS_MAX], maxSet[USING_ITEMS_MAX];
  double      min   [USING_ITEMS_MAX], max   [USING_ITEMS_MAX];

  // Read data range
  {
   int i,pos=PARSE_tabulate_0range_list,nr=0;
   const int o1 = PARSE_tabulate_min_0range_list;
   const int o2 = PARSE_tabulate_max_0range_list;
   for (i=0; i<USING_ITEMS_MAX; i++) minSet[i]=0;
   for (i=0; i<USING_ITEMS_MAX; i++) maxSet[i]=0;
   while (stk[pos].objType == PPLOBJ_NUM)
    {
     pos = (int)round(stk[pos].real);
     if (pos<=0) break;
     if (nr>=USING_ITEMS_MAX)
      {
       sprintf(c->errStat.errBuff,"Too many ranges specified; a maximum of %d are allowed.", USING_ITEMS_MAX);
       TBADD2(ERR_SYNTAX,0);
       return;
      }
     if ((stk[pos+o1].objType==PPLOBJ_NUM)||(stk[pos+o1].objType==PPLOBJ_DATE)||(stk[pos+o1].objType==PPLOBJ_BOOL))
      {
       unit[nr]=stk[pos+o1];
       min[nr]=unit[nr].real;
       minSet[nr]=1;
      }
     if ((stk[pos+o2].objType==PPLOBJ_NUM)||(stk[pos+o2].objType==PPLOBJ_DATE)||(stk[pos+o2].objType==PPLOBJ_BOOL))
      {
       if ((minSet[nr])&&(unit[nr].objType!=stk[pos+o2].objType))
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting types of <%s> and <%s>.", nr+1, pplObjTypeNames[unit[nr].objType], pplObjTypeNames[stk[pos+o2].objType]);
         TBADD2(ERR_TYPE,in->stkCharPos[pos+PARSE_tabulate_min_0range_list]);
         return;
        }
       if ((minSet[nr])&&(!ppl_unitsDimEqual(&unit[nr],&stk[pos+o2])))
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting physical units of <%s> and <%s>.", nr+1, ppl_printUnit(c,&unit[nr],NULL,NULL,0,0,0), ppl_printUnit(c,&stk[pos+o2],NULL,NULL,1,0,0));
         TBADD2(ERR_UNIT,in->stkCharPos[pos+PARSE_tabulate_min_0range_list]);
         return;
        }
       unit[nr]=stk[pos+o2];
       max[nr]=unit[nr].real;
       maxSet[nr]=1;
      }
     nr++;
    }
  }

  // Work out filename for output file
  filename = c->set->term_current.output;
  if ((filename==NULL)||(filename[0]=='\0')) filename = "pyxplot.txt";

  // Perform expansion of shell filename shortcuts such as ~
  if ((wordexp(filename, &wordExp, 0) != 0) || (wordExp.we_wordc <= 0)) { sprintf(c->errcontext.tempErrStr, "Could not find directory containing filename '%s'.", filename); ppl_error(&c->errcontext,ERR_FILE,-1,-1,NULL); return; }
  if  (wordExp.we_wordc > 1) { sprintf(c->errcontext.tempErrStr, "Filename '%s' is ambiguous.", filename); ppl_error(&c->errcontext,ERR_FILE,-1,-1,NULL); return; }
  strncpy(filenameTemp, wordExp.we_wordv[0], FNAME_LENGTH);
  filenameTemp[FNAME_LENGTH-1]='\0';
  wordfree(&wordExp);
  filename = filenameTemp;

  // Open output file and write header at the top of it
  ppl_createBackupIfRequired(c,filename);
  output = fopen(filename,"w");
  if (output==NULL) { sprintf(c->errcontext.tempErrStr, "The tabulate command could not open output file '%s' for writing.", filename); ppl_error(&c->errcontext,ERR_FILE,-1,-1,NULL); return; }
  fprintf(output, "# Datafile generated by Pyxplot %s\n# Timestamp: %s\n", VERSION, ppl_strStrip(ppl_friendlyTimestring(),c->errcontext.tempErrStr));
  fprintf(output, "# User: %s\n# Pyxplot command: %s\n\n", ppl_unixGetIRLName(&c->errcontext), pl->linetxt);

  // Loop over datasets being tabulated
  pos = PARSE_tabulate_0tabulate_list;
  j   = 1;
  while (stk[pos].objType == PPLOBJ_NUM)
   {
    char      *format, datafname[FNAME_LENGTH]="";
    int        w, errCount=5;
    dataTable *data=NULL;
    pos = (int)round(stk[pos].real);
    if (pos<=0) break;
    fprintf(output, "\n\n\n# Index %ld\n", j); // Put a heading at the top of the new data index

    // Read display format
    if (stk[pos+PARSE_tabulate_format_0tabulate_list].objType==PPLOBJ_STR) format=(char *)stk[pos+PARSE_tabulate_format_0tabulate_list].auxil;
    else                                                                   format=NULL;

    for (w=0 ; ; w++)
     {
      // Descend into new memory context
      int contextLocal  = ppl_memAlloc_DescendIntoNewContext();
      int status        = 0;

      // Fetch data
      ppldata_fromCmd(c, &data, pl, in, w, datafname, dataSpool, PARSE_TABLE_tabulate_, pos, -1, 0, min, minSet, max, maxSet, unit, 0, &status, c->errcontext.tempErrStr, &errCount, iterDepth);

      // Print data
      if ((status)||(data==NULL))
       {
        if (w==0) ppl_error(&c->errcontext,ERR_GENERIC,-1,-1,NULL);
        ppl_memAlloc_AscendOutOfContext(contextLocal);
        break;
       }
      else
       {
        if (datafname[0]!='\0')
         {
          if (w>0) { j++; fprintf(output, "\n\n\n# Index %ld\n", j); }
          fprintf(output, "\n# %s\n\n", datafname);
         }
        ppl_tab_dataGridDisplay(c, output, data, minSet, min, maxSet, max, unit, format);
       }

      // Ascend out of memory content
      ppl_memAlloc_AscendOutOfContext(contextLocal);
     }
    j++;
   }

  fclose(output);
  return;
 }

