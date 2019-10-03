import cPickle
from glob import glob
import subprocess as sub
import numpy as np
import pandas as pd
import time
from scipy.ndimage import gaussian_filter1d

from evosoro.softbot import Genotype, Phenotype
from evosoro.base import Env
from evosoro.tools.read_write_voxelyze import write_voxelyze_file
from evosoro.tools.utils import quadruped

import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42


font = {'size': 22}
matplotlib.rc('font', **font)


sns.set_style("ticks")

colors = ["grey", "dark pink", "ocean green", "tan"]
sns.set_palette(sns.xkcd_palette(colors))  # , desat=.9)

np.random.seed(1)

FLIP_ON_BACK = 0

SEND_ROBOTS_TO_SIM = False
COLLECT_FITNESS_FILES = False
LOAD_DF_FROM_PICKLE = False
HISTO = True

ADDITIONAL_TIME = 0  # 70.0 + 80.0

NOISE_SCALE = 0.10
PROB_FLIP_MAT = 0  #

NUM_RAND_CONTROLLERS = 20  # 32
BATCH_SIZE = 30
SEC_BETWEEN_BATCHES = 14  # 60*1.5

TIME_BETWEEN_TRACES = 0  # 0.05

GEN = 1000

start_run = 1
RUNS = 100

EXP_NAME = "XENO_3"
PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results"
IND_SIZE = (8, 8, 7)
STIFFNESS = 5e6
INIT_TIME = 1
SIM_TIME = 10.0 + ADDITIONAL_TIME + INIT_TIME  # includes init time
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

    sub.call("mkdir {0}/Robustness_{1}_{2}".format(PICKLE_DIR, EXP_NAME, FLIP_ON_BACK), shell=True)

    for run in range(start_run, RUNS + 1):

        sub.call("mkdir {0}/Robustness_{1}_{3}/Run_{2}".format(PICKLE_DIR, EXP_NAME, run, FLIP_ON_BACK), shell=True)

        sub.call("mkdir {0}/Robustness_{1}_{3}/Run_{2}/voxelyzeFiles && "
                 "mkdir {0}/Robustness_{1}_{3}/Run_{2}/fitnessFiles".format(PICKLE_DIR, EXP_NAME, run, FLIP_ON_BACK),
                 shell=True)

    MyPhenotype = Phenotype
    MyGenotype = Genotype

    orig_fit_dict = {}

    # for exp_name in EXP_NAMES:
    for run in range(start_run, RUNS+1):

        # clear directories
        sub.call("rm {0}/Robustness_{1}_{3}/Run_{2}/voxelyzeFiles/*".format(PICKLE_DIR, EXP_NAME, run,
                                                                            FLIP_ON_BACK), shell=True)
        sub.call("rm {0}/Robustness_{1}_{3}/Run_{2}/fitnessFiles/*".format(PICKLE_DIR, EXP_NAME, run, FLIP_ON_BACK),
                 shell=True)

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

        save_dir = "{0}/Robustness_{1}_{3}/Run_{2}/".format(PICKLE_DIR, EXP_NAME, run, FLIP_ON_BACK)  # voxelyzeFiles

        flipped = False

        for r in range(NUM_RAND_CONTROLLERS):
            for name, details in best_ind.genotype.to_phenotype_mapping.items():

                print name

                if name == "phase_offset":
                    if NOISE_SCALE > 0:
                        print "Phase randomization ", run
                        details["state"] += np.random.normal(0, NOISE_SCALE, IND_SIZE)
                        if FLIP_ON_BACK:
                            print "flipping"
                            state = details["state"]
                            details["state"] = state[:, :, ::-1]

                if name == "material":

                    if FLIP_ON_BACK and not flipped:
                        print "flipping"
                        state = details["state"]
                        details["state"] = state[:, :, ::-1]
                        flipped = True

                    if PROB_FLIP_MAT > 0:
                        print "Material randomization", run
                        state = details["state"]
                        passive_idx = state == 1
                        cardiac_idx = state == 3
                        flip_idx = np.random.rand(*IND_SIZE) < PROB_FLIP_MAT
                        switch_to_cardiac = np.logical_and(passive_idx, flip_idx)
                        switch_to_passive = np.logical_and(cardiac_idx, flip_idx)
                        state[switch_to_cardiac] = 3
                        state[switch_to_passive] = 1

            best_ind.id = r

            write_voxelyze_file(optimizer.sim, my_env, best_ind, save_dir, "Brownian")

    with open('{0}/{1}_Orig_Fit_Dict.pickle'.format(PICKLE_DIR, EXP_NAME), 'wb') as handle:
        cPickle.dump(orig_fit_dict, handle, protocol=cPickle.HIGHEST_PROTOCOL)

    # evaluate all robots in simulator #
    count = 1
    for run in range(start_run, RUNS + 1):
        robots = "{0}/Robustness_{1}_{3}/Run_{2}/voxelyzeFiles/*".format(PICKLE_DIR, EXP_NAME, run, FLIP_ON_BACK)
        for vxa in glob(robots):
            print "sending robot {} to the simulator".format(count)
            sub.Popen("/home/sam/Projects/research_code/evosoro/_voxcad/voxelyzeMain/voxelyze -f " + vxa, shell=True)

            if count % BATCH_SIZE == 0:
                print "GOING TO SLEEP..."
                time.sleep(SEC_BETWEEN_BATCHES)
                print "WAKING UP!"

            count += 1


