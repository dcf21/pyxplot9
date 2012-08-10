# MAKEFILE
#
# The code in this file is part of Pyxplot
# <http://www.pyxplot.org.uk>
#
# Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
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

# Makefile for Pyxplot documentation directory

PYXPLOT    = ../bin/pyxplot
EXAMPLEDIR = examples
FRAGDIR    = fragments
EX_EPSDIR  = $(EXAMPLEDIR)/eps
EX_TEXDIR  = $(EXAMPLEDIR)/tex
EX_FRAGDIR = $(FRAGDIR)/tex
FIGFILES   = $(wildcard $(EXAMPLEDIR)/*.ppl)
FRAGFILES  = $(wildcard $(FRAGDIR)/*.ppl)
SOURCES    = $(wildcard *.tex)

all: $(EX_EPSDIR)/timestamp $(EX_TEXDIR)/timestamp $(EX_FRAGDIR)/timestamp \
     pyxplot-plain.ps pyxplot-plain.pdf pyxplot.pdf

clean:
	rm -Rf $(EX_EPSDIR) $(EX_TEXDIR) $(EX_FRAGDIR)
	rm -f *.pdf *.ps *.dvi *.aux *.idx *.ilg *.ind *.lob *.loe *.lof *.log *.out *.toc

$(EX_EPSDIR)/timestamp: $(FIGFILES) $(PYXPLOT) .pyxplotrc
	python makeFigureEps.py $(PYXPLOT)
	date > $(EX_EPSDIR)/timestamp

$(EX_TEXDIR)/timestamp: $(FIGFILES) $(PYXPLOT) .pyxplotrc
	python makeFigureTex.py
	date > $(EX_TEXDIR)/timestamp

$(EX_FRAGDIR)/timestamp: $(FRAGFILES) $(PYXPLOT) .pyxplotrc
	python makeFragmentTex.py
	date > $(EX_FRAGDIR)/timestamp

pyxplot-plain.pdf: pyxplot-plain.ps
	echo "Making plain pdf documentation..."
	ps2pdf $< $@

# Dependency on pyxplot-plain.dvi is a fudge so that 'make -j' doesn't parallelise pdflatex and latex
pyxplot.pdf: $(SOURCES) pyxplot-plain.dvi
	echo "Making hyperref pdf documentation..."
	rm -f *.aux *.idx *.ilg *.ind *.lob *.loe *.lof *.toc
	pdflatex pyxplot
	pdflatex pyxplot
	pdflatex pyxplot
	makeindex pyxplot.idx
	pdflatex pyxplot

%.ps: %.dvi
	echo "Making postscript documentation..."
	dvips -Pcmz -Pamz $< -o $@

pyxplot-plain.dvi: $(SOURCES) $(EX_EPSDIR)/timestamp $(EX_TEXDIR)/timestamp $(EX_FRAGDIR)/timestamp
	echo "Compiling documentation..."
	rm -f *.aux *.idx *.ilg *.ind *.lob *.loe *.lof
	latex pyxplot-plain.tex
	latex pyxplot-plain.tex
	latex pyxplot-plain.tex
	makeindex pyxplot-plain.idx
	latex pyxplot-plain.tex

EPSTESTFILES = $(EX_EPSDIR)/ex_axistics.eps $(EX_EPSDIR)/ex_barchart2.eps $(EX_EPSDIR)/ex_lenses.eps $(EX_EPSDIR)/ex_surface_polynomial.eps

test-examples: $(EX_EPSDIR)/timestamp
	for e in $(EPSTESTFILES); do \
	   ls -l $$e; \
	   if [ ! -s "$$e" ]; then \
			echo "$$e has zero length; tests failed"; exit 2; \
	   fi \
	done

.PHONY: test-examples

