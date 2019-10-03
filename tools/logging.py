import copy
import time
import sys
import networkx as nx
import subprocess as sub
import numpy as np
from glob import glob

from tools.utils import find_between


# TODO: double tabs to csv?
# TODO: save networks and pareto front functions are non operational


def time_stamp():
    # return time.strftime("[%Y/%m/%d-%H:%M:%S]")
    return ""


class PrintLog(object):
    def __init__(self):
        self.timers = {"start": time.time(), "last_call": time.time()}

    def add_timer(self, name):
        assert name not in self.timers
        self.timers[name] = time.time()

    def reset_timer(self, name):
        assert name in self.timers
        if name != "start":
            self.timers[name] = time.time()

    def seconds_from(self, timer_name):
        return time.time() - self.timers[timer_name]

    def message(self, content, timer_name=None, reset=True):
        if timer_name is None:
            print(time_stamp() + ' ' + content)
        else:
            s = self.seconds_from(timer_name)
            m, h = s / 60.0, s / 3600.0
            print(time_stamp() + ' ' + content + ' \t (time from ' + timer_name + ': %0.2fs %0.2fm %0.2fh)' % (s, m, h))
            if reset:
                self.reset_timer(timer_name)
        sys.stdout.flush()
        sys.stderr.flush()


def make_header(population, path):
    _file = open(path, 'w')
    # header_string = "gen\t\tid\t\tage\t\ttype"

    # # columns for objectives
    # for rank in range(len(population.objective_dict)):
    #     objective = population.objective_dict[rank]
    #     header_string += "\t\t" + objective["name"] + "\t\t" + "parent_" + objective["name"]

    header_string = "gen\t\tid\t\tdom\t\tparent_id\t\tvariation_type"

    # columns for objectives
    for rank in range(len(population.objective_dict)):
        objective = population.objective_dict[rank]
        header_string += "\t\t{}".format(objective["name"]) + "\t\t{}".format("parent_"+objective["name"])

    # columns for network outputs
    ind = population[0]
    output_names = ind.genotype.all_networks_outputs
    output_names.sort()
    # for name in output_names:
    for name, details in ind.genotype.to_phenotype_mapping.items():
        # details = ind.genotype.to_phenotype_mapping[name]
        if details["logging_stats"] is not None:
            header_string += "\t\t" + name + "_different_from_parent"
            for stat in details["logging_stats"]:
                header_string += "\t\t" + stat.__name__ + "_" + name
                header_string += "\t\t" + stat.__name__ + "_parent_" + name
                header_string += "\t\t" + stat.__name__ + "_parent_diff_" + name

    # hard coded devo window bc no way (yet) to implement stats that depend on multiple networks
    devo_params = [name.split("final_")[1] for name in ind.genotype.all_networks_outputs if "final" in name]
    for name in devo_params:
        header_string += "\t\t" + "window_" + name
        header_string += "\t\t" + "window_parent_" + name
        header_string += "\t\t" + "window_parent_diff_" + name

    # let's just track the entire network
    for name in devo_params:
        header_string += "\t\t" + "init_" + name
        header_string += "\t\t" + "final_" + name

    _file.write(header_string + "\n")
    _file.close()


