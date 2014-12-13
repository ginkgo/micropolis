#!/usr/bin/env python3

from benchmark import *

def parse_args():
    parser = OptionParser(usage='Usage: %prog <tablefile>')
    options, args = parser.parse_args()

    if len(args) < 1:
        parser.print_help()
        exit(0)
    elif len(args) > 1:
        parser.print_help()
        exit(1)

    return options, args[0]

file_regex = re.compile(r'[\w/]+/(\w+)\.mscene')
def extract_scenename(filename):
    name = file_regex.match(filename).group(1).lower()
    
    r = ''
    for c in name.split('_'):
        r += c[0].upper() + c[1:]
        
    return r
    
def latexify_filename(filename):
    name = file_regex.match(filename).group(1).lower()
    
    r = ''
    for c in name.split('_'):
        r += c[0].upper() + c[1:]

    return r'\textsc{%s}' % r
    
if __name__=='__main__':

    #options, tablefile = parse_args()
    binary_name = './micropolis'

    benchmark_file = '/tmp/benchmark.trace'
    stat_file = '/tmp/benchmark.statistics'
    
    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=None, repeat=5)
                          
    benchmark.add_option('dump_after', 5)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    benchmark.add_option('statistics_file', stat_file)
    
    benchmark.add_option('window_size', '512 512')
    benchmark.add_option('bound_n_split_limit', 16)
    benchmark.add_option('reyes_patch_size', 16)
    benchmark.add_option('dummy_render', 'true')
    benchmark.add_option('reyes_patches_per_pass', 20000)
    benchmark.add_option('max_split_depth', 15)

    benchmark.add_alternative_options('input_file', ['mscene/teapot.mscene',
                                                     'mscene/killeroo.mscene',
                                                     'mscene/bigguy.mscene',
                                                     ])
    benchmark.add_alternative_options('bound_n_split_method', ['MULTIPASS','BREADTHFIRST'])


    measurements = []
    reduced = []
    def datapoint_handler(config, summaries):
        durations = sorted([s.duration for s in summaries])
        durations = durations[1:][:-1] 
        duration_ms = np.average(durations)
        
        memusage = np.average([s.statistics['opencl_mem@bound&split'] for s in summaries])

        stats = summaries[0].statistics
        
        processed_count = stats['processed_patches_per_frame']
        processing_rate = 1000*processed_count/duration_ms

        method_lut = {'BREADTHFIRST':'\\textsc{Breadth}',
                      'MULTIPASS':'\\textsc{Bounded}',
                      'LOCAL':'\\textsc{Local}'}
        
        measurements.append((config['bound_n_split_method'],
                             extract_scenename(config['input_file']),
                             duration_ms,
                             memusage/(1024**2),
                             stats['max_patches'],
                             stats['patches_per_frame'],
                             processed_count,
                             processing_rate/1000000))
        
        
            
    benchmark.datapoint_handler = datapoint_handler

    benchmark.perform()

    benchmark.clear_options(['dummy_render', 'window_size'])
        
    benchmark.add_option('window_size', '800 800')
    benchmark.add_option('dummy_render', 'false')

    benchmark.perform()

    headers = ['method', 'scene', 'time[ms]',
               'mem usage[MiB]', 'max patches',
               'output patches', 'bound patches', 'bound rate[M#/s]']
    
    print (tabulate(measurements, headers=headers))

    # reduced_headers = ['scene', 'method', 'time[ms]',
    #                    'used memory[MiB]', 'max patches',
    #                    'processed patches', 'processing rate[M\\#/s]']
        
    # with open(tablefile, 'w') as outfile:
    #     outfile.write(tabulate(reduced, headers=reduced_headers, tablefmt='latex', floatfmt='.2f'))
