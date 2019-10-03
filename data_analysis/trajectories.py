import cPickle
import numpy as np
import os.path
import subprocess as sub

from evosoro.softbot import Genotype, Phenotype
from evosoro.tools.utils import quadruped

import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib
import matplotlib.cm as cm
import matplotlib.colors as colors
from mpl_toolkits.mplot3d import Axes3D

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

sns.set(color_codes=True, context="poster")
sns.set_style("white", {'font.family': 'serif', 'font.serif': 'Times New Roman'})

cc = ["light red", "cyan", "apricot"]
sns.set_palette(sns.xkcd_palette(cc), desat=.9)

GEN = 1000
EXP_NAME = "XENO_3"

RUN = 2  # 16, 52, 6, 2
AZIM = -60  # -120, -120, -90,  -60

PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results/{0}_Gen_{1}".format(EXP_NAME, GEN)

MyGenotype = Genotype
MyPhenotype = Phenotype

# Traces
trace_pickle = "/home/sam/Projects/research_code/evosoro/data_analysis/results/{}_Trace_DataFrame.pickle".format(EXP_NAME)
with open(trace_pickle, 'rb') as handle:
    traces_df = cPickle.load(handle)


# spun traces
spun_trace_pickle = "/home/sam/Projects/research_code/evosoro/data_analysis/results/{}_Trace_DataFrame_Spun.pickle".format(EXP_NAME)
with open(spun_trace_pickle, 'rb') as handle:
    spun_traces_df = cPickle.load(handle)

all_time = spun_traces_df["Time"]
min_time = np.min(all_time)
max_time = np.max(all_time)
minX, maxX = spun_traces_df["TraceX"].min(),  spun_traces_df["TraceX"].max()
minY, maxY = spun_traces_df["TraceY"].min(),  spun_traces_df["TraceY"].max()
minZ, maxZ = spun_traces_df["TraceZ"].min(),  spun_traces_df["TraceZ"].max()


def get_orientation(x, y):
    a = np.arctan2(y[-1], x[-1])
    # print a
    if -np.pi < a <= -3*np.pi/4.0:
        # print y[-1], x[-1]
        return 2  # looks good
    if 3*np.pi/4.0 < a <= np.pi:
        # print y[-1], x[-1]
        return 2  # so this is good
    if -3*np.pi/4.0 < a <= -np.pi/4.0:
        # print y[-1], x[-1]
        return 1
    if np.pi/4.0 < a <= 3*np.pi/4.0:
        # print y[-1], x[-1]
        return -1  # correct
    if -np.pi/4.0 < a <= np.pi/4.0:
        # print y[-1], x[-1]
        # print a
        if a < 0:   # -np.pi/8.0:
            # print "adjust +"
            return 1  # adjust for plotting angle
        # if a > np.pi/5.0:
        #     print "adjust -"
        #     return -1
        return 0

    print " whoops"


def xeno_quad(output_state):
    shape = quadruped((8, 8, 7), mat=1)
    mat = np.greater(output_state, 0)*3
    mat[mat == 0] = 1
    mat[shape == 0] = 0
    return mat


fig = plt.figure()

pickle = "{3}/Exp_{0}_Run_{1}_Gen_{2}.pickle".format(EXP_NAME, RUN, GEN, PICKLE_DIR)

with open(pickle, 'rb') as handle:
    [optimizer, random_state, numpy_random_state] = cPickle.load(handle)

pop = optimizer.pop

best_ind = None
best_fit_so_far = 0

for n, ind in enumerate(pop):

    if ind.fitness > best_fit_so_far:
        best_ind = ind
        best_fit_so_far = ind.fitness

size = best_ind.genotype.orig_size_xyz

this_trace = traces_df[traces_df["robot"] == RUN]
spin = get_orientation(this_trace["TraceX"].values, this_trace["TraceY"].values)

this_spun_trace = spun_traces_df[traces_df["robot"] == RUN]
thisX, thisY, thisZ = this_spun_trace["TraceX"], this_spun_trace["TraceY"], this_spun_trace["TraceZ"]

# print spin

for name, details in best_ind.genotype.to_phenotype_mapping.items():
    if name == "material":
        shape = details["state"]
        shape = np.rot90(shape, k=spin, axes=(0, 1))

ax = fig.add_subplot(1, 1, 1, projection='3d')

# print 'tracing behavior'
time = this_trace["Time"]
norm_time = (time-min_time)/(max_time-min_time)
# norm_time = [cm.jet(t) for t in norm_time]
normX = (thisX-minX)/(maxX-minX)
normY = (thisY-minY)/(maxY-minY)
normZ = (thisZ-minZ)/(maxZ-minZ)

this_maxX = normX.max()
this_maxY = normY.max()
this_minZ = normZ.min()
this_maxZ = normZ.max()
print this_minZ, this_maxZ

ax.scatter(normX, normY, normZ*0.25,
           c=norm_time,
           cmap="spring_r",
           s=60,
           alpha=0.5
           )

# ax.quiver(normX.values[-1], normY.values[-1], normZ.values[-1],
#           normX.values[-1]-normX.values[-2],
#           normY.values[-1]-normY.values[-2],
#           normZ.values[-1]-normZ.values[-2],
#           length=25, arrow_length_ratio=2, pivot="tail", color=cm.spring_r(0.999999), linewidth=1)

ax.set_xlim([0, 1])
ax.set_ylim([0, 1])
ax.set_zlim([0, 1])

ax.set_aspect('equal')
ax.view_init(elev=0, azim=AZIM)
ax.set_axis_off()

# save it
# fig.subplots_adjust(wspace=-0.8, hspace=-0.1)  # 100
# fig.subplots_adjust(wspace=-0.6, hspace=-0.1)  # 25
bbox = fig.get_window_extent().transformed(fig.dpi_scale_trans.inverted())
# dpi = 300*bbox.width/3.125
# dpi = 600*bbox.height/9.0
dpi = 300  # 900
print 'dpi = ', dpi
plt.savefig("plots/{0}_trace_{1}.png".format(EXP_NAME, RUN), bbox_inches='tight', dpi=int(dpi), transparent=True)

