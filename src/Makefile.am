INCLUDES = -I. -I$(top_srcdir) -I$(top_srcdir)/src \
	   -I$(top_builddir) -I$(top_builddir)/src \
	   $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS) $(CFLAGS)

bin_PROGRAMS = xml

xml_SOURCES = binsert.c stack.c trans.c xml_edit.c xml_select.c xml_trans.c xml_validate.c xml_format.c xml_elem.c xml.c binsert.h stack.h trans.h
xml_LDFLAGS = 
xml_DEPENDENCIES = $(DEPS)
xml_LDADD = $(LDADDS)

EXTRA_DIST = binsert.c stack.c trans.c xml_edit.c xml_select.c xml_trans.c xml_validate.c xml_format.c xml_elem.c xml.c binsert.h stack.h trans.h

LDADDS = \
	 @LIBXML_LIBS@ @LIBXSLT_LIBS@ $(EXTRA_LIBS)

