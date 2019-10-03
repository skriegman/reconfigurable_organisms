import random
import numpy as np
import networkx as nx
from networkx import DiGraph
from copy import deepcopy
from collections import OrderedDict

from tools.utils import neg_abs, neg_square, sqrt_abs, neg_sqrt_abs, normalize, sigmoid, vox_xyz_from_id, add_patch


class OrderedGraph(DiGraph):
    """Create a graph object that tracks the order nodes and their neighbors are added."""
    node_dict_factory = OrderedDict
    adjlist_dict_factory = OrderedDict


class Network(object):
    """Base class for networks."""

    input_node_names = []

    def __init__(self, output_node_names):
        self.output_node_names = output_node_names
        self.graph = OrderedGraph()  # preserving order is necessary for checkpointing
        self.freeze = False
        self.allow_neutral_mutations = False
        self.num_consecutive_mutations = 1
        self.switch = False
        self.direct_encoding = False

    def __deepcopy__(self, memo):
        """Override deepcopy to apply to class level attributes"""
        cls = self.__class__
        new = cls.__new__(cls)
        new.__dict__.update(deepcopy(self.__dict__, memo))
        return new

    def set_input_node_states(self, *args, **kwargs):
        raise NotImplementedError

    def mutate(self, *args, **kwargs):
        raise NotImplementedError