if COLLECT_FITNESS_FILES:

    # with open('{0}/{1}_Orig_Fit_Dict.pickle'.format(PICKLE_DIR, EXP_NAME), 'rb') as handle:
    #     orig_fit_dict = cPickle.load(handle)

    robot_fit_dict = {run: {
                            "normAbsoluteDisplacement": [],
                            # "Time": [], "TraceX": [], "TraceY": [], "TraceZ": []
                            }
                      for run in range(1, RUNS+1)}

    all_tag_keys = [("<"+k+">", k) for k, v in robot_fit_dict[1].items()]

    for run in range(start_run, RUNS+1):

        # print "collecting data for robot", run
        robots = glob("{0}/Robustness_{1}_{3}/Run_{2}/fitnessFiles/*".format(PICKLE_DIR, EXP_NAME, run,
                                                                             FLIP_ON_BACK))

        for robot in robots:
            name = int(robot[robot.find("id_")+3:-4])
            this_robot = open(robot)
            for line in this_robot:
                for tag, key in all_tag_keys:
                    if tag in line:
                        robot_fit_dict[run][key] += [float(line[line.find(tag) + len(tag):line.find("</" + tag[1:])])]
                        # robot_fit_dict[run][key][-1] /= orig_fit_dict[run]

    with open('{0}/{1}_Robustness_Dict_{2}.pickle'.format(PICKLE_DIR, EXP_NAME, FLIP_ON_BACK), 'wb') as handle:
        cPickle.dump(robot_fit_dict, handle, protocol=cPickle.HIGHEST_PROTOCOL)

    df = pd.DataFrame()
    robot = []
    for run in range(start_run, RUNS+1):
        this_df = pd.DataFrame.from_dict(robot_fit_dict[run])
        this_df.columns = ['fit']
        robot += [run]*len(this_df)
        df = pd.concat([df, this_df])

    df['robot'] = robot

    df.to_pickle('{0}/{1}_Robustness_DataFrame_{2}.pickle'.format(PICKLE_DIR, EXP_NAME, FLIP_ON_BACK))


