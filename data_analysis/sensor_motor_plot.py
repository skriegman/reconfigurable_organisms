import cPickle
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib
# import matplotlib.cm as cm

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42


# sns.set(color_codes=True, context="poster")
sns.set_style("ticks")  # , {'font.family': 'serif', 'font.serif': 'Times New Roman'})
# sns.set_palette("Set2", 8)

colors = ["ocean green", "dark pink", "grey", "tan"]
sns.set_palette(sns.xkcd_palette(colors), desat=.9)

pickle = "/home/sam/Projects/research_code/evosoro/data_analysis/results/Robot_Trace_DataFrame.pickle"
with open(pickle, 'rb') as handle:
    df = cPickle.load(handle)

# print df.head()
#
# print len(df)
# df = df[df['robot'] < 10]
# print len(df)
#
# time = df["Time"]
# min_time = np.min(time)
# max_time = np.max(time)
# norm_time = (time-min_time)/(max_time-min_time)
# df["Colors"] = norm_time
#
# print df.head()

grid = sns.FacetGrid(df, col="robot", col_wrap=10)  # hue="scenario", palette="tab20c")

grid.map(plt.scatter, "TraceX", "TraceY", "Time")
# grid.map_dataframe(sns.tsplot, ci=99, value="fitness", condition="treatment", unit="run", time="gen",
#                    color=sns.xkcd_palette(colors),
#                    # estimator=np.median,
#                    # err_style="unit_traces"
#                    )

grid.set(  # xticks=[0, 100, 200, 300, 400, 500], yticks=[0, 0.25, 0.5, 0.75, 1, 1.25, 1.5],
         # xlim=(-1, 1), ylim=(-1, 1)
         )

# np.mean(pre_damage_fit)
# grid = (grid.map(plt.axhline, y=1, ls=":", c=sns.color_palette()[2]).set_axis_labels("Generations since damage", "Relative displacement"))
# grid = (grid.set_titles("{col_name}"))

# plt.legend([mpatches.Patch(color=sns.color_palette()[i]) for i in range(3)], ['morphology', 'controller', 'original'],
#            bbox_to_anchor=(1.05, 0), loc=3)

# for i in range(10):
#     grid.axes[i].lines[1].set_linestyle("--")
#
# grid.axes[9].legend(
#                     # [mpatches.Patch(color=sns.color_palette()[i]) for i in range(3)],
#                     handlelength=2.5,
#                     labels=['morphological adaptation', 'controller adaptation', 'original (predamage)'], loc=2)
#
# grid.fig.tight_layout(w_pad=1)

# sns.despine()
# plt.tight_layout()

plt.savefig("plots/XENO_Trace.png", bbox_inches='tight', dpi=100)


