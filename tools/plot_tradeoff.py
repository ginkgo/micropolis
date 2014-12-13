#!/usr/bin/env python3

from optparse import OptionParser
import pickle
from tabulate import tabulate
from collections import defaultdict

import re
import matplotlib.pyplot as plt

def parse_args():
    parser = OptionParser(usage='Usage: %prog <benchfile> {outpdf}')
    options, args = parser.parse_args()

    if len(args) > 2:
        parser.print_help()
        exit(1)

    return options, args[0], args[1] if len(args) > 1 else None

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
    
def plot_tradeoff(ms, figure):
    name = re.compile(r'[\w/]+/(\w+)\.mscene').match(ms[0][1]).group(1)

    ms = sorted(ms, key=lambda m: m[7]) # sort by render performace
    
    B   = []
    M   = []
    Ma  = []
    P   = []

    bmin, bmax = float('inf'), -float('inf')
    mmin, mmax = float('inf'), -float('inf')
    pmin, pmax = float('inf'), -float('inf')    
    
    for method,_,b,t,m,mp,_,p,ip in ms:
        if method == 'BREADTHFIRST':
            continue

        m_per_patch = m/(b*23+ip)
        
        ma = mp * m_per_patch
        
        B.append(b)
        M.append(m)
        Ma.append(ma)
        P.append(p)
        
        bmin,bmax = min(bmin,b), max(bmax,b)
        mmin,mmax = min(mmin,m), max(mmax,m)
        mmin,mmax = min(mmin,ma), max(mmax,ma)
        pmin,pmax = min(pmin,p), max(pmax,p)
        
    Pr = [100*p/pmax for p in P]
    
    plot1 = figure.plot(Pr,M,'k-', label=latexify_name(name))
    #plot2 = figure.plot(Pr,Ma,'--', label=None, color=plot1[0].get_color())

    prmin = 100*pmin/pmax
    prmax = 100*pmax/pmax
    
    return prmin, prmax, bmin,bmax, mmin,mmax
            
    

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
    
    #ax1.set_yscale('log')
    #ax1.set_xscale('log')
    ax1.set_ylabel('relative memory usage [MiB]')
    ax1.set_xlabel('relative processing rate [\% of maximum]')


    pmin, pmax = float('inf'), -float('inf')
    bmin, bmax = float('inf'), -float('inf')
    mmin, mmax = float('inf'), -float('inf')

    ranges = find_ranges(measurements)
    c = len(ranges)
    
    for ms in ranges:
        pi,pa, bi,ba, mi,ma = plot_tradeoff(ms, figure1)

        pmin,pmax = min(pmin,pi), max(pmax,pa)
        bmin,bmax = min(bmin,bi), max(bmax,ba)
        mmin,mmax = min(mmin,mi), max(mmax,ma)

    ax1.set_xlim(xmin=pmin, xmax=pmax)
    
    #figure1.legend(loc='upper left', prop={'size':10})
    
    plt.tight_layout()

    if outpdf:
        plt.savefig(outpdf)
    else:
        plt.show()

if __name__ == '__main__':
    main()    
