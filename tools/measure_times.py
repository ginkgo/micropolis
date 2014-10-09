#!/usr/bin/env python3

from benchmark import *

def parse_args():
    parser = OptionParser(usage='Usage: %prog <csvfile>')
    options, args = parser.parse_args()

    if len(args) < 1:
        parser.print_help()
        exit(0)
    elif len(args) > 1:
        parser.print_help()
        exit(1)

    return options, args[0]


if __name__=='__main__':

    options, csv_name = parse_args()
    binary_name = './micropolis'

    benchmark_file = '/tmp/benchmark.trace'
    stat_file = '/tmp/benchmark.statistics'
    
    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=None, repeat=5)
                          
    benchmark.add_option('dump_after', 5)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    benchmark.add_option('statistics_file', stat_file)
    
    benchmark.add_option('window_size', '1600 1200')
    benchmark.add_option('bound_n_split_limit', 8)
    benchmark.add_option('dummy_render', 'true')
    benchmark.add_option('reyes_patches_per_pass', 20000)
    benchmark.add_option('max_split_depth', 23)

    benchmark.add_alternative_options('input_file', ['mscene/teapot.mscene',
                                                     'testscene/tree.mscene',
                                                     'testscene/depth_complexity.mscene',
                                                     'testscene/eyesplit.mscene',
                                                     'testscene/pillars.mscene',
                                                     'mscene/hair.mscene',
                                                     ])
    benchmark.add_alternative_options('bound_n_split_method', [ 'BREADTHFIRST', 'MULTIPASS', 'LOCAL'])


    measurements = []
    def datapoint_handler(config, summaries):
        durations = sorted([s.duration for s in summaries])
        #durations = durations[3:][:-3] 
        duration_ms = np.average(durations)
        
        memusage = np.average([s.statistics['opencl_mem@bound&split'] for s in summaries])

        stats = summaries[0].statistics
        
        processed_count = stats['processed_patches_per_frame']
        processing_rate = 1000*processed_count/duration_ms
        
        measurements.append((config['bound_n_split_method'],
                             config['input_file'],
                             duration_ms,
                             memusage/(1024**2),
                             stats['max_patches'],
                             stats['patches_per_frame'],
                             processed_count,
                             processing_rate/1000000))
        
            
    benchmark.datapoint_handler = datapoint_handler

    benchmark.add_option('bound_n_split_method', 'BREADTHFIRST')

    benchmark.perform()
    
    print (tabulate(measurements, headers=['method', 'scene', 'time[ms]', 'mem usage[MiB]', 'max patches', 'output patches', 'bound patches', 'bound rate[M#/s]']))
