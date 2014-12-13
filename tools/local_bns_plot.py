
import numpy as np
import pickle
from sys import argv

import matplotlib.pyplot as plt

def save_benchmark(filename, T,M,p, I):
    with open(filename,'wb') as f:
        pickle.dump((T,M,p,I),f)

def load_benchmark(filename):
    with open(filename,'rb') as f:
        t = pickle.load(f)

        return t

def scatter_and_fit_speedup(x,p,T, figure):

    T1 = T[0]*p[0]
    Tinf = T[-1]

    speedup = T1/T
    
    figure.plot(x,speedup,'k--')
    figure.plot(x,speedup,'b.')
    
    figure.set_ylim(ymin=1, ymax=np.max(speedup)*1.1)
    figure.set_xlim(xmin=x[0],xmax=x[-1])

def scatter_and_fit_times(x,p,T, figure):

    T1 = T[0]*p[0]
    Tinf = T[-1]
    
    figure.plot(x,T,'k--')
    figure.plot(x,T,'b.')
    
    figure.set_ylim(ymin=1, ymax=np.max(T)*0.125)
    figure.set_xlim(xmin=x[0],xmax=x[-1])


def plot_benchmark(T,M,p, I):

    dpi = 72 * 1.5
    w = 800
    h = 600
    
    # if I:
    #     print (I)
    #     plt.plot(p,1/np.array(I))
    #     plt.show()
    
    fig = plt.figure(figsize=(w/dpi,h/dpi))
    figure = fig.add_subplot(111)
    
    scatter_and_fit_speedup(p,p,T, figure)
    figure.set_xlabel('number of work-groups')
    figure.set_ylabel('speedup')

    plt.tight_layout()
    plt.show()

    
    fig = plt.figure(figsize=(w/dpi,h/dpi))
    figure = fig.add_subplot(111)
    
    scatter_and_fit_times(p,p,T, figure)
    figure.set_xlabel('batch size')
    figure.set_ylabel('time[ms]')
  
    plt.tight_layout()
    plt.show()
    


if __name__=='__main__':

    if (len(argv) < 2):
        print('Usage: %s <file>' % argv[0])
    
    filename = argv[1]

    B = load_benchmark(filename)

    plot_benchmark(*B)
