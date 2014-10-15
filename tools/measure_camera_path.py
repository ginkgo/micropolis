#!/usr/bin/env python3

from benchmark import *

import matplotlib.pyplot as plt

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

    options, memfile = parse_args()
    binary_name = './micropolis'

    benchmark_file = '/tmp/benchmark.trace'
    stat_file = '/tmp/benchmark.statistics'
    
    benchmark = Benchmark(binary_name, benchmark_file, stat_file, timeout=None, repeat=5)
                          
    benchmark.add_option('dump_after', 1)
    benchmark.add_option('verbosity_level', 0)
    benchmark.add_option('trace_file', benchmark_file)
    benchmark.add_option('statistics_file', stat_file)
    
    benchmark.add_option('window_size', '1280 720')
    benchmark.add_option('bound_n_split_limit', 8)
    benchmark.add_option('dummy_render', 'true')
    benchmark.add_option('reyes_patches_per_pass', 20000)
    benchmark.add_option('max_split_depth', 23)

    benchmark.add_option('input_file', 'testscene/zinkia2.mscene')
    benchmark.add_alternative_options('bound_n_split_method', ['BREADTHFIRST'])

    xoff = 0
    yoff = -2.87
    
    benchmark.add_option('camera_x_offset', xoff)
    benchmark.add_option('camera_y_offset', yoff)
    benchmark.add_float_range_options('camera_z_offset', -0, -120, 501)

    measurements = []
    reduced = []
    frameid = 0

    X = []
    Y = []
    T = []
    O = []
    def datapoint_handler(config, summaries):
        global frameid
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
        
        measurements.append((frameid,
                             duration_ms,
                             memusage/(1024**2),
                             stats['max_patches'],
                             stats['patches_per_frame'],
                             processed_count,
                             processing_rate/1000000))
        
        X.append(frameid)
        Y.append(memusage/(1024**2))    
        O.append((xoff,yoff,config['camera_z_offset']))
        T.append(duration_ms)
        
        frameid += 1
        
            
    benchmark.datapoint_handler = datapoint_handler

    benchmark.perform()

    headers = ['frame', 'time[ms]',
               'mem usage[MiB]', 'max patches',
               'output patches', 'bound patches', 'bound rate[M#/s]']
    
    print (tabulate(measurements, headers=headers))

    with open(memfile,'wb') as outfile:
        pickle.dump((X,Y,T,O), outfile)
        
