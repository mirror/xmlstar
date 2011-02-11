EXTRA_DIST = README TODO Copyright COPYING
WIN32_DIST = $(EXTRA_DIST) AUTHORS ChangeLog INSTALL NEWS xml.exe doc

ACLOCAL_AMFLAGS = -I m4
if GCC
AM_CFLAGS = -Wall -pedantic
endif

dist-hook: xmlstarlet.spec
	@ if [ -d .git ] ; then \
	    if [ $(VERSION) != `git describe --tags --dirty` ] ; then \
		echo 'ERROR: version mismatch, rerun autoreconf -f' ; exit 1 ; fi ; \
	    $(SED) 's/\[m4_esyscmd_s(\[git describe --tags --dirty\])\]/[$(VERSION)]/' \
	      $(srcdir)/configure.ac > $(distdir)/configure.ac ; fi
	-cp xmlstarlet.spec $(distdir)
	(cd $(srcdir) ; $(AMTAR) -cf - --exclude CVS doc examples solaris tests) | (cd $(distdir); $(AMTAR) xf -)

cleantar:
	@(rm -f xmlstarlet*.tar.gz)

rpm: cleantar
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

$(distdir)-win32.zip: xml.exe
	mkdir -p /tmp/$(distdir)
	cp -r $(WIN32_DIST) /tmp/$(distdir)
	rm /tmp/$(distdir)/doc/gen-doc
	strip /tmp/$(distdir)/xml.exe
	cd /tmp && zip -9 --symlinks --recurse-paths --to-crlf --move \
	  $(distdir)-win32.zip $(distdir)
	mv /tmp/$(distdir)-win32.zip .

dist-win32: $(distdir)-win32.zip

version.h: VERSION
	@if [ -d .git ] ; then \
	    VERSION=`git describe --tags --dirty`; \
	else VERSION=$(VERSION) ; fi; \
	NEW_VERSION="#define VERSION \"$$VERSION\""; \
	OLD_VERSION=`cat version.h 2>/dev/null` ; \
	if [ "$$NEW_VERSION" != "$$OLD_VERSION" ] ; then \
	    echo "$$NEW_VERSION" > version.h ; \
	    echo "version.h: $$NEW_VERSION"; fi
VERSION:

clean-local:
	-rm version.h

.PHONY: cleantar rpm dist-win32 VERSION

# testing
include examples/tests.mk

# building executable
bin_PROGRAMS = xml

include src/sources.mk
xml_SOURCES += version.h

# doc
man_MANS = doc/xmlstarlet.1
EXTRA_DIST += doc/xmlstarlet.txt
