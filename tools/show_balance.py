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

    plt.bar(range(len(balance)), balance)
    plt.xlim(0, len(balance)-1)
    plt.show()

    
