# makeFigureTex.py
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

import os,glob,re

os.system("rm -Rf examples/tex")
os.system("mkdir  examples/tex")

def line_texify(line):
  if line.beginswith("#NC "): line=line[4:]
  line = re.sub(r'examples/eps/ex_','',line)
  line = re.sub(r'examples/ex_','',line)
  line = re.sub(r'examples/','',line)
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

def makeTeX(fname, counter, linelist):
  fname  = os.path.join("examples","tex",os.path.split(fname)[1][:-4]+"_%d.tex"%counter)
  output = open(fname,"w")
  fns    = max([len(l) for l in linelist]) > 50
  first  = True
  if fns: output.write("{\\footnotesize\n")
  for line in linelist:
    if (not first):
      output.write("\\newline\n")
    if (len(line.strip())==0):
      if fns: output.write("}\\\\{\\footnotesize\n")
      else  : output.write("\\\\\n")
      first = True
      continue
    first = False
    line = line_texify(line)
    for i in range(len(line)):
      if (line[i]!=' '):
        break;
    if (i>0):
      line2 = "\\phantom{"
      for j in range(i): line2 += "x"
      line2 += "}" + line.strip()
      line = line2
    else:
      line = line.strip()
    output.write("\\noindent{\\tt %s}"%line)
  if fns: output.write("\n}")
  output.close()

files = glob.glob("examples/ex_*.ppl")
files.sort()
for fname in files:
  print "Converting example to latex <%s>..."%os.path.split(fname)[1]
  buffer    = []
  buffering = False
  counter   = 1
  for line in open(fname):
    if (line.strip()=="# BEGIN"):
      buffering = True
      buffer    = []
      continue
    if (line.strip()=="# END"):
      makeTeX(fname, counter, buffer)
      counter   = counter+1
      buffering = False
      continue
    if (buffering):
      buffer.append(line)

