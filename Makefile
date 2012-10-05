# Makefile for lrexlib

# See src/*/Makefile for user-definable settings

REGNAMES = gnu pcre posix oniguruma tre
PROJECT = lrexlib
VERSION = 2.7.0
PROJECT_VERSIONED = $(PROJECT)-$(VERSION)
DISTFILE = $(PROJECT_VERSIONED).zip

install:
	@for i in *.rockspec; do \
	  luarocks make $$i; \
	done

check:
	@if test -z "$(LUA)"; then echo "Set LUA to run tests"; exit 1; fi
	@for i in $(REGNAMES); do \
	  LUA_PATH="test/?.lua;$(LUA_PATH)" $(LUA) test/runtest.lua -dsrc/$$i $$i; \
	done

docs:
	@make -C doc

rockspecs:
	lua mkrockspecs.lua $(VERSION) `md5sum $(DISTFILE)`

dist: docs
	git2cl > ChangeLog
	cd .. && rm -f $(DISTFILE) && zip $(DISTFILE) -r $(PROJECT) -x "lrexlib/.git/*" "*.gitignore" "*.o" "*.a" "*.so" "*.so.*" "*.zip" "*SciTE.properties" "*scite.properties" && mv $(DISTFILE) $(PROJECT) && cd $(PROJECT) && unzip $(DISTFILE) && mv $(PROJECT) $(PROJECT_VERSIONED) && rm -f $(DISTFILE) && zip $(DISTFILE) -r $(PROJECT_VERSIONED) && rm -rf $(PROJECT_VERSIONED)

WOGER_ARGS = package=$(PROJECT) package_name=$(PROJECT) version=$(VERSION) description="Lua binding for regex libraries" notes=release-notes dist_type="zip" github_user=rrthomas

release: dist check
	agrep -d 'Release' $(VERSION) NEWS | tail -n +3 | head -n -2 > release-notes && \
	git diff --exit-code && \
	git tag -a -m "Release tag" rel-`echo $(VERSION) | sed -e 's/\./-/g'` && \
	git push && git push --tags && \
	woger github $(WOGER_ARGS) && \
	for i in $(REGNAMES); do \
	  LUAROCKS_CONFIG=luarocks-config.lua luarocks --tree=luarocks build $$i; \
	done && \
	woger lua $(WOGER_ARGS)
	@echo "Don't forget to upload release to github!"
	rm -f release-notes
