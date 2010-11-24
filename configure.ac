AC_PREREQ([2.62])
AC_INIT([XMLStarlet], [m4_esyscmd_s([git describe --tags])],
        [http://sourceforge.net/projects/xmlstar/support],
        [],
        [http://xmlstar.sourceforge.net/])
AC_REVISION(AC_PACKAGE_VERSION)

AM_INIT_AUTOMAKE([1.11 foreign -Wall subdir-objects])
AM_SILENT_RULES
AM_MAINTAINER_MODE

[LIBXML_REQUIRED_VERSION=2.6.12]
[LIBXSLT_REQUIRED_VERSION=1.1.9]

XSTAR_LIB_ARGS_WITH([LIBXML], [with-src])
XSTAR_LIB_ARGS_WITH([LIBXSLT], [with-src])
XSTAR_LIB_ARGS_WITH([LIBICONV])


dnl
dnl Check the environment
dnl

AC_CANONICAL_HOST
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_ARG_PROGRAM          dnl Transforming Program Names When Installing

XSTAR_LIB_CHECK([LIBXML], [xml2-config])
AS_IF([test "x$LIBXML_SRCDIR" != x],
      [LIBXML_INCDIR="$LIBXML_SRCDIR/include"],
      [: ${LIBXML_INCDIR="$LIBXML_PREFIX/include/libxml2"}])
[LIBXML_LIBS="$LIBXML_LIBDIR/libxml2.a"]
[LIBXML_CPPFLAGS="-DLIBXML_STATIC -I$LIBXML_INCDIR"]

XSTAR_LIB_CHECK([LIBXSLT], [xslt-config])
[: ${LIBXSLT_INCDIR="$LIBXSLT_PREFIX/include"}]
AS_IF([test "x$LIBXSLT_SRCDIR" != x],
      [LIBXSLT_INCDIR="$LIBXSLT_SRCDIR"
       LIBXSLT_LIBS="$LIBXSLT_SRCDIR/libexslt/.libs/libexslt.a $LIBXSLT_SRCDIR/libxslt/.libs/libxslt.a"],
      [LIBXSLT_LIBS="$LIBXSLT_LIBDIR/libexslt.a $LIBXSLT_LIBDIR/libxslt.a"])
[: ${LIBSLT_INCDIR="$LIBXSLT_PREFIX/include"}]
[LIBXSLT_CPPFLAGS="-I$LIBXSLT_INCDIR"]

[WIN32_EXTRA_LDFLAGS=]
[WIN32_EXTRA_LIBS=]

AS_CASE(["${host}"],
  [*-hpux*],
      [AS_IF([test "x$GCC" != "xyes"],
             [CFLAGS="-Ae"])],
  [*mingw*],
      [WIN32_EXTRA_LDFLAGS='-no-undefined'
       WIN32_EXTRA_LIBS='-lwsock32'
       AC_DEFINE([_WINSOCKAPI_],1,[Using the Win32 Socket implementation])
       AC_DEFINE([snprintf],[_snprintf],[Win32 Std C name mangling work-around])
       AC_DEFINE([vsnprintf],[_vsnprintf],[Win32 Std C name mangling work-around])])

AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# The trick with USER_LIBS allows make LIBS=-lfoo to add rather than
# replace the libraries from ./configure
[USER_LIBS="$LIBS"]
[LIBS=]
AC_SEARCH_LIBS([dlopen], [dl], [], [], "$USER_LIBS")
AC_SEARCH_LIBS([sqrt], [m], [], [], "$USER_LIBS")
AC_SEARCH_LIBS([iconv_open], [iconv], [], [], "$USER_LIBS")
AC_SEARCH_LIBS([libiconv_open], [iconv], [], [], "$USER_LIBS")
AC_SEARCH_LIBS([clock_gettime], [rt], [], [], "$USER_LIBS")
AC_SEARCH_LIBS([deflate], [z], [], [], "$USER_LIBS")
AC_SEARCH_LIBS([gcry_cipher_encrypt], [gcrypt], [], [], "$USER_LIBS")

# Checks for inet libraries:
AC_SEARCH_LIBS([gethostent], [nsl], [], [], "$USER_LIBS")
AC_SEARCH_LIBS([setsockopt], [socket net network], [], [], "$USER_LIBS")
AC_SEARCH_LIBS([connect], [inet], [], [], "$USER_LIBS")

AC_CHECK_FUNCS_ONCE([gettimeofday mktime localtime asctime time gmtime ftime strdup])

AC_SUBST([AM_CPPFLAGS], ["$LIBXSLT_CPPFLAGS $LIBXML_CPPFLAGS"])
AC_SUBST([AM_LDFLAGS], ["$WIN32_EXATRA_LDFLAGS"])
AC_SUBST([LDADD], ["$LIBXSLT_LIBS $LIBXML_LIBS $WIN32_EXTRA_LIBS $LIBS"])
AC_SUBST([LIBS], ["$USER_LIBS"])


AM_SUBST_NOTMAKE(PACKAGE_NAME)
AM_SUBST_NOTMAKE(PACKAGE_TARNAME)
AM_SUBST_NOTMAKE(PACKAGE_VERSION)

dnl for the spec file

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
xmlstarlet.spec
Makefile
solaris/package/sol8-sparc/pkginfo
solaris/package/sol9-sparc/pkginfo
])

AC_OUTPUT

