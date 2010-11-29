#!/bin/sh

TEST=`basename $1`
cd "$srcdir"/examples || exit 77

srcdir="$srcdir"/.. ./$TEST | tr -d "\r" \
    | diff - results/$TEST.out >/dev/null 2>&1
