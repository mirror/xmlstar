EXTRA_DIST = README TODO Copyright COPYING
WIN32_DIST = $(EXTRA_DIST) AUTHORS ChangeLog INSTALL NEWS xml.exe doc

ACLOCAL_AMFLAGS = -I m4
if GCC
AM_CFLAGS = -Wall
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

VERSION:
	@if [ -d .git ] ; then \
	    VERSION=`git describe --tags --dirty`; \
	else VERSION=$(VERSION) ; fi; \
	NEW_VERSION="#define VERSION \"$$VERSION\""; \
	OLD_VERSION=`cat version.h` ; \
	if [ "$$NEW_VERSION" != "$$OLD_VERSION" ] ; then \
	    echo "$$NEW_VERSION" > version.h ; \
	    echo "version.h: $$NEW_VERSION"; fi
version.h : VERSION

.PHONY: cleantar rpm dist-win32 VERSION

# testing
egdir = $(srcdir)/examples
TESTS_ENVIRONMENT = srcdir=$(srcdir) $(srcdir)/tests/runTest.sh
TESTS = $(egdir)/bigxml-dtd $(egdir)/bigxml-embed                       \
$(egdir)/bigxml-embed-ref $(egdir)/bigxml-relaxng                       \
$(egdir)/bigxml-well-formed $(egdir)/bigxml-xsd $(egdir)/count1         \
$(egdir)/countnode1 $(egdir)/c14n-default-attr $(egdir)/c14n1           \
$(egdir)/c14n2 $(egdir)/delete1 $(egdir)/dtd1 $(egdir)/dtd2             \
$(egdir)/dtd3 $(egdir)/dtd4 $(egdir)/ed-literal $(egdir)/elem1          \
$(egdir)/elem2 $(egdir)/elem3 $(egdir)/escape1 $(egdir)/exslt1          \
$(egdir)/findfile1 $(egdir)/genxml1 $(egdir)/hello1                     \
$(egdir)/localname1 $(egdir)/look1 $(egdir)/move1 $(egdir)/noindent1    \
$(egdir)/ns1 $(egdir)/recover1 $(egdir)/rename-attr1                    \
$(egdir)/rename-elem1 $(egdir)/schema1 $(egdir)/sel-literal             \
$(egdir)/sel1 $(egdir)/sort1 $(egdir)/structure1 $(egdir)/sum1          \
$(egdir)/tab1 $(egdir)/table1 $(egdir)/table2 $(egdir)/table3           \
$(egdir)/update-attr1 $(egdir)/unicode1 $(egdir)/update-elem1           \
$(egdir)/valid1 $(egdir)/xinclude1 $(egdir)/xsl-param1                  \
$(egdir)/xsl-sum1 $(egdir)/sel-xpath-c $(egdir)/sel-xpath-v             \
$(egdir)/sel-xpath-m $(egdir)/sel-xpath-i $(egdir)/ed-namespace

XFAIL_TESTS = $(egdir)/bigxml-dtd $(egdir)/ed-namespace

# building executable
bin_PROGRAMS = xml

xml_SOURCES = src/xml.c src/xml_edit.c src/xml_ls.c src/xml_escape.c    \
src/xml_pyx.c src/xml_depyx.c src/xml_select.c src/xml_trans.c          \
src/xml_validate.c src/xml_format.c src/xml_elem.c src/xml_C14N.c       \
src/trans.c src/binsert.c src/stack.c src/binsert.h src/stack.h         \
src/trans.h src/escape.h src/strdup.h src/strdup.c version.h

# doc
man_MANS = doc/xmlstarlet.1
EXTRA_DIST += doc/xmlstarlet.txt
