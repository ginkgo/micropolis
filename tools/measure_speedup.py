from benchmark import *

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
    
    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=None, repeat=20)

    benchmark.add_option('dump_after', 5)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    benchmark.add_option('statistics_file', stat_file)
    
    benchmark.add_option('window_size', '1024 768')
    benchmark.add_option('bound_n_split_limit', '8')
    benchmark.add_option('input_file', 'mscene/teapot.mscene')
    benchmark.add_option('dummy_render', 'true')

    duration_list = []
    memory_list = []
    batch_size_list = []
    max_patches_list = []
    pass_counts = []
    
    def datapoint_handler(config,summaries):
        durations = sorted([s.duration_for_tasks(['bound&split']) for s in summaries])
        durations = durations[3:][:3] #remove outliers
        
        duration_list.append(np.average(durations))
        memory_list.append(sum((s.statistics['opencl_mem@bound&split'] for s in summaries))/len(summaries))
        batch_size_list.append(config['reyes_patches_per_pass'] if 'reyes_patches_per_pass' in config else 2048)
        max_patches_list.append(summaries[0].statistics['max_patches'])
        pass_counts.append(summaries[0].statistics['pass_count'])
    
    benchmark.datapoint_handler = datapoint_handler

    benchmark.add_option('bound_n_split_method', 'BREADTHFIRST')
    benchmark.add_option('reyes_patches_per_pass', 2048)

    benchmark.perform()
    max_parallel_patches = max_patches_list[0]
    print('max %d patches in parallel (%d passes)' % (max_parallel_patches,pass_counts[0]))
    
    duration_list, memory_list, batch_size_list, max_patches_list, pass_counts = [],[],[],[],[]
    benchmark.clear_options(['reyes_patches_per_pass', 'bound_n_split_method'])
    
    benchmark.add_alternative_options('reyes_patches_per_pass',
                                      [1]+list(intspace(64, max_parallel_patches*1.25, 50)))
    benchmark.add_option('bound_n_split_method', 'MULTIPASS')
    
    benchmark.perform()

    T = np.array(duration_list)
    M = np.array(memory_list) / 1000000.0
    p = np.array(batch_size_list)
    max_p = max_parallel_patches

    save_benchmark(outfile, T,M,p, max_p, pass_counts)
    plot_benchmark(T,M,p, max_p, pass_counts)
    
    
