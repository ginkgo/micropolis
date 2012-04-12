#!/bin/sh

tmpfile=/tmp/`basename $1`.pdf

python `dirname $0`/parse_trace.py $1 $tmpfile || exit 1
evince $tmpfile 2> /dev/null