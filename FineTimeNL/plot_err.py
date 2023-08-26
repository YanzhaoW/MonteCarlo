import math
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from scipy import stats


def delta(value):
    return math.sqrt(3) * (0.5*0.5/value - 0.5 + 1/3 - 1/3 * value)


def err_pre(prob_a, entry_n, prob_b):
    base_value = entry_n * entry_n / 12 * prob_b * prob_b
    residual = entry_n*(prob_a - prob_a*prob_a-prob_b*prob_a + prob_b/3 - prob_b*prob_b/3)
    return np.sqrt(base_value + residual)


def err_pre_approx(prob_a, entry_n, prob_b):
    base_value = entry_n / np.sqrt(12) * prob_b
    residual = np.sqrt(3)/prob_b * (- (prob_a - 0.5) * (prob_a - 0.5) + 0.25)
    return base_value + residual


def err_pre_base(entry_n, prob_b):
    base_value = entry_n * entry_n / 12 * prob_b * prob_b
    # residual = entry_n*(prob_a - prob_a*prob_a-prob_b*prob_a + prob_b/3)
    return np.sqrt(base_value)


def plot_prob():
    csv_filename = "data.csv"
    sns.set_theme()
    data = pd.read_csv(csv_filename, skipinitialspace=True)
    data = pd.melt(data, id_vars=['mean', 'stderr'], value_vars=[
                   'pre_size', 'mid_size', 'post_size'], var_name='type', value_name='bin_size')
    sns.scatterplot(data=data, x="bin_size", y='stderr', hue='type')
    plt.show()


def plot_samples():
    csv_filename = "samples.csv"
    sns.set_theme()
    data = pd.read_csv(csv_filename, skipinitialspace=True)
    data = data.sort_values(by=['sample_size'])
    data = data[250:]

    x_arr = data['sample_size'].to_numpy()
    y_arr = data['stderr'].to_numpy()
    res = stats.linregress(x_arr, y_arr)
    print(res)
    print(f"expected slope: {0.1/math.sqrt(12)} and offest {1/math.sqrt(3)}")


def plot_pres():
    '''plot err values with different pa'''
    csv_filename = "pa.csv"
    entryN = 100000
    pb = 0.01
    sns.set_theme()
    data = pd.read_csv(csv_filename, skipinitialspace=True)
    data = data.sort_values(by=['pa'])

    # print("delta %f" % delta(0.01))
    # fitting:
    # x_arr = data['pre'].to_numpy()
    # y_arr = data['stderr'].to_numpy()
    # func = lambda p,x: (p[0]*(x-p[1])*(x-p[1]) + p[2])
    # func = lambda p,x: (p[0]*(x-0.5)*(x-0.5) + p[1])
    # model = odr.Model(func)
    # fitdata = odr.RealData(x=x_arr, y=y_arr)
    # reg = odr.ODR(fitdata, model, maxit = 10000, beta0 = [-100, 0.5, 100])
    # res = reg.run()
    # res.pprint()

    xaxis = np.linspace(0, 1, num=400)
    # plt.plot(xaxis, err_pre(xaxis, entry_n=entryN, prob_b=pb), 'r-',
    #          label=f"prediction: N = {entryN}, pb = {pb}")
    plt.plot(xaxis, err_pre(xaxis, entry_n=entryN, prob_b=pb), 'r-',
             label="exact solution")
    plt.plot(xaxis, err_pre_approx(xaxis, entry_n=entryN, prob_b=pb), 'g-',
             label="approx solution")
    plt.plot(xaxis, np.full(400, err_pre_base(entry_n=entryN, prob_b=pb)), 'b-',
             label="base solution")

    sns.scatterplot(data=data, x="pa", y='stderr')
    plt.xlabel("probability_a")
    plt.ylabel("error")
    # plt.show()
    plt.savefig('prob_a.png', bbox_inches='tight', dpi=400)


if __name__ == '__main__':
    # plot_samples()
    plot_pres()
