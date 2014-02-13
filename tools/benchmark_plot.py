
import numpy as np
import pickle
from sys import argv

import matplotlib.pyplot as plt

def save_benchmark(filename, T,M,p, max_p):
    with open(filename,'wb') as f:
        pickle.dump((T,M,p,max_p),f)

def load_benchmark(filename):
    with open(filename,'rb') as f:
        return pickle.load(f)

def scatter_and_fit(p,T,max_p):
    i = np.searchsorted(p>=max_p,True)

    coeffs = np.polyfit(p,p*T,1)

    T1 = T[0]*p[0]
    Tinf = np.average(T[i:])

    xT1 = coeffs[1]
    xTinf = coeffs[0]

    print("T1  = %f\t, Tinf = %f" % (float(T1), float(Tinf)))
    print("T1' = %f\t, Tinf'= %f" % (float(xT1), float(xTinf)))
    
    fy = T1/(T1/p+xTinf)
    speedup = T1/T

    plt.plot(p,fy,'k--')
    plt.plot(p[:i],speedup[:i],'b.')
    plt.plot(p[i:],speedup[i:],'r.')

def plot_benchmark(T,M,p, max_p):
    
    plt.figure()
    scatter_and_fit(p,T, max_p)
    plt.xlabel('batch size')
    plt.ylabel('speedup')
    plt.ylim(ymin=0)
    plt.xlim(xmin=0,xmax=p[-1])
    plt.show()


if __name__=='__main__':

    if (len(argv) < 2):
        print('Usage: %s <file>' % argv[0])
    
    filename = argv[1]

    T,M,p,max_p = load_benchmark(filename)

    plot_benchmark(T,M,p, max_p)
