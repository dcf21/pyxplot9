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

EPSTESTFILES = $(EX_EPSDIR)/ex_3d_helix.eps $(EX_EPSDIR)/ex_apachelog.eps $(EX_EPSDIR)/ex_axisatzero2.eps \
$(EX_EPSDIR)/ex_axisatzero3.eps $(EX_EPSDIR)/ex_axisatzero.eps $(EX_EPSDIR)/ex_axislabs.eps \
$(EX_EPSDIR)/ex_axistics2.eps $(EX_EPSDIR)/ex_axistics.eps $(EX_EPSDIR)/ex_axisunits.eps \
$(EX_EPSDIR)/ex_barchart1.eps $(EX_EPSDIR)/ex_barchart2.eps $(EX_EPSDIR)/ex_branch_cuts.eps \
$(EX_EPSDIR)/ex_cmbrtemp.eps $(EX_EPSDIR)/ex_colgradient.eps $(EX_EPSDIR)/ex_col_vs_mono.eps \
$(EX_EPSDIR)/ex_contourmap.eps $(EX_EPSDIR)/ex_cover.eps $(EX_EPSDIR)/ex_datagrid.eps \
$(EX_EPSDIR)/ex_ellipse.eps $(EX_EPSDIR)/ex_eqnsolve.eps $(EX_EPSDIR)/ex_euclid_I_47.eps \
$(EX_EPSDIR)/ex_fft2.eps $(EX_EPSDIR)/ex_fft.eps $(EX_EPSDIR)/ex_fitting.eps \
$(EX_EPSDIR)/ex_funcsplice2.eps $(EX_EPSDIR)/ex_funcsplice.eps $(EX_EPSDIR)/ex_gallery.eps \
$(EX_EPSDIR)/ex_hlines.eps $(EX_EPSDIR)/ex_hrdiagram.eps $(EX_EPSDIR)/ex_integration.eps \
$(EX_EPSDIR)/ex_interpolation.eps $(EX_EPSDIR)/ex_intro_bessel.eps $(EX_EPSDIR)/ex_intro_func.eps \
$(EX_EPSDIR)/ex_intro_func_splice.eps $(EX_EPSDIR)/ex_intro_legendre.eps $(EX_EPSDIR)/ex_intro_sine.eps \
$(EX_EPSDIR)/ex_lenses.eps $(EX_EPSDIR)/ex_linestyles2.eps $(EX_EPSDIR)/ex_linestyles3.eps \
$(EX_EPSDIR)/ex_linestyles.eps $(EX_EPSDIR)/ex_lowerlimit.eps $(EX_EPSDIR)/ex_mandelbrot.eps \
$(EX_EPSDIR)/ex_map.eps $(EX_EPSDIR)/ex_multiaxes.eps $(EX_EPSDIR)/ex_nanotubes.eps \
$(EX_EPSDIR)/ex_newton.eps $(EX_EPSDIR)/ex_noentry.eps $(EX_EPSDIR)/ex_notice.eps \
$(EX_EPSDIR)/ex_palettelist.eps $(EX_EPSDIR)/ex_pendulum2.eps $(EX_EPSDIR)/ex_pendulum.eps \
$(EX_EPSDIR)/ex_piechart2.eps $(EX_EPSDIR)/ex_piechart.eps $(EX_EPSDIR)/ex_pi_estimation.eps \
$(EX_EPSDIR)/ex_plotstyletab.eps $(EX_EPSDIR)/ex_polygon1.eps $(EX_EPSDIR)/ex_polygon2.eps \
$(EX_EPSDIR)/ex_quadrupole.eps $(EX_EPSDIR)/ex_set_terminal.eps $(EX_EPSDIR)/ex_spectrum_1.eps \
$(EX_EPSDIR)/ex_spectrum_bbcol.eps $(EX_EPSDIR)/ex_spectrum_biref.eps $(EX_EPSDIR)/ex_spectrum_colmix1.eps \
$(EX_EPSDIR)/ex_spectrum_colmix2.eps $(EX_EPSDIR)/ex_spirograph.eps $(EX_EPSDIR)/ex_surface_log.eps \
$(EX_EPSDIR)/ex_surface_polynomial.eps $(EX_EPSDIR)/ex_surface_sinc.eps $(EX_EPSDIR)/ex_tempscale.eps \
$(EX_EPSDIR)/ex_text1.eps $(EX_EPSDIR)/ex_text2.eps $(EX_EPSDIR)/ex_torus.eps \
$(EX_EPSDIR)/ex_trajectories.eps $(EX_EPSDIR)/ex_trefoil.eps $(EX_EPSDIR)/ex_triangle.eps \
$(EX_EPSDIR)/ex_upperlimit.eps $(EX_EPSDIR)/ex_vortex.eps $(EX_EPSDIR)/ex_windowfuncs.eps \
$(EX_EPSDIR)/ex_zeta_arg.eps

test-examples: $(EX_EPSDIR)/timestamp
	for e in $(EPSTESTFILES); do \
	   ls -l $$e; \
	   if [ ! -s "$$e" ]; then \
			echo "$$e has zero length; tests failed"; exit 2; \
	   fi \
	done

.PHONY: test-examples