class CPPN(Network):
    """A Compositional Pattern Producing Network"""

    input_node_names = ['x', 'y', 'z', 'd', 'b']
    activation_functions = [np.sin, np.abs, neg_abs, np.square, neg_square, sqrt_abs, neg_sqrt_abs]

    def __init__(self, output_node_names):
        Network.__init__(self, output_node_names)
        self.set_minimal_graph()
        self.mutate()

    def set_minimal_graph(self):
        """Create a simple graph with each input attached to each output"""
        for name in self.input_node_names:
            self.graph.add_node(name, type="input", function=None)

        for name in self.output_node_names:
            self.graph.add_node(name, type="output", function=sigmoid)

        for input_node in nx.nodes(self.graph):
            if self.graph.node[input_node]["type"] == "input":
                for output_node in nx.nodes(self.graph):
                    if self.graph.node[output_node]["type"] == "output":
                        self.graph.add_edge(input_node, output_node, weight=0.0)

    def set_input_node_states(self, orig_size_xyz):
        input_x = np.zeros(orig_size_xyz)
        input_y = np.zeros(orig_size_xyz)
        input_z = np.zeros(orig_size_xyz)
        for x in range(orig_size_xyz[0]):
            for y in range(orig_size_xyz[1]):
                for z in range(orig_size_xyz[2]):
                    input_x[x, y, z] = x
                    input_y[x, y, z] = y
                    input_z[x, y, z] = z

        input_x = normalize(input_x)
        input_y = normalize(input_y)
        input_z = normalize(input_z)
        input_d = normalize(np.power(np.power(input_x, 2) + np.power(input_y, 2) + np.power(input_z, 2), 0.5))
        input_b = np.ones(orig_size_xyz)

        for name in self.graph.nodes():
            if name == "x":
                self.graph.node[name]["state"] = input_x
                self.graph.node[name]["evaluated"] = True
            if name == "y":
                self.graph.node[name]["state"] = input_y
                self.graph.node[name]["evaluated"] = True
            if name == "z":
                self.graph.node[name]["state"] = input_z
                self.graph.node[name]["evaluated"] = True
            if name == "d":
                self.graph.node[name]["state"] = input_d
                self.graph.node[name]["evaluated"] = True
            if name == "b":
                self.graph.node[name]["state"] = input_b
                self.graph.node[name]["evaluated"] = True

    def mutate(self, num_random_node_adds=10, num_random_node_removals=0, num_random_link_adds=10,
               num_random_link_removals=5, num_random_activation_functions=100, num_random_weight_changes=100):

        # TODO: control over initialization in __init__()
        # TODO: set default arg val via brute force search
        # TODO: weight std is defaulted to 0.5, to change this we can't just put it in args of mutate() b/c getargspec
        # is used in create_new_children_through_mutation() to automatically pick the mutation type.

        variation_degree = None
        variation_type = None

        for _ in range(num_random_node_adds):
            variation_degree = self.add_node()
            variation_type = "add_node"

        for _ in range(num_random_node_removals):
            variation_degree = self.remove_node()
            variation_type = "remove_node"

        for _ in range(num_random_link_adds):
            variation_degree = self.add_link()
            variation_type = "add_link"

        for _ in range(num_random_link_removals):
            variation_degree = self.remove_link()
            variation_type = "remove_link"

        for _ in range(num_random_activation_functions):
            variation_degree = self.mutate_function()
            variation_type = "mutate_function"

        for _ in range(num_random_weight_changes):
            variation_degree = self.mutate_weight()
            variation_type = "mutate_weight"

        self.prune_network()
        return variation_type, variation_degree

    ###############################################
    #   Mutation functions
    ###############################################

    def add_node(self):
        # choose two random nodes (between which a link could exist)
        if len(self.graph.edges()) == 0:
            return "NoEdges"
        this_edge = random.choice(self.graph.edges())
        node1 = this_edge[0]
        node2 = this_edge[1]

        # create a new node hanging from the previous output node
        new_node_index = self.get_max_hidden_node_index()
        self.graph.add_node(new_node_index, type="hidden", function=random.choice(self.activation_functions))
        # random activation function here to solve the problem with admissible mutations in the first generations
        self.graph.add_edge(new_node_index, node2, weight=1.0)

        # if this edge already existed here, remove it
        # but use it's weight to minimize disruption when connecting to the previous input node
        if (node1, node2) in nx.edges(self.graph):
            weight = self.graph.edge[node1][node2]["weight"]
            self.graph.remove_edge(node1, node2)
            self. graph.add_edge(node1, new_node_index, weight=weight)
        else:
            self.graph.add_edge(node1, new_node_index, weight=1.0)
            # weight 0.0 would minimize disruption of new edge
            # but weight 1.0 should help in finding admissible mutations in the first generations
        return ""

    def remove_node(self):
        hidden_nodes = list(set(self.graph.nodes()) - set(self.input_node_names) - set(self.output_node_names))
        if len(hidden_nodes) == 0:
            return "NoHiddenNodes"
        this_node = random.choice(hidden_nodes)

        # if there are edge paths going through this node, keep them connected to minimize disruption
        incoming_edges = self.graph.in_edges(nbunch=[this_node])
        outgoing_edges = self.graph.out_edges(nbunch=[this_node])

        for incoming_edge in incoming_edges:
            for outgoing_edge in outgoing_edges:
                w = self.graph.edge[incoming_edge[0]][this_node]["weight"] * \
                    self.graph.edge[this_node][outgoing_edge[1]]["weight"]
                self.graph.add_edge(incoming_edge[0], outgoing_edge[1], weight=w)

        self.graph.remove_node(this_node)
        return ""

    def add_link(self):
        done = False
        attempt = 0
        while not done:
            done = True

            # choose two random nodes (between which a link could exist, *but doesn't*)
            node1 = random.choice(self.graph.nodes())
            node2 = random.choice(self.graph.nodes())
            while (not self.new_edge_is_valid(node1, node2)) and attempt < 999:
                node1 = random.choice(self.graph.nodes())
                node2 = random.choice(self.graph.nodes())
                attempt += 1
            if attempt > 999:  # no valid edges to add found in 1000 attempts
                done = True

            # create a link between them
            if random.random() > 0.5:
                self.graph.add_edge(node1, node2, weight=0.1)
            else:
                self.graph.add_edge(node1, node2, weight=-0.1)

            # If the link creates a cyclic graph, erase it and try again
            if self.has_cycles():
                self.graph.remove_edge(node1, node2)
                done = False
                attempt += 1
            if attempt > 999:
                done = True
        return ""

    def remove_link(self):
        if len(self.graph.edges()) == 0:
            return "NoEdges"
        this_link = random.choice(self.graph.edges())
        self.graph.remove_edge(this_link[0], this_link[1])
        return ""

    def mutate_function(self):
        this_node = random.choice(self.graph.nodes())
        while this_node in self.input_node_names:
            this_node = random.choice(self.graph.nodes())
        old_function = self.graph.node[this_node]["function"]
        while self.graph.node[this_node]["function"] == old_function:
            self.graph.node[this_node]["function"] = random.choice(self.activation_functions)
        return old_function.__name__ + "-to-" + self.graph.node[this_node]["function"].__name__

    def mutate_weight(self, mutation_std=0.5):
        if len(self.graph.edges()) == 0:
            return "NoEdges"
        this_edge = random.choice(self.graph.edges())
        node1 = this_edge[0]
        node2 = this_edge[1]
        old_weight = self.graph[node1][node2]["weight"]
        new_weight = old_weight
        while old_weight == new_weight:
            new_weight = random.gauss(old_weight, mutation_std)
            new_weight = max(-1.0, min(new_weight, 1.0))
        self.graph[node1][node2]["weight"] = new_weight
        return float(new_weight - old_weight)

    ###############################################
    #   Helper functions for mutation
    ###############################################

    def prune_network(self):
        """Remove erroneous nodes and edges post mutation."""
        done = False
        while not done:
            done = True
            for node in self.graph.nodes():
                if len(self.graph.in_edges(nbunch=[node])) == 0 and \
                                node not in self.input_node_names and \
                                node not in self.output_node_names:
                    self.graph.remove_node(node)
                    done = False

            for node in self.graph.nodes():
                if len(self.graph.out_edges(nbunch=[node])) == 0 and \
                                node not in self.input_node_names and \
                                node not in self.output_node_names:
                    self.graph.remove_node(node)
                    done = False

    def has_cycles(self):
        """Return True if the graph contains simple cycles (elementary circuits).

        A simple cycle is a closed path where no node appears twice, except that the first and last node are the same.

        """
        return sum(1 for _ in nx.simple_cycles(self.graph)) != 0

    def get_max_hidden_node_index(self):
        max_index = 0
        for input_node in nx.nodes(self.graph):
            if self.graph.node[input_node]["type"] == "hidden" and int(input_node) >= max_index:
                max_index = input_node + 1
        return max_index

    def new_edge_is_valid(self, node1, node2):
        if node1 == node2:
            return False
        if self.graph.node[node1]['type'] == "output":
            return False
        if self.graph.node[node2]['type'] == "input":
            return False
        if (node2, node1) in nx.edges(self.graph):
            return False
        if (node1, node2) in nx.edges(self.graph):
            return False
        return True


