INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src \
	   -I$(top_builddir) -I$(top_builddir)/src \
	   $(LIBXML_CFLAGS) $(CFLAGS)

bin_PROGRAMS = xml

xml_SOURCES = xml.c
xml_LDFLAGS =
xml_DEPENDENCIES = $(DEPS)
xml_LDADD = $(LDADDS)

LDADDS = \
	 @LIBXML_LIBS@ $(EXTRA_LIBS)

