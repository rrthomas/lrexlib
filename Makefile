# Makefile for lrexlib

VERSION = 2.9.0
PROJECT = lrexlib
PROJECT_VERSIONED = $(PROJECT)-$(VERSION)

# Commands
LUA = lua
LUAROCKS = luarocks
CP = cp -a
RM = rm -f
RST2HTML = rst2html
REGNAMES = gnu pcre pcre2 posix oniguruma tre
LUAROCKS_COMMAND = make


.SUFFIXES: .txt .html

HTML = doc/index.html doc/manual.html

.txt.html:
	$(RST2HTML) --stylesheet-path=doc/lrexlib.css --link-stylesheet --initial-header-level=2 --date --time $< $@

build:
	$(MAKE) install LUAROCKS="$(LUAROCKS) --tree=luarocks"

install: rockspecs
	for i in *.rockspec; do \
	  $(LUAROCKS) $(LUAROCKS_COMMAND) $$i; \
	done

rockspecs:
	rm -f *.rockspec
	$(LUA) mkrockspecs.lua $(PROJECT) $(VERSION)

doc/index.txt: README.rst
	$(CP) $< $@

check: build
	for i in $(REGNAMES); do \
	  LUA_PATH="test/?.lua;$(LUA_PATH);" $(LUA) test/runtest.lua -dsrc/$$i $$i; \
	done

clean:
	$(RM) $(HTML) doc/index.txt *.rockspec

release: check
	agrep -d 'Release' $(VERSION) NEWS | tail -n +3 | head -n -2 > release-notes && \
	git diff --exit-code && \
	git tag -a -m "Release tag" rel-`echo $(VERSION) | sed -e 's/\./-/g'` && \
	git push && git push --tags && \
	$(MAKE) build LUAROCKS_COMMAND=build && \
	woger lua package=$(PROJECT) package_name=$(PROJECT) version=$(VERSION) description="Lua binding for regex libraries" notes=release-notes home="`$(LUA) -e'version="'$(VERSION)'"; flavour="none"; t = require "rockspecs"; print(t.default.description.homepage)'`"
	rm -f release-notes