class DirectEncoding(Network):
    def __init__(self, output_node_name, orig_size_xyz, lower_bound=-1, upper_bound=1, func=None, symmetric=True,
                 p=None, scale=None, start_val=None, mutate_start_val=False, allow_neutral_mutations=False,
                 sub_vox_dict=None, frozen_vox=None, patch_mode=False, vox_options=None):

        Network.__init__(self, [output_node_name])

        self.vox_options = vox_options

        self.direct_encoding = True
        self.allow_neutral_mutations = allow_neutral_mutations
        self.size = orig_size_xyz
        self.lower_bound = lower_bound
        self.upper_bound = upper_bound
        if p is None:
            p = 1/np.product(self.size, dtype='f')
        self.p = p
        self.scale = scale
        self.func = func
        self.symmetric = symmetric
        self.start_value = start_val

        self.patch_mode = patch_mode

        if sub_vox_dict is None:
            self.sub_vox_dict = dict()
        else:
            self.sub_vox_dict = sub_vox_dict

        self.mutable_vox = np.ones(shape=orig_size_xyz, dtype=bool)

        if frozen_vox is not None:
            for idx in frozen_vox:
                x, y, z = vox_xyz_from_id(idx, self.size)
                self.mutable_vox[x, y, z] = False

        if start_val is None:
            self.values = np.random.uniform(lower_bound, upper_bound, size=orig_size_xyz)
        else:
            self.values = np.ones(shape=orig_size_xyz) * start_val
            if mutate_start_val:
                self.mutate()

        if vox_options is not None:
            self.values = np.random.choice(vox_options, orig_size_xyz)

        self.enforce_symmetry()

        self.regulate_sub_voxels()

        if self.func is not None:
            self.values = self.func(self.values)

        self.values = np.clip(self.values, self.lower_bound, self.upper_bound)

    # @property
    # def values(self):
    #     return self._values
    #
    # @values.setter
    # def values(self, values):
    #     if self.func is None:
    #         self._values = values
    #     else:
    #         self._values = self.func(values)

    def set_input_node_states(self, *args, **kwargs):
        pass

    def mutate(self, rate=None):

        if self.patch_mode:
            self.values, sub_vox_dict = add_patch(self.values)

            for parent, child in sub_vox_dict.items():
                self.sub_vox_dict[parent] = [child]

            return "patched", 1

        else:
            if rate is None:
                rate = self.p

            scale = self.scale
            if self.scale is None:
                scale = np.abs(1/self.values)
                # scale = np.clip(self.values**0.5, self.start_value**0.5, self.upper_bound)
                # this was for meta mutations

            selection = np.random.random(self.size) < rate
            selection = np.logical_and(selection, self.mutable_vox)

            if self.vox_options is not None:
                change = np.random.choice(self.vox_options, self.size)
                self.values[selection] = change[selection]
            else:
                change = np.random.normal(scale=scale, size=self.size)
                self.values[selection] += change[selection]

            self.values = np.clip(self.values, self.lower_bound, self.upper_bound)

            self.enforce_symmetry()

            self.regulate_sub_voxels()

            if self.func is not None:
                self.values = self.func(self.values)

            return "gaussian", self.scale

    def enforce_symmetry(self):
        if self.symmetric:
            reversed_array = self.values[::-1, :, :]
            self.values[:int(self.size[0]/2.0), :, :] = reversed_array[:int(self.size[0]/2.0), :, :]

    def regulate_sub_voxels(self):
        if len(self.sub_vox_dict) == 0:
            return

        self.mutable_vox = np.zeros(self.size, dtype=bool)

        for parent, children in self.sub_vox_dict.items():
            px, py, pz = vox_xyz_from_id(parent, self.size)
            self.mutable_vox[px, py, pz] = True
            group_val = self.values[px, py, pz] / float(len(children))
            self.values[px, py, pz] = group_val
            for child in children:
                cx, cy, cz = vox_xyz_from_id(child, self.size)
                self.values[cx, cy, cz] = group_val


