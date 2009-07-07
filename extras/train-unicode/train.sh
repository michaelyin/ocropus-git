#!/bin/bash

# usage:
#         train.sh <text-file> [output-directory] [book-name]

if [ $# -le 1 ] ; then
	outdir="."
else
	outdir=$2
fi

if [ $# -le 2 ] ; then
	bookname=`basename $1`
else
	bookname=$3
fi

./ocrogenft -b $bookname -o $outdir $1
ocropus trainseg ${outdir}/${bookname}.model ${outdir}/$1


