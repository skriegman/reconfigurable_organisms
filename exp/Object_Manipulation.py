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
from tools.utils import make_material_tree, count_occurrences

# sub.call("cp ../_voxcad/voxelyzeMain/voxelyze .", shell=True)
sub.call("cp ~/tmp/research_code/evosoro/_voxcad/voxelyzeMain/voxelyze .", shell=True)
sub.call("chmod 755 voxelyze", shell=True)

SEED = int(sys.argv[1])
MAX_TIME = float(sys.argv[2])
IND_SIZE = (8, 8, 7)
BLOCK_POS = 2
BLOCK_MAT = 8
FITNESS_TAG = "<BlockPos>"  # "<normAbsoluteDisplacement>"
MIN_PERCENT_FULL = 0.5
POP_SIZE = 50
MAX_GENS = 1001
NUM_RANDOM_INDS = 1
INIT_TIME = 1

# diff from main
SIM_TIME = 30.0 + INIT_TIME  # was 10+init  # includes init time
FREQ = 2

TEMP_AMP = 39.4714242553  # 50% volumetric change with temp_base=25: (1+0.01*(39.4714242553-25))**3-1=0.5
DT_FRAC = 0.9  # 0.3
STIFFNESS = 5e6
GRAV_ACC = -0.1
VOXEL_SIZE = 0.05
# DRAW_SHADOW = True  # todo
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
RUN_NAME = "AquaticBlockPushers"


class MyGenotype(Genotype):
    def __init__(self):
        Genotype.__init__(self, orig_size_xyz=IND_SIZE)

        self.add_network(DirectEncoding(output_node_name="phase_offset", orig_size_xyz=IND_SIZE, symmetric=False),
                         freeze=True)

        self.to_phenotype_mapping.add_map(name="phase_offset", tag="<PhaseOffset>", logging_stats=None)

        self.add_network(CPPN(output_node_names=["shape", "muscleOrTissue"]))

        self.to_phenotype_mapping.add_map(name="material", tag="<Data>", func=make_material_tree, output_type=int,
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
                 lattice_dimension=VOXEL_SIZE, grav_acc=GRAV_ACC, frequency=FREQ, muscle_stiffness=STIFFNESS,
                 block_position=BLOCK_POS, block_material=BLOCK_MAT, external_block=True)

    my_objective_dict = ObjectiveDict()
    my_objective_dict.add_objective(name="fitness", maximize=True, tag=FITNESS_TAG,
                                    # meta_func=min_energy
                                    )
    my_objective_dict.add_objective(name="age", maximize=False, tag=None)

    my_objective_dict.add_objective(name="n_muscle", maximize=False, tag=None,
                                    node_func=partial(count_occurrences, keys=[3]),
                                    output_node_name="material")

    # logging only:
    my_objective_dict.add_objective(name="n_vox", maximize=False, tag=None, logging_only=True,
                                    node_func=partial(count_occurrences, keys=[1, 3]),
                                    output_node_name="material")

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




