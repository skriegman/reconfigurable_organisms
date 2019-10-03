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

cc = ["light red", "bright green", "apricot"]
sns.set_palette(sns.xkcd_palette(cc), desat=.9)

GEN = 1000
EXP_NAME = "XENO_3"

GET_FRESH_PICKLES = False

RESOLUTIONS = [1]  # [1, 2, 4, 6, 8, 10]

LINE_WIDTHS = [0.30]  # [0.30, 0.25, 0.2, 0.15, 0.1, 0.05]

ELEV = -10  # -15  # -5
AZIM = 10  # 40  # -25

RUNS = 1#00
THIS_BOT_ONLY = 52
FORCE_SPIN = 1

N_ROWS = 10
N_COLS = 10

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

for r, lw in zip(RESOLUTIONS, LINE_WIDTHS):

    print "plotting with: res={0}, lw={1}".format(r, lw)

    resolution = (8*r, 8*r, 7*r)

    for BOT in range(1, RUNS+1):

        if RUNS == 1:
            BOT = THIS_BOT_ONLY

        print "plotting ", BOT

        fig = plt.figure()

        pickle = "{3}/Exp_{0}_Run_{1}_Gen_{2}.pickle".format(EXP_NAME, BOT, GEN, PICKLE_DIR)

        with open(pickle, 'rb') as handle:
            [optimizer, random_state, numpy_random_state] = cPickle.load(handle)

        pop = optimizer.pop
        best_ind = None
        best_fit_so_far = 0

        for n, ind in enumerate(pop):

            if ind.fitness > best_fit_so_far:
                best_ind = ind
                best_fit_so_far = ind.fitness

        best_ind.genotype.orig_size_xyz = resolution
        best_ind.genotype.express()

        this_trace = traces_df[traces_df["robot"] == BOT]
        spin = get_orientation(this_trace["TraceX"].values, this_trace["TraceY"].values)

        this_spun_trace = spun_traces_df[traces_df["robot"] == BOT]
        thisX, thisY, thisZ = this_spun_trace["TraceX"], this_spun_trace["TraceY"], this_spun_trace["TraceZ"]

        if FORCE_SPIN is not None:
            spin = FORCE_SPIN

        # print spin

        for name, details in best_ind.genotype.to_phenotype_mapping.items():
            if name == "material":
                shape = details["state"]
                shape = np.rot90(shape, k=spin, axes=(0, 1))

        ax = fig.add_subplot(1, 1, 1, projection='3d')

        # # print 'tracing'
        # time = this_trace["Time"]
        # norm_time = (time-min_time)/(max_time-min_time)
        # # norm_time = [cm.jet(t) for t in norm_time]
        # normX = (thisX-minX)/(maxX-minX)
        # normY = (thisY-minY)/(maxY-minY)
        # normZ = (thisZ-minZ)/(maxZ-minZ)
        #
        # this_maxX = normX.max()
        # this_maxY = normY.max()
        # this_minZ = normZ.min()
        # this_maxZ = normZ.max()
        # print this_minZ, this_maxZ
        #
        # # newXYZ = -1 + normX*10+(1-this_maxX)*10, -1 + normY*10+(1-this_maxY)*10, -1 + 10*(0.5*normZ-normZ[len(normZ)/2])  # quads
        # newXYZ = -1 + normX*10+(1-this_maxX)*10, -1 + normY*10+(1-this_maxY)*10, -1 + 10*(normZ-this_minZ)  # free
        #
        # ax.scatter(newXYZ[0], newXYZ[1], newXYZ[2],
        #            c=norm_time, cmap="jet",
        #            # c="black",
        #            # s=0.1,
        #            # s=0.25,
        #            s=norm_time**2+0.001,
        #            alpha=0.5
        #            )
        #
        # # ax.quiver(newXYZ[0].values[-1], newXYZ[1].values[-1], newXYZ[2].values[-1],
        # #           newXYZ[0].values[-1]-newXYZ[0].values[-2],
        # #           newXYZ[1].values[-1]-newXYZ[1].values[-2],
        # #           newXYZ[2].values[-1]-newXYZ[2].values[-2],
        # #           length=100, arrow_length_ratio=2, pivot="tail", color=cm.jet(0.999999), linewidth=0.5)

        size = best_ind.genotype.orig_size_xyz

        ax.set_xlim([0, size[0]])
        ax.set_ylim([0, size[1]])
        ax.set_zlim([0, size[2]])
        ax.set_aspect('equal')
        ax.view_init(elev=ELEV, azim=AZIM)
        ax.set_axis_off()

        cell_count = 0

        for x in range(size[0]):
            for y in range(size[1]):
                for z in range(size[2]):
                    if shape[x, y, z]:
                        cell_count += 1
                        c = sns.color_palette()[1] if shape[x, y, z] == 1 else sns.color_palette()[0]
                        ax.bar3d(x, y, z, 1, 1, 1, color=c, linewidth=lw, edgecolor='black')

        # save it
        # fig.subplots_adjust(wspace=-0.8, hspace=-0.1)  # 100
        # fig.subplots_adjust(wspace=-0.6, hspace=-0.1)  # 25
        bbox = fig.get_window_extent().transformed(fig.dpi_scale_trans.inverted())
        # dpi = 300*bbox.width/3.125
        # dpi = 600*bbox.height/9.0
        dpi = 900
        # print 'dpi = ', dpi

        print "Scaled by {0}, there are {1} cells".format(r, cell_count)

        if FORCE_SPIN is None:
            plt.savefig("plots/{0}_run_champ_{1}.png".format(EXP_NAME, BOT), bbox_inches='tight', dpi=int(dpi),
                        transparent=True)
        else:
            plt.savefig("plots/Champ{1}ScaledBy{2}_{3}cells_lw{4}.png".format(EXP_NAME, BOT, r, cell_count, int(lw*100)),
                        bbox_inches='tight', dpi=int(dpi), transparent=True)


