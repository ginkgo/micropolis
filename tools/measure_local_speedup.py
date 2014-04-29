from benchmark import *

import local_bns_plot

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
    
    benchmark.add_option('window_size', '1280 1024')
    benchmark.add_option('bound_n_split_limit', '8')
    benchmark.add_option('input_file', 'mscene/hair.mscene')
    benchmark.add_option('dummy_render', 'true')

    duration_list = []
    memory_list = []
    batch_size_list = []
    max_patches_list = []
    pass_counts = []
    
    def datapoint_handler(config,summaries):
        durations = sorted([s.duration_for_tasks(['bound & split']) for s in summaries])
        durations = durations[3:][:3] #remove outliers
        
        duration_list.append(np.average(durations))
        memory_list.append(config['local_bns_work_groups'] * 24 * 15 * 64)
        batch_size_list.append(config['local_bns_work_groups'])
        max_patches_list.append(summaries[0].statistics['max_patches'])
        pass_counts.append(summaries[0].statistics['pass_count'])
    
    benchmark.datapoint_handler = datapoint_handler

    benchmark.add_option('bound_n_split_method', 'LOCAL')
    benchmark.add_option('reyes_patches_per_pass', 4096*2)

    
    benchmark.add_alternative_options('local_bns_work_groups', list(intspace(1,150,32)))
    
    benchmark.perform()

    T = np.array(duration_list)
    M = np.array(memory_list) / 1000000.0
    p = np.array(batch_size_list)

    local_bns_plot.save_benchmark(outfile, T,M,p, pass_counts)
    local_bns_plot.plot_benchmark(T,M,p,  pass_counts)
    
    