if LOAD_DF_FROM_PICKLE:

    # vivo_data = pd.read_csv("/home/sam/Desktop/vivo_data.csv")  # original 5
    vivo_data = pd.read_csv("../data_analysis/vivo_data.csv")  # additional cardiacBot
    # print vivo_data.head()

    vivo_data["X"] -= vivo_data["Start_X"]
    vivo_data["Y"] -= vivo_data["Start_Y"]

    # print np.min(vivo_data["Y"])

    vivo_data["Y"][vivo_data["Robot"] == 6] = -vivo_data["Y"][vivo_data["Robot"] == 6]

    if FLIP_ON_BACK:
        X_vivo, Y_vivo = vivo_data["X"][vivo_data["Upright"] == 0], vivo_data["Y"][vivo_data["Upright"] == 0]
        vivo_time = vivo_data["Time"][vivo_data["Upright"] == 0]
        vivo_run = vivo_data["Run"][vivo_data["Upright"] == 0]
    else:
        X_vivo, Y_vivo = vivo_data["X"][vivo_data["Upright"] == 1], vivo_data["Y"][vivo_data["Upright"] == 1]
        vivo_time = vivo_data["Time"][vivo_data["Upright"] == 1]
        vivo_run = vivo_data["Run"][vivo_data["Upright"] == 1]

    # X_vivo_smoothed = gaussian_filter1d(X_vivo, 20)
    # Y_vivo_smoothed = gaussian_filter1d(Y_vivo, 20)

    print len(X_vivo), len(Y_vivo), len(vivo_time)

    X_vivo_smoothed = []
    Y_vivo_smoothed = []
    tmpX, tmpY = [X_vivo.values[0]], [Y_vivo.values[0]]
    for n in range(1, len(vivo_time)):
        if vivo_time.values[n] < vivo_time.values[n-1]:
            # print vivo_run.values[n] == vivo_run.values[n-1]
            X_vivo_smoothed += gaussian_filter1d(tmpX, 30).tolist()
            Y_vivo_smoothed += gaussian_filter1d(tmpY, 30).tolist()
            tmpX, tmpY = [X_vivo.values[n]], [Y_vivo.values[n]]
        else:
            tmpX += [X_vivo.values[n]]
            tmpY += [Y_vivo.values[n]]

        if n == len(vivo_time)-1:
            X_vivo_smoothed += gaussian_filter1d(tmpX, 30).tolist()
            Y_vivo_smoothed += gaussian_filter1d(tmpY, 30).tolist()

    print len(X_vivo_smoothed), len(Y_vivo_smoothed), len(vivo_time)

    # print X_vivo_smoothed

    # X_vivo = vivo_data["X"][vivo_data["Upright"] == 1][vivo_data["Robot"] == 5][vivo_data["Run"] == 3]
    # Y_vivo = vivo_data["Y"][vivo_data["Upright"] == 1][vivo_data["Robot"] == 5][vivo_data["Run"] == 3]
    # vivo_time = vivo_data["Time"][vivo_data["Upright"] == 1][vivo_data["Robot"] == 5][vivo_data["Run"] == 3]

    vivo_time = np.asarray(vivo_time, dtype=float)
    t_vivo = (vivo_time - np.min(vivo_time)) / (np.max(vivo_time) - np.min(vivo_time))

    with open('{0}/XENO_3_Robustness_DataFrame_{1}.pickle'.format(PICKLE_DIR, FLIP_ON_BACK), 'rb') as handle:
        df = cPickle.load(handle)

    # print df

    X, Y, Z = df["TraceX"], df["TraceY"], df["TraceZ"]
    time = df["Time"]
    t = (np.array(time) - np.min(time)) / (np.max(time) - np.min(time))

    X, Y, Z, time = X[time < 61], Y[time < 61], Z[time < 61], time[time < 61]  # only use one minute in silico

    fig, ax = plt.subplots(1, 1, figsize=(3, 3))

    ax.scatter((X[:len(X)*25/32+1]-0.05*4)*2.5, (Y[:len(X)*25/32+1]-0.05*4)*2.5,  # 1/(0.05*8) is one body length
               c=time[:len(X)*25/32+1],
               cmap="spring_r",
               s=0.25,  # (t+1)**3-0.9,
               alpha=0.5
               )

    ax.scatter(np.array(Y_vivo_smoothed)*-1, np.array(X_vivo_smoothed),
               c=t_vivo,
               cmap="winter_r",
               s=0.25,
               alpha=0.5
               )

    ax.set_ylim(-3.0, 3)
    ax.set_yticks([-3, -2, -1, 0, 1, 2, 3])
    ax.set_yticklabels(["-3", "-2", "-1", "0", "1", "2", "3"], fontsize=8)

    ax.set_xlim(-1.5, 4.5)
    ax.set_xticks([-1, 0, 1, 2, 3, 4])
    ax.set_xticklabels(["-1", "0", "1", "2", "3", "4"], fontsize=8)

    # ax.set_xlim(-0.5, 2.5)
    # xticks = [0, 0.75, 1.5, 2.25]
    # xticks = [0.05*4+x for x in xticks]
    # ax.set_xticks(xticks)
    # ax.set_xticklabels([round((x-0.05*4)/0.05/8.0/float(SIM_TIME-INIT_TIME)*60.0, 1) for x in xticks], fontsize=8)
    #
    # ax.set_ylim(-1.5, 1.5)
    # yticks = [-1.5, -0.75, 0, 0.75, 1.5]
    # yticks = [0.05*4+y for y in yticks]
    # ax.set_yticks(yticks)
    # ax.set_yticklabels([round((y-0.05*4)/0.05/8.0/float(SIM_TIME-INIT_TIME)*60.0, 1) for y in yticks], fontsize=8)
    #
    # frame1 = plt.gca()
    # frame1.axes.get_xaxis().set_visible(False)
    # frame1.axes.get_yaxis().set_visible(False)
    #
    # sns.despine(left=1, bottom=1)
    # # sns.despine()
    plt.tight_layout()
    plt.savefig("../data_analysis/plots/XENO_3_RunChamp_52_Traces_{}.png".format(FLIP_ON_BACK),
                bbox_inches='tight', dpi=600, transparent=True)


