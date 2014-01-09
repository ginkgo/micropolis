#!/usr/bin/python3
# -*- coding: utf-8 -*-

from optparse import OptionParser
from parse_trace import *

from collections import defaultdict
from tabulate import tabulate

class Summary:
    def __init__(self, traceitems):
        
        tmin,tmax = find_time_range(traceitems)

        self.duration = milliseconds(tmax - tmin)
        self.percentages = {}
        self.times = {}
    
        ti_times  = defaultdict(float)
        
        for ti in traceitems:
            ti_times[ti.name] += ti.duration_ms()

        for name,ti_duration in ti_times.items():
            self.times[name] = ti_duration
            self.percentages[name] = 100 * ti_duration/self.duration

    def sorted_percentages(self):
        return reversed([(n,p) for p,n in sorted((p,n) for n,p in self.percentages.items())])

    def sorted_times(self):
        return reversed([(n,p) for p,n in sorted((p,n) for n,p in self.times.items())])
            
    def print(self):
        self.print_duration()
        self.print_times()
        self.print_percentages()

    def print_duration(self):
        print('total frame time: %.2fms' % self.duration)

    def print_percentages(self):
        table = self.sorted_percentages()
        header = ["Task", "%"]
        print (tabulate(table, header, tablefmt='simple', floatfmt=".1f"))

    def print_times(self):
        table = self.sorted_times()
        header = ["Task", "ms"]
        print (tabulate(table, header, tablefmt='simple', floatfmt=".3f"))

def parse_trace_file_and_create_summary(filename):
    with open(filename, 'r') as tracefile:
        traceitems = parse(tracefile)

    return Summary(traceitems)
        
def parse_args():
    parser = OptionParser(usage='Usage: %prog <tracefile>')
    options, args = parser.parse_args()

    if len(args) < 1:
        parser.print_help()
        exit(0)
    elif len(args) > 1:
        parser.print_help()
        exit(1)

    return options, args[0]

if __name__=='__main__':
    
    options, filename = parse_args()

    summary = parse_trace_file_and_create_summary(filename)

    summary.print()
