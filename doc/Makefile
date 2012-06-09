# MAKEFILE
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

# Makefile for PyXPlot documentation directory

PYXPLOT    = ../bin/pyxplot
EXAMPLEDIR = examples
EX_EPSDIR  = $(EXAMPLEDIR)/eps
EX_TEXDIR  = $(EXAMPLEDIR)/tex
FIGFILES   = $(wildcard $(EXAMPLEDIR)/*.ppl)
SOURCES    = $(wildcard *.tex)

all: $(EX_EPSDIR)/timestamp $(EX_TEXDIR)/timestamp \
     pyxplot-plain.ps pyxplot-plain.pdf pyxplot.pdf

clean:
	rm -Rf $(EX_EPSDIR) $(EX_TEXDIR)
	rm -f *.pdf *.ps *.dvi *.aux *.idx *.ilg *.ind *.lob *.loe *.lof *.log *.out *.toc

$(EX_EPSDIR)/timestamp: $(FIGFILES) $(PYXPLOT) .pyxplotrc
	mkdir -p $(EX_EPSDIR)
	python makeFigureEps.py $(PYXPLOT)
	date > $(EX_EPSDIR)/timestamp

$(EX_TEXDIR)/timestamp: $(FIGFILES) $(PYXPLOT) .pyxplotrc
	mkdir -p $(EX_TEXDIR)
	python makeFigureTex.py
	date > $(EX_TEXDIR)

pyxplot-plain.pdf: pyxplot-plain.ps
	echo "Making plain pdf documentation..."
	ps2pdf $< $@ > /dev/null 2> /dev/null

# Dependency on pyxplot-plain.dvi is a fudge so that 'make -j' doesn't parallelise pdflatex and latex
pyxplot.pdf: $(SOURCES) pyxplot-plain.dvi
	echo "Making hyperref pdf documentation..."
	rm -f *.aux *.idx *.ilg *.ind *.lob *.loe *.lof *.toc
	pdflatex pyxplot > /dev/null 2> /dev/null
	pdflatex pyxplot > /dev/null 2> /dev/null
	pdflatex pyxplot > /dev/null 2> /dev/null
	makeindex pyxplot.idx > /dev/null 2> /dev/null
	pdflatex pyxplot > /dev/null 2> /dev/null

%.ps: %.dvi
	echo "Making postscript documentation..."
	dvips -Pcmz -Pamz $< -o $@ > /dev/null 2> /dev/null

pyxplot-plain.dvi: $(SOURCES) $(EX_EPSDIR)/timestamp $(EX_TEXDIR)/timestamp
	echo "Compiling documentation..."
	rm -f *.aux *.idx *.ilg *.ind *.lob *.loe *.lof
	latex pyxplot-plain.tex > /dev/null 2> /dev/null
	latex pyxplot-plain.tex > /dev/null 2> /dev/null
	latex pyxplot-plain.tex > /dev/null 2> /dev/null
	makeindex pyxplot-plain.idx > /dev/null 2> /dev/null
	latex pyxplot-plain.tex > /dev/null 2> /dev/null

