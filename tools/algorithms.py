import random
import time
import cPickle
import numpy as np
import subprocess as sub
from functools import partial
from itertools import product, islice

from es import PEPG
from evaluation import evaluate_all
from selection import pareto_selection, pareto_tournament_selection, parallel_hill_climber
from mutation import create_new_children_through_mutation, genome_wide_mutation
from logging import PrintLog, initialize_folders, make_gen_directories, write_gen_stats


class Optimizer(object):
    def __init__(self, sim, env, evaluation_func=evaluate_all):
        self.sim = sim
        self.env = env
        if not isinstance(env, list):
            self.env = [env]
        self.evaluate = evaluation_func
        self.curr_env_idx = 0
        self.start_time = None

    def elapsed_time(self, units="s"):
        if self.start_time is None:
            self.start_time = time.time()
        s = time.time() - self.start_time
        if units == "s":
            return s
        elif units == "m":
            return s / 60.0
        elif units == "h":
            return s / 3600.0

    def save_checkpoint(self, directory, gen):
        random_state = random.getstate()
        numpy_random_state = np.random.get_state()
        data = [self, random_state, numpy_random_state]
        with open('{0}/pickledPops/Gen_{1}.pickle'.format(directory, gen), 'wb') as handle:
            cPickle.dump(data, handle, protocol=cPickle.HIGHEST_PROTOCOL)

    def run(self, *args, **kwargs):
        raise NotImplementedError


class PopulationBasedOptimizer(Optimizer):
    def __init__(self, sim, env, pop, selection_func, mutation_func):
        Optimizer.__init__(self, sim, env)
        self.pop = pop
        self.select = selection_func
        self.mutate = mutation_func
        self.num_env_cycles = 0
        self.autosuspended = False
        self.max_gens = None
        self.directory = None
        self.name = None
        self.num_random_inds = 0

    def update_env(self):
        if self.num_env_cycles > 0:
            switch_every = self.max_gens / float(self.num_env_cycles)
            self.curr_env_idx = int(self.pop.gen / switch_every % len(self.env))
            print " Using environment {0} of {1}".format(self.curr_env_idx+1, len(self.env))

    def run(self, max_hours_runtime=29, max_gens=3000, num_random_individuals=1, num_env_cycles=0,
            directory="tests_data", name="TestRun",
            max_eval_time=60, time_to_try_again=30,
            checkpoint_every=100, save_vxa_every=100, save_pareto=False,
            save_nets=False, save_lineages=False, continued_from_checkpoint=False,
            batch_size=None, update_survivors_age=True,
            max_fitness=None):

        if self.autosuspended:
            sub.call("rm %s/AUTOSUSPENDED" % directory, shell=True)

        self.autosuspended = False
        self.max_gens = max_gens  # can add additional gens through checkpointing

        print_log = PrintLog()
        print_log.add_timer("evaluation")
        self.start_time = print_log.timers["start"]  # sync start time with logging

        # sub.call("clear", shell=True)

        if not continued_from_checkpoint:  # generation zero
            self.directory = directory
            self.name = name
            self.num_random_inds = num_random_individuals
            self.num_env_cycles = num_env_cycles

            initialize_folders(self.pop, self.directory, self.name, save_nets, save_lineages=save_lineages)
            make_gen_directories(self.pop, self.directory, save_vxa_every, save_nets)
            sub.call("touch {}/RUNNING".format(self.directory), shell=True)
            self.evaluate(self.sim, self.env[self.curr_env_idx], self.pop, print_log, save_vxa_every, self.directory,
                          self.name, max_eval_time, time_to_try_again, save_lineages, batch_size)
            self.select(self.pop)  # only produces dominated_by stats, no selection happening (population not replaced)
            write_gen_stats(self.pop, self.directory, self.name, save_vxa_every, save_pareto, save_nets,
                            save_lineages=save_lineages)

        while self.pop.gen < max_gens:

            if self.pop.gen % checkpoint_every == 0:
                print_log.message("Saving checkpoint at generation {0}".format(self.pop.gen+1), timer_name="start")
                self.save_checkpoint(self.directory, self.pop.gen)

            if self.elapsed_time(units="h") > max_hours_runtime or self.pop.best_fit_so_far == max_fitness:
                self.autosuspended = True
                print_log.message("Autosuspending at generation {0}".format(self.pop.gen+1), timer_name="start")
                self.save_checkpoint(self.directory, self.pop.gen)
                sub.call("touch {0}/AUTOSUSPENDED && rm {0}/RUNNING".format(self.directory), shell=True)
                break

            self.pop.gen += 1
            print_log.message("Creating folders structure for this generation")
            make_gen_directories(self.pop, self.directory, save_vxa_every, save_nets)

            # update ages
            self.pop.update_ages(update_survivors_age)

            # mutation
            print_log.message("Mutation starts")
            new_children = self.mutate(self.pop, print_log=print_log)
            print_log.message("Mutation ends: successfully generated %d new children." % (len(new_children)))

            # combine children and parents for selection
            print_log.message("Now creating new population")
            self.pop.append(new_children)
            for _ in range(self.num_random_inds):
                print_log.message("Random individual added to population")
                self.pop.add_random_individual()
            print_log.message("New population size is %d" % len(self.pop))

            # evaluate fitness
            print_log.message("Starting fitness evaluation", timer_name="start")
            print_log.reset_timer("evaluation")
            self.update_env()
            self.evaluate(self.sim, self.env[self.curr_env_idx], self.pop, print_log, save_vxa_every, self.directory,
                          self.name, max_eval_time, time_to_try_again, save_lineages, batch_size)
            print_log.message("Fitness evaluation finished", timer_name="evaluation")  # record total eval time in log

            # perform selection by pareto fronts
            new_population = self.select(self.pop)

            # print population to stdout and save all individual data
            print_log.message("Saving statistics")
            write_gen_stats(self.pop, self.directory, self.name, save_vxa_every, save_pareto, save_nets,
                            save_lineages=save_lineages)

            # replace population with selection
            self.pop.individuals = new_population
            print_log.message("Population size reduced to %d" % len(self.pop))

        if not self.autosuspended:  # print end of run stats
            print_log.message("Finished {0} generations".format(self.pop.gen + 1))
            print_log.message("DONE!", timer_name="start")
            sub.call("touch {0}/RUN_FINISHED && rm {0}/RUNNING".format(self.directory), shell=True)


