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

PYXPLOT = ../../pyxplot8/bin/pyxplot
EXAMPLEDIR = examples
EPSDIR = $(EXAMPLEDIR)/eps
EPSFILES = $(EPSDIR)/ex_3d_helix.eps $(EPSDIR)/ex_apachelog.eps $(EPSDIR)/ex_axisatzero.eps $(EPSDIR)/ex_axislabs.eps $(EPSDIR)/ex_axistics.eps $(EPSDIR)/ex_axistics2.eps $(EPSDIR)/ex_axisunits.eps $(EPSDIR)/ex_barchart1.eps $(EPSDIR)/ex_barchart2.eps $(EPSDIR)/ex_branch_cuts.eps $(EPSDIR)/ex_cmbrtemp.eps $(EPSDIR)/ex_col_vs_mono.eps $(EPSDIR)/ex_contourmap.eps $(EPSDIR)/ex_cover.eps $(EPSDIR)/ex_datagrid.eps $(EPSDIR)/ex_eqnsolve.eps $(EPSDIR)/ex_ellipse.eps $(EPSDIR)/ex_euclid_I_47.eps $(EPSDIR)/ex_fft.eps $(EPSDIR)/ex_fitting.eps $(EPSDIR)/ex_funcsplice.eps $(EPSDIR)/ex_funcsplice2.eps $(EPSDIR)/ex_gallery.eps $(EPSDIR)/ex_hlines.eps $(EPSDIR)/ex_hrdiagram.eps $(EPSDIR)/ex_integration.eps $(EPSDIR)/ex_interpolation.eps $(EPSDIR)/ex_intro_bessel.eps $(EPSDIR)/ex_intro_func.eps $(EPSDIR)/ex_intro_func_splice.eps $(EPSDIR)/ex_intro_legendre.eps $(EPSDIR)/ex_intro_sine.eps $(EPSDIR)/ex_lenses.eps $(EPSDIR)/ex_linestyles.eps $(EPSDIR)/ex_lowerlimit.eps $(EPSDIR)/ex_mandelbrot.eps $(EPSDIR)/ex_map.eps $(EPSDIR)/ex_multiaxes.eps $(EPSDIR)/ex_quadrupole.eps $(EPSDIR)/ex_nanotubes.eps $(EPSDIR)/ex_newton.eps $(EPSDIR)/ex_noentry.eps $(EPSDIR)/ex_notice.eps $(EPSDIR)/ex_pendulum.eps $(EPSDIR)/ex_pi_estimation.eps $(EPSDIR)/ex_piechart.eps $(EPSDIR)/ex_set_terminal.eps $(EPSDIR)/ex_spirograph.eps $(EPSDIR)/ex_surface_log.eps $(EPSDIR)/ex_surface_polynomial.eps $(EPSDIR)/ex_surface_sinc.eps $(EPSDIR)/ex_tempscale.eps $(EPSDIR)/ex_text1.eps $(EPSDIR)/ex_text2.eps $(EPSDIR)/ex_torus.eps $(EPSDIR)/ex_triangle.eps $(EPSDIR)/ex_trajectories.eps $(EPSDIR)/ex_trefoil.eps $(EPSDIR)/ex_vortex.eps $(EPSDIR)/ex_windowfuncs.eps $(EPSDIR)/ex_zeta_arg.eps

SOURCES = calculations.tex changelog.tex colours.tex configuration.tex constants.tex data.tex definitions.tex first_steps.tex fit_maths.tex functions.tex gnuplot_diffs.tex installation.tex introduction.tex linestyles.tex main.tex other_apps.tex papersizes.tex plotting.tex programming.tex pyxplot.tex pyxplot-plain.tex reference.tex terminals.tex units.tex vector_graphics.tex $(EPSFILES)

all: $(EPSFILES) pyxplot-plain.ps pyxplot-plain.pdf pyxplot.pdf

clean:
	rm -Rf $(EPSDIR)
	rm -f *.pdf *.ps *.dvi *.aux *.idx *.ilg *.ind *.lob *.loe *.lof *.log *.out *.toc

$(EPSDIR)/%.eps: $(EXAMPLEDIR)/%.ppl $(PYXPLOT) .pyxplotrc
	echo "Producing example plot $*..."
	mkdir -p $(EPSDIR)
	$(PYXPLOT) $< > /dev/null

$(EPSDIR)/%.pdf: $(EPSDIR)/%.eps

pyxplot-plain.pdf: pyxplot-plain.ps
	echo "Making plain pdf documentation..."
	ps2pdf $< $@ > /dev/null 2> /dev/null

# Dependency on pyxplot-plain.dvi is a fudge so that 'make -j 2' doesn't parallelise pdflatex and latex
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

pyxplot-plain.dvi: $(SOURCES)
	echo "Compiling documentation..."
	rm -f *.aux *.idx *.ilg *.ind *.lob *.loe *.lof
	latex pyxplot-plain.tex > /dev/null 2> /dev/null
	latex pyxplot-plain.tex > /dev/null 2> /dev/null
	latex pyxplot-plain.tex > /dev/null 2> /dev/null
	makeindex pyxplot-plain.idx > /dev/null 2> /dev/null
	latex pyxplot-plain.tex > /dev/null 2> /dev/null

