# Author Kris Thielemans
# Copyright (C) 2004-2007, Hammersmith Imanet Ltd
# Copyright (C) 2014, University College London
# This file is part of STIR
# SPDX-License-Identifier: Apache-2.0


all: STIR-glossary.pdf STIR-general-overview.pdf \
     STIR-developers-overview.pdf \
     STIR-UsersGuide.pdf \
     STIR-version2-vs-version1.pdf

clean:
	$(RM) -rf STIR-*.pdf *.log *.dvi *.aux *.toc *.out STIR-UsersGuide/

doc_as_html: STIR-UsersGuide/homepage.html	


.PHONY: all clean doc_as_html

# rule to make pdf from tex source
# note: need to run twice to get page refs etc ok.
%.pdf: %.tex
	pdflatex $<
	pdflatex $<

STIR-UsersGuide/homepage.html: STIR-UsersGuide.pdf
	rm -rf STIR-UsersGuide;
	latex2html -no_auto_link -split 7 -local_icons STIR-UsersGuide.tex; 
	pushd STIR-UsersGuide/;
	rm *.pl *.tex *.log WARNINGS;
	cp STIR-UsersGuide.html index.html
	cp STIR-UsersGuide.html homepage.html
	popd