class ParetoOptimization(PopulationBasedOptimizer):
    def __init__(self, sim, env, pop):
        PopulationBasedOptimizer.__init__(self, sim, env, pop, pareto_selection, create_new_children_through_mutation)


class ParetoTournamentOptimization(PopulationBasedOptimizer):
    def __init__(self, sim, env, pop):
        PopulationBasedOptimizer.__init__(self, sim, env, pop, pareto_tournament_selection,
                                          create_new_children_through_mutation)


class GenomeWideMutationOptimization(PopulationBasedOptimizer):
    def __init__(self, sim, env, pop):
        PopulationBasedOptimizer.__init__(self, sim, env, pop, pareto_selection, genome_wide_mutation)


class SetMutRateOptimization(PopulationBasedOptimizer):
    def __init__(self, sim, env, pop, mut_net_probs):
        PopulationBasedOptimizer.__init__(self, sim, env, pop, pareto_selection,
                                          partial(create_new_children_through_mutation,
                                                  mutate_network_probs=mut_net_probs))


class GenerateMutProbOptimization(PopulationBasedOptimizer):
    def __init__(self, sim, env, pop, prob_generating_func):
        PopulationBasedOptimizer.__init__(self, sim, env, pop, pareto_selection,
                                          partial(create_new_children_through_mutation,
                                                  prob_generating_func=prob_generating_func))


class HillClimbingOptimization(PopulationBasedOptimizer):
    def __init__(self, sim, env, pop):
        PopulationBasedOptimizer.__init__(self, sim, env, pop, parallel_hill_climber,
                                          create_new_children_through_mutation)


