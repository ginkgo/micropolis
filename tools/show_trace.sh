#!/bin/sh

python `dirname $0`/parse_trace.py $1 /tmp/$1.pdf || exit 1
evince /tmp/$1.pdf 2> /dev/null