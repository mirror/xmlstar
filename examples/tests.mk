TESTS_ENVIRONMENT = abs_builddir=$(abs_builddir) EXEEXT=$(EXEEXT) SED=$(SED) \
 $(srcdir)/tests/runTest.sh

TESTS = examples/bigxml-dtd\
examples/bigxml-embed-ref\
examples/bigxml-embed\
examples/bigxml-relaxng\
examples/bigxml-well-formed\
examples/bigxml-xsd\
examples/c14n-default-attr\
examples/c14n-newlines\
examples/c14n1\
examples/c14n2\
examples/command-help\
examples/count1\
examples/countnode1\
examples/delete1\
examples/dtd1\
examples/dtd2\
examples/dtd3\
examples/dtd4\
examples/ed-2op\
examples/ed-append\
examples/ed-expr\
examples/ed-insert\
examples/ed-literal\
examples/ed-move\
examples/ed-namespace\
examples/ed-nop\
examples/ed-subnode\
examples/elem1\
examples/elem2\
examples/elem3\
examples/elem-depth\
examples/elem-uniq\
examples/escape1\
examples/exslt-ed\
examples/exslt1\
examples/external-entity\
examples/findfile1\
examples/genxml1\
examples/hello1\
examples/localname1\
examples/look1\
examples/move1\
examples/N-order\
examples/noindent1\
examples/ns1\
examples/pyx\
examples/recover1\
examples/rename-attr1\
examples/rename-elem1\
examples/schema1\
examples/sel-literal\
examples/sel-if\
examples/sel-many-values\
examples/sel-root\
examples/sel-xpath-c\
examples/sel-xpath-i\
examples/sel-xpath-m\
examples/sel-xpath-v\
examples/sel1\
examples/sort1\
examples/sort2\
examples/sort3\
examples/structure1\
examples/sum1\
examples/tab1\
examples/table1\
examples/table2\
examples/table3\
examples/unicode1\
examples/update-attr1\
examples/update-elem1\
examples/valid1\
examples/xinclude1\
examples/xsl-param1\
examples/xsl-sum1

XFAIL_TESTS =\
examples/bigxml-dtd\
examples/ed-namespace

if !HAVE_EXSLT_XPATH_REGISTER
XFAIL_TESTS += examples/exslt-ed
endif
