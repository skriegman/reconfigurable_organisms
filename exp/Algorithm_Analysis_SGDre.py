import random
import os
import sys
import numpy as np
import subprocess as sub

from base import Sim, Env, ObjectiveDict
from networks import DirectEncoding
from softbot import Genotype, Phenotype, Population
from tools.algorithms import GradientAscent
from tools.checkpointing import continue_from_checkpoint
from tools.utils import muscle_fat, make_one_shape_only
from tools.es import PEPG


sub.call("cp ~/tmp/research_code/evosoro/_voxcad/voxelyzeMain/voxelyze .", shell=True)
sub.call("chmod 755 voxelyze", shell=True)

SEED = int(sys.argv[1])
MAX_TIME = float(sys.argv[2])


class Solver(PEPG):
    def __init__(self, num_params, popsize):
        PEPG.__init__(self, num_params,  # number of model parameters
                      popsize=popsize,  # population size
                      sigma_init=0.10,  # initial standard deviation    # todo: was 0.10
                      sigma_alpha=0.20,  # learning rate for standard deviation
                      sigma_decay=0.999,  # anneal standard deviation  # todo: was 0.999
                      sigma_limit=0.01,  # stop annealing if less than this
                      sigma_max_change=0.2,  # clips adaptive sigma to 20%
                      learning_rate=0.01,  # learning rate for standard deviation
                      learning_rate_decay=0.9999,  # annealing the learning rate
                      learning_rate_limit=0.01,  # stop annealing learning rate
                      elite_ratio=0,  # if > 0, then ignore learning_rate
                      average_baseline=True,  # set baseline to average of batch
                      weight_decay=0.01,  # weight decay coefficient
                      rank_fitness=True,  # use rank rather than fitness numbers
                      forget_best=True)  # don't keep the historical best solution  # todo: was True


SOLVER = Solver

IND_SIZE = (2, 2, 2)
MIN_PERCENT_FULL = 0

VOXEL_SIZE = 0.05  # meters

STIFFNESS = 5e6

POP_SIZE = 50
MAX_GENS = 21  # restart after this
NUM_RESTARTS = 10000000

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
EXTRA_GENS = MAX_GENS  # todo: continue looping with this size chunks

# RUN_DIR = "run_{}".format(SEED)
RUN_NAME = "Swimmers"


def encoding(material):
    material = np.reshape(material, IND_SIZE)
    material = muscle_fat(material, empty_less_than=-0.05, fat_greater_than=0.05)
    mask = make_one_shape_only(material)
    new = np.array(mask)
    new[material == 1] *= 1
    new[material == 3] *= 3
    return new


class MyGenotype(Genotype):
    def __init__(self):
        Genotype.__init__(self, orig_size_xyz=IND_SIZE)

        self.add_network(DirectEncoding(output_node_name="phase_offset", orig_size_xyz=IND_SIZE, symmetric=False),
                         freeze=True)
        self.add_network(DirectEncoding(output_node_name="material", orig_size_xyz=IND_SIZE, symmetric=False,
                                        lower_bound=0, upper_bound=3),
                         freeze=True)

        self.to_phenotype_mapping.add_map(name="phase_offset", tag="<PhaseOffset>", logging_stats=None)
        self.to_phenotype_mapping.add_map(name="material", tag="<Data>", output_type=int, logging_stats=None)


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
                if np.sum(state > 1) == 0:  # make sure has at least one muscle voxel for movement
                    return False

        return True


random.seed(SEED)
np.random.seed(SEED)

# print np.sum(np.random.rand(*IND_SIZE))  # seed=1; 2.210708869600546  # 226.98967645847415
MyGenotype.NET_DICT = {"phase_offset": np.random.rand(*IND_SIZE)}

my_sim = Sim(dt_frac=DT_FRAC, simulation_time=SIM_TIME, fitness_eval_init_time=INIT_TIME)

my_env = Env(temp_amp=TEMP_AMP, fluid_environment=FLUID_ENV, aggregate_drag_coefficient=AGGREGATE_DRAG_COEF,
             lattice_dimension=VOXEL_SIZE, grav_acc=GRAV_ACC, frequency=FREQ, muscle_stiffness=STIFFNESS)

my_objective_dict = ObjectiveDict()
my_objective_dict.add_objective(name="fitness", maximize=True, tag="<normAbsoluteDisplacement>")

for restart in range(NUM_RESTARTS):
    print
    print "-------------------- RESTARTING --------------------"
    print

    RUN_DIR = "run_{0}_{1}".format(SEED, restart)

    my_pop = Population(my_objective_dict, MyGenotype, MyPhenotype, pop_size=POP_SIZE)
    my_optimization = GradientAscent(my_sim, my_env, my_pop, SOLVER)
    my_optimization.encoding = encoding

    # my_optimization.pop.gen += restart*MAX_GENS

    # print np.sum(MyGenotype.NET_DICT["phase_offset"]), np.sum(my_optimization.pop.genotype.NET_DICT["phase_offset"])

    my_optimization.run(max_hours_runtime=MAX_TIME, max_gens=MAX_GENS,  # +restart*MAX_GENS,
                        directory=RUN_DIR, name=RUN_NAME, max_eval_time=MAX_EVAL_TIME,
                        time_to_try_again=TIME_TO_TRY_AGAIN, checkpoint_every=CHECKPOINT_EVERY,
                        save_vxa_every=SAVE_VXA_EVERY, save_lineages=SAVE_LINEAGES)

    # print np.sum(MyGenotype.NET_DICT["phase_offset"]), np.sum(my_optimization.pop.genotype.NET_DICT["phase_offset"])

