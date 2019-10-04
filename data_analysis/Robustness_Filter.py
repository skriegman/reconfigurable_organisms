import cPickle
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42
font = {'size': 22}
matplotlib.rc('font', **font)
sns.set_style("ticks")
colors = ["grey", "dark pink", "ocean green", "tan"]
sns.set_palette(sns.xkcd_palette(colors))  # , desat=.9)

np.random.seed(1)

EXP_NAME = "XENO_3"
PICKLE_DIR = "/home/sam/Projects/research_code/evosoro/data_analysis/results"

with open('{0}/{1}_Robustness_DataFrame_{2}.pickle'.format(PICKLE_DIR, EXP_NAME, 0), 'rb') as handle:
    df = cPickle.load(handle)

medians = df.groupby(['robot']).median()
print medians.nlargest(50, 'fit')
# print medians.nsmallest(50, 'fit')

fig, ax = plt.subplots(1, 1, figsize=(4, 3))

mask = df['robot'].values != 52
sns.kdeplot(df["fit"][mask],#*6/8.,
            shade=True, legend=False, bw=0.4, gridsize=10000, clip=(0, 8))

mask = df['robot'].values == 52
sns.kdeplot(df["fit"][mask],#*6/8.,
            shade=True, legend=False, bw=0.4, gridsize=10000, clip=(0, 8))

ax.set_ylim([-0.01, 0.5])

ticks = matplotlib.ticker.FuncFormatter(lambda x, pos: '{0:g}'.format(x * 6/8.0))
ax.xaxis.set_major_formatter(ticks)

ax.set_xticks([0, 2*8/6.0, 4*8/6.0, 6*8/6.0, 8*8/6.0])


sns.despine()
plt.tight_layout()
plt.savefig("../data_analysis/plots/XENO_3_Histogram.png", bbox_inches='tight', dpi=600, transparent=True)
