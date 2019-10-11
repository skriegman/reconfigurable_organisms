import networkx as nx
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import seaborn as sns

from matplotlib.offsetbox import OffsetImage, AnnotationBbox
from matplotlib.cbook import get_sample_data

mpl.rcParams['pdf.fonttype'] = 42
mpl.rcParams['ps.fonttype'] = 42
sns.set_style("white")
sns.set_palette("husl", 7)

import cPickle
from softbot import Genotype, Phenotype
from tools.utils import sigmoid

np.random.seed(1)

MyGenotype = Genotype
MyPhenotype = Phenotype

pickle = "/home/sam/Projects/research_code/evosoro/data_analysis/results/" \
         "XENO_3_Gen_1000/Exp_XENO_3_Run_52_Gen_1000.pickle"

with open(pickle, 'rb') as handle:
    [optimizer, random_state, numpy_random_state] = cPickle.load(handle)

pop = optimizer.pop
best_ind = None
best_fit_so_far = 0

for n, ind in enumerate(pop):
    if ind.fitness > best_fit_so_far:
        best_ind = ind
        best_fit_so_far = ind.fitness


# for n in best_ind.genotype.networks[1].graph:
#     print n, best_ind.genotype.networks[1].graph.node[n]["function"]  # best_ind.genotype.networks[1].graph[n], \

# node_color = ["green"]*5 + ["red"]*2 + ["orange", "yellow", "blue", "yellow", "purple",
#                                         "yellow", "blue", "orange", "blue"]

palette = sns.color_palette(desat=0.9)  #
node_color = [3]*5 + [5]*2 + [0]*9
# node_color = [0]*5 + [1, 2, 3, 4, 5, 4, 6, 4, 5, 3, 5]
node_color = [palette[i] for i in node_color]

G = best_ind.genotype.networks[1].graph

# nx.write_gpickle(G, "/home/sam/Desktop/graph52.pickle")
#
# G = nx.read_gpickle("/home/sam/Desktop/graph52.pickle")
#

fig, ax = plt.subplots()

frame1 = plt.gca()
frame1.axes.get_xaxis().set_visible(False)
frame1.axes.get_yaxis().set_visible(False)

pos = nx.layout.spring_layout(G, iterations=50000)

nodes = nx.draw_networkx_nodes(G, pos, node_size=2000, node_color=node_color)
edges = nx.draw_networkx_edges(G, pos, edge_color="grey", width=3, arrows=False)

nx.draw_networkx(G, pos, with_labels=False, width=1.5, node_color=node_color, edge_color="grey")

input_dict = {'x': '$x$', 'y': '$y$', 'z': '$z$', 'd': '$d$', 'b': '1'}

func_dict = {"shape": "sine",
             "muscleOrTissue": "sine",
             1: 'abs',
             4: "neg_sqrt_abs",
             6: "neg_square",
             7: "neg_sqrt_abs",
             8: 'square',
             11: "neg_sqrt_abs",
             14: "neg_square",
             15: 'abs',
             16: "neg_square"
             }

for n, p in pos.items():
    if n in input_dict:
        plt.text(p[0], p[1], input_dict[n], ha='center', va='center', fontsize=24)

for n, p in pos.items():
    if n in func_dict:
        arr_img = plt.imread("plots/{}.png".format(func_dict[n]), format='png')
        # new = fig.add_axes((p[0], p[1], 0.1, 0.05))
        # new.imshow(arr_img)
        # new.axes.get_xaxis().set_visible(False)
        # new.axes.get_yaxis().set_visible(False)

        imagebox = OffsetImage(arr_img, zoom=0.012)
        imagebox.image.axes = ax
        ab = AnnotationBbox(imagebox, p, frameon=False)
        ax.add_artist(ab)


# # set alpha value for each edge
# for i in range(M):
#     edges[i].set_alpha(edge_alphas[i])

# pc = mpl.collections.PatchCollection(edges, cmap=plt.cm.Blues)
# pc.set_array(edge_colors)
# plt.colorbar(pc)

# # activation funcs
# t = np.arange(-3*np.pi, 3*np.pi, 0.01)
# s = np.abs(t)
# # s = sigmoid(t)
# # s = np.sin(t)
# # s = np.sqrt(np.abs(t))
# # s = t**2
# ax.plot(t, s, linewidth=30, color="k")

# ax.set_xlim([-3*np.pi*1.11, 3*np.pi*1.11])
# ax.set_ylim([-1.175, 1.175])
# ax.set_ylim([-0.16, 3.3])

sns.despine(left=1, bottom=1)
plt.tight_layout()
plt.savefig("plots/cppn.png", bbox_inches='tight', dpi=300, transparent=True)




