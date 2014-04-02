from benchmark import *

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
    stat_file = '/tmp/benchmark.statistics'
    
    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=None, repeat=20)

    benchmark.add_option('dump_after', 5)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    benchmark.add_option('statistics_file', stat_file)
    
    benchmark.add_option('window_size', '1280 1024')
    benchmark.add_option('bound_n_split_limit', '8')
    benchmark.add_option('dummy_render', 'true')
    benchmark.add_option('reyes_patches_per_pass', 4096)

    benchmark.add_alternative_options('bound_n_split_method', ['BREADTHFIRST', 'MULTIPASS', 'LOCAL', 'BALANCED'])
    benchmark.add_alternative_options('input_file', ['mscene/teapot.mscene', 
						     'mscene/bigguy.mscene', 
						     'mscene/killeroo.mscene', 
						     'mscene/hair.mscene'])


    measurements = []
    def datapoint_handler(config, summaries):
        durations = sorted([s.duration for s in summaries])
        durations = durations[3:][:-3] 
        
        measurements.append((config['bound_n_split_method'],config['input_file'],np.average(durations)))
        
            
    benchmark.datapoint_handler = datapoint_handler

    benchmark.add_option('bound_n_split_method', 'BREADTHFIRST')

    benchmark.perform()

    for m in measurements:
        print ('%s:%s\t%f' % m)
        
    
    
