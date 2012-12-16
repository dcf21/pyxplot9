# makeFigureEps.py
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

import glob,os,sys,re

if len(sys.argv)>=2: pyxplot = sys.argv[1]
else               : pyxplot = "../bin/pyxplot"

if (len(sys.argv)>=3) and (sys.argv[2]=="test"): testing = True
else                                           : testing = False

os.system("rm -Rf examples/eps")
os.system("mkdir  examples/eps")

if testing: files = ["examples/%s.ppl"%i.strip() for i in open("examples.testlist")]
else      : files = glob.glob("examples/ex_*.ppl")

files.sort()

for fname in files:
  print "Working on example <%s>..."%os.path.split(fname)[1]
  status = os.system("%s %s"%(pyxplot,fname))
  if (status): raise RuntimeError("pyxplot failed")

# Make pdf and png versions of all figures
if not testing:
  files = glob.glob("examples/eps/*.eps")
  files.sort()
  for eps in files:
    print "Converting example <%s> to pdf..."%os.path.split(eps)[1]
    pdf = re.sub(r"\.eps",".pdf",eps)
    #png = re.sub(r"\.eps",".png",eps)
    os.system("gs -dQUIET -dSAFER -P- -dBATCH -dNOPAUSE -dEPSCrop -sDEVICE=pdfwrite -sOutputFile=%s %s"%(pdf,eps))
    #os.system("gs -dQUIET -dSAFER -P- -dBATCH -dNOPAUSE -dEPSCrop -sDEVICE=png16m -r72 -dGraphicsAlphaBits=4 -dTextAlphaBits=4 -sOutputFile=%s %s"%(png,eps))

