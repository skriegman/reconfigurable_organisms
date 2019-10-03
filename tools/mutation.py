import numpy as np
import random
import copy
import inspect


def create_new_children_through_mutation(pop, print_log, new_children=None, mutate_network_probs=None,
                                         prob_generating_func=None, max_mutation_attempts=1500):
    """Create copies, with modification, of existing individuals in the population.

    Parameters
    ----------
    pop : Population class
        This provides the individuals to mutate.

    print_log : PrintLog()
        For logging

    new_children : a list of new children created outside this function (may be empty)
        This is useful if creating new children through multiple functions, e.g. Crossover and Mutation.

    mutate_network_probs : probability, float between 0 and 1 (inclusive)
        The probability of mutating each network.

    prob_generating_func : func
        Used to recalculate the mutate_network_probs for each individual

    max_mutation_attempts : int
        Maximum number of invalid mutation attempts to allow before giving up on mutating a particular individual.

    Returns
    -------
    new_children : list
        A list of new individual SoftBots.

    """
    if new_children is None:
        new_children = []

    random.shuffle(pop.individuals)

    repeats = max(pop.learning_trials, 1)

    while len(new_children) < pop.pop_size*repeats:
        for ind in pop:

            clone = copy.deepcopy(ind)

            if prob_generating_func is not None:
                mutate_network_probs = prob_generating_func()

            if mutate_network_probs is None:
                required = 0
            else:
                required = mutate_network_probs.count(1)

            selection = []
            if mutate_network_probs is None:
                # uniformly select networks
                selection = np.random.random(len(clone.genotype)) < 1 / float(len(clone.genotype))
            else:
                # use probability distribution
                selection = np.random.random(len(clone.genotype)) < mutate_network_probs

            # don't select any frozen networks (used to freeze aspects of genotype during evolution)
            for idx in range(len(selection)):
                if clone.genotype[idx].freeze or clone.genotype[idx].switch:  # also don't select a switch
                    selection[idx] = False

            # if none selected, choose one
            if np.sum(selection) <= required:
                order = np.random.permutation(range(len(selection)))
                for idx in order:
                    if not clone.genotype[idx].freeze and not clone.genotype[idx].switch:
                        selection[idx] = True
                        break
            # it's possible that none are selected if using learning trials and the only unfrozen nets are also switches

            selected_networks = np.arange(len(clone.genotype))[selection].tolist()

            for rank, goal in pop.objective_dict.items():
                setattr(clone, "parent_{}".format(goal["name"]), getattr(clone, goal["name"]))

            clone.parent_genotype = ind.genotype
            clone.parent_id = clone.id
            # sam: moved the id increment to end for learning trials
            # clone.id = pop.max_id
            # pop.max_id += 1

            # for network in clone.genotype:
            #     for node_name in network.graph:
            #         network.graph.node[node_name]["old_state"] = network.graph.node[node_name]["state"]

            for name, details in clone.genotype.to_phenotype_mapping.items():
                details["old_state"] = copy.deepcopy(details["state"])

            # old_individual = copy.deepcopy(clone)

            for selected_net_idx in selected_networks:
                mutation_counter = 0
                done = False
                while not done:
                    mutation_counter += 1
                    candidate = copy.deepcopy(clone)

                    # perform mutation(s)
                    for _ in range(candidate.genotype[selected_net_idx].num_consecutive_mutations):
                        if not clone.genotype[selected_net_idx].direct_encoding:
                            # using CPPNs
                            mut_func_args = inspect.getargspec(candidate.genotype[selected_net_idx].mutate)
                            mut_func_args = [0 for _ in range(1, len(mut_func_args.args))]  # this is correct.
                            choice = random.choice(range(len(mut_func_args)))
                            mut_func_args[choice] = 1
                            variation_type, variation_degree = candidate.genotype[selected_net_idx].mutate(*mut_func_args)
                        else:
                            # direct encoding with possibility of evolving mutation rate
                            # TODO: enable cppn mutation rate evolution
                            rate = None
                            for net in clone.genotype:
                                if "mutation_rate" in net.output_node_names:
                                    rate = net.values  # evolved mutation rates, one for each voxel
                            if "mutation_rate" not in candidate.genotype[selected_net_idx].output_node_names:
                                # use evolved mutation rates
                                variation_type, variation_degree = candidate.genotype[selected_net_idx].mutate(rate)
                            else:
                                # this is the mutation rate itself (use predefined meta-mutation rate)
                                variation_type, variation_degree = candidate.genotype[selected_net_idx].mutate()

                    if variation_degree != "":
                        candidate.variation_type = "{0}({1})".format(variation_type, variation_degree)
                    else:
                        candidate.variation_type = str(variation_type)
                    candidate.genotype.express()

                    if candidate.genotype[selected_net_idx].allow_neutral_mutations:
                        done = True
                        clone = copy.deepcopy(candidate)  # SAM: ensures change is made to every net
                        break
                    else:
                        for name, details in candidate.genotype.to_phenotype_mapping.items():
                            new = details["state"]
                            old = details["old_state"]
                            changes = np.array(new != old, dtype=np.bool)
                            if np.any(changes) and candidate.phenotype.is_valid():
                                done = True
                                clone = copy.deepcopy(candidate)  # SAM: ensures change is made to every net
                                break
                        # for name, details in candidate.genotype.to_phenotype_mapping.items():
                        #     if np.sum( details["old_state"] != details["state"] ) and candidate.phenotype.is_valid():
                        #         done = True
                        #         break

                    if mutation_counter > max_mutation_attempts:
                        print_log.message("Couldn't find a successful mutation in {} attempts! "
                                          "Skipping this network.".format(max_mutation_attempts))
                        num_edges = len(clone.genotype[selected_net_idx].graph.edges())
                        num_nodes = len(clone.genotype[selected_net_idx].graph.nodes())
                        print_log.message("num edges: {0}; num nodes {1}".format(num_edges, num_nodes))
                        break

                # end while

                if not clone.genotype[selected_net_idx].direct_encoding:
                    for output_node in clone.genotype[selected_net_idx].output_node_names:
                        clone.genotype[selected_net_idx].graph.node[output_node]["old_state"] = ""

            # reset all objectives we calculate in VoxCad to unevaluated values
            for rank, goal in pop.objective_dict.items():
                if goal["tag"] is not None:
                    setattr(clone, goal["name"], goal["worst_value"])

            if pop.learning_trials <= 1:  # default is zero but one is equivalent for now
                clone.id = pop.max_id
                pop.max_id += 1
                new_children.append(clone)

            else:
                clone.learning_id = clone.id

                for this_net in clone.genotype:
                    if this_net.switch and not this_net.freeze:
                        this_net.mutate()

                new_children += pop.get_learning_trials_for_single_ind(clone)

                # this_learning_id = int(pop.max_id)
                #
                # for trial in range(pop.learning_trials):
                #     learner = copy.deepcopy(clone)
                #     learner.md5 = learner.learning_md5[trial]
                #     learner.learning_id = this_learning_id  # same for all trials with this genotype
                #     learner.id = pop.max_id
                #     pop.max_id += 1
                #
                #     for this_net in learner.genotype:
                #         if this_net.switch:
                #             this_morphology = pop.learning_morphology[trial]
                #             this_input = pop.learning_data[trial]
                #             this_net.update(this_morphology, this_input)
                #
                #     learner.genotype.express()
                #     new_children.append(learner)

        # SAM: random individuals are now added in algorithms.py after mutation
        # while len(new_children) < spots_to_fill:
        #     valid = False
        #     while not valid:
        #         ind = softbot_class(pop.max_id, pop.objective_dict, pop.genotype, pop.phenotype)
        #         if ind.phenotype.is_valid():
        #             new_children.append(ind)
        #             pop.max_id += 1
        #             valid = True

    return new_children


def genome_wide_mutation(pop, print_log):
    mutate_network_probs = [1 for _ in range(len(pop[0].genotype))]
    return create_new_children_through_mutation(pop, print_log, mutate_network_probs=mutate_network_probs)

