import cPickle
import numpy as np
import os.path
import subprocess as sub
from glob import glob

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
sns.set_style("white")

cc = ["light red", "cyan", "yellow"]
sns.set_palette(sns.xkcd_palette(cc), desat=.9)

IND_SIZE = (10, 10, 10)

N_ROWS = 3
N_COLS = 8

MyGenotype = Genotype
MyPhenotype = Phenotype

X = "1110000111111100111101111111100011111100000111100000011110000011111100011111111011110011111110000111"
E = "0111111110011111111001110000000111000000011111111001111111100111000000011100000001111111100111111110"
N = "1110000111111000111111100111111110111111111111111111111111111111110111111110011111110001111110000111"
O = "0011111100011111111011110011111110000111110000001111000000111110000111111100111101111111100011111100"
B = "1111111000111111110011000011101100001110111111110011111111001100001110110000111011111111001111111000"
T = "0000110000000011000000001100000000110000000011000000001100000000110000000011000011111111111111111111"
P = "1100000000110000000011000000001100000000111111000011111110001100011000110001100011111110001111110000"
U = "0011111100011111111011110011111110000111110000001111000000111100000011110000001111000000111100000011"
S = "0111111110111111111111100001110000000111011111111111111111101110000000111000011111111111110111111110"


XENOBOTS = [X, E, N, O, B, O, T, S]


# def xeno_quad(output_state):
#     shape = quadruped((8, 8, 7), mat=1)
#     mat = np.greater(output_state, 0)*3
#     mat[mat == 0] = 1
#     mat[shape == 0] = 0
#     return mat


def evaluate(sim, env, pop, *args):
    pass


fig = plt.figure()

SHAPES_TO_PRINT = []
for n in range(len(XENOBOTS)):
    TARGET_SHAPE = np.zeros(IND_SIZE, dtype=int)
    for x in range(IND_SIZE[0]):
        for y in range(IND_SIZE[1]):
            for z in range(3):
                TARGET_SHAPE[x, y, z] = XENOBOTS[n][x + y*IND_SIZE[1]]

    shape = TARGET_SHAPE
    shape = np.rot90(shape, k=-1, axes=(1, 0))
    SHAPES_TO_PRINT += [shape]


run_champs = []
for pop_size in [50, 500]:
    for letter in [0, 1, 2, 3, 4, 3, 5, 6]:
        try:
            pickles = "../pbs/pop_{0}_run_{1}/pickledPops/Gen_*.pickle".format(pop_size, letter)
            pickles = glob(pickles)
            sorted_pickles = sorted(pickles, reverse=True)
            pickle = sorted_pickles[0]

            if "Gen_0.pickle" in pickle:
                raise IndexError

            with open(pickle, 'rb') as handle:
                [optimizer, random_state, numpy_random_state] = cPickle.load(handle)

            pop = optimizer.pop

            best_ind = None
            best_fit_so_far = -np.inf

            for n, ind in enumerate(pop):
                if ind.fitness > best_fit_so_far:
                    best_ind = ind
                    best_fit_so_far = ind.fitness

            run_champs += [best_ind]

        except IndexError:
            pass


for n, ind1 in enumerate(run_champs):
    for name, details in ind1.genotype.to_phenotype_mapping.items():
        if name == "material_present":
            shape = details["state"]

    shape = np.rot90(shape, k=-1, axes=(1, 0))
    SHAPES_TO_PRINT += [shape]


for n, shape in enumerate(SHAPES_TO_PRINT):

    print shape.T[0]

    ax = fig.add_subplot(N_ROWS, N_COLS, n+1, projection='3d')
    ax.set_xlim([0, IND_SIZE[0]])
    ax.set_ylim([0, IND_SIZE[1]])
    ax.set_zlim([0, IND_SIZE[2]])

    ax.set_aspect('equal')
    ax.view_init(elev=70, azim=0)
    ax.set_axis_off()
    for x in range(IND_SIZE[0]):
        for y in range(IND_SIZE[1]):
            for z in range(3):
                if shape[x, y, z]:
                    ax.bar3d(x, y, z, 1, 1, 1, color=sns.color_palette()[1], linewidth=0.1, edgecolor='black')

    if n == 0:
        ax.text(5.5, -4, 3, 'Target', 'x', fontsize=6, fontweight='bold', ha='center', va='center')
    if n == 8:
        ax.text(5.5, -4, 3, 'Optimization\n(from paper)', 'x', fontsize=6, ha='center', va='center')
    if n == 16:
        ax.text(5.5, -4, 3, 'Optimization\n(10x pop size)', 'x', fontsize=6, ha='center', va='center')

# save it
fig.subplots_adjust(wspace=-0.3, hspace=-0.6)
dpi = 900
plt.savefig("plots/XENOBOTS.png", bbox_inches='tight', dpi=int(dpi), transparent=True)

