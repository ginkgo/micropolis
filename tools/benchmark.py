#!/usr/bin/python3
# -*- coding: utf-8 -*-

import itertools

from subprocess import call, DEVNULL
from numpy import linspace
from optparse import OptionParser
from glob import glob

from trace_summary import *

def intspace(start, stop, steps):
    return (int(round(f)) for f in linspace(start,stop,steps))

class Benchmark:
    def __init__(self, binary_name, trace_file, timeout=10):
        self.binary = binary_name
        self.trace_file = trace_file
        self.timeout = timeout
        self.coptions = []
        self.aoptions = []

    def add_option(self, option, value):
        self.coptions.append((option, value))

    def add_alternative_options(self, option, values):
        self.aoptions.append((option, values))

    def add_float_range_options(self, option, start, end, steps):
        self.aoptions.append((option, list(linspace(start,end,steps))))

    def add_int_range_options(self, option, start, end, steps):
        self.aoptions.append((option, list(intspace(start, end, steps))))

    def perform(self):
        combinations = itertools.product(*([(option,value) for value in values] for option,values in self.aoptions))

        for combination in combinations:
            print('(%s): ' % (', '.join((str(value) for option,value in combination))), end='')
            sys.stdout.flush()
            
            summary = self.perform_single_test(combination)

            if summary:
                print('%.2fms' % summary.duration)
            else:
                print('timed out')
                

    def perform_single_test(self, aoptions):
        cargs = ['--{0}={1}'.format(option, value) for option,value in self.coptions]
        aargs = ['--{0}={1}'.format(option, value) for option,value in aoptions]

        args = cargs + aargs
        
        try:
            call([self.binary] + args, timeout=self.timeout, stdout=DEVNULL, stderr=DEVNULL)
        except:
            return None
            
        return parse_trace_file_and_create_summary(self.trace_file)
        
        
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

    options, binary_name = parse_args()

    benchmark_file = '/tmp/benchmark.trace'
    
    benchmark = Benchmark(binary_name, benchmark_file, timeout=5)

    benchmark.add_option('dump_mode', 'true')
    benchmark.add_option('dump_after', 10)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    #benchmark.add_alternative_options('transfer_buffer_mode', ['PINNED', 'UNPINNED'])
    benchmark.add_alternative_options('bound_n_split_method', ['CPU', 'LOCAL', 'MULTIPASS'])
    benchmark.add_alternative_options('input_file', glob('mscene/*.mscene'))

    benchmark.perform()
    
