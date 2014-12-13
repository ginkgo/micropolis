#!/usr/bin/env python3

from optparse import OptionParser
import pickle
from tabulate import tabulate
from collections import defaultdict

import re
import matplotlib.pyplot as plt
import matplotlib

def parse_args():
    parser = OptionParser(usage='Usage: %prog <benchfile> <outpdf>')
    options, args = parser.parse_args()

    if len(args) < 1:
        parser.print_help()
        exit(1)

    return options, args[0], args[1] if len(args) >=2 else None

def find_ranges(measurements):
    ranges = {}
    scenes = []

    for m in measurements:
        s = (m[0], m[1])
        if s not in ranges:
            ranges[s] = []
            scenes.append(s)
            
        ranges[s].append(m)
        
    return [ranges[s] for s in scenes]

def latexify_name(name):
    name = name.lower()
    r = ''
    for c in name.split('_'):
        r += c[0].upper() + c[1:]

    return r'\textsc{%s}' % r

hue = 0.0
color_map = {}
def get_color(name):
    global hue
    d = 0.15

    if name not in color_map:
        hue += d
        color_map[name] = matplotlib.colors.hsv_to_rgb([hue, 1.0, 0.9]);

    return color_map[name]

def plot_bound_rate(ms, figure):
    method = {'LOCAL':'LOCAL','MULTIPASS':'BOUNDED'}[ms[0][0]]
    scene = ms[0][1]
    name = re.compile(r'[\w/]+/(\w+)\.mscene').match(scene).group(1)#+'+_'+method

    X = []
    Y = []
    max_y = None

    bmin, bmax = float('inf'), -float('inf')
    mmin, mmax = float('inf'), -float('inf')    
    
    for method,_,b,t,m,_,_,r in ms:
        if method == 'BREADTHFIRST':
            continue

        X.append(b)
        Y.append(r)

        bmin,bmax = min(bmin,b), max(bmax,b)
        mmin,mmax = min(mmin,b), max(mmax,b)
            
    p = figure.plot(X,Y,'-' if method=='LOCAL' else '--', label=latexify_name(name) if method=='LOCAL' else None, color=get_color(scene))

    return bmin,bmax, mmin,mmax, p,name
            
    

def main():
    
    options, benchfile, outpdf = parse_args()

    with open(benchfile, 'rb') as infile:
        measurements = pickle.load(infile)

    # print (tabulate(measurements, headers=['method', 'scene', 'batch size', 'time[ms]', 'mem usage[MiB]',
    #                                        'max patches', 'bound patches', 'bound rate[M#/s]']))

    # print (find_ranges(measurements))

    dpi = 72 * 1.5
    w = 800
    h = 600

    fig = plt.figure(figsize=(w/dpi,h/dpi))

    figure1 = fig.add_subplot(111)
    ax1 = plt.gca()
    ax1.set_yscale('log')
    ax1.set_xscale('log')
    ax1.set_ylabel('processing rate [Mpatches/s]')
    ax1.set_xlabel('batch size')


    bmin, bmax = float('inf'), -float('inf')
    mmin, mmax = float('inf'), -float('inf')

    H=[]
    L=[]
    
    for ms in find_ranges(measurements):
        bi,ba, mi,ma, h,l = plot_bound_rate(ms, figure1)

        H.append(h)
        L.append(l)
        
        bmin,bmax = min(bmin,bi), max(bmax,ba)
        mmin,mmax = min(mmin,mi), max(mmax,ma)

    ax1.set_xlim(xmin=mmin, xmax=mmax)

    figure1.legend(loc='upper left', prop={'size':10})
    
    plt.tight_layout()

    if outpdf:
        plt.savefig(outpdf)
    else:
        plt.show()

if __name__ == '__main__':
    main()    