def record_individuals_data(pop, path, num_inds_to_save=None, print_to_terminal=False):

    if num_inds_to_save is None:
        num_inds_to_save = len(pop)

    pop.sort_by_objectives()

    if print_to_terminal:
        header_string = "gen\t\tid\t\tdom"

        # columns for objectives
        for rank in range(len(pop.objective_dict)):
            objective = pop.objective_dict[rank]
            header_string += "\t\t{:15}".format(objective["name"]) + "\t\t{:15}".format("parent_"+objective["name"])
        header_string += "\t\tparent_id"
        header_string += "\t\tvariation_type"

        print "\n"+header_string

    output_names = pop[0].genotype.all_networks_outputs
    output_names.sort()

    recording_file = open(path, 'a')
    n = 0
    while n < num_inds_to_save and n < len(pop):
        ind = pop[n]

        objectives_string = ""
        objectives_string_print = ""

        # objectives
        for rank in range(len(pop.objective_dict)):
            objective = pop.objective_dict[rank]
            # objectives_string += str(getattr(ind, objective["name"])) + "\t\t" + \
            #                      str(getattr(ind, "parent_{}".format(objective["name"]))) + "\t\t"
            objectives_string += "{}\t\t".format(getattr(ind, objective["name"])) + \
                                 "{}\t\t".format(getattr(ind, "parent_{}".format(objective["name"])))
            objectives_string_print += "{:15}\t\t".format(str(getattr(ind, objective["name"]))) + \
                                       "{:15}\t\t".format(str(getattr(ind, "parent_{}".format(objective["name"]))))

        # network outputs
        # for name in output_names:
        #     details = ind.genotype.to_phenotype_mapping[name]
        for name, details in ind.genotype.to_phenotype_mapping.items():
            if details["logging_stats"] is not None:
                for network, parent_network in zip(ind.genotype, ind.parent_genotype):
                    if name in network.output_node_names:
                        if not network.direct_encoding:
                            state = network.graph.node[name]["state"]
                            parent_state = parent_network.graph.node[name]["state"]
                        else:
                            state = network.values
                            parent_state = parent_network.values
                        diff = state - parent_state
                        any_changes = np.any(state != parent_state)
                        objectives_string += "{}\t\t".format(any_changes)
                        for stat in details["logging_stats"]:
                            objectives_string += "{}\t\t".format(stat(state))
                            objectives_string += "{}\t\t".format(stat(parent_state))
                            objectives_string += "{}\t\t".format(stat(diff))

        # recording_file.write(str(pop.gen) + "\t\t" + str(ind.id) + "\t\t" + str(ind.age) + "\t\t" +
        #                      ind.variation_type + objectives_string + "\n")

        # # hard coded devo window bc no way (yet) to implement stats that depend on multiple networks
        # devo_params = [name.split("final_")[1] for name in ind.genotype.all_networks_outputs if "final" in name]
        # for param in devo_params:
        #     init_state = 0
        #     final_state = 0
        #     parent_init_state = 0
        #     parent_final_state = 0
        #     for network, parent_network in zip(ind.genotype, ind.parent_genotype):
        #         if "init_"+param in network.output_node_names:
        #             init_state = network.values
        #
        #         if "final_"+param in network.output_node_names:
        #             final_state = network.values
        #
        #         if "init_"+param in parent_network.output_node_names:
        #             parent_init_state = parent_network.values
        #
        #         if "final_"+param in parent_network.output_node_names:
        #             parent_final_state = parent_network.values
        #
        #     window = np.sum(np.abs(init_state-final_state))
        #     parent_window = np.sum(np.abs(parent_init_state - parent_final_state))
        #     diff = window - parent_window
        #     objectives_string += "{}\t\t".format(window)
        #     objectives_string += "{}\t\t".format(parent_window)
        #     objectives_string += "{}\t\t".format(diff)
        #
        # # hard coded (again) we are writing the entire network
        # for param in devo_params:
        #     for net in ind.genotype:
        #         if "init_"+param in net.output_node_names:
        #             init_net = list(net.values.flatten())
        #
        #         if "final_"+param in net.output_node_names:
        #             final_net = list(net.values.flatten())
        #
        #     objectives_string += "{}\t\t".format(init_net)
        #     objectives_string += "{}\t\t".format(final_net)

        recording_file.write("{}\t\t".format(int(pop.gen)) +
                             "{}\t\t".format(int(ind.id)) +
                             "{}\t\t".format(int(len(ind.dominated_by))) +
                             "{}\t\t".format(int(ind.parent_id)) +
                             ind.variation_type + "\t\t" +
                             objectives_string + "\n")

        if print_to_terminal:
            print("{:5d}\t".format(int(pop.gen)) +
                  "{:5d}\t".format(int(ind.id)) +
                  "{:5d}\t".format(int(len(ind.dominated_by))) +
                  objectives_string_print +
                  "{:5d}\t\t".format(int(ind.parent_id)) +
                  ind.variation_type)

        n += 1
    if print_to_terminal:
        print

    recording_file.close()


def initialize_folders(population, run_directory, run_name, save_networks, save_all_individual_data=True,
                       save_lineages=True):

    # sub.call("mkdir " + run_directory + "/" + run_name + "/" + " 2>/dev/null", shell=True)
    ret = sub.call("mkdir " + run_directory + "/" + " 2>/dev/null", shell=True)
    if ret != 0:
        response = raw_input("****************************************************\n"
                             "** WARNING ** A directory named " + run_directory +
                             " may exist already and would be erased.\n ARE YOU SURE YOU WANT TO CONTINUE? (y/n): ")
        if not (("Y" in response) or ("y" in response)):
            quit("Please change run name with -n DifferentName. Quitting.\n"
                 "****************************************************\n\n")
        else:
            print "****************************************************\n"

    # clear directory
    sub.call("rm -rf " + run_directory + "/* 2>/dev/null", shell=True)

    sub.call("mkdir " + run_directory + "/voxelyzeFiles 2> /dev/null", shell=True)
    sub.call("mkdir " + run_directory + "/tempFiles 2> /dev/null", shell=True)
    sub.call("mkdir " + run_directory + "/fitnessFiles 2> /dev/null", shell=True)

    sub.call("mkdir " + run_directory + "/bestSoFar 2> /dev/null", shell=True)
    sub.call("mkdir " + run_directory + "/bestSoFar/paretoFronts 2> /dev/null", shell=True)
    sub.call("mkdir " + run_directory + "/bestSoFar/fitOnly 2>/dev/null", shell=True)

    sub.call("mkdir " + run_directory + "/pickledPops 2> /dev/null", shell=True)

    champ_file = run_directory + "/bestSoFar/bestOfGen.txt"
    make_header(population, champ_file)

    if save_all_individual_data:
        sub.call("mkdir " + run_directory + "/allIndividualsData", shell=True)
        sub.call("rm -f " + run_directory + "/allIndividualsData/* 2>/dev/null", shell=True)  # TODO: why clear these

    if save_networks:
        sub.call("mkdir " + run_directory + "/network_gml", shell=True)
        sub.call("rm -rf " + run_directory + "/network_gml/* 2>/dev/null", shell=True)

    if save_lineages:
        sub.call("mkdir " + run_directory + "/ancestors 2> /dev/null", shell=True)


