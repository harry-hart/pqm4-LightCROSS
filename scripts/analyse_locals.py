import argparse
import json
import pathlib
import random
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
# Using for interactive graph portability
import numpy as np
import pandas as pd
import plotly.graph_objects as go
import plotly.subplots as subplots

def load_locals(file: pathlib.Path):
    dec = json.JSONDecoder()
    with open(file, "r") as f:
        data = dec.decode(f.read())
    return data

def func_df(func_locals: dict):
    df = pd.DataFrame(func_locals)
    df = df.reset_index().rename(columns={'index': 'step'})
    df = pd.melt(df, id_vars='step', var_name='var_name', value_name='value')
    return df

def plot_func(fname: str, local_df: pd.DataFrame, func_locals: dict, fig, row=None, col=None):
    #fig, ax = plt.subplots()
    steps = [i for i in range(len(list(func_locals.values())[0]))]
    #ax.stackplot(steps, func_locals.values(), labels=func_locals.keys(), alpha=0.8)
    #ax.legend(loc='right', reverse=True)
    #ax.set_title(fname)
    #ax.set_xlabel('Source Line')
    #ax.set_ylabel('Bytes Used')
# ad#d tick at every 200 million people
    ##ax.yaxis.set_minor_locator(mticker.MultipleLocator(.2))
    #plt.show()
    #fig = subplots.make_subplots(rows=3, cols=1)
    for var, vals in func_locals.items():
        fig.add_trace(go.Scatter(
            x=steps, 
            y=vals,
            hoveron='points+fills',
            hovertemplate='size: %{y}',
            #hovertext="test",
            hoverinfo="all",
            mode='lines',
            stackgroup='one',
            name=var,
        ), row=row, col=col)
    #px.area(local_df, x="step", y="value", color="var_name", line_group="var_name", title=fname)
    #fig.update_traces(
    #    hoveron="fills",
    #)

def plot_all_func_av_bar(data: dict, fig, ax):
    work_ax = ax
    for key, var_vals in data.items():
        bottom = 0
        for var, vals in var_vals.items():
            work_ax.bar(key, np.average(vals), width=0.5, label=var, bottom=bottom)
            bottom += np.average(vals)
        work_ax.set_ylabel("Bytes")
        work_ax.set_ylim(bottom=0, top=bottom)
        work_ax = ax.twinx()

    ax.set_title("Average Variable Memory Usage (Bytes)")

def plot_func_av(data: dict, fig, axs, combined=True, prefix=""):
    subplts = 1
    for i, key_var_vals in enumerate(data.items()):
        if not combined:
            work_ax = axs[0]
            work_ax.clear()
            #fig.clear()
        else:
            work_ax = axs[i]
        key, var_vals = key_var_vals
        label_names = []
        values = []
        total = 0
        for var, vals in var_vals.items():
            label_names.append(var)
            values.append(np.average(vals))
            total += values[-1]
        values = np.array(values)
        percents = (values/total)*100.0
        #patches, texts, _ = work_ax.pie(values, labels=labels, autopct='%1.1f%%', textprops={'size': 'smaller'})
        patches, texts = work_ax.pie(values)
        #patches, texts = work_ax.pie(y, colors=colors, startangle=90, radius=1.2)
        labels = ['{0} - {1:1.2f} %'.format(i,j) for i,j in zip(label_names, percents)]

        sort_legend = True
        if sort_legend:
            patches, labels, dummy =  zip(*sorted(zip(patches, labels, values),
                                                key=lambda x: x[2],
                                                  reverse=True)[:15])

        plt.legend(patches, labels, loc='center left', bbox_to_anchor=(-0.5, 0.5),
           fontsize=8)
        fig.suptitle(f"{key} - Average Variable Memory Usage Ratio")
        if not combined:
            fig.savefig(f"{prefix}-{key}.pdf")

