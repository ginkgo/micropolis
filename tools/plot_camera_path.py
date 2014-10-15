
import sys
import pickle
import matplotlib.pyplot as plt
    
with open(sys.argv[1], 'rb') as infile:
    X,Y,T,O = pickle.load(infile)

dpi = 72 * 1.5
w = 800
h = 600

fig = plt.figure(figsize=(w/dpi,h/dpi))

figure1 = fig.add_subplot(111)
ax1 = plt.gca()
ax1.set_xlabel('frame number')
ax1.set_ylabel('memory usage [MiB]')

figure1.plot(X,Y,'-');    

plt.show()