def make_gen_directories(population, run_directory, save_vxa_every, save_networks):

    print "\n\n"
    print "----------------------------------"
    print "---------- GENERATION", population.gen, "----------"
    print "----------------------------------"
    print "\n"

    if population.gen % save_vxa_every == 0 and save_vxa_every > 0:
        sub.call("mkdir " + run_directory + "/Gen_%04i" % population.gen, shell=True)

    if save_networks:
        sub.call("mkdir " + run_directory + "/network_gml/Gen_%04i" % population.gen, shell=True)


def write_gen_stats(population, run_directory, run_name, save_vxa_every, save_pareto, save_networks,
                    save_all_individual_data=True, num_inds_to_save=None, save_lineages=False):

    write_champ_file(population, run_directory)

    if save_all_individual_data:
        write_gen_individuals_data(population, run_directory, num_inds_to_save)

    if save_lineages:  # must be performed every generation
        # each vxa is saved during evaluation in the ancestors folder
        remove_old_lineages(population, run_directory)

    if population.gen % save_vxa_every == 0 and save_vxa_every > 0 and save_networks:
        write_networks(population, run_directory)

    if population.gen % save_vxa_every == 0 and save_vxa_every > 0 and save_pareto:
        write_pareto_front(population, run_directory, run_name)

    sub.call("rm " + run_directory + "/voxelyzeFiles/* 2>/dev/null", shell=True)  # clear the voxelyzeFiles folder


def write_champ_file(population, run_directory):
    champ_file = run_directory + "/bestSoFar/bestOfGen.txt"
    record_individuals_data(population, champ_file, 1)


def write_gen_individuals_data(population, run_directory, num_inds_to_save):
    gen_file = run_directory + "/allIndividualsData/Gen_%04i.txt" % population.gen
    make_header(population, gen_file)
    record_individuals_data(population, gen_file, num_inds_to_save, print_to_terminal=True)


def remove_old_lineages(population, run_directory):
    population.update_lineages()
    print " Length of best lineage: {}".format(len(population.lineage_dict[population[0].id]))

    ancestors_ids = [ind.id for ind in population]  # include current generation
    for child, lineage in population.lineage_dict.items():
        for parent in lineage:
            if parent not in ancestors_ids:
                ancestors_ids += [parent]

    for vxa in glob(run_directory + "/ancestors/*"):
        this_id = int(find_between(vxa, "--id_", ".vxa"))
        if this_id not in ancestors_ids:
            sub.call("rm " + vxa, shell=True)


def write_pareto_front(population, run_directory, run_name):
    """Save the vxa of all individuals with min dominance number (top list)."""
    sub.call("mkdir " + run_directory + "/bestSoFar/paretoFronts/Gen_%04i" % population.gen, shell=True)
    ind = population[0]  # first individual
    for individual in population:
        if len(individual.dominated_by) == 0:
            sub.call("mv " + run_directory + "/voxelyzeFiles/" + run_name + "--id_%05i.vxa" % individual.id +
                     " " + run_directory + "/bestSoFar/paretoFronts/Gen_%04i/" % population.gen + "/" +
                     run_name + "Gen_%04i--Fit_%.08f--id_%05i--dom_%d.vxa" %
                     (population.gen, individual.fitness, individual.id, len(individual.dominated_by)), shell=True)
        else:
            break


def write_networks(population, run_directory):
    for individual in population:
        clone = copy.deepcopy(individual)
        net_idx = 0
        for network in clone.genotype:
            for name in network.graph.nodes():
                network.graph.node[name]["state"] = ""  # remove state information to reduce file size
                network.graph.node[name]["evaluated"] = 0
            nx.write_gml(network.graph, run_directory +
                         "/network_gml/Gen_%04i/network--%02i--fit_%.08f--id_%05i" %
                         (population.gen, net_idx, clone.fitness, clone.id) + ".txt")
            net_idx += 1
