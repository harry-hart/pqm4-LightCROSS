import pathlib
import matplotlib.pyplot as plt
import matplotlib.axis as ax
import matplotlib.ticker as mtick
import numpy as np
import pandas as pd

# Function to place labels at the center of each bar
def add_labels(x, y):
    for i in range(len(x)):
        plt.text(i, y[i] // 2, y[i], ha='center')
        
def profile_process():
    results_dir = pathlib.Path("./benchmarks/profile")
    for algo_type in results_dir.iterdir():
        for scheme in algo_type.iterdir():
            for impl in scheme.iterdir():
                for res in impl.iterdir():
                    print(res)
                    benchmarks = []
                    with open(res, 'r') as f:
                        names_benchmark = []
                        function_benchmark = []
                        lines = f.readlines()
                        line_i = 0
                        while line_i != len(lines):
                            line = lines[line_i]
                            val = int(lines[line_i + 1])
                            if (val != 0):
                                names_benchmark.append(line)
                                function_benchmark.append(val)
                            if line.endswith("total cycles:\n"):
                                benchmarks.append((names_benchmark, function_benchmark))
                                function_benchmark = []
                                names_benchmark = []
                            line_i += 2
                    print(benchmarks)
                    fig, axs = plt.subplots(ncols=3, nrows=1, figsize=(30, 10))
                    ax_i = 0
                    for names, vals in benchmarks:
                        if len(vals) == 1:
                            continue
                        # Convert to numpy array
                        percentages = np.array(vals[:-1])/vals[-1]
                        ax = axs[ax_i]
                        bars = ax.bar(names[:-1], percentages)
                        ax.yaxis.set_major_formatter(mtick.PercentFormatter(1.0))
                        ax.set_xticklabels(names[:-1], rotation=45, ha='right')
                        #ax.set_layout('tight')
                        ax.set_ylim([0.0, 1.0])
                        ax.bar_label(bars, labels=np.round(percentages * 100.0), fmt="%g\%")
                        ax.set_title(names[-1].split()[0])
                        ax_i += 1
                        #plt.show()
                    fig.tight_layout()
                    fig.savefig(f"./plots/{scheme.name}.pdf")




def main():
    profile_process()

if __name__ == "__main__":
    main()
