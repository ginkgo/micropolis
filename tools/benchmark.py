#!/usr/bin/python3
# -*- coding: utf-8 -*-

import sys
import signal
import itertools

from subprocess import call, DEVNULL, TimeoutExpired
import numpy as np
from numpy import linspace
from optparse import OptionParser
from glob import glob

from benchmark_plot import *
from trace_summary import *

def intspace(start, stop, steps):
    return (int(round(f)) for f in linspace(start,stop,steps))

class Benchmark:
    def __init__(self, binary_name, trace_file, stat_file, timeout=10, repeat=1):
        self.binary = binary_name
        self.trace_file = trace_file
        self.stat_file = stat_file
        self.repeat = repeat
        self.timeout = timeout
        self.coptions = []
        self.aoptions = []
        self.measurements = []
        self.datapoint_handler = lambda config,summaries: None

        self.add_option('dump_mode', 'true')
        self.add_option('dump_count', repeat)

    def add_option(self, option, value):
        self.coptions.append((option, value))

    def add_alternative_options(self, option, values):
        self.aoptions.append((option, values))

    def add_float_range_options(self, option, start, end, steps):
        self.aoptions.append((option, list(linspace(start,end,steps))))

    def add_int_range_options(self, option, start, end, steps):
        r = list(intspace(start, end, steps))
        self.aoptions.append((option, r))

        
    def create_datapoint(self, combination, summary):
        d = {}
        for option,value in list(combination):
            d[option] = value
            
        self.datapoint_handler(d, summary)
        
        
    def perform(self):
        combinations = itertools.product(*([(option,value) for value in values] for option,values in self.aoptions))

        for combination in combinations:
            print('(%s):' % (', '.join((str(value) for option,value in combination))), end='')
            sys.stdout.flush()
            
            summaries = self.perform_single_test(combination)

            for summary in summaries:
                print(' %.2fms' % summary.duration , end='')

            self.create_datapoint(combination, summaries)

            print()
                
                

    def perform_single_test(self, aoptions):
        cargs = ['--{0}={1}'.format(option, value) for option,value in self.coptions]
        aargs = ['--{0}={1}'.format(option, value) for option,value in aoptions]

        args = cargs + aargs
        
        try:
            call([self.binary] + args, timeout=self.timeout, stdout=DEVNULL, stderr=DEVNULL)
        except TimeoutExpired:
            return []
        except:
            print ()
            exit(1)

        summaries = []
        for i in range(self.repeat):
            summaries.append(parse_trace_file_and_create_summary(self.trace_file+str(i), self.stat_file+str(1)))

        return summaries

    def clear_options(self, removed_options):

        # print (self.coptions)
        
        for option in self.coptions:
            if option[0] in removed_options:
                print('removing option %s' % option[0])
                self.coptions.remove(option)
        for option in self.aoptions:
            if option[0] in removed_options:
                print('removing option %s' % option[0])
                self.aoptions.remove(option)

        # print (self.coptions)

    
    
        
