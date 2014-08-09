# This makefile automates the building of xmlstarlet and its
# dependancies (including downloading) with the mingw cross compiler.

# Things that are somewhat specific to my setup and should probably be
# generalized:

#   - Uses i686-w64-mingw32- as the cross compiler prefix which I
#     think is Debian (derivative) specific.

include versions.mk

PREFIX := $(CURDIR)/build
# Debian specific (I think)
HOST := i686-w64-mingw32

CONFIGURE_FLAGS := --prefix=$(PREFIX) --host=$(HOST) \
--with-gnu-ld --enable-silent-rules \
CPPFLAGS=-I$(PREFIX)/include \
LDFLAGS=-L$(PREFIX)/lib \
CFLAGS='-O2'

.DEFAULT_GOAL := xmlstarlet

####################
# ZLIB
####################

ZLIB := $(PREFIX)/lib/libz.a

zlib-$(ZLIB_VERSION).tar.xz :
	wget --output-document $@ \
 'http://prdownloads.sourceforge.net/libpng/zlib-$(ZLIB_VERSION).tar.xz?download'

zlib-$(ZLIB_VERSION) : zlib-$(ZLIB_VERSION).tar.xz
	tar -xf $<
	touch $@

$(ZLIB) : zlib-$(ZLIB_VERSION)
	cd $< \
 && make -f win32/Makefile.gcc \
 PREFIX=/usr/bin/$(HOST)- \
 CFLAGS='-Os' \
 prefix=$(PREFIX) \
 INCLUDE_PATH=$(PREFIX)/include \
 LIBRARY_PATH=$(PREFIX)/lib \
 BINARY_PATH=$(PREFIX)/bin \
 INSTALL='install -p' \
 install \
 && touch $@

zlib : $(ZLIB)
.PHONY : zlib

####################
# ICONV
####################

ICONV := $(PREFIX)/lib/libiconv.a

libiconv-$(ICONV_VERSION).tar.gz :
	wget --output-document $@ \
 'http://ftpmirror.gnu.org/libiconv/libiconv-$(ICONV_VERSION).tar.gz'

libiconv-$(ICONV_VERSION) : libiconv-$(ICONV_VERSION).tar.gz
	tar -xf $<
	touch $@

$(ICONV) : libiconv-$(ICONV_VERSION)
	cd $< \
 && ./configure $(CONFIGURE_FLAGS) --enable-static --disable-shared \
 && make install \
 && touch $@

iconv : $(ICONV)
.PHONY : iconv

####################
# LIBXML
####################

LIBXML := $(PREFIX)/lib/libxml2.a

libxml2-$(XML2_VERSION).tar.gz :
	wget --output-document $@ \
 'ftp://xmlsoft.org/libxml2/libxml2-$(XML2_VERSION).tar.gz'

libxml2-$(XML2_VERSION) : libxml2-$(XML2_VERSION).tar.gz
	tar -xf $<
	touch $@

$(LIBXML) : libxml2-$(XML2_VERSION) $(ICONV) $(ZLIB)
	cd $< \
 && ./configure $(CONFIGURE_FLAGS) \
    --disable-shared \
    --without-debug \
    --without-legacy \
    --without-python \
    --without-readline \
    --without-sax1 \
    --without-threads \
    --without-modules \
 && make install \
 && touch $@

libxml2 : $(LIBXML)
.PHONY : libxml2

####################
# LIBXSLT
####################

LIBXSLT := $(PREFIX)/lib/libxslt.a

libxslt-$(XSLT_VERSION).tar.gz :
	wget --output-document $@ \
 'ftp://xmlsoft.org/libxslt/libxslt-$(XSLT_VERSION).tar.gz'

libxslt-$(XSLT_VERSION) : libxslt-$(XSLT_VERSION).tar.gz
	tar -xf $<
	touch $@

$(LIBXSLT) : libxslt-$(XSLT_VERSION) $(LIBXML)
	cd $< \
 && ./configure $(CONFIGURE_FLAGS) \
    --with-libxml-prefix=$(PREFIX) \
    --disable-shared \
    --without-plugins \
    --without-debugger \
    --without-python \
 && make install \
 && touch $@

libxslt : $(LIBXSLT)
.PHONY : libxslt

####################
# XMLSTARLET
####################

xmlstarlet-$(XMLSTARLET_VERSION) :
	mkdir $@

# see http://machine-cycle.blogspot.co.il/2010/07/mingw-64bit-cross-compilation.html
export ac_cv_func_realloc_0_nonnull := yes
export ac_cv_func_malloc_0_nonnull := yes

xmlstarlet-$(XMLSTARLET_VERSION)/xmlstarlet-$(XMLSTARLET_VERSION)-win32.zip : xmlstarlet-$(XMLSTARLET_VERSION) $(LIBXML) $(LIBXSLT)
	rm -rf xmlstarlet-$(XMLSTARLET_VERSION) \
 && mkdir xmlstarlet-$(XMLSTARLET_VERSION) \
 && cd xmlstarlet-$(XMLSTARLET_VERSION) \
 && ../../configure $(CONFIGURE_FLAGS) \
    --with-libxml-prefix=$(PREFIX) \
    --with-libxslt-prefix=$(PREFIX) \
    --enable-static-libs \
    --enable-build-docs \
 && make dist-win32 \
 && cd .. \
 && touch $@

xmlstarlet : xmlstarlet-$(XMLSTARLET_VERSION)/xmlstarlet-$(XMLSTARLET_VERSION)-win32.zip
.PHONY : xmlstarlet

clean :
	rm -rf xmlstarlet-$(XMLSTARLET_VERSION)

distclean : clean
	rm -rf */ *.tar.[gx]z
