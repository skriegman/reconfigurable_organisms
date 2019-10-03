import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from xml.etree.cElementTree import parse


def get_output_values(filename, output_name):
    data = parse(filename).getroot().find('VXC').find('Structure').find(output_name)
    return [float(string) for layer in data for string in layer.text.split(', ') if string != ""]


def get_data_from_layer(layer):
    return layer[layer.find("<Layer><![CDATA[") + len("<Layer><![CDATA["):layer.find("]]></Layer>")]


def get_all_data(paths_to_files, delimiter="\t\t", lineterminator='\n', engine='python', drop_duplicates_subset=None):
    run = []
    restart = []
    df = pd.DataFrame()
    for n, path in enumerate(paths_to_files):
        this_df = pd.read_table(path, delimiter=delimiter, lineterminator=lineterminator, engine=engine)

        if path[path.find("run_") + 5] == "_":
            # print path[path.find("run_")+4], path[path.find("run_")+6:path.find("/bestSo")]
            run += [int(path[path.find("run_")+4])] * len(this_df)
            restart += [int(path[path.find("run_")+6:path.find("/bestSo")])] * len(this_df)
            # print restart[-1]

        else:
            # run += [n] * len(this_df)
            # print int(path[path.find("run_")+4:path.find("/bestSo")])
            run += [int(path[path.find("run_")+4:path.find("/bestSo")])] * len(this_df)
            restart += [0] * len(this_df)

        df = pd.concat([df, this_df])

        if drop_duplicates_subset is not None:
            start_length = len(df)
            df = df.drop_duplicates(subset=drop_duplicates_subset)
            num_duplicates = start_length - len(df)
            # print "{} duplicates".format(num_duplicates)
            run = run[num_duplicates:]

    df['run'] = run
    df['restart'] = restart
    return df


def combine_experiments(exp_dfs, names):
    condition = []
    for n in range(len(exp_dfs)):
        condition += [names[n]] * len(exp_dfs[n])
    df = pd.concat(exp_dfs)
    df['condition'] = condition
    return df


def plot_time_series(combined_exp_dfs, title):
    sns.tsplot(data=combined_exp_dfs, value="fitness", condition="condition", unit="run", time="gen")
    plt.title(title)
    plt.savefig("{}.pdf".format(title))


def bootstrapped_percentile(data, nboot=5000, stat=np.median, q=95):
    values = []
    for _ in range(nboot):
        sample = np.random.choice(data, len(data))
        values += [stat(sample)]

    return np.percentile(values, q)
