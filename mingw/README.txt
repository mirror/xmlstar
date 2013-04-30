These are scripts that semi-automate the building of xmlstarlet and
its dependancies with the mingw cross compiler.

Things that are somewhat specific to my setup and should probably be
generalized:

  - They use i586-mingw32msvc- as the cross compiler prefix which I
    think is Debian (derivative) specific.

  - The libraries are installed to /usr/local/mingw32 using sudo.



The non (or yet-to-be) automated part is

ZLIB_VERSION=1.2.7
ICONV_VERSION=1.14
XML2_VERSION=2.9.0 # --> 2.9.1
XSLT_VERSION=1.1.28
XMLSTARLET_VERSION=1.4.2 # --> 1.5.0

## Download and untar zlib, libiconv, libxml2, libxslt.

(cd zlib-$ZLIB_VERSION && ../make.zlib)
for lib in libiconv-$ICONV_VERSION libxml2-$XML2_VERSION libxslt-$XSLT_VERSION
do
    (cd $lib && ../config.$lib && make && sudo make install)
done

# untar xmlstarlet
cd xmlstarlet-$XMLSTARLET_VERSION && ../config.xmlstarlet && make dist-win32

# xmlstarlet-$XMLSTARLET_VERSION.zip should now be ready
