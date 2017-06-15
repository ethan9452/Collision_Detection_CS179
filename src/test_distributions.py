
import random
import numpy as np
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt


# NORMAL
mu, sigma = 100, 15
# x = mu + sigma*np.random.randn(100)

# UNIFORM
# x = []
# for i in range(10000):
# 	x.append(random.random() * 100)


# UNIFORM SQUARED
x = []
for i in range(1000):
	x.append((random.random() * 100)**2 / 100)



# the histogram of the data
n, bins, patches = plt.hist(x, 50, normed=1, facecolor='green', alpha=0.75)

# add a 'best fit' line
y = mlab.normpdf( bins, mu, sigma)
l = plt.plot(bins, y, 'r--', linewidth=1)

plt.xlabel('Smarts')
plt.ylabel('Probability')
plt.title(r'$\mathrm{Histogram\ of\ IQ:}\ \mu=100,\ \sigma=15$')
plt.axis([-20, 160, 0, 0.1])
plt.grid(True)

plt.show()