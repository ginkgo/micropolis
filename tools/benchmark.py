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
        for option in self.coptions:
            if option[0] in removed_options:
                self.coptions.remove(option)
        for option in self.aoptions:
            if option[0] in removed_options:
                self.aoptions.remove(option)

    
    
        
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


if __name__=='__main__':

    options, binary_name, outfile = parse_args()

    benchmark_file = '/tmp/benchmark.trace'
    stat_file = '/tmp/benchmark.statistics'
    
    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=200, repeat=20)

    benchmark.add_option('dump_after', 5)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    benchmark.add_option('statistics_file', stat_file)
    
    benchmark.add_option('window_size', '1024 768')
    benchmark.add_option('bound_n_split_limit', '16')
    benchmark.add_option('input_file', 'mscene/teapot.mscene')
    benchmark.add_option('dummy_render', 'true')

    duration_list = []
    memory_list = []
    batch_size_list = []
    max_patches_list = []
    
    def datapoint_handler(config,summaries):
        duration_list.append(sum((s.duration_for_tasks(['bound&split']) for s in summaries))/len(summaries))
        memory_list.append(sum((s.statistics['opencl_mem@bound&split'] for s in summaries))/len(summaries))
        batch_size_list.append(config['reyes_patches_per_pass'] if 'reyes_patches_per_pass' in config else 2048)
        max_patches_list.append(summaries[0].statistics['max_patches'])
    
    benchmark.datapoint_handler = datapoint_handler


    benchmark.add_option('bound_n_split_method', 'BREADTHFIRST')
    benchmark.add_option('reyes_patches_per_pass', 2048)

    benchmark.perform()
    max_parallel_patches = max_patches_list[0]
    print('max %d patches in parallel' % max_parallel_patches)
    
    duration_list, memory_list, batch_size_list, max_patches_list = [],[],[],[]
    benchmark.clear_options(['reyes_patches_per_pass', 'bound_n_split_limit'])
    
    benchmark.add_int_range_options('reyes_patches_per_pass', 1, 400, 100)
    benchmark.add_option('bound_n_split_method', 'MULTIPASS')
    
    benchmark.perform()

    T = np.array(duration_list)
    M = np.array(memory_list) / 1000000.0
    p = np.array(batch_size_list)
    max_p = max_parallel_patches

    save_benchmark(outfile, T,M,p, max_p)
    plot_benchmark(T,M,p, max_p)
    
    
