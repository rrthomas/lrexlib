# Documentation Makefile

.SUFFIXES: .txt .html

APP = rst2html
CSS = --stylesheet-path=lrexlib.css --link-stylesheet
HDR = --initial-header-level=2
DT  = --date --time

PAGES = index.html manual.html

.txt.html:
	$(APP) $(CSS) $(HDR) $(DT) $< $@

all: $(PAGES)

index.txt: ../README
	cp -a $< $@

clean:
	rm $(PAGES)