if HISTO:

    with open('{0}/{1}_Robustness_DataFrame_{2}.pickle'.format(PICKLE_DIR, EXP_NAME, FLIP_ON_BACK), 'rb') as handle:
        df = cPickle.load(handle)

    x = df.groupby(['robot']).median()
    m = x.mean()
    s = x.std()
    print m + s
    print df[df['robot'] == 52].median()
    print
    # print df.groupby(['robot']).median()

    medians = df.groupby(['robot']).median()
    print medians.nsmallest(50, 'fit')

    # print df

    fig, ax = plt.subplots(1, 1, figsize=(4, 3))

    mask = df['robot'].values != 52
    sns.kdeplot(df["fit"][mask],#*6/8.,
                shade=True, legend=False, bw=0.4, gridsize=10000, clip=(0, 8))

    mask = df['robot'].values == 52
    sns.kdeplot(df["fit"][mask],#*6/8.,
                shade=True, legend=False, bw=0.4, gridsize=10000, clip=(0, 8))

    # ax.set_xlim([None, 8*8/6.0])
    ax.set_ylim([-0.01, 0.5])

    ticks = matplotlib.ticker.FuncFormatter(lambda x, pos: '{0:g}'.format(x * 6/8.0))
    ax.xaxis.set_major_formatter(ticks)

    # mask = df['robot'].values != 52
    # sns.distplot(df["fit"][mask] * 6 / 8., kde=False, norm_hist=True)
    #
    # mask = df['robot'].values == 52
    # sns.distplot(df["fit"][mask] * 6 / 8., kde=False, norm_hist=True)
    #
    # ax.set_xlim([-1, 8])
    # ax.set_ylim([-0.01, 0.6])
    ax.set_xticks([0, 2*8/6.0, 4*8/6.0, 6*8/6.0, 8*8/6.0])
    # ax.set_yticks([0, .2, .4, .6])

    # frame1 = plt.gca()
    # frame1.axes.get_xaxis().set_visible(False)
    # frame1.axes.get_yaxis().set_visible(False)

    sns.despine()
    plt.tight_layout()
    plt.savefig("../data_analysis/plots/XENO_3_Histogram.png", bbox_inches='tight', dpi=600, transparent=True)