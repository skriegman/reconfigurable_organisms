import random
import os
import cPickle
import numpy as np
import subprocess as sub
from glob import glob
from tools.utils import natural_sort


def continue_from_checkpoint(directory="tests_data", additional_gens=0, max_hours_runtime=29,
                             max_eval_time=60, time_to_try_again=30,
                             checkpoint_every=1, save_vxa_every=1,
                             save_pareto=False, save_nets=False, save_lineages=False, batch_size=None):

    if os.path.isfile("./" + directory + "/RUNNING"):
        sub.call("touch {}/DUPLICATE".format(directory), shell=True)
        print "Duplicate job submitted"

    else:
        sub.call("touch {}/RUNNING".format(directory), shell=True)
        sub.call("rm -f %s/voxelyzeFiles/*" % directory, shell=True)  # remove partial results

        successful_restart = False
        pickle_idx = 0
        while not successful_restart:
            try:
                pickled_pops = glob(directory + "/pickledPops/*")
                last_gen = natural_sort(pickled_pops, reverse=True)[pickle_idx]
                with open(last_gen, 'rb') as handle:
                    [optimizer, random_state, numpy_random_state] = cPickle.load(handle)
                successful_restart = True

            except EOFError:
                # something went wrong writing the checkpoint : use previous checkpoint and redo last generation
                sub.call("touch {}/IO_ERROR_$(date +%F_%R)".format(directory), shell=True)
                pickle_idx += 1
                pass

        random.setstate(random_state)
        np.random.set_state(numpy_random_state)
        max_gens = optimizer.max_gens + additional_gens
        if optimizer.pop.gen < max_gens:
            optimizer.run(continued_from_checkpoint=True, max_hours_runtime=max_hours_runtime, max_gens=max_gens,
                          max_eval_time=max_eval_time, time_to_try_again=time_to_try_again,
                          checkpoint_every=checkpoint_every, save_vxa_every=save_vxa_every,
                          save_lineages=save_lineages, save_nets=save_nets, save_pareto=save_pareto,
                          batch_size=batch_size)
