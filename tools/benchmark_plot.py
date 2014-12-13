
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

def scatter_and_fit_speedup(x,p,T,max_p, figure):

            
    i = np.searchsorted(p>=max_p,True)

    coeffs = np.polyfit(p,p*T,2)

    T1 = T[0]*p[0]
    Tinf = np.average(T[-1])

    xxx,xTinf,xT1 = coeffs

    print("T1  = %f\t, Tinf = %f, speedup = %f" % (float(T1), float(Tinf), float(T1/Tinf)))
    print("T1' = %f\t, Tinf'= %f, speedup'= %f, xxx=%f" % (float(xT1), float(xTinf), float(xT1/xTinf),xxx))
    
    fy = T1/(xT1/p+xTinf+xxx*p)
    speedup = T1/T

    # fy = xT1/p+xTinf+xxx*p
    # speedup = T
    
    figure.plot(x,fy,'k--')
    figure.plot(x,speedup,'b.')

    #figure.set_yscale('log')
    
    figure.set_ylim(ymin=1, ymax=np.max(speedup)*1.1)
    figure.set_xlim(xmin=x[0],xmax=x[-1])

def scatter_and_fit_times(x,p,T,max_p, figure):

            
    i = np.searchsorted(p>=max_p,True)

    coeffs = np.polyfit(p,p*T,2)

    T1 = T[0]*p[0]
    Tinf = np.average(T[-1])

    xxx,xTinf,xT1 = coeffs

    print("T1  = %f\t, Tinf = %f, speedup = %f" % (float(T1), float(Tinf), float(T1/Tinf)))
    print("T1' = %f\t, Tinf'= %f, speedup'= %f, xxx=%f" % (float(xT1), float(xTinf), float(xT1/xTinf),xxx))
    
    fy = xT1/p+xTinf+xxx*p
    
    figure.plot(x,fy,'k--')
    figure.plot(x,T,'b.')
    
    figure.set_ylim(ymin=0, ymax=T[2]*1.1)
    figure.set_xlim(xmin=(x[0]+x[0]) * 0.5,xmax=x[-1])

def plot_benchmark(T,M,p, max_p,I):

    dpi = 72 * 1.5
    w = 800
    h = 600
    
    # if I:
    #     print (I)
    #     plt.plot(p,1/np.array(I))
    #     plt.show()
    
    fig = plt.figure(figsize=(w/dpi,h/dpi))
    figure = fig.add_subplot(111)
    
    scatter_and_fit_speedup(p,p,T, 10000000000000, figure)
    figure.set_xlabel('batch size')
    figure.set_ylabel('speedup')

    ax2=figure.twiny()
    ax2.set_xlim(xmin=M[0],xmax=M[-1])
    ax2.set_xlabel('memory[MB]')
    
    plt.tight_layout()
    plt.show()
    
    fig = plt.figure(figsize=(w/dpi,h/dpi))
    figure = fig.add_subplot(111)
    
    scatter_and_fit_times(p,p,T, 10000000000000, figure)
    figure.set_xlabel('batch size')
    figure.set_ylabel('time[ms]')

    ax2=figure.twiny()
    ax2.set_xlim(xmin=(M[0]+M[0])*0.5,xmax=M[-1])
    ax2.set_xlabel('memory[MB]')
    
    plt.tight_layout()
    plt.show()
    


if __name__=='__main__':

    if (len(argv) < 2):
        print('Usage: %s <file>' % argv[0])
    
    filename = argv[1]

    B = load_benchmark(filename)

    plot_benchmark(*B)
