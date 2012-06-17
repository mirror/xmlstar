userguide = doc/xmlstarlet-ug.pdf doc/xmlstarlet-ug.ps doc/xmlstarlet-ug.html
userguide_src = doc/xmlstarlet-ug.xml

txtguide = doc/xmlstarlet.txt
txtguide_src = doc/gen-doc

manpage = doc/xmlstarlet.1
manpage_src = doc/xmlstarlet-man.xml

DOCBOOK_PARAMS = \
--param section.autolabel 1 \
--stringparam generate.toc 'book toc,title'

EDIT_XML = xml ed -P \
-u '//db:phrase[@role="VERSION"]' -v '$(VERSION)' \
-u '//db:phrase[@role="PROG"]' -v "`echo xml | sed '$(program_transform_name)'`"

.xml.html:
	$(EDIT_XML) $< | xsltproc $(DOCBOOK_PARAMS) \
  --stringparam html.stylesheet html.css \
  http://docbook.sourceforge.net/release/xsl-ns/current/html/docbook.xsl \
  - > $@

.xml.fo:
	$(EDIT_XML) $< | xsltproc $(DOCBOOK_PARAMS) doc/xmlstar-fodoc-style.xsl - > $@

.fo.pdf:
	fop -q $< $@
.pdf.ps:
	pdf2ps $< $@

$(manpage): $(manpage_src)
	 $(EDIT_XML) $< | xsltproc -o $@ \
	  http://docbook.sourceforge.net/release/xsl-ns/current/manpages/docbook.xsl \
	  -

$(txtguide): $(txtguide_src) xml
	srcdir=$(srcdir) transform='$(program_transform_name)' $< ./xml > $@
