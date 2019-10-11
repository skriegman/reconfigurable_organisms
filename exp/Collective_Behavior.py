import cPickle
import numpy as np
import os.path
import subprocess as sub

from softbot import Genotype, Phenotype
from tools.read_write_voxelyze import write_voxelyze_file
from tools.utils import quadruped

NUM_RUNS = 5
RUNS = np.random.randint(1, 100, NUM_RUNS)
RUNS = [6, 52, 2, 16, 15]
print RUNS

GEN = 1000
EXP_NAME = "XENO_3"

DEBRIS_LAYERS = 5

SHAPE = (8, 8, 7)

new_x = SHAPE[0]*NUM_RUNS+NUM_RUNS-1+2*5*DEBRIS_LAYERS
new_y = SHAPE[1]+2*5*DEBRIS_LAYERS

GET_FRESH_PICKLES = False

PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results/{0}_Gen_{1}".format(EXP_NAME, GEN)

MyGenotype = Genotype
MyPhenotype = Phenotype


def xeno_quad(output_state):
    shape = quadruped(SHAPE, mat=1)
    mat = np.greater(output_state, 0)*3
    mat[mat == 0] = 1
    mat[shape == 0] = 0
    return mat


if GET_FRESH_PICKLES:
    # for exp_name in EXP_NAMES:
    sub.call("mkdir {0}".format(PICKLE_DIR), shell=True)

    for run in range(1, NUM_RUNS+1):

        this_pickle = "{3}/Exp_{0}_Run_{1}_Gen_{2}.pickle".format(EXP_NAME, run, GEN, PICKLE_DIR)

        if not os.path.isfile(this_pickle):
            print "getting pickle {}".format(run)

            sub.call("scp skriegma@bluemoon-user1.uvm.edu:/users/s/k/skriegma/scratch/"
                     "{0}/run_{1}/pickledPops/Gen_{2}.pickle  {3}".format(EXP_NAME, run, GEN, this_pickle),
                     shell=True)

run_champs = []

for run in RUNS:
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


for n, champ in enumerate(run_champs):
    champ.genotype.orig_size_xyz = (new_x, new_y, SHAPE[2])

combined = run_champs[0]

for name, details in combined.genotype.to_phenotype_mapping.items():
    for n in range(1, len(run_champs)):
        this_champ = run_champs[n]
        this_phenotype = this_champ.genotype.to_phenotype_mapping[name]['state']
        padded_state = np.pad(this_phenotype, pad_width=((1, 0), (0, 0), (0, 0)), mode='constant', constant_values=0)
        details['state'] = np.concatenate([details['state'], padded_state])
        # print details['state'].shape

    details['state'] = np.pad(details['state'], pad_width=((5*DEBRIS_LAYERS, 5*DEBRIS_LAYERS),
                                                           (5*DEBRIS_LAYERS, 5*DEBRIS_LAYERS),
                                                           (0, 0)),
                              mode='constant', constant_values=0)
    if name == "material":
        # for x in range(0, new_x, 4):
        #     details['state'][x:x+2, :2, :2] = 8
        #     details['state'][x:x+2, -2:, :2] = 8
        # for y in range(0, new_y, 4):
        #     details['state'][:2, y:y+2, :2] = 8
        #     details['state'][-2:, y:y+2, :2] = 8
        for x in range(0, new_x, 4):
            for y in range(0, new_y, 4):
                if np.sum(details['state'][x:x+4, y:y+4, :]) == 0:
                    details['state'][x:x+2, y:y+2, :1] = 8


save_dir = "results/multibot"  # location of voxelyzeFiles dir
sub.call("mkdir {}".format(save_dir), shell=True)
sub.call("mkdir {}/voxelyzeFiles".format(save_dir), shell=True)

my_env = optimizer.env[0]
my_env.block_position = 0
my_env.block_material = 8
my_env.block_density = 1e6
my_env.block_static_friction = 0.5  # 1
my_env.block_dynamic_friction = 0.25  # 0.5

write_voxelyze_file(optimizer.sim, my_env, combined, save_dir, "{0}-{1}".format(EXP_NAME, n))


