all: doc/doc.html oie.hex emu

emu: oie.c emu.c
	gcc -std=c99 $^ -o emu

PROJECT = oie
C_FILES = oie.c hw.c
include avr.mk

doc/doc.html: doc/doc.rst doc/mycss.css doc/mytemplate.txt
	rst2html --template=doc/mytemplate.txt --stylesheet=doc/mycss.css doc/doc.rst doc/doc.html

clean: cleanemu cleandocs

cleandocs:
	rm -f doc/doc.html 
cleanemu:
	rm -f emu 