class GradientAscent(PopulationBasedOptimizer):
    def __init__(self, sim, env, pop, solver=PEPG):
        PopulationBasedOptimizer.__init__(self, sim, env, pop, None, None)
        self.encoding = None  # should raise notImplementedError...
        if solver is None:
            self.solver = None
        else:
            self.solver = solver(np.prod(self.pop[0].genotype.orig_size_xyz), popsize=self.pop.pop_size)

    def make_children(self):
        return self.solver.ask()

    def run(self, max_hours_runtime=29, max_gens=3000, num_random_individuals=1, num_env_cycles=0,
            directory="tests_data", name="TestRun", max_eval_time=60, time_to_try_again=30, checkpoint_every=100,
            save_vxa_every=100, save_pareto=False, save_nets=False, save_lineages=False,
            continued_from_checkpoint=False, batch_size=None, update_survivors_age=True, max_fitness=None):

        if self.autosuspended:
            sub.call("rm %s/AUTOSUSPENDED" % directory, shell=True)

        self.autosuspended = False
        self.max_gens = max_gens  # can add additional gens through checkpointing

        print_log = PrintLog()
        print_log.add_timer("evaluation")
        self.start_time = print_log.timers["start"]  # sync start time with logging

        if not continued_from_checkpoint:  # generation zero
            self.directory = directory
            self.name = name
            self.num_random_inds = num_random_individuals
            self.num_env_cycles = num_env_cycles

            initialize_folders(self.pop, self.directory, self.name, save_nets, save_lineages=save_lineages)
            sub.call("touch {}/RUNNING".format(self.directory), shell=True)

        while self.pop.gen < max_gens:

            if self.pop.gen % checkpoint_every == 0:
                print_log.message("Saving checkpoint at generation {0}".format(self.pop.gen + 1), timer_name="start")
                self.save_checkpoint(self.directory, self.pop.gen)

            if self.elapsed_time(units="h") > max_hours_runtime or self.pop.best_fit_so_far == max_fitness:
                self.autosuspended = True
                print_log.message("Autosuspending at generation {0}".format(self.pop.gen + 1), timer_name="start")
                self.save_checkpoint(self.directory, self.pop.gen)
                sub.call("touch {0}/AUTOSUSPENDED && rm {0}/RUNNING".format(self.directory), shell=True)
                break

            print_log.message("Creating folders structure for this generation")
            make_gen_directories(self.pop, self.directory, save_vxa_every, save_nets)

            # mutation
            print_log.message("Mutation starts")
            new_solutions = self.make_children()
            new_children = [self.encoding(t) for t in new_solutions]
            print_log.message("Mutation ends: successfully generated %d new children." % (len(new_children)))

            # replace old pop
            print_log.message("Replacing population with new designs")
            self.pop.individuals = []
            for mat in new_children:
                self.pop.add_random_individual()
                for old_net in self.pop[-1].genotype.networks:
                    if old_net.output_node_names[0] == "material":   # TODO: make this dynamic
                        old_net.values = mat
                self.pop[-1].genotype.express()
            print_log.message("New population size is %d" % len(self.pop))

            # evaluate fitness
            print_log.message("Starting fitness evaluation", timer_name="start")
            print_log.reset_timer("evaluation")
            self.update_env()
            self.evaluate(self.sim, self.env[self.curr_env_idx], self.pop, print_log, save_vxa_every,
                          self.directory, self.name, max_eval_time, time_to_try_again, save_lineages, batch_size)
            print_log.message("Fitness evaluation finished", timer_name="evaluation")  # record total eval time in log

            if self.solver is not None:  # update solver
                self.solver.tell([ind.fitness for ind in self.pop])

            # print population to stdout and save all individual data
            print_log.message("Saving statistics")
            write_gen_stats(self.pop, self.directory, self.name, save_vxa_every, save_pareto, save_nets,
                            save_lineages=save_lineages)

            self.pop.gen += 1

        if not self.autosuspended:  # print end of run stats
            print_log.message("Finished {0} generations".format(self.pop.gen + 1))
            print_log.message("DONE!", timer_name="start")
            sub.call("touch {0}/RUN_FINISHED && rm {0}/RUNNING".format(self.directory), shell=True)


class ExhaustiveSearch(GradientAscent):
    def __init__(self, sim, env, pop, vox_types):
        GradientAscent.__init__(self, sim, env, pop, None)
        self.search_space = np.array(list(product(vox_types, repeat=np.product(self.pop[0].genotype.orig_size_xyz))))
        self.encoding = None

    def make_children(self):
        # print len(self.search_space),  self.pop.gen*self.pop.pop_size, (self.pop.gen+1)*self.pop.pop_size
        # return islice(self.search_space, self.pop.gen*self.pop.pop_size, (self.pop.gen+1)*self.pop.pop_size)
        return self.search_space[self.pop.gen*self.pop.pop_size:(self.pop.gen+1)*self.pop.pop_size]



