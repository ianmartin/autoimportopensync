
default: OpenSync.pdf

%.dvi:	%.tex
	pslatex $<

%.ps:	%.dvi
	dvi2ps $<

%.pdf:	%.ps
	ps2pdf $<

%.pdf:	%.tex
	pdflatex $<
