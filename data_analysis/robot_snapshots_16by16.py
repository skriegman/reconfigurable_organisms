import cPickle
import numpy as np
import os.path
import subprocess as sub

from softbot import Genotype, Phenotype
from tools.utils import quadruped, make_material_tree

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

cc = ["light red", "cyan", "yellow"]
sns.set_palette(sns.xkcd_palette(cc), desat=.9)

GEN = 1000
EXP_NAME = "XENO_3_d322_AFPO"  # "XENO_3_block_push"

GET_FRESH_PICKLES = True

RUNS = 5

N_ROWS = 1
N_COLS = 5

PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results/{0}_Gen_{1}".format(EXP_NAME, GEN)

MyGenotype = Genotype
MyPhenotype = Phenotype


# def embedded_pill(this_softbot, *args, **kwargs):
#     mat = make_material_tree(this_softbot, *args, **kwargs)
#     mat[3:5, 3:5, :] = 8
#     return mat
#
#
# def xeno_quad(output_state):
#     shape = quadruped((8, 8, 7), mat=1)
#     mat = np.greater(output_state, 0)*3
#     mat[mat == 0] = 1
#     mat[shape == 0] = 0
#     return mat


def encoding(material):
    material = np.reshape(material, (2, 2, 2))
    if np.sum(material == 3) == 0:
        new = np.zeros((3, 3, 2))
        new[0, 0, 0] = 3
    return material


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

for run in range(1, RUNS+1):
    try:
        pickle = "{3}/Exp_{0}_Run_{1}_Gen_{2}.pickle".format(EXP_NAME, run, GEN, PICKLE_DIR)

        with open(pickle, 'rb') as handle:
            [optimizer, random_state, numpy_random_state] = cPickle.load(handle)

        pop = optimizer.pop

        best_ind = None
        best_fit_so_far = 0

        for n, ind in enumerate(pop):

            if ind.fitness > best_fit_so_far:
                best_ind = ind
                best_fit_so_far = ind.fitness

        run_champs += [best_ind]

    except IOError:
        print "error reading pickle"
        pass

# for n, ind1 in enumerate(all_inds):
for n, ind1 in enumerate(run_champs):

    size = ind1.genotype.orig_size_xyz

    for name, details in ind1.genotype.to_phenotype_mapping.items():
        if name == "material":
            shape = details["state"]

    print 'Robot ', n+1
    ax = fig.add_subplot(N_ROWS, N_COLS, n+1, projection='3d')
    # ax.set_xlim([0, size[0]+3])
    # ax.set_ylim([0, size[1]+3])
    # ax.set_zlim([0, size[2]+3])
    ax.set_xlim([0, size[0]])
    ax.set_ylim([0, size[0]])
    ax.set_zlim([0, size[0]])

    ax.set_aspect('equal')
    ax.view_init(elev=-10, azim=-60)
    ax.set_axis_off()
    for x in range(size[0]):
        for y in range(size[1]):
            for z in range(size[2]):
                if shape[x, y, z]:
                    c = sns.color_palette()[1] if shape[x, y, z] == 1 else sns.color_palette()[0]
                    if shape[x, y, z] == 8 or shape[x, y, z] == 6:
                        c = sns.color_palette()[2]
                    ax.bar3d(x, y, z, 1, 1, 1, color=c, linewidth=0.25, edgecolor='black')

    # for x in [9, 10]:
    #     for y in [9, 10]:
    #         for z in [0, 1]:
    #             ax.bar3d(x, y, z, 1, 1, 1, color=sns.color_palette()[2], linewidth=0.25, edgecolor='black')

# save it
fig.subplots_adjust(wspace=-0.2, hspace=0)
bbox = fig.get_window_extent().transformed(fig.dpi_scale_trans.inverted())
# dpi = 300*bbox.width/3.125
# dpi = 600*bbox.height/9.0
dpi = 600  # 900
print 'dpi = ', dpi
plt.savefig("plots/{}_run_champs.png".format(EXP_NAME), bbox_inches='tight', dpi=int(dpi), transparent=True)

