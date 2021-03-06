# parser_data.py
#
# The code in this file is part of Pyxplot
# <http://www.pyxplot.org.uk>
#
# Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
#               2008-2012 Ross Church
#
# $Id$
#
# Pyxplot is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# You should have received a copy of the GNU General Public License along with
# Pyxplot; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA  02110-1301, USA

# ----------------------------------------------------------------------------

# Generate table of positions of variables by name
globalVarNames = []
globalVarTable = {}

def processVarTable(vt, directive, setOption):
 global globalVarNames,globalVarTable
 if '%' in directive: return
 key = "%s_%s"%(directive,setOption)
 globalVarTable[key] = vt
 for name,pos in list(vt.items()):
  if name not in globalVarNames: globalVarNames.append(name)

def printVarTable():
 global globalVarNames,globalVarTable,f_h,f_c
 globalVarNames.sort()
 for i in range(len(globalVarNames)):
   f_h.write("#define PARSE_INDEX_%s %d\n"%(sanitize(globalVarNames[i]),i))
 keys = list(globalVarTable.keys())
 keys.sort()
 for k in keys:
   d = globalVarTable[k]
   f_h.write("extern const int PARSE_TABLE_%s[];\n"%k)
   f_c.write("const int PARSE_TABLE_%s[] = {"%k)
   for i in range(len(globalVarNames)):
     item = globalVarNames[i]
     if (i!=0)   : f_c.write(",")
     if item in d: f_c.write("%d"%d[item])
     else        : f_c.write("-1");
   f_c.write("};\n")

# Generate specification of commands that Pyxplot recognises

def sanitize(instr):
  outstr = ""
  for i in range(len(instr)):
    if (instr[i].isalnum() or instr[i]=='_'): outstr+=instr[i]
  return outstr

f_in = open("buildScripts/parser_data.dat","r")

# Output C files containing command definitions

f_h  = open("src/parser/cmdList.h","w")
f_c  = open("src/parser/cmdList.c","w")
includeKeys = {}

f_h.write("""// This file autogenerated by parser_data.py

#ifndef _CMDLIST_H
#define _CMDLIST_H 1

extern const char ppl_cmdList[];
""")

f_c.write("""// This file autogenerated by parser_data.py

const char ppl_cmdList[] = "\\
""")

# Loop through command definitions

linecount = 0
for line in f_in:
  linecount+=1
  line = line.strip();
  if len(line)<1: continue
  if line.startswith("#"): continue

  outline        = ""
  directive      = "undefined"
  setoption      = ""
  stack          = []
  stack_varnames = []
  listsizes      = []
  varnames       = {"directive":0 , "editno":1, "set_option":2, "X":3}
  vartable       = varnames.copy()
  varcount       = len(varnames)
  words = line.split()
  for word in words:
    if word in ["=","(","~",")","{","}","<","|",">","["]: # Grammar characters are passed straight through
      outline += "%s "%word
      if word=="[": # Lists are placed on the stack, and the number of variables in each list counted
        stack.append(varcount)
        stack_varnames.append(varnames)
        varcount=1 # Start counting from variable 1 inside a list, as first value is pointer to next list item
        varnames={}
      continue
    parts = word[1:].split(":")
    parts[0] = word[0]+parts[0] # If first character is a colon, it should match a literal :, not act as a separator
    subparts = parts[0].split("@") # Match string has form plot@1, meaning 'p' is short for 'plot'
    if len(parts)<2:
      parts.append("X") # If output variable name is not specified, stick it in 'X'.
    if len(subparts)>1:
      if subparts[1]=="n": subparts[1]=-1 # An auto-complete length of 'n' is stored as -1
      parts.append("%s"%subparts[1])
      parts[0] = subparts[0]
    else:
      parts.append("%s"%len(parts[0])) # If no auto-complete length is specified, whole string must be matched
    if len(parts)==3: parts.insert(2,"") # At this point, parts is [ match string , output variable , string to store (blank if equals match string) , auto-complete len ]
    assert len(parts)==4, "Syntax error in word '%s'."%word
    varname = parts[1]
    if word.startswith("]:"):
      parts = [ "]" , word[2:] , "" , "1" ]
      listsizes.append(varcount)
      varcount=stack.pop()
      for key,item in list(varnames.items()): stack_varnames[-1]["%s_%s"%(key,varname)] = item
      varnames=stack_varnames.pop()
    if varname=='directive': # Directive names are stored for use in #defines to convert variable names into output slot numbers
      if parts[2]=="": directive = parts[0]
      else           : directive = parts[2]
    if varname=='set_option': # Set options are also used in #defines
      if parts[2]=="": setoption = parts[0]+"_"
      else           : setoption = parts[2]+"_"
      if (setoption[0]=="%"): setoption=""
    if varname not in varnames:
      varnames[varname] = varcount
      vartable[varname] = varcount
      if   (parts[0]=="%p"): varcount += 2 # Position vectors require 2 or 3 slots
      elif (parts[0]=="%P"): varcount += 3
      else                 : varcount += 1 # Varcount keeps track of the slot number to place the next variable in
    elif (parts[0] in ["%p","%P"]): print(("Danger in command %s: sharing position variable name with other variables of different lengths"%directive))
    outnum = varnames[varname]
    parts.append("%s"%outnum) # parts[4] = slot number
    if word.startswith("]:"):
      initial = ""
      parts.append("%s"%listsizes[-1]) # parts[5] = list length for ] (number of slots needed for this list's variables)
    else:
      initial = "@"
      parts.append("0") # ... or zero otherwise
    outline += initial + "@".join(parts) + " "
  f_c.write("%d "%(varcount)) # First word on each statement definition line is the number of variables in the root slotspace
  f_c.write("%s\\n\\\n"%outline)
  processVarTable(vartable,directive,setoption)
  for i,j in list(varnames.items()):
   if '%' not in directive:
    key = "PARSE_%s_%s%s"%(directive,setoption,sanitize(i))
    if (key in includeKeys) and (includeKeys[key]!=j): print(("Repetition of key %s"%key))
    includeKeys[key] = j
    f_h.write("#define %s %d\n"%(key,j)) # Write #defines to convert variable names into slot numbers

# Finish up

f_c.write("""";\n""");
printVarTable()
f_h.write("""\n\n#endif"""); f_h.close()
f_c.close()

