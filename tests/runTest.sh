#!/bin/sh

TEST=`basename $1`
cd "$srcdir"/examples || exit 77

srcdir=.. $SHELL ./$TEST | tr -d "\r" \
    | diff -u - results/$TEST.out
