#!/bin/sh

# Run this to generate all the initial makefiles, etc after checking out from CVS

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=. 

THEDIR=`pwd`
cd $srcdir
DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile xmlstarlet."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}

#(libtool --version) < /dev/null > /dev/null 2>&1 || {
#	echo
#	echo "You must have libtool installed to compile xmlstarlet."
#	echo "Get ftp://ftp.gnu.org/pub/gnu/libtool/libtool-1.4.2.tar.gz"
#	echo "(or a newer version if it is available)"
#	DIE=1
#}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake 1.4 or newer installed to compile xmlstarlet"
	echo "Get ftp://sources.redhat.com/pub/automake/automake-1.4l.tar.gz"
	echo "(or a newer version if it is available)."              
	DIE=1
}

if test "$DIE" -eq 1; then
	exit 1
fi

test -f src/xml.c || {
	echo "You must run this script in the top-level xmlstarlet directory"
	exit 1
}

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

#libtoolize --copy --force
aclocal $ACLOCAL_FLAGS
automake --add-missing
autoconf

cd $THEDIR

if test x$OBJ_DIR != x; then
    mkdir -p "$OBJ_DIR"
    cd "$OBJ_DIR"
fi

$srcdir/configure "$@"

echo 
echo "Now type 'make' to compile xmlstarlet."

