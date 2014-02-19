
import numpy as np
import pickle
from sys import argv

import matplotlib.pyplot as plt

def save_benchmark(filename, T,M,p, max_p,I):
    with open(filename,'wb') as f:
        pickle.dump((T,M,p,max_p,I),f)

def load_benchmark(filename):
    with open(filename,'rb') as f:
        t = pickle.load(f)

        if len(t) < 5:
            return tuple(list(t)+[None])
        else:
            return t

def scatter_and_fit(x,p,T,max_p):
    i = np.searchsorted(p>=max_p,True)

    coeffs = np.polyfit(p,p*T,2)

    T1 = T[0]*p[0]
    Tinf = np.average(T[-1])

    xxx,xTinf,xT1 = coeffs

    print("T1  = %f\t, Tinf = %f, speedup = %f" % (float(T1), float(Tinf), float(T1/Tinf)))
    print("T1' = %f\t, Tinf'= %f, speedup'= %f, xxx=%f" % (float(xT1), float(xTinf), float(xT1/xTinf),xxx))
    
    fy = T1/(xT1/p+xTinf+xxx*p)
    speedup = T1/T

    plt.plot(x,fy,'k--')
    plt.plot(x[:i],speedup[:i],'b.')
    plt.plot(x[i:],speedup[i:],'r.')

    plt.ylim(ymin=0, ymax=np.max(speedup)*1.1)
    plt.xlim(xmin=0,xmax=x[-1])

def plot_benchmark(T,M,p, max_p,I):

    dpi = 72 * 1.5
    w = 1280
    h = 720
    
    # if I:
    #     print (I)
    #     plt.plot(p,1/np.array(I))
    #     plt.show()
    
    plt.figure(figsize=(w/dpi,h/dpi))
    scatter_and_fit(p,p,T, 10000000000000)
    plt.xlabel('batch size')
    plt.ylabel('speedup')
    plt.tight_layout()
    plt.show()
    
    plt.figure(figsize=(w/dpi,h/dpi))
    scatter_and_fit(M,p,T, 1000000000000)
    plt.xlabel('memory[MB]')
    plt.ylabel('speedup')
    plt.tight_layout()
    plt.show()


if __name__=='__main__':

    if (len(argv) < 2):
        print('Usage: %s <file>' % argv[0])
    
    filename = argv[1]

    B = load_benchmark(filename)

    plot_benchmark(*B)
