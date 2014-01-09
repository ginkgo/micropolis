#!/bin/sh

tmpfile=/tmp/`basename $1`.pdf

python3 `dirname $0`/visualize_trace.py $@ $tmpfile || exit 1
evince $tmpfile 2> /dev/null
