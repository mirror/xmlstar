#
SUBDIRS = src doc
DIST_SUBDIRS = src doc

EXTRA_DIST = \
	README TODO Copyright COPYING

ACLOCAL_AMFLAGS = -I m4

dist-hook: xmlstarlet.spec
	-cp xmlstarlet.spec $(distdir)
	(cd $(srcdir) ; $(AMTAR) -cf - --exclude CVS doc examples solaris tests) | (cd $(distdir); $(AMTAR) xf -)

cleantar:
	@(rm -f xmlstarlet*.tar.gz)

rpm: cleantar
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

dummy:

check-local: tests

tests: dummy 
	@(cd $(top_srcdir)/tests; ./runTests)