class LearningTrialsContainer(Network):

    def __init__(self, name, learning_data=None):

        Network.__init__(self, [name])
        self.direct_encoding = True
        self.values = []
        self.learning_data = learning_data

    def update(self, values, *args):
        self.values = values
        return

    def mutate(self, *args):
        raise ArithmeticError

    def set_input_node_states(self, *args, **kwargs):
        pass


class PatchNet(Network):

    def __init__(self, name, orig_geom, patch_mat, inputs, weights=None, num_hidden_neurons=0, input_bias=False,
                 hidden_bias=False, evo_strategies=False, already_patched=False):

        Network.__init__(self, [name])
        self.direct_encoding = True
        self.allow_neutral_mutations = False
        self.size = orig_geom.shape
        self.orig_geom = orig_geom
        self.values = np.array(orig_geom)
        self.patch_mat = patch_mat
        self.evo_strategies = evo_strategies
        self.already_patched = already_patched

        # assert (len(inputs) == 3)  # roll, pitch, yaw
        self.has_input_bias = input_bias
        self.has_hidden_bias = hidden_bias
        self.input_layer = inputs + [1.0]*input_bias
        self.hidden_layer = [0]*num_hidden_neurons + [1]*hidden_bias
        self.output_layer = [0]*3  # x, y, z

        self.weights = weights
        if weights is None:
            if num_hidden_neurons > 0:
                num_bias_weights = input_bias*num_hidden_neurons + hidden_bias*3
                n = (len(inputs)+3)*num_hidden_neurons + num_bias_weights
            elif evo_strategies:
                n = len(self.output_layer)
            else:
                n = 3*(len(inputs)+input_bias)
            self.weights = 2*np.random.rand(n)-1

        self.update()

    def update(self, orig_geom=None, inputs=None, loc=None):

        if loc is not None:
            self.values, _ = add_patch(orig_geom, loc=loc, mat=self.patch_mat)
            return

        if orig_geom is not None:
            self.orig_geom = orig_geom

        if self.already_patched or self.freeze:
            self.values = self.orig_geom
            return

        if inputs is not None:
            self.input_layer = inputs + [1.0]*self.has_input_bias

        # track weight index
        w = 0

        if len(self.hidden_layer) > 0:

            # hidden layer
            new_hidden_layer = [0]*(len(self.hidden_layer) - self.has_hidden_bias)
            for h in range(len(new_hidden_layer)):
                for i in range(len(self.input_layer)):
                    new_hidden_layer[h] += self.input_layer[i] * self.weights[w]
                    w += 1

            self.hidden_layer = list(np.tanh(new_hidden_layer)) + [1.0]*self.has_hidden_bias

            # output layer
            new_output_layer = [0]*len(self.output_layer)
            for o in range(len(new_output_layer)):
                for h in range(len(self.hidden_layer)):
                    new_output_layer[o] += self.hidden_layer[h] * self.weights[w]
                    w += 1

            self.output_layer = np.tanh(new_output_layer)

        elif self.evo_strategies:
            self.output_layer = self.weights

        else:  # no hidden layer input goes straight to output

            # output layer
            new_output_layer = [0]*len(self.output_layer)
            for o in range(len(new_output_layer)):
                for i in range(len(self.input_layer)):
                    new_output_layer[o] += self.input_layer[i] * self.weights[w]
                    w += 1

            self.output_layer = np.tanh(new_output_layer)

        shifted_output = 0.5*self.output_layer+0.5

        # if self.evo_strategies:
        #     w = 3
        #     shifted_output = 0.5*shifted_output+0.2
        #
        #     if self.input_layer[0] == 0:
        #         shifted_output[0] = 1-(shifted_output[0]+0.1)
        #         shifted_output[1] = 1-(shifted_output[1]+0.1)
        #
        #     if self.input_layer[0] == 1:
        #         shifted_output[0] = 1-(shifted_output[0]+0.1)
        #
        #     if self.input_layer[0] == 2:
        #         shifted_output[1] = 1-(shifted_output[1]+0.1)

        loc = [0, 0, 0]
        for i in range(3):
            loc[i] = self.size[i]*shifted_output[i]

        # new geometry: just a single patch different from original geometry
        self.values, _ = add_patch(self.orig_geom, loc=loc, mat=self.patch_mat)

        if w < len(self.weights):
            n = len(self.weights) - w
            print "only need the first {0} weights (deleting the extra {1})".format(w, n)
            self.weights = self.weights[:w]

    def mutate(self, *args):
        new = np.array(self.weights)
        selection = np.random.rand(new.size) < 1/float(new.size)
        change = np.random.normal(scale=new*new, size=new.size)
        new[selection] += change[selection]
        new = np.clip(new, -1, 1)
        self.weights = list(new)
        self.update()
        return "normal", 1

    def set_input_node_states(self, *args, **kwargs):
        pass


