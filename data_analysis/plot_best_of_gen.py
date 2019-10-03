import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42


# sns.set(color_codes=True, context="poster")
sns.set_style("ticks")  # , {'font.family': 'serif', 'font.serif': 'Times New Roman'})
# sns.set_palette("Set2", 8)

# colors = ["dark pink", "ocean green", "sea blue", "grey", "tan"]
# sns.set_palette(sns.xkcd_palette(colors), desat=.9)

RES = 332

df = pd.read_csv('results/best_of_gen_XENO_3_d{}.csv'.format(RES), low_memory=False)
df = df.fillna(0)
# df.ix[df.fitness <= 0, 'fitness'] = 0

# orig_len = len(df)
# print len(df[df.duplicated(subset=["id", "condition", "gen", "run"])]) == len(df[df.duplicated(subset=["id", "condition", "gen", "run", "fitness"])])
# print len(df[df.duplicated(subset=["id", "condition", "gen", "run"])]), " duplicates"
# print "Note: Duplicates are appended to BestOfGen.txt if gen is re-simulated.\n"
# df = df.drop_duplicates(subset=["id", "condition", "gen", "run"], keep='last')


# ###############################################################################################
# MIN_GEN = 500
# valid_runs = {}
#
# for condition in df['condition'].unique():
#     valid_runs[condition] = []
#     for run in df['run'].unique():
#         condition_run = df[(df['condition'] == condition) & (df['run'] == run)]
#         if len(condition_run[condition_run["gen"] > MIN_GEN]) > 0:
#             valid_runs[condition] += [run]
#
# print valid_runs
# print [len(v) for k, v in valid_runs.items()]
#
# dfa = pd.DataFrame()
# for condition in df['condition'].unique():
#     dfb = df[(df['condition'] == condition) & (df['run'].isin(valid_runs[condition]))]
#     dfa = pd.concat([dfa, dfb])
#
# df = dfa
# ################################################################################################

# print df

df['gen'] *= 51

df = df[df['gen'] < 140001]

exhaustive = df.loc[df.condition == 'XENO_3_d{}_Exhaustive'.format(RES)]
sgd_re = df.loc[df.condition == 'XENO_3_d{}_SGD_restarts'.format(RES)]

sgd_re['gen'] += sgd_re['restart']*20*50
sgd_re = sgd_re.sort_values('gen')

sgd = df.loc[df.condition == 'XENO_3_d{}_SGD'.format(RES)]
afpo = df.loc[df.condition == 'XENO_3_d{}_AFPO'.format(RES)]

for run in range(1, 6):
    optimal = afpo.loc[afpo.run == run, 'fitness'].max()  # exhaustive.loc[exhaustive.run == run, 'fitness'].max()

    print optimal, sgd_re.loc[sgd_re.run == run, 'fitness'].max()

    # optimal_idx = afpo.loc[afpo.run == run, 'fitness'].argmax()
    # print afpo['gen'][optimal_idx]

    afpo.loc[afpo.run == run, 'fitness'] *= 1/float(optimal)
    sgd.loc[sgd.run == run, 'fitness'] *= 1/float(optimal)
    sgd_re.loc[sgd_re.run == run, 'fitness'] *= 1/float(optimal)

    sgd.loc[sgd.run == run, 'fitness'] = sgd.loc[sgd.run == run, 'fitness'].cummax()
    sgd_re.loc[sgd_re.run == run, 'fitness'] = sgd_re.loc[sgd_re.run == run, 'fitness'].cummax()


def estimator(x, *args, **kwargs):
    return np.mean(x, *args, **kwargs)*-2


# fig, ax = plt.subplots(1, 1, figsize=(4, 3))
fig, ax = plt.subplots(1, 1, figsize=(8, 3))

c = sns.tsplot(data=sgd, value="fitness", condition="condition", unit="run", time="gen",
               # ci=95,
               legend=False, color=sns.color_palette()[0],
               # err_style="unit_traces"
               )

b = sns.tsplot(data=sgd_re, value="fitness", condition="condition", unit="run", time="gen",
               # ci=95,
               legend=False, color=sns.color_palette()[1],
               # err_style="unit_traces"
               )

a = sns.tsplot(data=afpo, value="fitness", condition="condition", unit="run", time="gen",
               # ci=95,
               # estimator=estimator,
               # err_style="unit_traces",
               # err_kws={'alpha': 1},
               legend=False,
               color=sns.color_palette()[2]
               # ax=ax[0]
               )

