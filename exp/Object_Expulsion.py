import random
import os
import sys
import numpy as np
import subprocess as sub
from functools import partial

from base import Sim, Env, ObjectiveDict
from networks import CPPN, DirectEncoding
from softbot import Genotype, Phenotype, Population
from tools.algorithms import ParetoOptimization
from tools.checkpointing import continue_from_checkpoint
from tools.utils import make_material_tree, count_neighbors


# sub.call("cp ~/pkg/public_repo/evosoro/_voxcad_land_water/voxelyzeMain/voxelyze .", shell=True)
sub.call("cp ~/tmp/research_code/evosoro/_voxcad/voxelyzeMain/voxelyze .", shell=True)
sub.call("chmod 755 voxelyze", shell=True)

SEED = int(sys.argv[1])
MAX_TIME = float(sys.argv[2])

FITNESS_TAG = "<BlockPos>"

IND_SIZE = (8, 8, 10)
MIN_PERCENT_FULL = 0.25

VOXEL_SIZE = 0.05  # meters
STIFFNESS = 5e6

POP_SIZE = 50
MAX_GENS = 5000
NUM_RANDOM_INDS = 1

INIT_TIME = 1
SIM_TIME = 10.0 + INIT_TIME  # includes init time
TEMP_AMP = 39.4714242553  # 50% volumetric change with temp_base=25: (1+0.01*(39.4714242553-25))**3-1=0.5
FREQ = 2

DT_FRAC = 0.9

# Swimming Parameters
GRAV_ACC = -0.1
FLUID_ENV = 1  # if 1 drag forces are added
RHO_FLUID = 1000.0  # water density
C_DRAG = 1.5  # fluid drag associated to a triangular facet
AGGREGATE_DRAG_COEF = 0.5 * C_DRAG * RHO_FLUID  # aggregate drag coefficient

TIME_TO_TRY_AGAIN = 25
MAX_EVAL_TIME = 61

SAVE_VXA_EVERY = MAX_GENS + 1
SAVE_LINEAGES = False
CHECKPOINT_EVERY = 1
EXTRA_GENS = 0

RUN_DIR = "run_{}".format(SEED)
RUN_NAME = "AquaticCatapults"


def block(x):
    tmp = make_material_tree(x)
    tmp[:, :, 7:] = 0
    tmp[2:6, 2:6, 7:] = 0
    tmp[3:5, 3:5, 8:] = 8
    return tmp


def dont_shape_block(x):
    tmp = np.array(x)
    tmp[:, :, 8:] = 0
    return tmp


def favor_appendages(thrown_object_displacement, ind):
    total_vox = 0
    total_neigh = 0
    for name, details in ind.genotype.to_phenotype_mapping.items():
        if name == "material":
            locations = details['state'] > 0
            total_vox = np.sum(locations)
            neigh = np.reshape(count_neighbors(details['state']), IND_SIZE)
            total_neigh = np.sum(neigh[locations])

    return total_vox/float(total_neigh) * thrown_object_displacement


class MyGenotype(Genotype):
    def __init__(self):
        Genotype.__init__(self, orig_size_xyz=IND_SIZE)

        # self.add_network(DirectEncoding(output_node_name="phase_offset", orig_size_xyz=IND_SIZE, symmetric=False),
        #                  freeze=True)

        self.add_network(CPPN(output_node_names=["phase_offset"]))
        self.to_phenotype_mapping.add_map(name="phase_offset", tag="<PhaseOffset>", logging_stats=None)

        self.add_network(CPPN(output_node_names=["shape", "muscleOrTissue"]))

        self.to_phenotype_mapping.add_map(name="material", tag="<Data>", func=block, output_type=int,
                                          dependency_order=["shape", "muscleOrTissue"], logging_stats=None)

        self.to_phenotype_mapping.add_output_dependency(name="shape", dependency_name=None, requirement=None,
                                                        material_if_true=None, material_if_false="0")

        self.to_phenotype_mapping.add_output_dependency(name="muscleOrTissue", dependency_name="shape",
                                                        requirement=True, material_if_true="3", material_if_false="1")


class MyPhenotype(Phenotype):
    def is_valid(self, min_percent_full=MIN_PERCENT_FULL):

        for name, details in self.genotype.to_phenotype_mapping.items():
            if np.isnan(details["state"]).any():
                return False
            if name == "material":
                state = details["state"]
                num_vox = np.sum(state > 0)
                if num_vox < np.product(self.genotype.orig_size_xyz) * min_percent_full:
                    return False
                if np.sum(state == 3) == 0:  # make sure has at least one muscle voxel for movement
                    return False

        return True


if not os.path.isfile("./" + RUN_DIR + "/pickledPops/Gen_0.pickle"):

    random.seed(SEED)
    np.random.seed(SEED)

    my_sim = Sim(dt_frac=DT_FRAC, simulation_time=SIM_TIME, fitness_eval_init_time=INIT_TIME)

    my_env = Env(temp_amp=TEMP_AMP, fluid_environment=FLUID_ENV, aggregate_drag_coefficient=AGGREGATE_DRAG_COEF,
                 lattice_dimension=VOXEL_SIZE, grav_acc=GRAV_ACC, frequency=FREQ, muscle_stiffness=STIFFNESS)

    my_objective_dict = ObjectiveDict()
    my_objective_dict.add_objective(name="fitness", maximize=True, tag=FITNESS_TAG, meta_func=favor_appendages)
    my_objective_dict.add_objective(name="age", maximize=False, tag=None)

    my_pop = Population(my_objective_dict, MyGenotype, MyPhenotype, pop_size=POP_SIZE)

    my_optimization = ParetoOptimization(my_sim, my_env, my_pop)
    my_optimization.run(max_hours_runtime=MAX_TIME, max_gens=MAX_GENS, num_random_individuals=NUM_RANDOM_INDS,
                        directory=RUN_DIR, name=RUN_NAME, max_eval_time=MAX_EVAL_TIME,
                        time_to_try_again=TIME_TO_TRY_AGAIN, checkpoint_every=CHECKPOINT_EVERY,
                        save_vxa_every=SAVE_VXA_EVERY, save_lineages=SAVE_LINEAGES)

else:
    continue_from_checkpoint(directory=RUN_DIR, additional_gens=EXTRA_GENS, max_hours_runtime=MAX_TIME,
                             max_eval_time=MAX_EVAL_TIME, time_to_try_again=TIME_TO_TRY_AGAIN,
                             checkpoint_every=CHECKPOINT_EVERY, save_vxa_every=SAVE_VXA_EVERY,
                             save_lineages=SAVE_LINEAGES)




