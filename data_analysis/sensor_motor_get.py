import cPickle
from glob import glob
import subprocess as sub
import numpy as np
import pandas as pd
import time

from softbot import Genotype, Phenotype
from base import Env
from tools.read_write_voxelyze import write_voxelyze_file
from tools.utils import convert_voxelyze_index, quadruped

GEN = 1000
RUNS = 100
EXP_NAME = "XENO_3"


def xeno_quad(output_state):
    shape = quadruped((8, 8, 7), mat=1)
    mat = np.greater(output_state, 0)*3
    mat[mat == 0] = 1
    mat[shape == 0] = 0
    return mat


GET_FRESH_PICKLES = False
SEND_ROBOTS_TO_SIM = False
COLLECT_FITNESS_FILES = True
ROTATE = True  # can't flip this on until after non-rotated results are gathered, then need to repeat the loop
DEBUG = False

PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results"

TIME_BETWEEN_TRACES = 0.01

BATCH_SIZE = 10
SEC_BETWEEN_BATCHES = 35

IND_SIZE = (8, 8, 7)
STIFFNESS = 5e6
INIT_TIME = 1
SIM_TIME = 60.0 + INIT_TIME  # includes init time
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


# XENO 6
if EXP_NAME == "XENO_6":
    STIFFNESS = 1e7
    INIT_TIME = 0.4
    SIM_TIME = 3 + INIT_TIME  # includes init time
    FREQ = 5
    DT_FRAC = 0.9
    VOXEL_SIZE = 0.01  # meters
    GRAV_ACC = -9.81
    FLUID_ENV = 0


if ROTATE:
    # Original Traces
    trace_pickle = "/home/sam/Projects/research_code/evosoro/data_analysis/results/{}_Trace_DataFrameXXX.pickle".format(EXP_NAME)
    with open(trace_pickle, 'rb') as handle:
        traces_df = cPickle.load(handle)


    def get_orientation(x, y):
        a = np.arctan2(y[-1], x[-1])
        if -np.pi < a <= -3*np.pi/4.0:
            return 2
        if 3*np.pi/4.0 < a <= np.pi:
            return 2
        if -3*np.pi/4.0 < a <= -np.pi/4.0:
            return 1
        if np.pi/4.0 < a <= 3*np.pi/4.0:
            return -1
        if -np.pi/4.0 < a <= np.pi/4.0:
            if a < 0:
                return 1  # adjust for plotting angle
            return 0


    def spin(a, run):
        this_trace = traces_df[traces_df["robot"] == run]
        k = get_orientation(this_trace["TraceX"].values, this_trace["TraceY"].values)
        return np.rot90(a, k=k, axes=(0, 1))


if GET_FRESH_PICKLES:
    # for exp_name in EXP_NAMES:
    sub.call("mkdir {0}/Exp_{1}".format(PICKLE_DIR, EXP_NAME), shell=True)

    for run in range(1, RUNS+1):

        print run

        sub.call("mkdir {0}/Exp_{1}/Run_{2}".format(PICKLE_DIR, EXP_NAME, run), shell=True)

        sub.call("mkdir {0}/Exp_{1}/Run_{2}/voxelyzeFiles && mkdir {0}/Exp_{1}/Run_{2}/fitnessFiles && "
                 "mkdir {0}/Exp_{1}/Run_{2}/pickledPops".format(PICKLE_DIR, EXP_NAME, run), shell=True)

        sub.call("scp skriegma@bluemoon-user1.uvm.edu:/users/s/k/skriegma/scratch/"
                 "{0}/run_{1}/pickledPops/Gen_{2}.pickle "
                 " {3}/Exp_{0}/Run_{1}/pickledPops/Gen_{2}.pickle".format(EXP_NAME, run, GEN, PICKLE_DIR),
                 shell=True)


