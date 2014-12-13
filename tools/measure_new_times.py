#!/usr/bin/env python3

from benchmark import *

def parse_args():
    parser = OptionParser(usage='Usage: %prog <tablefile>')
    options, args = parser.parse_args()

    if  len(args) > 1:
        parser.print_help()
        exit(1)

    return options, args[0] if len(args)>0 else None

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
    
def latexify_method_name(name, batch_size=None):
    
    method_lut = {'BREADTHFIRST':'\\textsc{Breadth}',
                  'MULTIPASS':'\\textsc{Bounded}',
                  'LOCAL':'\\textsc{Local}'}

    if name == 'BREADTHFIRST' or not batch_size:
        return method_lut[name]
    else:
        return method_lut[name]+('$_{%s}$'%batch_size)
    
if __name__=='__main__':

    options, tablefile = parse_args()
    binary_name = './micropolis'

    benchmark_file = '/tmp/benchmark.trace'
    stat_file = '/tmp/benchmark.statistics'
    
    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=None, repeat=5)
                          
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
                                                     'testscene/zinkia1.mscene',
                                                     'testscene/zinkia2.mscene',
                                                     'testscene/zinkia3.mscene',
                                                     'testscene/eye_split.mscene',
                                                     ])
    benchmark.add_alternative_options('bound_n_split_method', [ 'BREADTHFIRST', 'MULTIPASS'])
    benchmark.add_alternative_options('reyes_patches_per_pass', [10000, 40000, 200000])

    measurements = []
    reduced = []
    processed_count_dict = {}
    def datapoint_handler(config, summaries):

        method = config['bound_n_split_method']
        filename = config['input_file']
        
        if method=='BREADTHFIRST' and filename in processed_count_dict:
            return
        
        durations = sorted([s.duration for s in summaries])
        durations = durations[1:][:-1] 
        duration_ms = np.average(durations)
        
        memusage = np.average([s.statistics['opencl_mem@bound&split'] for s in summaries])

        stats = summaries[0].statistics

        if filename in processed_count_dict:
            processed_count = processed_count_dict[filename]
        else:
            processed_count = stats['processed_patches_per_frame']
            processed_count_dict[filename] = processed_count

        processing_rate = 1000*processed_count/duration_ms

        measurements.append((latexify_method_name(config['bound_n_split_method'],config['reyes_patches_per_pass']),
                             extract_scenename(config['input_file']),
                             duration_ms,
                             memusage/(1024**2),
                             stats['max_patches'],
                             stats['patches_per_frame'],
                             processed_count,
                             processing_rate/1000000))
        
        reduced.append((latexify_filename(config['input_file']),
                        latexify_method_name(config['bound_n_split_method']),
                        config['reyes_patches_per_pass'] if method!='BREADTHFIRST' else stats['max_patches'],
                        duration_ms,
                        memusage/(1024**2),
                        stats['max_patches'],
                        processed_count,
                        processing_rate/1000000))
        
            
    benchmark.datapoint_handler = datapoint_handler

    benchmark.add_option('bound_n_split_method', 'BREADTHFIRST')

    benchmark.perform()

    headers = ['method', 'scene', 'time[ms]',
               'mem usage[MiB]', 'max patches',
               'output patches', 'bound patches', 'bound rate[M#/s]']
    
    print (tabulate(measurements, headers=headers))

    if tablefile:
        reduced_headers = ['scene', 'method', 'batch size', 'time[ms]',
                           'memory[MiB]', 'max patches',
                           'bound patches', 'processing rate[M\\#/s]']

        with open(tablefile, 'w') as outfile:
            outfile.write(tabulate(reduced, headers=reduced_headers, tablefmt='latex', floatfmt='.2f'))
