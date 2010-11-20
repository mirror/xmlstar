#!/bin/sh
cat <<EOF >&2
Just use autoreconf -sif, autogen.sh is obsolete
See http://www.gnu.org/software/automake/manual/automake.html#Future-of-aclocal
EOF

exit 1