# for n in range(len(ax)):
#     ax[n].set_xlabel('Generation')
#     ax[n].legend([mpatches.Patch(color=sns.color_palette()[i]) for i in range(2)],
#                  ['uniform', 'dynamic'], loc=4)

# ax[0].set_title(r'$\bf{Morphological}$')
ax.set_title(r'$3\times3\times2$')

ax.set_ylabel('Performance relative to optimal')
ax.set_xlabel('Number of evaluations')
# # ax[1].set_ylabel('Displacement  (cm)')
# for n in range(len(ax)):
#     ax[n].set_ylim([-0.5, 0])
#     ax[n].set_xlim([-1, 100])

# ax.set_xlim([-500, 10000])
ax.set_xlim([-2000, 140000])
# ax.set_xticks([0, 50000, 100000, 150000,])
ax.set_ylim([0.5, 1.01])
#
# df = df[df['gen'] == 1500]
# print df['fitness']


# pre_damage_fit = np.array([34.6149, 29.4332, 32.2900, 27.8435, 30.0080])
# pre_damage_fit *= 4/3.0  # originals ran for 3 sec but post damage had 4 sec
#
# for run in range(20):
#     # print run % 5
#     df.loc[df.run == run, 'fitness'] *= 1/float(pre_damage_fit[run % 5])
#
#
# df['amputation_num'] = df.condition.str[4]
# df['treatment'] = df.condition.str[5]
#
#
# scenarios = ["amputation = 1/2 leg", "amputation = 1 leg", "amputation = 2 adj. legs", "amputation = 3 legs",
#              "amputation = 4 legs", "amputation = 2 diag. legs", "amputation = 1/4 body", "amputation = 1/2 body",
#              "amputation = 3/4 body"]
#
# df['amputation'] = df['amputation_num'].map({str(i): scenarios[i] for i in range(9)})
#
# # print df.head()
#
#
# df2 = df.copy()
# df2['amputation'] = "$\\bf{Overall}$"
# df2['run'] = df['amputation_num'].astype(int)*20 + df2['run']
#
# # print df2
# # print df2['run']
#
# df = pd.concat([df, df2])
#
# order = ["amputation = 1/2 leg", "amputation = 1 leg", "amputation = 2 adj. legs", "amputation = 2 diag. legs",
#          "amputation = 3 legs", "amputation = 4 legs", "amputation = 1/4 body", "amputation = 1/2 body",
#          "amputation = 3/4 body", "$\\bf{Overall}$"]
#
# grid = sns.FacetGrid(df, col="amputation", col_wrap=2, col_order=order)  # hue="scenario", palette="tab20c")
#
# # grid.map(plt.plot, "gen", "fitness")
# grid.map_dataframe(sns.tsplot, ci=99, value="fitness", condition="treatment", unit="run", time="gen",
#                    color=sns.xkcd_palette(colors),
#                    # estimator=np.median,
#                    # err_style="unit_traces"
#                    )
#
# grid.set(xticks=[0, 100, 200, 300, 400, 500], yticks=[0, 0.25, 0.5, 0.75, 1, 1.25, 1.5],
#          xlim=(-10, 500), ylim=(0, 1.5))
#
# # np.mean(pre_damage_fit)
# grid = (grid.map(plt.axhline, y=1, ls=":", c=sns.color_palette()[2]).set_axis_labels("Generations since damage",
#                                                                                      "Relative displacement"))
# grid = (grid.set_titles("{col_name}"))
#
# # plt.legend([mpatches.Patch(color=sns.color_palette()[i]) for i in range(3)], ['morphology', 'controller', 'original'],
# #            bbox_to_anchor=(1.05, 0), loc=3)
#
# for i in range(10):
#     grid.axes[i].lines[1].set_linestyle("--")
#
# grid.axes[9].legend(
#                     # [mpatches.Patch(color=sns.color_palette()[i]) for i in range(3)],
#                     handlelength=2.5,
#                     labels=['morphological adaptation', 'controller adaptation', 'original (predamage)'], loc=2)
#
# grid.fig.tight_layout(w_pad=1)

plt.legend([mpatches.Patch(color=sns.color_palette()[i]) for i in [2, 0, 1]], ['EA', 'SGD', 'SGDre'], loc=4)

sns.despine()
plt.tight_layout()
plt.savefig("plots/d{}.png".format(RES), dpi=600, bbox_inches='tight')


