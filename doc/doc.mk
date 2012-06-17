userguide = doc/xmlstarlet-ug
userguide_gen = $(userguide).pdf $(userguide).ps $(userguide).html
userguide_src = $(userguide).xml

txtguide = doc/xmlstarlet.txt
txtguide_src = doc/gen-doc

manpage = doc/xmlstarlet.1
manpage_src = doc/xmlstarlet-man.xml

DOCBOOK_PARAMS = \
--param section.autolabel 1 \
--stringparam generate.toc 'book toc,title'

EDIT_XML = ./xml ed -P \
-u '//db:phrase[@role="VERSION"]' -v '$(VERSION)' \
-u '//db:phrase[@role="PROG"]' -v "`echo xml | sed '$(program_transform_name)'`"

if BUILD_DOCS

.xml.html:
	$(EDIT_XML) $< | xsltproc $(DOCBOOK_PARAMS) \
  --stringparam html.stylesheet html.css \
  http://docbook.sourceforge.net/release/xsl-ns/current/html/docbook.xsl \
  - > $@

.xml.fo:
	$(EDIT_XML) $< | xsltproc $(DOCBOOK_PARAMS) doc/xmlstar-fodoc-style.xsl - > $@

if HAVE_FOP
.fo.pdf:
	$(FOP) -q $< $@
endif
if HAVE_PDF2PS
.pdf.ps:
	$(PDF2PS) $< $@
endif

$(userguide).html : $(userguide).xml | xml$(EXEEXT)

$(manpage): $(manpage_src)
	 $(EDIT_XML) $< | xsltproc -o $@ \
	  http://docbook.sourceforge.net/release/xsl-ns/current/manpages/docbook.xsl \
	  -

$(txtguide): $(txtguide_src) | xml$(EXEEXT)
	srcdir=$(srcdir) transform='$(program_transform_name)' $< ./xml > $@

endif # BUILD_DOCS
