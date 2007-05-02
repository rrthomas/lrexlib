# Documentation Makefile

.SUFFIXES: .txt .html

APP = rst2html
CSS = --stylesheet-path=lrexlib.css --link-stylesheet
HDR = --initial-header-level=2
DT  = --date --time
#GT  = -gt

.txt.html:
	$(APP) $(CSS) $(GT) $(HDR) $(DT) $< $@

all: index.html manual.html
