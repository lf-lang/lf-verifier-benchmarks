import re, seaborn as sns
import numpy as np

from matplotlib import pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.colors import ListedColormap

# generate data
x = [62,37,24,29,118,38,48,39,54,33,12,115,39,70,37,70,59,69,31,38]
y = [35,3,2,18,33,33,55,16,24,19,3,20,8,21,4,13,10,16,3,4]
z = [291.72,6.59,5.37,21.41,1363.16,34.61,248.45,13.56,154.56,13.97,5,162.47,15.9,74.69,6.51,30.27,13.55,17.27,5.66,6.52]

# axes instance
fig = plt.figure(figsize=(6,6))
ax = Axes3D(fig)
fig.add_axes(ax)

# get colormap from seaborn
cmap = ListedColormap(sns.color_palette("husl", 256).as_hex())

# plot
sc = ax.scatter(x, y, z, s=40, c=x, marker='o', cmap=cmap, alpha=1)
ax.vlines(, 0, z)

ax.set_xlabel('LF LOC')
ax.set_ylabel('CT')
ax.set_zlabel('Total Time')

# legend
plt.legend(*sc.legend_elements(), bbox_to_anchor=(1.05, 1), loc=2)

# save
plt.savefig("scatter_hue", bbox_inches='tight')
