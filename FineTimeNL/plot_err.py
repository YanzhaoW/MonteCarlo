import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt


sns.set_theme()

csv_filename = "build/data.csv"

data = pd.read_csv(csv_filename, skipinitialspace = True );

data = pd.melt(data, id_vars = ['mean', 'stderr'], value_vars = ['pre_prob', 'mid_prob', 'post_prob'], var_name = 'type', value_name= 'probability')

sns.scatterplot(data = data, x = "probability", y = 'stderr', hue='type')

plt.show()
# print(data)
