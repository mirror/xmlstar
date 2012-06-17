userguide = doc/xmlstarlet-ug.pdf doc/xmlstarlet-ug.ps doc/xmlstarlet-ug.html
userguide_src = doc/xmlstarlet-ug.xml

txtguide = doc/xmlstarlet.txt
txtguide_src = doc/gen-doc

manpage = doc/xmlstarlet.1
manpage_src = doc/xmlstarlet-man.xml

DOCBOOK_PARAMS = \
--param section.autolabel 1 \
--stringparam generate.toc 'book toc,title'

.xml.html:
	xsltproc $(DOCBOOK_PARAMS) \
  --stringparam html.stylesheet html.css \
  http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl \
  $< > $@

.xml.fo:
	xsltproc $(DOCBOOK_PARAMS) doc/xmlstar-fodoc-style.xsl $< > $@

.fo.pdf:
	fop -q $< $@
.pdf.ps:
	pdf2ps $< $@

$(manpage): $(manpage_src)
	xsltproc -o $@ \
	  http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl \
	  $<

$(txtguide): $(txtguide_src) xml
	srcdir=$(srcdir) $< ./xml > $@
