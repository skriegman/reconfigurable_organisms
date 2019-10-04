import cPickle
import numpy as np
from collections import OrderedDict
from evosoro.softbot import Genotype, Phenotype
from evosoro.tools.utils import make_one_shape_only, get_neighbors

np.random.seed(1)

robustness_filter = [90, 75, 43, 5, 48, 69, 51, 88, 20, 76, 79, 12, 84, 29, 52, 99,
                     # 58, 67, 9, 59, 2, 1, 91, 85, 39, 36, 55, 68, 17, 72, 13, 70,
                     # 3, 33, 46, 32, 42, 16, 83, 10, 87, 93, 21, 35, 94, 95, 80, 25,
                     # 15, 45, 37, 38, 77, 97, 81, 62, 61, 74, 24, 56, 92, 96, 4, 71,
                     # 49, 18, 8, 54, 60, 66, 6, 44, 82, 28, 98, 64, 41, 40, 19, 27,
                     # 14, 50, 22, 89, 65, 34, 100, 47, 7, 63, 23, 53, 57, 86, 78, 26,
                     # 30, 73, 11,
                     31
                     ]

RUNS = 100

EXP_NAME = "XENO_3"
GEN = 1000
PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results"

MyPhenotype = Phenotype
MyGenotype = Genotype


def mostly_passive(mat):
    n_passive = np.sum(mat == 1)
    n_contractile = np.sum(mat == 3)
    return n_passive / float(n_contractile + n_passive) > 0.5


def gaps_at_least_two_voxels_wide(mat):

    neigh = get_neighbors(mat)

    for z in range(7):
        for x in range(8):
            for y in range(8):
                if mat[x, y, z] == 0:
                    if neigh[x, y, z][0] and neigh[x, y, z][1] or neigh[x, y, z][2] and neigh[x, y, z][3] or neigh[x, y, z][4] and neigh[x, y, z][5]:
                        # print neigh
                        return False

    return True


def contiguous_tissue_regions(mat):
    passive_orig = np.array(mat == 1, dtype=int)
    contractile_orig = np.array(mat == 3, dtype=int)
    passive_contiguous = make_one_shape_only(passive_orig)
    contractile_contiguous = make_one_shape_only(contractile_orig)
    return np.all(passive_contiguous == passive_orig) and np.all(contractile_contiguous == contractile_orig)


def single_layer_in_xy_plane(mat):
    bottom_tissue_type = 0
    for z in range(7):
        if np.sum(mat[:, :, z] > 0) == 0:
            continue
        else:
            n_passive = np.sum(mat[:, :, z] == 1)
            n_contractile = np.sum(mat[:, :, z] == 3)
            if n_passive > 0 and n_contractile > 0:
                # print "beep"
                return False
            elif n_passive > 0:
                bottom_tissue_type = 1
            else:
                bottom_tissue_type = 3
            break

    top_tissue_type = 0
    for z in range(6, -1, -1):
        if np.sum(mat[:, :, z] > 0) == 0:
            continue
        else:
            n_passive = np.sum(mat[:, :, z] == 1)
            n_contractile = np.sum(mat[:, :, z] == 3)
            if n_passive > 0 and n_contractile > 0:
                # print "boop"
                return False
            elif n_passive > 0:
                top_tissue_type = 1
            else:
                top_tissue_type = 3
            break

    # print bottom_tissue_type, top_tissue_type
    return True


material_dict = OrderedDict()

# for run in range(1, RUNS+1):
for run in robustness_filter:

    pickle = "{0}/Exp_{1}/Run_{2}/pickledPops/Gen_{3}.pickle".format(PICKLE_DIR, EXP_NAME, run, GEN)
    with open(pickle, 'rb') as handle:
        [optimizer, random_state, numpy_random_state] = cPickle.load(handle)

    # load current population from pickle
    pop = optimizer.pop

    # get the current run champion
    best_ind = None
    best_fit_so_far = 0
    for ind in pop:
        # if ind.fitness == pop.best_fit_so_far:
        #     best_ind = ind
        if ind.fitness > best_fit_so_far:
            best_ind = ind
            best_fit_so_far = ind.fitness

    material_dict[run] = best_ind.genotype.to_phenotype_mapping["material"]["state"]

designs_to_manufacture = []
for robot, morphology in material_dict.items():

    fails = []

    if not contiguous_tissue_regions(morphology) or not single_layer_in_xy_plane(morphology):
        fails += ["B1"]

    if not gaps_at_least_two_voxels_wide(morphology):
        fails += ["B2"]

    if not mostly_passive(morphology):
        fails += ["B3"]

    print "robot {0}: {1} ".format(robot, fails)

#     if contiguous_tissue_regions(morphology):
#
#         if single_layer_in_xy_plane(morphology):
#
#             if mostly_passive(morphology):
#
#                 if gaps_at_least_two_voxels_wide(morphology):
#
#                     designs_to_manufacture += [robot]
#
# print "designs that pass through: ", designs_to_manufacture

