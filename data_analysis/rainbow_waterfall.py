from glob import glob
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

sns.set_style("ticks")
sns.set_palette("hls", 8)

RUN = 100
GENS = 1000

run_directory = "/home/sam/Archive/skriegma/XENO_3/run_{}/".format(RUN)
all_of_gen_files = glob(run_directory + "allIndividualsData/Gen_*.txt")
sorted_all_of_gen_files = sorted(all_of_gen_files, reverse=False)

line_hist = []
gen_age_fit_dict = {}

for gen in range(GENS+1):

    gen_age_fit_dict[gen] = {0: 0.0}

    with open(sorted_all_of_gen_files[gen], 'r') as infile:

        next(infile)  # skip header

        for line in infile:
            # this_gen = int(line.split()[0])
            # print this_gen == gen
            # this_id = int(line.split()[1])
            # parent_id = int(line.split()[3])
            # variation_type = line.split()[4]
            this_fit = float(line.split()[5])
            this_age = int(line.split()[7])

            if this_age not in gen_age_fit_dict[gen] or this_fit > gen_age_fit_dict[gen][this_age]:  # ord by fit anyway
                gen_age_fit_dict[gen][this_age] = this_fit  # most fit at each age level

            # if gen > 0 and this_age > 0 and gen_age_fit_dict[gen-1][this_age-1] > gen_age_fit_dict[gen][this_age]:
            #     print "asdf"

    if gen > 0:

        for age in gen_age_fit_dict[gen-1]:

            if age+1 not in gen_age_fit_dict[gen] or gen == 1000:  # extinction

                this_line = []
                n = 0
                while age-n > -1:
                    this_line += [gen_age_fit_dict[gen-1-n][age-n] * 6 / 8.0]
                    n += 1

                # if len(this_line) > 1 and this_line[1] > this_line[0]:
                #     print this_line

                pre_fill = [None]*(gen-age)
                # post_fill = [None]*(GENS-gen)
                line_hist += [pre_fill + list(this_line[::-1])]


fig, ax = plt.subplots(1, 1, figsize=(4, 3))

for line in line_hist:
    ax.plot(range(len(line)), line, linewidth=0.8)

ax.set_xlim([-10, GENS])
ax.set_ylim([-0.1, 8])
ax.text(50, 7, "Run {}".format(RUN), fontsize=20)
# ax.set_ylabel('Displacement')
# ax.set_xlabel('Generation')
# ax.set_xticklabels([0, 0, 200/50, 400/50, 600/50, 800/50, 1000/50], fontsize=12)
ax.set_xticklabels([0, 0, 200, 400, 600, 800, 1000], fontsize=12)
ax.set_yticklabels([0, 0, 2, 4, 6, 8], fontsize=12)

sns.despine()
plt.tight_layout()
plt.savefig("plots/XENO_3_rainbow_waterfall_run_{}.png".format(RUN), bbox_inches='tight', dpi=600)
