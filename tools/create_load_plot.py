#!/usr/bin/python3

import trace_summary

import numpy as np
import matplotlib.pyplot as plt
import matplotlib

hue = 0.0
color_map = {}
def get_color(name):
    global hue
    d = 0.15

    if name not in color_map:
        hue += d
        color_map[name] = matplotlib.colors.hsv_to_rgb([hue, 1.0, 0.9]);

    return color_map[name]



def latexify(name):
    name = name.lower()
    r = ''
    for c in name.split('_'):
        r += c[0].upper() + c[1:]

    return r'\textsc{%s}' % r
    


def create_plot(method):
    scene_list = ['teapot', 'hair', 'columns', 'zinkia1', 'zinkia2', 'zinkia3']

    summaries = []


    for scene in scene_list:
        summary = trace_summary.parse_trace_file_and_create_summary('traces/%s_%s.trace0' % (scene,method))

        print()
        print(scene.upper())
        summary.print_percentages()
    
        summaries.append(summary)



    names = ['sample', 'shade', 'dice', 'subdivision', 'clear', 'idle']


    dpi = 72 * 1.5
    w,h = 800,600
    bar_width = 0.8

    N = len(scene_list)
    ind = np.arange(N)

    #fig = plt.figure(figsize=(w/dpi,h/dpi))

    
    plt.bar(ind, range(N),bar_width)

    offsets = np.zeros(N)
    for n in names:
        l = []
    
        for s in summaries:
            l.append(s.percentages[n])
    
        plt.bar(ind, l, bar_width, bottom=offsets, label=n, color=get_color(n))
    
        offsets += l
        
    plt.xticks(ind+bar_width/2, [latexify(s) for s in scene_list])
    plt.ylim(0,110)
    plt.yticks([0,20,40,60,80,100])
    plt.xlim(-(1-bar_width)/2,N-(1-bar_width)/2)
    plt.xlabel('scene')
    plt.ylabel('percentage of render time')
    plt.legend(prop={'size':11.5}, loc='upper left',ncol=N)
    
    plt.tight_layout()
    plt.show()

create_plot('local')
create_plot('bounded')
