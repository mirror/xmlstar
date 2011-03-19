
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
	fop $< $@
.pdf.ps:
	pdf2ps $< $@

userguide = doc/xmlstarlet-ug.pdf doc/xmlstarlet-ug.ps doc/xmlstarlet-ug.html
userguide_src = doc/xmlstarlet-ug.xml

doc/xmlstarlet.1: doc/xmlstarlet-man.xml
	cd doc && xsltproc \
  http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl \
  xmlstarlet-man.xml

doc/xmlstarlet.txt: doc/gen-doc xml
	cd doc && ./gen-doc > xmlstarlet.txt

docs_src = $(userguide_src) doc/xmlstarlet-man.xml doc/gen-doc
docs_noman_gen = $(userguide) doc/xmlstarlet.txt
docs_gen = $(docs_noman_gen) doc/xmlstarlet.1