def plot_func_av_bar(func_key, data: dict, fig, ax):
    work_ax = ax
    patch_data = []
    patch_dict = {}
    colours = deque(mcolors.XKCD_COLORS.keys())
    for impl, impl_data in data:
        var_vals = impl_data[func_key]
        bottom = 0
        for var, vals in var_vals.items():
            colour = colours.popleft()
            label = var
            if (patch := patch_dict.get(var, None)) is not None:
                colours.append(colour)
                colour = patch.get_facecolor()
                label = "_" + var
            patch = work_ax.bar(str(impl), np.average(vals), width=0.5, label=label, bottom=bottom, color=colour)
            if patch_dict.get(var, None) is None:
                patch_dict[var] = patch[0]
                patch_data.append((patch, var, np.average(vals)))
            bottom += np.average(vals)
        work_ax.set_ylabel("Bytes")
    # Link the same variable across the diagram
    #names = set()
    #for i, p in enumerate(patch_data):    # this is the loop to change Labels and colors
    #    if p.get_label() in names[:i]:    # check for Name already exists
    #        idx = names.index(p.get_label())       # find ist index
    #        p.set_c(ax.get_lines()[idx].get_c())   # set color
    #        p.set_label('_' + p.get_label()) 
    # Add legend of top 10 
    patches, labels, dummy =  zip(*sorted(patch_data,
                                            key=lambda x: x[2],
                                                reverse=True)[:15])
    work_ax.legend(patches, labels, loc='center right', bbox_to_anchor=(1.5, 0.5),fontsize=8)
        #work_ax.set_ylim(bottom=0, top=bottom)
        #work_ax = ax.twinx() 

    ax.set_title(f"{func_key} Average Variable Memory Usage (Bytes)")

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-l", "--locals", help="Locals file generated by local_size.py script.")
    parser.add_argument("-c", "--combined", help="Combine all charts as subgraphs in the same figure (may cause them to be quite small)", action='store_true')
    parser.add_argument("-s", "--static", help="Create a static bar plot from average value over the life of the function", action='store_true')

    args = parser.parse_args()

    local_data = []
    for f in pathlib.Path("./").glob(args.locals):
        print(f.stem)
        local_data.append((str(f.stem), load_locals(f)))

    # Remove starting prefix that matches
    done = False
    prefix = ""
    for i in range(len(local_data[0][0])):
        pre = local_data[0][0][:i]
        for impl, _ in local_data:
            if not impl.startswith(pre):
                done = True
                break
        if done:
            break
        prefix = pre
    for i, impl_locals in enumerate(local_data):
        new_name = impl_locals[0].removeprefix(prefix)
        print(new_name)
        local_data[i] = (new_name, impl_locals[1])
        print(local_data[i][0])


    #print("local data", local_data[0])
    funcs = list(local_data[0][1].keys())
    print(funcs)

    if args.static:
        if args.combined:
            fig, axs = plt.subplots(1,3)
        else:
            fig, ax = plt.subplots(1,1, layout="constrained")
            axs = [ax]
    else:
        if args.combined:
            fig = subplots.make_subplots(rows=3, cols=1, subplot_titles=funcs)
        else:
            fig = go.Figure()

    if args.static:
        #plot_func_av(local_data, fig, axs, args.combined, prefix=f"static-mem-{args.locals.stem}")
        for func in local_data[0][1].keys():
            plot_func_av_bar(func, local_data, fig, ax)
            fig.savefig(f"static-mem-{func}.pdf", bbox_inches='tight')
            fig.show()
        if args.combined:
            fig.savefig(f"static-mem-{args.locals.stem}.pdf")
            fig.show()
    else:
        for i, key in enumerate(funcs):
            local_df = func_df(local_data[0][1][key])
            print(local_df["var_name"].unique())
            if args.combined:
                plot_func(key, local_df, local_data[0][1][key], fig, i+1, 1)
            else:
                plot_func(key, local_df, local_data[0][1][key], fig)
                fig.update_layout(showlegend=False, title_text=f"{local_data[0][0]} - {key}", 
                    xaxis=dict(
                        title=dict(
                            text="Source Line"
                        )
                    ),
                    yaxis=dict(
                        title=dict(
                            text="Memory (Bytes)"
                        )
                    ),
                )
                fig.write_html(f"mem-{local_data[0][0]}_{key}.html")
                fig.show()
                fig.data = []
        if args.combined:
            fig.update_layout(showlegend=False, title_text=args.locals.stem)
            fig.write_html(f"mem-{args.locals.stem}_combined.html")
            fig.show()

if __name__ == "__main__":
    main()
