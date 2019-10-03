import random
import math


def pareto_selection(population):
    """Return a list of selected individuals from the population.

    All individuals in the population are ranked by their level, i.e. the number of solutions they are dominated by.
    Individuals are added to a list based on their ranking, best to worst, until the list size reaches the target
    population size (population.pop_size).

    Parameters
    ----------
    population : Population
        This provides the individuals for selection.

    Returns
    -------
    new_population : list
        A list of selected individuals.

    """
    new_population = []

    # SAM: moved this into calc_dominance()
    # population.sort(key="id", reverse=False) # <- if tied on all objectives, give preference to newer individual

    # if multiple learning trials then reduce all trials to a single individual
    if population.learning_trials > 1:
        population.aggregate_learning_trials()

    # (re)compute dominance for each individual
    population.calc_dominance()

    # sort the population multiple times by objective importance
    population.sort_by_objectives()

    # divide individuals into "pareto levels":
    # pareto level 0: individuals that are not dominated,
    # pareto level 1: individuals dominated one other individual, etc.
    done = False
    pareto_level = 0
    while not done:
        this_level = []
        size_left = population.pop_size - len(new_population)
        for ind in population:
            if len(ind.dominated_by) == pareto_level:
                this_level += [ind]

        # add best individuals to the new population.
        # add the best pareto levels first until it is not possible to fit them in the new_population
        if len(this_level) > 0:
            if size_left >= len(this_level):  # if whole pareto level can fit, add it
                new_population += this_level

            else:  # otherwise, select by sorted ranking within the level
                new_population += [this_level[0]]
                while len(new_population) < population.pop_size:
                    random_num = random.random()
                    log_level_length = math.log(len(this_level))
                    for i in range(1, len(this_level)):
                        if math.log(i) / log_level_length <= random_num < math.log(i + 1) / log_level_length and \
                                        this_level[i] not in new_population:
                            new_population += [this_level[i]]
                            continue

        pareto_level += 1
        if len(new_population) == population.pop_size:
            done = True

    for ind in population:
        if ind in new_population:
            ind.selected = 1
        else:
            ind.selected = 0

    return new_population


def pareto_tournament_selection(population):
    """Reduce the population pairwise.

    Two individuals from the population are randomly sampled and the inferior individual is removed from the population.
    This process repeats until the population size is reduced to either the target population size (population.pop_size)
    or the number of non-dominated individuals / Pareto front (population.non_dominated_size).

    Parameters
    ----------
    population : Population
        This provides the individuals for selection.

    Returns
    -------
    new_population : list
        A list of selected individuals.

    """
    # population.add_random_individual()  # adding in random ind in algorithms.py
    population.calc_dominance()
    random.shuffle(population.individuals)
    print "The nondominated size is", population.non_dominated_size

    while len(population) > population.pop_size and len(population) > population.non_dominated_size:

        inds = random.sample(range(len(population)), 2)
        ind0 = population[inds[0]]
        ind1 = population[inds[1]]

        if population.dominated_in_multiple_objectives(ind0, ind1):
            print "(fit) {0} dominated by {1}".format(ind0.fitness, ind1.fitness)
            print "(age) {0} dominated by {1}".format(ind0.age, ind1.age)
            population.pop(inds[0])
        elif population.dominated_in_multiple_objectives(ind1, ind0):
            print "(fit) {1} dominated by {0}".format(ind0.fitness, ind1.fitness)
            print "(age) {1} dominated by {0}".format(ind0.age, ind1.age)
            population.pop(inds[1])
        # else:
        #     population.pop(random.choice(inds))

    population.sort_by_objectives()

    return population.individuals


def parallel_hill_climber(population):
    """Children compete with their parents only."""
    new_population = []
    population.calc_dominance()
    population.sort_by_objectives()

    robots_to_check = [ind for ind in population]

    while len(robots_to_check) > 0:
        this_id, parent_id = robots_to_check[0].id, robots_to_check[0].parent_id

        for n, other_bot in enumerate(robots_to_check):

            if (other_bot.parent_id == this_id) or (other_bot.id == parent_id):

                if other_bot.fitness > robots_to_check[0]:
                    new_population += [other_bot]
                else:
                    new_population += [robots_to_check[0]]

                robots_to_check.pop(0)
                robots_to_check.pop(n)

                break

    for ind in population:
        if ind in new_population:
            ind.selected = 1
        else:
            ind.selected = 0

    return new_population

