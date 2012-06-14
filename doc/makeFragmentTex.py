# makeFragmentTex.py
#
# The code in this file is part of PyXPlot
# <http://www.pyxplot.org.uk>
#
# Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
#
# $Id$
#
# PyXPlot is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# You should have received a copy of the GNU General Public License along with
# PyXPlot; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA  02110-1301, USA

# ----------------------------------------------------------------------------

# Turn scripts of examples into latex

import os,sys,glob,re,subprocess

os.system("rm -Rf fragments/tex")
os.system("mkdir  fragments/tex")

if len(sys.argv)>=2: pyxplot = sys.argv[1]
else               : pyxplot = "../bin/pyxplot"

def line_texify(line):
  line = re.sub(r'[\\]', r'gpzywxqqq', line) # LaTeX does not like backslashs
  line = re.sub(r'[_]', r'\\_', line) # LaTeX does not like underscores....
  line = re.sub(r'[&]', r'\\&', line) # LaTeX does not like ampersands....
  line = re.sub(r'[%]', r'\\%', line) # LaTeX does not like percents....
  line = re.sub(r'[$]', r'\\$', line) # LaTeX does not like $s....
  line = re.sub(r'[{]', r'\\{', line) # LaTeX does not like {s....
  line = re.sub(r'[}]', r'\\}', line) # LaTeX does not like }s....
  line = re.sub(r'[#]', r'\\#', line) # LaTeX does not like #s....
  line = re.sub(r'[\^]', r'\\^{}', line) # LaTeX does not like carets....
  line = re.sub(r'[~]', r'$\\sim$', line) # LaTeX does not like tildas....
  line = re.sub(r'[<]', r'$<$', line) # LaTeX does not like < outside of mathmode....
  line = re.sub(r'[>]', r'$>$', line) # LaTeX does not like > outside of mathmode....
  line = re.sub(r'gpzywxqqq', r'$\\backslash$', line) # LaTeX does not like backslashs
  line = re.sub(r' ', r'~', line)
  return line

files = glob.glob("fragments/*.ppl")
files.sort()
for fname in files:
  print "Converting fragment to latex <%s>..."%os.path.split(fname)[1]
  out       = os.path.join("fragments","tex",os.path.split(fname)[1][:-4]+".tex")
  linecount = 0
  lines     = open(fname).readlines()
  out       = open(out,"w")
  first     = True
  prompt    = "pyxplot"
  for i in range(len(lines)):
    if (len(lines[i].strip())<1): continue
    if (not first): out.write("\\newline\n")
    first = False
    out.write(r"\noindent{\tt pyxplot> {\bf %s}}"%(line_texify(lines[i].strip())))
    if lines[i].strip()[-1]=="\\":
      prompt = "......."
      continue
    prompt = "pyxplot"
    sp = subprocess.Popen([pyxplot], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    o  = sp.communicate(input="\n".join([ ll.strip() for ll in lines[0:i+1]])) # returns (stdout,stderr)
    if (len(o[1])>0): raise RuntimeError("pyxplot failed: %s"%o[1])
    olines = o[0].strip().split('\n')
    if (olines==['']): olines=[]
    linecountNew = len(olines)
    if linecountNew<=linecount: continue
    olines = olines[linecount:]
    linecount = linecountNew
    for line in olines:
      if (len(line.strip())<1): continue
      out.write("\\newline\n\\noindent{\\tt %s}"%line)
  out.write("\n")
  out.close()
