#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt

import re

def parse_balance(filename, propname):
    pattern = re.compile(r'([a-zA-Z0-9\-@_&]+)\s*=\s*((\d+ )+);')

    with open(filename,'r') as f:
        for line in f.readlines():
            line = line.strip()
            match = pattern.match(line)
            
            if match and match.group(1) == propname:
                return [int(si) for si in match.group(2).strip().split(' ')]
    return []
    
if __name__=='__main__':
    filename = 'reyes.statistics'
    propname = 'bound_n_split_balance'

    balance = parse_balance(filename, propname)

    if len(balance) == 0:
        print ('No worload-balance information available')
        exit()

    avg_load = np.average(balance)
    max_load = np.max(balance)    
    text = 'Possible speedup: %.1f percent' % float(100 - 100 * avg_load/max_load)
    
    plt.bar(range(len(balance)), balance)
    plt.plot([0, len(balance)], [avg_load]*2,'r-')
    plt.text(len(balance)/2,avg_load, text,color='red')
    plt.xlim(0, len(balance)-1)
    plt.show()

    print (text)

    

    
