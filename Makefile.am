#
SUBDIRS = \
 	src

DIST_SUBDIRS = src

EXTRA_DIST = \
	README TODO Copyright COPYING

confexecdir=$(libdir)
m4datadir=$(datadir)/aclocal

dist-hook: xmlstarlet.spec
	-cp xmlstarlet.spec $(distdir)
	(cd $(srcdir) ; tar -cf - --exclude CVS win32 macos vms test result) | (cd $(distdir); tar xf -)

cleantar:
	@(rm -f xmlstarlet*.tar.gz)

rpm: cleantar
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

