import cPickle
from glob import glob
import subprocess as sub
import numpy as np
import pandas as pd

from softbot import Genotype, Phenotype
from base import Env
from tools.read_write_voxelyze import write_voxelyze_file
from tools.utils import quadruped

np.random.seed(1)

EXP_NAME = "XENO_6"
PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results"

SEND_ROBOTS_TO_SIM = False
COLLECT_FITNESS_FILES = False
LOAD_DF_FROM_PICKLE = True

TIME_BETWEEN_TRACES = 0.01

GEN = 1000
start_run = 1
RUNS = 100

IND_SIZE = (8, 8, 7)
STIFFNESS = 5e6
INIT_TIME = 1
SIM_TIME = 10.0 + INIT_TIME  # includes init time
TEMP_AMP = 39.4714242553  # 50% volumetric change with temp_base=25: (1+0.01*(39.4714242553-25))**3-1=0.5
FREQ = 2
DT_FRAC = 0.9
VOXEL_SIZE = 0.05  # meters
GRAV_ACC = -0.1
DRAW_SHADOW = True  # todo
FLUID_ENV = 1  # if 1 drag forces are added
RHO_FLUID = 1000.0  # water density
C_DRAG = 1.5  # fluid drag associated to a triangular facet
AGGREGATE_DRAG_COEF = 0.5 * C_DRAG * RHO_FLUID  # aggregate drag coefficient


def xeno_quad(output_state):
    shape = quadruped((8, 8, 7), mat=1)
    mat = np.greater(output_state, 0)*3
    mat[mat == 0] = 1
    mat[shape == 0] = 0
    return mat


if SEND_ROBOTS_TO_SIM:

    sub.call("mkdir {0}/GroundPenetration_{1}".format(PICKLE_DIR, EXP_NAME), shell=True)

    for run in range(start_run, RUNS + 1):

        sub.call("mkdir {0}/GroundPenetration_{1}/Run_{2}".format(PICKLE_DIR, EXP_NAME, run), shell=True)

        sub.call("mkdir {0}/GroundPenetration_{1}/Run_{2}/voxelyzeFiles && "
                 "mkdir {0}/GroundPenetration_{1}/Run_{2}/fitnessFiles".format(PICKLE_DIR, EXP_NAME, run),
                 shell=True)

    MyPhenotype = Phenotype
    MyGenotype = Genotype

    orig_fit_dict = {}

    # for exp_name in EXP_NAMES:
    for run in range(start_run, RUNS+1):

        # clear directories
        sub.call("rm {0}/GroundPenetration_{1}/Run_{2}/voxelyzeFiles/*".format(PICKLE_DIR, EXP_NAME, run), shell=True)
        sub.call("rm {0}/GroundPenetration_{1}/Run_{2}/fitnessFiles/*".format(PICKLE_DIR, EXP_NAME, run), shell=True)

        pickle = "{0}/Exp_{1}/Run_{2}/pickledPops/Gen_{3}.pickle".format(PICKLE_DIR, EXP_NAME, run, GEN)
        with open(pickle, 'rb') as handle:
            [optimizer, random_state, numpy_random_state] = cPickle.load(handle)

        optimizer.sim.simulation_time = SIM_TIME

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

        orig_fit_dict[run] = best_fit_so_far

        my_env = Env(temp_amp=TEMP_AMP, fluid_environment=FLUID_ENV, aggregate_drag_coefficient=AGGREGATE_DRAG_COEF,
                     lattice_dimension=VOXEL_SIZE, grav_acc=GRAV_ACC, frequency=FREQ, muscle_stiffness=STIFFNESS,
                     time_between_traces=TIME_BETWEEN_TRACES)

        # this time save traces (if TIME_BETWEEN_TRACES > 0)
        my_env.time_between_traces = TIME_BETWEEN_TRACES

        save_dir = "{0}/GroundPenetration_{1}/Run_{2}/".format(PICKLE_DIR, EXP_NAME, run)  # voxelyzeFiles

        write_voxelyze_file(optimizer.sim, my_env, best_ind, save_dir, "GroundPenetration")

    # evaluate all robots in simulator #
    for run in range(start_run, RUNS + 1):
        robots = "{0}/GroundPenetration_{1}/Run_{2}/voxelyzeFiles/*".format(PICKLE_DIR, EXP_NAME, run)

        for vxa in glob(robots):
            print "sending robot {} to the simulator".format(run)
            sub.Popen("/home/sam/Projects/research_code/evosoro/_voxcad/voxelyzeMain/voxelyze -f " + vxa, shell=True)


if COLLECT_FITNESS_FILES:

    ground_touch_dict = {
        run: {"Time": [], "TraceX": [], "TraceY": [], "TraceZ": [], "NumTouchingGround": []}
        for run in range(1, RUNS + 1)
    }

    all_tag_keys = [("<" + k + ">", k) for k, v in ground_touch_dict[1].items()]

    for run in range(start_run, RUNS+1):

        # print "collecting data for robot", run
        robots = glob("{0}/GroundPenetration_{1}/Run_{2}/fitnessFiles/*".format(PICKLE_DIR, EXP_NAME, run))

        for robot in robots:
            name = int(robot[robot.find("id_")+3:-4])
            this_robot = open(robot)
            for line in this_robot:
                for tag, key in all_tag_keys:
                    if tag in line:
                        ground_touch_dict[run][key] += [float(line[line.find(tag) + len(tag):line.find("</" + tag[1:])])]

    with open('{0}/{1}_GroundPenetration_Dict.pickle'.format(PICKLE_DIR, EXP_NAME), 'wb') as handle:
        cPickle.dump(ground_touch_dict, handle, protocol=cPickle.HIGHEST_PROTOCOL)

    df = pd.DataFrame()
    robot = []
    for run in range(start_run, RUNS+1):
        this_df = pd.DataFrame.from_dict(ground_touch_dict[run])
        robot += [run]*len(this_df)
        df = pd.concat([df, this_df])

    df['robot'] = robot

    df.to_pickle('{0}/{1}_GroundPenetration_DataFrame.pickle'.format(PICKLE_DIR, EXP_NAME))


if LOAD_DF_FROM_PICKLE:

    with open('{0}/{1}_GroundPenetration_DataFrame.pickle'.format(PICKLE_DIR, EXP_NAME), 'rb') as handle:
        df = cPickle.load(handle)

    # print df

    penetration = df["NumTouchingGround"]
    X, Y, Z = df["TraceX"], df["TraceY"], df["TraceZ"]
    time = df["Time"]
    t = (np.array(time) - np.min(time)) / (np.max(time) - np.min(time))

    # df = df[t > 0.25]

    c = []
    for n in range(1, 100):
        c += [np.sum(df.loc[df.robot==n, "NumTouchingGround"]>0) / float(len(df.loc[df.robot==1, "NumTouchingGround"]))]
    print np.mean(c)


