#!/bin/sh

TEST=`basename $1`
cd "$srcdir"/examples || exit 77

srcdir="$srcdir"/.. ./$TEST | tr -d "\r" \
    | diff -c - results/$TEST.out
