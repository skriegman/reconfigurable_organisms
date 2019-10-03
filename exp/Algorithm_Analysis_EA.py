import random
import os
import sys
import numpy as np
import subprocess as sub

from base import Sim, Env, ObjectiveDict
from networks import DirectEncoding
from softbot import Genotype, Phenotype, Population
from tools.algorithms import ParetoOptimization
from tools.checkpointing import continue_from_checkpoint
from tools.utils import muscle_fat, make_one_shape_only


sub.call("cp ~/tmp/research_code/evosoro/_voxcad/voxelyzeMain/voxelyze .", shell=True)
sub.call("chmod 755 voxelyze", shell=True)

SEED = int(sys.argv[1])
MAX_TIME = float(sys.argv[2])

IND_SIZE = (2, 2, 2)

VOXEL_SIZE = 0.05  # meters

STIFFNESS = 5e6

POP_SIZE = 50
MAX_GENS = 10000000
NUM_RANDOM_INDS = 1

INIT_TIME = 1
SIM_TIME = 10.0 + INIT_TIME  # includes init time
TEMP_AMP = 39.4714242553  # 50% volumetric change with temp_base=25: (1+0.01*(39.4714242553-25))**3-1=0.5

FREQ = 2

DT_FRAC = 0.9  # 0.3

# Swimming Parameters
GRAV_ACC = -0.1
FLUID_ENV = 1  # if 1 drag forces are added
RHO_FLUID = 1000.0  # water density
C_DRAG = 1.5  # fluid drag associated to a triangular facet
AGGREGATE_DRAG_COEF = 0.5 * C_DRAG * RHO_FLUID  # aggregate drag coefficient

TIME_TO_TRY_AGAIN = 10
MAX_EVAL_TIME = 25

SAVE_VXA_EVERY = MAX_GENS + 1
SAVE_LINEAGES = False
CHECKPOINT_EVERY = 1
EXTRA_GENS = 0

RUN_DIR = "run_{}".format(SEED)
RUN_NAME = "Swimmers"


def encoding(material):

    material = np.reshape(material, IND_SIZE)

    # mask = make_one_shape_only(material)
    # new = np.array(mask)
    # new[material == 1] *= 1
    # new[material == 3] *= 3

    if np.sum(material == 3) == 0:
        new = np.zeros(IND_SIZE)
        new[0, 0, 0] = 3

    return material


class MyGenotype(Genotype):
    def __init__(self):
        Genotype.__init__(self, orig_size_xyz=IND_SIZE)

        self.add_network(DirectEncoding(output_node_name="phase_offset", orig_size_xyz=IND_SIZE, symmetric=False),
                         freeze=True)
        self.add_network(DirectEncoding(output_node_name="material", orig_size_xyz=IND_SIZE, func=encoding, scale=1,
                                        symmetric=False, lower_bound=-100, upper_bound=100, vox_options=[0, 1, 3]))

        self.to_phenotype_mapping.add_map(name="phase_offset", tag="<PhaseOffset>", logging_stats=None)
        self.to_phenotype_mapping.add_map(name="material", tag="<Data>", output_type=int, logging_stats=None)


if not os.path.isfile("./" + RUN_DIR + "/pickledPops/Gen_0.pickle"):

    random.seed(SEED)
    np.random.seed(SEED)

    # print np.sum(np.random.rand(*IND_SIZE))  # seed=1; 226.98967645847415
    MyGenotype.NET_DICT = {"phase_offset": np.random.rand(*IND_SIZE)}

    my_sim = Sim(dt_frac=DT_FRAC, simulation_time=SIM_TIME, fitness_eval_init_time=INIT_TIME)

    my_env = Env(temp_amp=TEMP_AMP, fluid_environment=FLUID_ENV, aggregate_drag_coefficient=AGGREGATE_DRAG_COEF,
                 lattice_dimension=VOXEL_SIZE, grav_acc=GRAV_ACC, frequency=FREQ, muscle_stiffness=STIFFNESS)

    my_objective_dict = ObjectiveDict()
    my_objective_dict.add_objective(name="fitness", maximize=True, tag="<normAbsoluteDisplacement>")
    my_objective_dict.add_objective(name="age", maximize=False, tag=None)

    my_pop = Population(my_objective_dict, MyGenotype, Phenotype, pop_size=POP_SIZE)

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




