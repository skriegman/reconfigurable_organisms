import time
import subprocess as sub
import uuid
import os

from read_write_voxelyze import read_voxlyze_results, write_voxelyze_file


# TODO: make eval times relative to the number of simulated voxels
# TODO: right now just saving files gen-id-fitness; but this should be more flexible (as option in objective dict?)
# TODO: fitness isn't even necessarily the name of the top objective --> use pop.objective_dict[0]["name"] (?)
# getattr(ind, pop.objective_dict[0]["name"])
# TODO: the location of voxelyze and the data must be consistent and specified or more robust (cp for now)
# sub.call("cp ../_voxcad/voxelyzeMain/voxelyze .", shell=True)
# sub.call("cp ../_voxcad/qhull .", shell=True)


def evaluate_all(sim, env, pop, print_log, save_vxa_every, run_directory, run_name, max_eval_time=60,
                 time_to_try_again=10, save_lineages=False, batch_size=None):
    """Evaluate all individuals of the population in VoxCad.

    Parameters
    ----------
    sim : Sim
        Configures parameters of the Voxelyze simulation.

    env : Env
        Configures parameters of the Voxelyze environment.

    pop : Population
        This provides the individuals to evaluate.

    print_log : PrintLog()
        For logging with time stamps

    save_vxa_every : int
        Which generations to save information about individual SoftBots

    run_directory : string
        Where to save

    run_name : string
        Experiment name for files

    max_eval_time : int
        How long to run physical simulation per ind in pop

    time_to_try_again : int
        How long to wait until relaunching remaining unevaluated (crashed?) simulations

    save_lineages : bool
        Save the vxa of every ancestor of the surviving individual

    batch_size : int
        How many voxelyze processes start at once

    """
    start_time = time.time()
    num_evaluated_this_gen = 0
    ids_to_analyze = []

    for n, ind in enumerate(pop):

        # write the phenotype of a SoftBot to a file so that VoxCad can access for sim.
        ind.md5 = write_voxelyze_file(sim, env, ind, run_directory, run_name)

        # don't evaluate if invalid
        if not ind.phenotype.is_valid():
            for rank, goal in pop.objective_dict.items():
                if goal["name"] != "age":
                    setattr(ind, goal["name"], goal["worst_value"])
            print_log.message("Skipping invalid individual")

        # don't evaluate if identical phenotype has already been evaluated
        elif env.actuation_variance == 0 and ind.md5 in pop.already_evaluated:

            if not ind.previously_aggregated:  # don't overwrite the fitness for aggregations of multiple trials

                for rank, goal in pop.objective_dict.items():
                    if goal["tag"] is not None:
                        setattr(ind, goal["name"], pop.already_evaluated[ind.md5][rank])
                # print_log.message("Individual already evaluated:  cached fitness is {}".format(ind.fitness))

            if pop.gen % save_vxa_every == 0 and save_vxa_every > 0:
                sub.call("cp " + run_directory + "/voxelyzeFiles/" + run_name + "--id_%05i.vxa" % ind.id +
                         " " + run_directory + "/Gen_%04i/" % pop.gen + run_name +
                         "--Gen_%04i--fit_%.08f--id_%05i.vxa" % (pop.gen, ind.fitness, ind.id), shell=True)

        # ##############################################################################################################
        # # todo: there might be cases (generalize this!) in which we don't want to waste time evaluating in simulation
        # elif pop.learning_trials > 0 and \
        #         get_depths_of_material_from_shell(
        #             ind.genotype.to_phenotype_mapping['material']['state'], mat=8)[4] > 0:
        #     for rank, goal in pop.objective_dict.items():
        #         if goal["name"] != "age":
        #             setattr(ind, goal["name"], goal["default_value"])
        # ##############################################################################################################

        # otherwise evaluate with voxelyze
        else:
            num_evaluated_this_gen += 1
            pop.total_evaluations += 1
            ids_to_analyze += [ind.id]

            sub.Popen("./voxelyze  -f " + run_directory + "/voxelyzeFiles/" + run_name + "--id_%05i.vxa" % ind.id,
                      shell=True)

        # # pause if using mini-batches
        # batch_start_time = time.time()
        # if batch_size < pop.pop_size:
        #     if (n+1) % batch_size == 0:
        #         done_with_batch = False
        #         while not done_with_batch:
        #             ls_check = sub.check_output(["ls", run_directory + "/fitnessFiles/"])
        #             if len(ls_check.split()) == n+1:
        #                 done_with_batch = True
        #
        #             elif time.time() - batch_start_time > time_to_try_again*batch_size:
        #                 done_with_batch = True  # resend this robot later
        #
        #             else:
        #                 time.sleep(0.5)

    print_log.message("Launched {0} voxelyze calls, out of {1} individuals".format(num_evaluated_this_gen, len(pop)))

    num_evals_finished = 0
    all_done = False
    already_analyzed_ids = []
    redo_attempts = 1

    fitness_eval_start_time = time.time()

    if num_evaluated_this_gen == 0:
        all_done = True

    # if batch_size < pop.pop_size:
    #     fitness_eval_start_time = batch_start_time  # use timer from just before batch started

    while not all_done:

        time_waiting_for_fitness = time.time() - fitness_eval_start_time
        # this protects against getting stuck when voxelyze doesn't return a fitness value
        # (diverging simulations, crash, error reading .vxa)

        if time_waiting_for_fitness > pop.pop_size * max_eval_time:
            # TODO ** WARNING: This could in fact alter the sim and undermine the reproducibility **
            all_done = False  # something bad with this individual, probably sim diverged
            break

        if time_waiting_for_fitness > pop.pop_size * time_to_try_again * redo_attempts:
            # try to redo any simulations that crashed
            redo_attempts += 1
            non_analyzed_ids = [idx for idx in ids_to_analyze if idx not in already_analyzed_ids]
            print "Rerunning voxelyze for: ", non_analyzed_ids
            for idx in non_analyzed_ids:
                sub.Popen("./voxelyze  -f " + run_directory + "/voxelyzeFiles/" + run_name + "--id_%05i.vxa" % idx,
                          shell=True)

        # check to see if all are finished
        all_done = True
        for ind in pop:
            if ind.phenotype.is_valid() and ind.fitness == pop.objective_dict[0]["worst_value"]:
                all_done = False

        # check for any fitness files that are present
        ls_check = sub.check_output(["ls", run_directory + "/fitnessFiles/"])
        # duplicated ids issue: may be due to entering here two times for the same fitness file found in the directory.

        if ls_check:
            # ls_check = random.choice(ls_check.split())  # doesn't accomplish anything and undermines reproducibility
            ls_check = ls_check.split()[0]
            if "softbotsOutput--id_" in ls_check:
                this_id = int(ls_check.split("_")[1].split(".")[0])

                if this_id in already_analyzed_ids:
                    # workaround to avoid any duplicated ids when restarting sims
                    print_log.message("Duplicate voxelyze results found from THIS gen with id {}".format(this_id))
                    sub.call("rm " + run_directory + "/fitnessFiles/" + ls_check, shell=True)

                elif this_id in pop.all_evaluated_individuals_ids and pop.learning_trials <= 1:
                    print_log.message("Duplicate voxelyze results found from PREVIOUS gen with id {}".format(this_id))
                    sub.call("rm " + run_directory + "/fitnessFiles/" + ls_check, shell=True)

                else:
                    num_evals_finished += 1
                    already_analyzed_ids.append(this_id)

                    ind_filename = run_directory + "/fitnessFiles/" + ls_check
                    objective_values_dict = read_voxlyze_results(pop, print_log, ind_filename)

                    print_log.message("{0} fit = {1} ({2} / {3})".format(ls_check, objective_values_dict[0],
                                                                         num_evals_finished,
                                                                         num_evaluated_this_gen))

                    # now that we've read the fitness file, we can remove it
                    sub.call("rm " + run_directory + "/fitnessFiles/" + ls_check, shell=True)

                    # assign the values to the corresponding individual
                    for ind in pop:
                        if ind.id == this_id:
                            for rank, details in pop.objective_dict.items():
                                if objective_values_dict[rank] is not None:
                                    this_value = objective_values_dict[rank]
                                    if details["meta_func"] is not None:
                                        this_value = details["meta_func"](this_value, ind)
                                    setattr(ind, details["name"], this_value)
                                else:
                                    # for network in ind.genotype:
                                    #     for name in network.output_node_names:
                                    #         if name == details["output_node_name"]:
                                    #             print "here!"
                                    #             # apply the specified function to the specified output node
                                    #             state = network.graph.node[name]["state"]
                                    #             setattr(ind, details["name"], details["node_func"](state))
                                    for name, details_phenotype in ind.genotype.to_phenotype_mapping.items():
                                        if name == details["output_node_name"]:
                                            state = details_phenotype["state"]
                                            setattr(ind, details["name"], details["node_func"](state))

                            pop.already_evaluated[ind.md5] = [getattr(ind, details["name"])
                                                              for rank, details in
                                                              pop.objective_dict.items()]
                            pop.all_evaluated_individuals_ids += [this_id]

                            # update the run statistics and file management
                            new_run_champ = False
                            if pop.objective_dict[0]["maximize"]:
                                if ind.fitness > pop.best_fit_so_far:
                                    new_run_champ = True
                            elif ind.fitness < pop.best_fit_so_far:
                                new_run_champ = True

                            if new_run_champ:
                                pop.best_fit_so_far = ind.fitness
                                sub.call("cp " + run_directory + "/voxelyzeFiles/" + run_name + "--id_%05i.vxa" %
                                         ind.id + " " + run_directory + "/bestSoFar/fitOnly/" + run_name +
                                         "--Gen_%04i--fit_%.08f--id_%05i.vxa" %
                                         (pop.gen, ind.fitness, ind.id), shell=True)

                            if save_lineages:
                                sub.call("cp " + run_directory + "/voxelyzeFiles/" + run_name + "--id_%05i.vxa" %
                                         ind.id + " " + run_directory + "/ancestors/", shell=True)

                            if pop.gen % save_vxa_every == 0 and save_vxa_every > 0:
                                sub.call("mv " + run_directory + "/voxelyzeFiles/" + run_name + "--id_%05i.vxa" %
                                         ind.id + " " + run_directory + "/Gen_%04i/" % pop.gen +
                                         run_name + "--Gen_%04i--fit_%.08f--id_%05i.vxa" %
                                         (pop.gen, ind.fitness, ind.id), shell=True)
                            else:
                                sub.call("rm " + run_directory + "/voxelyzeFiles/" + run_name + "--id_%05i.vxa" %
                                         ind.id, shell=True)

                            break

            # wait a second and try again
            else:
                time.sleep(0.5)
        else:
            time.sleep(0.5)

    if not all_done:
        print_log.message("WARNING: Couldn't get a fitness value in time for some individuals. "
                          "The min fitness was assigned for these individuals")

    print_log.message("\nAll Voxelyze evals finished in {} seconds".format(time.time() - start_time))
    print_log.message("num_evaluated_this_gen: {0}".format(num_evaluated_this_gen))
    print_log.message("total_evaluations: {}".format(pop.total_evaluations))
