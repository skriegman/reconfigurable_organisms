import cPickle
import numpy as np
import pandas as pd
from statsmodels.stats import multitest
from scipy.ndimage import gaussian_filter1d
from scipy.stats import mannwhitneyu, binom_test, wilcoxon


EXP_NAME = "XENO_3"
PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results"

inverted = 0
with open('{0}/XENO_3_Robustness_DataFrame_{1}.pickle'.format(PICKLE_DIR, inverted), 'rb') as handle:
    df = cPickle.load(handle)
X, Y, Z = df["TraceX"], df["TraceY"], df["TraceZ"]
time = df["Time"]
X, Y, time = X[time < 61], Y[time < 61], time[time < 61]  # only use one minute in silico
X, Y = (X[:len(X)*25/32+1]-0.05*4)*2.5, (Y[:len(X)*25/32+1]-0.05*4)*2.5

inverted = 1
with open('{0}/XENO_3_Robustness_DataFrame_{1}.pickle'.format(PICKLE_DIR, inverted), 'rb') as handle:
    df_inv = cPickle.load(handle)
X_inv, Y_inv, Z_inv = df_inv["TraceX"], df_inv["TraceY"], df_inv["TraceZ"]
time_inv = df_inv["Time"]
X_inv, Y_inv, time_inv = X_inv[time_inv < 61], Y_inv[time_inv < 61], time_inv[time_inv < 61]
X_inv, Y_inv = (X_inv[:len(X_inv)*25/32+1]-0.05*4)*2.5, (Y_inv[:len(Y_inv)*25/32+1]-0.05*4)*2.5


# vivo_data = pd.read_csv("/home/sam/Desktop/vivo_data.csv")
vivo_data = pd.read_csv("../data_analysis/vivo_data.csv")  # additional cardiacBot

vivo_data["X"] -= vivo_data["Start_X"]
vivo_data["Y"] -= vivo_data["Start_Y"]

X_vivo_inv, Y_vivo_inv = vivo_data["X"][vivo_data["Upright"] == 0], vivo_data["Y"][vivo_data["Upright"] == 0]
vivo_time_inv = vivo_data["Time"][vivo_data["Upright"] == 0]
vivo_run_inv = vivo_data["Run"][vivo_data["Upright"] == 0]
vivo_robot_inv = vivo_data["Robot"][vivo_data["Upright"] == 0]

X_vivo, Y_vivo = vivo_data["X"][vivo_data["Upright"] == 1], vivo_data["Y"][vivo_data["Upright"] == 1]
vivo_time = vivo_data["Time"][vivo_data["Upright"] == 1]
vivo_run = vivo_data["Run"][vivo_data["Upright"] == 1]
vivo_robot = vivo_data["Robot"][vivo_data["Upright"] == 1]


vivo_final_dist = []
robot = []
run = []

for n in range(1, len(vivo_time)):
    if vivo_time.values[n] < vivo_time.values[n-1]:
        vivo_final_dist += [(X_vivo.values[n-1]**2 + Y_vivo.values[n-1]**2)**0.5]
        robot += [vivo_robot.values[n-1]]
        run += [vivo_run.values[n-1]]

    if n == len(vivo_time)-1:
        vivo_final_dist += [(X_vivo.values[n-1]**2 + Y_vivo.values[n-1]**2)**0.5]
        robot += [vivo_robot.values[n]]
        run += [vivo_run.values[n]]

vivo_final_dist_inv = []
robot_inv = []
run_inv = []

for n in range(1, len(vivo_time_inv)):
    if vivo_time_inv.values[n] < vivo_time_inv.values[n-1]:
        vivo_final_dist_inv += [(X_vivo_inv.values[n-1]**2 + Y_vivo_inv.values[n-1]**2)**0.5]
        robot_inv += [vivo_robot.values[n-1]]
        run_inv += [vivo_run.values[n-1]]

    if n == len(vivo_time)-1:
        vivo_final_dist_inv += [(X_vivo_inv.values[n-1]**2 + Y_vivo_inv.values[n-1]**2)**0.5]
        robot_inv += [vivo_robot.values[n]]
        run_inv += [vivo_run.values[n]]


diff = []
for bot, r, d in zip(robot, run, vivo_final_dist):
    for bot_inv, r_inv, d_inv in zip(robot_inv, run_inv, vivo_final_dist_inv):
        if bot == bot_inv:
            if r == r_inv or (r_inv == 1 and bot < 3):
                diff += [d-d_inv]

print len(np.unique(diff))
print len(diff), len(run)
p1 = wilcoxon(diff)[1]  # *3
print "vivo ", p1

silico_final_dist = []

for n in range(1, len(X)):
    if time.values[n] < time.values[n-1]:
        silico_final_dist += [(X.values[n-1]**2 + Y.values[n-1]**2)**0.5]

    if n == len(X)-1:
        silico_final_dist += [(X.values[n-1]**2 + Y.values[n-1]**2)**0.5]


silico_final_dist_inv = []

for n in range(1, len(X_inv)):
    if time_inv.values[n] < time_inv.values[n-1]:
        silico_final_dist_inv += [(X_inv.values[n-1]**2 + Y_inv.values[n-1]**2)**0.5]

    if n == len(X_inv)-1:
        silico_final_dist_inv += [(X_inv.values[n-1]**2 + Y_inv.values[n-1]**2)**0.5]

# print len(silico_final_dist), len(silico_final_dist_inv)

# print "silco 5x4 ", wilcoxon(silico_final_dist, silico_final_dist_inv[:len(silico_final_dist)*4/5])[1]*3

p2 = wilcoxon(silico_final_dist, silico_final_dist_inv)[1]  # *3
print "silco ", p2

# p = []
# for _ in range(5000):
#     p += [wilcoxon(silico_final_dist, np.random.choice(silico_final_dist_inv, len(silico_final_dist)))[1]]
# print "boot slico ", np.mean(p)*3


# print "vivo ", wilcoxon(vivo_final_dist, vivo_final_dist_inv)[1]*3
# print "vivo ", wilcoxon(np.random.choice(vivo_final_dist, len(vivo_final_dist_inv)), vivo_final_dist_inv)[1]*3

# p = []
# for _ in range(5000):
#     p += [wilcoxon(vivo_final_dist, np.random.choice(vivo_final_dist_inv, len(vivo_final_dist)))[1]]
# print "boot vivo ", np.mean(p)*3

# print "binom: ", binom_test(4, 5, 0.25)*3  # original 5
p3 = binom_test(5, 6, 0.25)  # *3  # with 6th
print "binom: ", p3  # with 6th
print "fdr: ", multitest.fdrcorrection([p1, p2, p3], alpha=0.0001, method='indep', is_sorted=False)


# np.random.choice(vivo_final_dist, )

