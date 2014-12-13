#!/usr/bin/python3
# -*- coding: utf-8 -*-

from optparse import OptionParser
from parse_trace import *

from collections import defaultdict
from tabulate import tabulate

class Summary:
    def __init__(self, traceitems, statistics={}):
        
        tmin,tmax = find_time_range(traceitems)

        self.duration = milliseconds(tmax - tmin)
        self.percentages = {}
        self.times = {}

        self.statistics = statistics
        
        ti_times  = defaultdict(float)
        occupied = 0

        nmap = {'sample':'sample',
                'dice':'dice',
                'shade':'shade',
                'initialize counter buffers':'subdivision',
                'initialize range buffers':'subdivision',
                'Clear out_range_cnt':'subdivision',
                'read range count':'subdivision',
                'read processed counts':'subdivision',
                'bound & split':'subdivision',
                'bound patches':'subdivision',
                'reduce':'subdivision',
                'split patches':'subdivision',
                'accumulate':'subdivision',
                'init patch ranges':'subdivision',
                'clear framebuffer':'clear',
                'clear depthbuffer':'clear',
                'initialize projection buffer':'subdivision',
                'buffer map':'subdivision'}
                
                
        for ti in traceitems:
            n = nmap[ti.name]
            occupied += ti.duration()
            ti_times[n] += ti.duration_ms()

        ti_times['idle'] = self.duration - milliseconds(occupied)
            
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
        self.print_stats()

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

    def print_stats(self):
        for k,v in self.statistics.items():
            print ('{0} = {1}'.format(k,str(v)))

    def duration_for_tasks(self, task_names):
        return sum((time for name,time in self.times.items() if name in task_names))        

def parse_stats(statfile):
    int_pattern = re.compile(r'([a-zA-Z0-9\-@_&]+)\s*=\s*(\d+);')

    stats = {}
    
    for line in statfile.readlines():
        line = line.strip()
        match = int_pattern.match(line)

        if match:
            stats[match.group(1)] = int(match.group(2))
            
    return stats
        
def parse_trace_file_and_create_summary(tracefile_name, statistics_file_name=''):
    with open(tracefile_name, 'r') as tracefile:
        traceitems = parse(tracefile)

    statistics = {}
    if statistics_file_name:
        with open(statistics_file_name, 'r') as statfile:
            statistics = parse_stats(statfile)

    return Summary(traceitems, statistics)

def parse_args():
    parser = OptionParser(usage='Usage: %prog <tracefile> <statfile>')
    options, args = parser.parse_args()

    if len(args) < 2:
        parser.print_help()
        exit(0)
    elif len(args) > 2:
        parser.print_help()
        exit(1)

    return options, args[0], args[1]

if __name__=='__main__':
    
    options, filename, statfilename = parse_args()

    summary = parse_trace_file_and_create_summary(filename, statfilename)

    summary.print()
