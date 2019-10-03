from glob import glob
import subprocess as sub
import sys
import os

sys.path.append(os.getcwd() + "/../..")

from evosoro.tools.data_analysis import get_all_data, combine_experiments


def make_plot(_lambda, experiment_names):
    save_location = "results/best_of_gen_{}.csv".format(_lambda)
    exp_data = []
    for exp_name in experiment_names:
        _file = glob('/media/sam/Luna/pnas/{}/run*/bestSoFar/bestOfGen.txt'.format(exp_name))
        print exp_name, len(_file)
        exp_data += [get_all_data(_file)]

    data = combine_experiments(exp_data, experiment_names)
    data.to_csv(save_location)

# exp_names = [x[7:-3] for x in glob('../pbs/RSS_*B_*')] + [x[7:-3] for x in glob('../pbs/RSS_*D_*')]
exp_names = ["XENO_3_d332_SGD", "XENO_3_d332_AFPO", "XENO_3_d332_SGD_restarts"]  # , "XENO_3_d222_Exhaustive"]

make_plot("XENO_3_d332", exp_names)

# sub.call("/home/sam/anaconda2/bin/python /home/sam/Projects/research_code/evosoro/data_analysis/plot_best_of_gen.py",
#          shell=True)

