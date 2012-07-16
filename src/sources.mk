generated_usage_sources =\
src/c14n-usage.c\
src/depyx-usage.c\
src/edit-usage.c\
src/elem-usage.c\
src/escape-usage.c\
src/format-usage.c\
src/ls-usage.c\
src/pyx-usage.c\
src/select-usage.c\
src/trans-usage.c\
src/unescape-usage.c\
src/validate-usage.c

.txt.c:
	$(AM_V_GEN)$(AWK) -f ./usage2c.awk $< > $@
$(generated_usage_sources) : usage2c.awk

nodist_xml_SOURCES = $(generated_usage_sources)

xml_SOURCES =\
src/escape.h\
src/trans.c\
src/trans.h\
src/xml.c\
src/xml_C14N.c\
src/xml_depyx.c\
src/xml_edit.c\
src/xml_elem.c\
src/xml_escape.c\
src/xml_format.c\
src/xml_ls.c\
src/xml_pyx.c\
src/xml_select.c\
src/xmlstar.h\
src/xml_trans.c\
src/xml_validate.c
