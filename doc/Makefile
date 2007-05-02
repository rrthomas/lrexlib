
APP = C:/Python24/python c:/python24/Scripts/rst2html.py
CSS = --stylesheet-path=lrexlib.css --link-stylesheet
HDR = --initial-header-level=2
DT  = --date --time
#GT  = -gt

.txt.html:
	$(APP) $(CSS) $(GT) $(HDR) $(DT) $< $@

all: manual.html

