#!/usr/bin/env python3

from benchmark import *
import pickle

def parse_args():
    parser = OptionParser(usage='Usage: %prog <benchfile>')
    options, args = parser.parse_args()

    if len(args) != 1:
        parser.print_help()
        exit(1)

    return options, args[0]
    
def main():
    
    options, benchfile = parse_args()
    binary_name = './micropolis'

    benchmark_file = '/tmp/benchmark.trace'
    stat_file = '/tmp/benchmark.statistics'

    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=None, repeat=15)

    
    benchmark.add_option('dump_after', 5)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    benchmark.add_option('statistics_file', stat_file)
    
    benchmark.add_option('window_size', '1280 720')
    benchmark.add_option('bound_n_split_limit', 8)
    benchmark.add_option('dummy_render', 'true')
    benchmark.add_option('max_split_depth', 23)

    
    benchmark.add_alternative_options('input_file', ['mscene/teapot.mscene',
                                                     'mscene/hair.mscene',
                                                     'testscene/columns.mscene',
                                                     #'testscene/tree.mscene',
                                                     #'testscene/pillars.mscene',
                                                     'testscene/zinkia.mscene',
                                                     'testscene/depth_complexity.mscene',
                                                     'testscene/eye_split.mscene',
                                                     ])
    
    benchmark.add_alternative_options('bound_n_split_method', [ 'MULTIPASS'])
    benchmark.add_int_log_range_options('reyes_patches_per_pass', 5000, 500000, 100, 2)

    measurements = []
    def datapoint_handler(config, summaries):
        durations = sorted([s.duration for s in summaries])
        durations = durations[3:][:-3] 
        duration_ms = np.average(durations)
        
        memusage = np.average([s.statistics['opencl_mem@bound&split'] for s in summaries])

        stats = summaries[0].statistics
        
        processed_count = stats['processed_patches_per_frame']
        processing_rate = 1000*processed_count/duration_ms
        
        measurements.append((config['bound_n_split_method'],
                             config['input_file'],
                             config['reyes_patches_per_pass'],
                             duration_ms,
                             memusage/(1024**2),
                             stats['max_patches'],
                             processed_count,
                             processing_rate/1000000))
        
            
    benchmark.datapoint_handler = datapoint_handler

    benchmark.perform()

    
    print (tabulate(measurements, headers=['method', 'scene', 'batch size', 'time[ms]', 'mem usage[MiB]', 'max patches', 'bound patches', 'bound rate[M#/s]']))

    with open(benchfile, 'wb') as outfile:
        pickle.dump(measurements, outfile)
        

if __name__ == '__main__':
    main()
