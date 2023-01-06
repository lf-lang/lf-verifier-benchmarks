import matplotlib.pyplot as plt
import mpl_toolkits.mplot3d.art3d as art3d
import numpy as np

fig = plt.figure()
ax = fig.add_subplot(1, 1, 1, projection='3d')

x = (62,37,24,29,118,38,48,39,54,33,12,115,39,70,37,70,59,69,31,38)
y = (35,3,2,18,33,33,55,16,24,19,3,20,8,21,4,13,10,16,3,4)
z = (291.72,6.59,5.37,21.41,1363.16,34.61,248.45,13.56,154.56,13.97,5,162.47,15.9,74.69,6.51,30.27,13.55,17.27,5.66,6.52)

for xi, yi, zi in zip(x, y, z):        
    line=art3d.Line3D(*zip((xi, yi, 0), (xi, yi, zi)), c="#79a739", marker='o', markevery=(1, 1))
    ax.add_line(line)

ax.set_xlim3d(0, 120)
ax.set_ylim3d(0, 60)
ax.set_zlim3d(0, 1100)    

ax.set_xlabel('LF LOC')
ax.set_ylabel('CT')
ax.set_zlabel('Total Time (s)')

plt.savefig("stem.svg")