if SEND_ROBOTS_TO_SIM:

    MyPhenotype = Phenotype
    MyGenotype = Genotype

    # for exp_name in EXP_NAMES:
    for run in [2, 6, 16, 52]:  # range(1, RUNS+1):

        # clear directories
        sub.call("rm {0}/Exp_{1}/Run_{2}/voxelyzeFiles/*".format(PICKLE_DIR, EXP_NAME, run), shell=True)
        sub.call("rm {0}/Exp_{1}/Run_{2}/fitnessFiles/*".format(PICKLE_DIR, EXP_NAME, run), shell=True)

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

        if ROTATE:
            # orient to the right
            for name, details in best_ind.genotype.to_phenotype_mapping.items():
                details["state"] = spin(details["state"], run)

        # output_node_names = ["fm_synapse_weights_{0:02d}".format(k) for k in range(NUM_FORWARD_MODEL_SYNAPSES)]
        # best_ind.genotype.add_network(CPPN(output_node_names=output_node_names))
        # for name in output_node_names:
        #     best_ind.genotype.to_phenotype_mapping.add_map(name=name, tag="<ForwardModelSynapseWeights>")
        #
        # best_ind.genotype.express()  # is this necessary?

        my_env = Env(temp_amp=TEMP_AMP, fluid_environment=FLUID_ENV, aggregate_drag_coefficient=AGGREGATE_DRAG_COEF,
                     lattice_dimension=VOXEL_SIZE, grav_acc=GRAV_ACC, frequency=FREQ, muscle_stiffness=STIFFNESS,
                     time_between_traces=TIME_BETWEEN_TRACES)

        # this time save traces
        my_env.time_between_traces = TIME_BETWEEN_TRACES

        save_dir = "{0}/Exp_{1}/Run_{2}".format(PICKLE_DIR, EXP_NAME, run)  # location of voxelyzeFiles dir
        write_voxelyze_file(optimizer.sim, my_env, best_ind, save_dir, "{0}-{1}".format(EXP_NAME, run))

    #
    # evaluate all robots in simulator
    count = 1
    # for exp_name in EXP_NAMES:
    for run in [2, 6, 16, 52]:  # range(1, RUNS+1):
        robots = "{0}/Exp_{1}/Run_{2}/voxelyzeFiles/*".format(PICKLE_DIR, EXP_NAME, run)
        for vxa in glob(robots):
            print "sending robot {} to the simulator".format(count)
            sub.Popen("/home/sam/Projects/research_code/evosoro/_voxcad/voxelyzeMain/voxelyze -f " + vxa, shell=True)

            if count % BATCH_SIZE == 0:
                print "GOING TO SLEEP..."
                time.sleep(SEC_BETWEEN_BATCHES)
                print "WAKING UP!"

            count += 1


if COLLECT_FITNESS_FILES:

    robot_trace_dict = {run: {
                              # "X": [], "Y": [], "Z": [],
                              "Time": [], "TraceX": [], "TraceY": [], "TraceZ": [],
                              # "Voltage": [], "Strain": [], "Stress": [], "Pressure": [], "Touch": [],
                              # "Roll": [], "Pitch": [], "Yaw": [],
                              # "Motor1": [], "Motor2": [], "Motor3": [], "Motor4": [], "Motor5": [], "Motor6": []
                              }
                        for run in range(1, RUNS+1)
                        }

    all_tag_keys = [("<"+k+">", k) for k, v in robot_trace_dict[1].items()]
    # for exp_name in EXP_NAMES:
    for run in [2, 6, 16, 52]:  # range(1, RUNS+1):
        print "collecting data for robot", run
        robots = glob("{0}/Exp_{1}/Run_{2}/fitnessFiles/*".format(PICKLE_DIR, EXP_NAME, run))
        robot = robots[0]
        name = int(robot[robot.find("id_")+3:-4])
        this_robot = open(robot)
        for line in this_robot:
            for tag, key in all_tag_keys:
                if tag in line:
                    robot_trace_dict[run][key] += [float(line[line.find(tag) + len(tag):line.find("</" + tag[1:])])]

    if ROTATE:
        with open('{0}/{1}_Trace_Dict_SpunXXX.pickle'.format(PICKLE_DIR, EXP_NAME), 'wb') as handle:
            cPickle.dump(robot_trace_dict, handle, protocol=cPickle.HIGHEST_PROTOCOL)
    else:
        with open('{0}/{1}_Trace_DictXXX.pickle'.format(PICKLE_DIR, EXP_NAME), 'wb') as handle:
            cPickle.dump(robot_trace_dict, handle, protocol=cPickle.HIGHEST_PROTOCOL)

    df = pd.DataFrame()
    robot = []
    for run in [2, 6, 16, 52]:  # range(1, RUNS+1):
        this_df = pd.DataFrame.from_dict(robot_trace_dict[run])

        # this_robot_trace_matrix = this_df.values
        # with open('{0}/Robot_Trace_Matrix_Run_{1}.pickle'.format(PICKLE_DIR, run), 'wb') as handle:
        #     cPickle.dump(this_robot_trace_matrix, handle, protocol=cPickle.HIGHEST_PROTOCOL)

        robot += [run]*len(this_df)
        df = pd.concat([df, this_df])

    df['robot'] = robot

    if ROTATE:
        df.to_pickle('{0}/{1}_Trace_DataFrame_SpunXXX.pickle'.format(PICKLE_DIR, EXP_NAME))
    else:
        df.to_pickle('{0}/{1}_Trace_DataFrameXXX.pickle'.format(PICKLE_DIR, EXP_NAME))


if DEBUG:

    with open('{0}/{1}_Trace_DataFrameXXX.pickle'.format(PICKLE_DIR, EXP_NAME), 'rb') as handle:
        df = cPickle.load(handle)

    # df = pd.read_pickle('{0}/Robot_Trace_DataFrame.pickle'.format(PICKLE_DIR))

    print max([convert_voxelyze_index(x) for x in df['X'].values])

