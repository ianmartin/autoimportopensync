
default: OpenSync.pdf

%.dvi:	%.tex
	pslatex $<
	pslatex $<

%.ps:	%.dvi
	dvips $<

%.pdf:	%.ps
	ps2pdf $<

#%.pdf:	%.tex
#	pdflatex $<
#	pdflatex $<

clean:
	rm -f *.ps *.dvi *.pdf

mrproper: clean
	rm -f *.log *.out *.aux *.toc
	
