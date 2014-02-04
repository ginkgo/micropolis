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

import matplotlib.pyplot as plt

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
        self.datapoint_handler = lambda config,summary: None

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

            for i in range(self.repeat):
                sys.stdout.flush()
                
                summary = self.perform_single_test(combination)

                if summary:
                    print(' %.2fms' % summary.duration , end='')
                    self.create_datapoint(combination, summary)
                else:
                    print(' timeout', end='')
            print()
                

    def perform_single_test(self, aoptions):
        cargs = ['--{0}={1}'.format(option, value) for option,value in self.coptions]
        aargs = ['--{0}={1}'.format(option, value) for option,value in aoptions]

        args = cargs + aargs
        
        try:
            call([self.binary] + args, timeout=self.timeout, stdout=DEVNULL, stderr=DEVNULL)
        except TimeoutExpired:
            return None
        except:
            print()
            exit(1)
            
        return parse_trace_file_and_create_summary(self.trace_file, self.stat_file)

    
    
        
def parse_args():
    parser = OptionParser(usage='Usage: %prog <tracefile> <csvfile>')
    options, args = parser.parse_args()

    if len(args) < 2:
        parser.print_help()
        exit(0)
    elif len(args) > 2:
        parser.print_help()
        exit(1)

    return options, args[0], args[1]

def scatter_and_fit(x,y):
    coefficients = np.polyfit(x,y,4)
    polynomial = np.poly1d(coefficients)

    fy = polynomial(x)

    plt.plot(x,y,'o')
    plt.plot(x,fy)

if __name__=='__main__':

    options, binary_name, outfile = parse_args()

    benchmark_file = '/tmp/benchmark.trace'
    stat_file = '/tmp/benchmark.statistics'
    
    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=200, repeat=1)

    benchmark.add_option('dump_mode', 'true')
    benchmark.add_option('dump_after', 5)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    benchmark.add_option('statistics_file', stat_file)
    
    #benchmark.add_alternative_options('input_file', glob('mscene/*.mscene'))

    # benchmark.add_alternative_options('bound_n_split_method', ['CPU', 'LOCAL', 'MULTIPASS'])
    # benchmark.add_alternative_options('transfer_buffer_mode', ['PINNED', 'UNPINNED'])
    # benchmark.add_alternative_options('do_event_polling', ['true', 'false'])

    benchmark.add_option('bound_n_split_method', 'MULTIPASS')
    benchmark.add_option('window_size', '1280 1024')
    benchmark.add_option('input_file', 'mscene/teapot.mscene')
    benchmark.add_int_range_options('reyes_patches_per_pass', 1, 8192, 50)

    duration_list = []
    memory_list = []
    batch_size_list = []
    
    def datapoint_handler(config,summary):
        duration_list.append(summary.duration)
        memory_list.append(summary.statistics['opencl_mem'])
        batch_size_list.append(config['reyes_patches_per_pass'])
    
    benchmark.datapoint_handler = datapoint_handler
    
    benchmark.perform()

    duration_list = np.array(duration_list)
    memory_list = np.array(memory_list)
    batch_size_list = np.array(batch_size_list)

    speedup = duration_list[0]*batch_size_list[0]/duration_list

    plt.figure()
    scatter_and_fit(memory_list / 1000000.0, speedup)
    plt.xlabel('memory[MB]')
    plt.ylabel('speedup')
    plt.show()

    plt.figure()
    scatter_and_fit(batch_size_list, speedup)
    plt.xlabel('batch size')
    plt.ylabel('speedup')
    plt.show()
    
    with open(outfile, 'w') as f:
        for line in zip(duration_list, memory_list, batch_size_list, speedup):
            f.write(';'.join((str(x) for x in line))+'\n')

    
    
    
