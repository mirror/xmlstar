INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src \
	   -I$(top_builddir) -I$(top_builddir)/src \
	   $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS) $(CFLAGS)

bin_PROGRAMS = xml

xml_SOURCES = trans.c xml_edit.c xml_select.c xml_trans.c xml_validate.c xml.c
xml_LDFLAGS = 
xml_DEPENDENCIES = $(DEPS)
xml_LDADD = $(LDADDS)

EXTRA_DIST = trans.c xml_edit.c xml_select.c xml_trans.c xml_validate.c xml.c

LDADDS = \
	 @LIBXML_LIBS@ @LIBXSLT_LIBS@ $(EXTRA_LIBS)

