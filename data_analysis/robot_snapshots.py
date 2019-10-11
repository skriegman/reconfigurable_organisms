import cPickle
import numpy as np
import os.path
import subprocess as sub

from softbot import Genotype, Phenotype
from tools.utils import quadruped

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

GEN = 1000  # 500
EXP_NAME = "XENO_3"  #"XENO_Quad_Big"

GET_FRESH_PICKLES = False

PLOT_TRACE = True

RUNS = 100
# NUM_INDS_TO_PLOT_PER_POP = 10

N_ROWS = 10  # 5
N_COLS = 10  # 5

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


if GET_FRESH_PICKLES:
    # for exp_name in EXP_NAMES:
    sub.call("mkdir {0}".format(PICKLE_DIR), shell=True)

    for run in range(1, RUNS+1):

        this_pickle = "{3}/Exp_{0}_Run_{1}_Gen_{2}.pickle".format(EXP_NAME, run, GEN, PICKLE_DIR)

        if not os.path.isfile(this_pickle):
            print "getting pickle {}".format(run)

            sub.call("scp skriegma@bluemoon-user1.uvm.edu:/users/s/k/skriegma/scratch/"
                     "{0}/run_{1}/pickledPops/Gen_{2}.pickle  {3}".format(EXP_NAME, run, GEN, this_pickle),
                     shell=True)


fig = plt.figure()

run_champs = []
# all_inds = []

for run in range(1, RUNS+1):
    try:
        pickle = "{3}/Exp_{0}_Run_{1}_Gen_{2}.pickle".format(EXP_NAME, run, GEN, PICKLE_DIR)

        with open(pickle, 'rb') as handle:
            [optimizer, random_state, numpy_random_state] = cPickle.load(handle)

        pop = optimizer.pop
        # pop.sort("fitness", reverse=True)

        best_ind = None
        best_fit_so_far = 0
        # efficient_ind = None
        # min_energy = np.inf

        for n, ind in enumerate(pop):

            # if n < NUM_INDS_TO_PLOT_PER_POP:
            #     all_inds += [ind]

            # if ind.fitness == pop.best_fit_so_far:
            #     best_ind = ind
            if ind.fitness > best_fit_so_far:
                best_ind = ind
                best_fit_so_far = ind.fitness

            # if ind.n_muscle < min_energy:
            #     min_energy = ind.n_muscle
            #     efficient_ind = ind

        run_champs += [best_ind]
        # all_inds += [best_ind, efficient_ind]
    except IOError:
        print "error reading pickle"
        pass

# for n, ind1 in enumerate(all_inds):
for n, ind1 in enumerate(run_champs):

    size = ind1.genotype.orig_size_xyz

    this_trace = traces_df[traces_df["robot"] == n + 1]
    spin = get_orientation(this_trace["TraceX"].values, this_trace["TraceY"].values)

    this_spun_trace = spun_traces_df[traces_df["robot"] == n + 1]
    thisX, thisY, thisZ = this_spun_trace["TraceX"], this_spun_trace["TraceY"], this_spun_trace["TraceZ"]

    # print spin

    for name, details in ind1.genotype.to_phenotype_mapping.items():
        if name == "material":
            shape = details["state"]
            shape = np.rot90(shape, k=spin, axes=(0, 1))

    print 'Robot ', n+1
    ax = fig.add_subplot(N_ROWS, N_COLS, n+1, projection='3d')

    if PLOT_TRACE:
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

        # newXYZ = -1 + normX*10+(1-this_maxX)*10, -1 + normY*10+(1-this_maxY)*10, -1 + 10*(0.5*normZ-normZ[len(normZ)/2])  # quads
        newXYZ = -1 + normX*10+(1-this_maxX)*10, -1 + normY*10+(1-this_maxY)*10, -1 + 10*(normZ-this_minZ)  # water
        # newXYZ = -8 + normX*20+(1-this_maxX)*10, -4 + normY*10+(1-this_maxY)*10, -2 + 2*(normZ-this_minZ)  # land

        ax.scatter(newXYZ[0], newXYZ[1], newXYZ[2],
                   c=norm_time, cmap="jet",
                   # c="black",
                   # s=0.1,
                   # s=0.25,
                   s=norm_time**2+0.001,
                   alpha=0.5
                   )

        # ax.quiver(newXYZ[0].values[-1], newXYZ[1].values[-1], newXYZ[2].values[-1],
        #           newXYZ[0].values[-1]-newXYZ[0].values[-2],
        #           newXYZ[1].values[-1]-newXYZ[1].values[-2],
        #           newXYZ[2].values[-1]-newXYZ[2].values[-2],
        #           length=100, arrow_length_ratio=2, pivot="tail", color=cm.jet(0.999999), linewidth=0.5)

        ax.set_xlim([0-1, size[0]+1])
        ax.set_ylim([0-1, size[1]+1])
        ax.set_zlim([0-1, size[2]+1])

    else:
        ax.set_xlim([0, size[0]])
        ax.set_ylim([0, size[1]])
        ax.set_zlim([0, size[2]])

    # ax.text(x=7.5, y=0, z=10, s='F={}'.format(round(ind1.fitness, 2)), ha='center', fontsize=8)
    # ax.set_xticks([])
    # ax.set_yticks([])
    ax.set_aspect('equal')
    ax.view_init(elev=-35, azim=-60)
    ax.set_axis_off()
    for x in range(size[0]):
        for y in range(size[1]):
            for z in range(size[2]):
                if shape[x, y, z]:
                    # c = color[x, y, z]  # cm.jet(c)
                    c = sns.color_palette()[1] if shape[x, y, z] == 1 else sns.color_palette()[0]
                    if PLOT_TRACE:
                        ax.bar3d(x-1, y-1, z+1, 1, 1, 1, color=c, linewidth=0.25, edgecolor='black')
                    else:
                        ax.bar3d(x, y, z, 1, 1, 1, color=c, linewidth=0.25, edgecolor='black')

# # Legend
# ax = fig.add_subplot(N_ROWS, N_COLS, RUNS + 1, projection='polar')
# n = 500
# t = np.linspace(0, 2*np.pi, n)
# r = np.linspace(.6, 1, 2)
# rg, tg = np.meshgrid(r, t)
# c = tg
# im = ax.pcolormesh(t, r, c.T, cmap='jet')
# ax.set_yticklabels([])
#
# ax.set_xticks(np.pi/180.*np.linspace(0, 360, 6, endpoint=False))
# ax.set_xticklabels(['$10^4$', '$10^5$', '$10^6$', '$10^7$', '$10^8$', '$10^8$', '$10^9$'], fontsize=6)
#
# thetaticks = np.arange(0, 360, 360/6.)
# ax.set_thetagrids(thetaticks, frac=0.75)
#
# ax.text(0, 0, 'Pa', ha='center', va='center', fontsize=8)
#
# ax.set_xlim([0, np.pi/1.5])
# ax.set_ylim([0, np.pi/1.5])
# ax.spines['polar'].set_visible(False)

# save it
fig.subplots_adjust(wspace=-0.8, hspace=-0.1)  # 100
# fig.subplots_adjust(wspace=-0.6, hspace=-0.1)  # 25
bbox = fig.get_window_extent().transformed(fig.dpi_scale_trans.inverted())
# dpi = 300*bbox.width/3.125
# dpi = 600*bbox.height/9.0
dpi = 300  # 900
print 'dpi = ', dpi
plt.savefig("plots/{}_run_champs_no_trace.png".format(EXP_NAME), bbox_inches='tight', dpi=int(dpi))

