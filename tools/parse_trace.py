#!/usr/bin/python

import sys
import re

def parse(file):
    pattern = re.compile("(.*):(\d+):(\d+):(\d+):(\d+)")

    items = []

    for line in file.readlines():
        match = pattern.match(line)

        if not match:
            print('Input file format mismatch.')
            exit(1)
        
        item = (match.group(1), 
                int(match.group(2)), 
                int(match.group(3)), 
                int(match.group(4)), 
                int(match.group(5)))
        items.append(item)
    
    return items
    
def milliseconds(nanoseconds):
    return nanoseconds / 1000000.0

def duration(item):
    return item[4] - item[3]

if (len(sys.argv) != 3):
    print('Usage: %s <tracefile> <outfile>' % sys.argv[0])
    exit(0)

tracefile = open(sys.argv[1], 'r')


items = parse(tracefile)


for item in items:
    print ('%s: %.2f ms' % (item[0], milliseconds(duration(item))))