class PatchCppn(CPPN):
    def __init__(self, name, orig_geom, patch_mat, inputs):

        Network.__init__(self, ["x", "y", "z", name])  # dummy node "name" to work with g->p mapping

        self.input_node_names = ["roll", "pitch", "yaw"]
        assert (len(inputs) == 3)  # roll, pitch, yaw
        self.inputs = inputs

        self.direct_encoding = True
        self.allow_neutral_mutations = False
        self.size = orig_geom.shape
        self.orig_geom = orig_geom
        self.values = np.array(orig_geom)
        self.patch_mat = patch_mat

        self.set_minimal_graph()
        super(PatchCppn, self).mutate()
        self.update()

    def set_input_node_states(self, *args):
        for name in self.graph.nodes():
            if name == "roll":
                self.graph.node[name]["state"] = self.inputs[0]
                self.graph.node[name]["evaluated"] = True
            if name == "pitch":
                self.graph.node[name]["state"] = self.inputs[1]
                self.graph.node[name]["evaluated"] = True
            if name == "yaw":
                self.graph.node[name]["state"] = self.inputs[2]
                self.graph.node[name]["evaluated"] = True
            if name == "bias":
                self.graph.node[name]["state"] = 1.0
                self.graph.node[name]["evaluated"] = True

    def calc_node_state(self, node_name):
        if self.graph.node[node_name]["evaluated"]:
            return self.graph.node[node_name]["state"]

        self.graph.node[node_name]["evaluated"] = True
        input_edges = self.graph.in_edges(nbunch=[node_name])
        new_state = 0

        for edge in input_edges:
            node1, node2 = edge
            new_state += self.calc_node_state(node1) * self.graph.edge[node1][node2]["weight"]

        self.graph.node[node_name]["state"] = new_state

        return self.graph.node[node_name]["function"](new_state)

    def update(self, orig_geom=None, inputs=None):

        if orig_geom is not None:
            self.orig_geom = orig_geom

        if inputs is not None:
            self.inputs = inputs

        for name in self.graph.nodes():
            self.graph.node[name]["evaluated"] = False  # flag all nodes as unevaluated

        self.set_input_node_states(self.inputs)  # reset the inputs

        for name in self.output_node_names:
            self.graph.node[name]["state"] = 0.0  # clear old outputs
            self.graph.node[name]["state"] = self.calc_node_state(name)  # calculate new outputs

        loc = [0, 0, 0]
        for n, name in enumerate(self.output_node_names[:3]):
            loc[n] = self.size[n] * (0.5*self.graph.node[name]["state"] + 0.5)

        # print self.size
        # print "loc: ", loc

        # new geometry: just a single patch different from original geometry
        self.values, _ = add_patch(self.orig_geom, loc=loc, mat=self.patch_mat)

    def mutate(self, *args):
        mut_func_args = [0 for _ in range(6)]
        choice = random.choice(range(6))
        mut_func_args[choice] = 1
        super(PatchCppn, self).mutate(*mut_func_args)
        self.update()


class GeneralizedCPPN(CPPN):

    def __init__(self, output_node_names, include_default_inputs=True, **additional_inputs):
        CPPN.__init__(self, output_node_names)

        if not include_default_inputs:
            self.input_node_names = []

        self.additional_inputs_dict = additional_inputs

        for key, val in additional_inputs.items():
            if key not in self.input_node_names:
                self.input_node_names += [key]

        self.graph = OrderedGraph()  # reset
        super(GeneralizedCPPN, self).set_minimal_graph()
        super(GeneralizedCPPN, self).mutate()

    def set_input_node_states(self, orig_size_xyz):

        super(GeneralizedCPPN, self).set_input_node_states(orig_size_xyz)

        for name in self.graph.nodes():

            for key, val in self.additional_inputs_dict.items():
                if name == key:
                    self.graph.node[name]["state"] = val
                    self.graph.node[name]["evaluated"] = True

