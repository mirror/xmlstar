INCLUDES = -I. -I$(top_srcdir) -I$(top_srcdir)/src \
	   -I$(top_builddir) -I$(top_builddir)/src \
	   $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS) $(LIBICONV_CFLAGS) $(CFLAGS)

bin_PROGRAMS = xml

xml_SOURCES = xml.c xml_edit.c xml_ls.c xml_escape.c xml_pyx.c xml_depyx.c xml_select.c xml_trans.c xml_validate.c xml_format.c xml_elem.c xml_C14N.c trans.c binsert.c stack.c binsert.h stack.h trans.h
xml_LDFLAGS = 
xml_DEPENDENCIES = $(DEPS)
xml_LDADD = $(LDADDS)

EXTRA_DIST = xml.c xml_edit.c xml_ls.c xml_escape.c xml_pyx.c xml_depyx.c xml_select.c xml_trans.c xml_validate.c xml_format.c xml_elem.c xml_C14N.c trans.c binsert.c stack.c binsert.h stack.h trans.h

LDADDS = \
	 @LIBXSLT_LIBS@ @LIBXML_LIBS@ @LIBICONV_LIBS@ $(EXTRA_LIBS) @WIN32_EXTRA_LIBADD@

