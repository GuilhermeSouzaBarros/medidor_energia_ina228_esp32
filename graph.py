import matplotlib.pyplot as pyplot
import pandas
import numpy

from pathlib import Path
from sys import argv
import os

files_path = ["preinferencia", "inferencia", "posinferencia"]
background_color = ["yellow", "red", "green"]
background_label = ["Pré-inferência", "Inferência", "Pós-inferência"]
measurement = "watt"

def plot_graph():
    if len(argv) < 2:
        print("No path provided")
        exit()

    if argv[1][-1] != "/": argv[1] += "/" 

    folder_path = Path("measurements/"+argv[1])
    if not folder_path.is_dir():
        print(str(folder_path), "is not a folder")
        exit()

    fig, ax1 = pyplot.subplots()
    ax2 = ax1.twinx()
    
    min_timestamp_raw = None
    datas = []
    for i, path in enumerate(files_path):
        file_path = folder_path.joinpath(path + ".csv")
        data = pandas.read_csv(file_path)
        min_timestamp_cur = min(data["timestamp"])
        if min_timestamp_raw is None or min_timestamp_raw > min_timestamp_cur:
            min_timestamp_raw = min_timestamp_cur
        datas.append(data)
    
    for i, data in enumerate(datas):
        data["timestamp"] = data["timestamp"] - min_timestamp_raw 
        data["timestamp_s"] = data["timestamp"] / 1000000
        ax1.axvspan(min(data["timestamp_s"]), max(data["timestamp_s"]), label=background_label[i], facecolor=background_color[i], alpha=0.5)

    data = pandas.concat(datas, axis=0, ignore_index=True)
    
    ax1.plot(data["timestamp_s"], data[measurement], label="Potência DC", linestyle="solid", linewidth=0.4, color="green", alpha=1.0)
    ax1.scatter(data["timestamp_s"], data[measurement], s=0.01, c="green", alpha=1.0)
    
    measurement_mean = numpy.mean(data[measurement])
    measurement_min = min(data[measurement])
    measurement_max = max(data[measurement])

    ax1.axhline(y=measurement_mean, label=f"Média = {measurement_mean:.3f} mW", linestyle="--", color="orange", alpha=1.0)
    ax1.axhline(y=measurement_min,  label=  f"Min = { measurement_min:.3f} mW", linestyle=":",  color="red"   , alpha=1.0)
    ax1.axhline(y=measurement_max,  label=  f"Max = { measurement_max:.3f} mW", linestyle=":",  color="red"   , alpha=1.0)

    ax1.set_xlabel("Tempo (s)")
    ax1.set_ylabel("Potência (mW)", color="green")
    ax1.tick_params("y", colors="green")
    ax1.grid(True)

    min_timestamp = min(data["timestamp_s"])
    max_timestamp = max(data["timestamp_s"])
    padding = (
        (max_timestamp - min_timestamp) * 0.025,
        (measurement_max - measurement_min) * 0.05,
    )
    ax1.axis([
        min_timestamp - padding[0],
        max_timestamp + padding[0],
        measurement_min - padding[1],
        measurement_max + padding[1]            
    ]) 

    power = [0]
    i = 1
    measurement_count = len(data[measurement])
    while i < measurement_count:
        power.append(float(
            power[-1] +
            (data[measurement][i] + data[measurement][i-1]) * (data["timestamp"][i] - data["timestamp"][i-1]) / 2000000
        ))
        i += 1

    ax2.plot(list(data["timestamp_s"]), power, label="Energia Acumulada", linestyle="solid", linewidth=1.5, color="blue", alpha=1.0)
    ax2.set_facecolor("none")
    ax2.set_ylabel("Energia Acumulada (mJ)", color="blue")
    ax2.tick_params("y", colors="blue")
    
    handles1, labels1 = ax1.get_legend_handles_labels()
    handles2, labels2 = ax2.get_legend_handles_labels()
    all_handles = handles1 + handles2
    all_labels = labels1 + labels2

    ax1.legend(all_handles, all_labels, loc='upper left')

    pyplot.tight_layout()

    path_figure = "figures/"+argv[1]
    os.makedirs(path_figure, exist_ok=True)
    pyplot.savefig(path_figure+"figure.png", format="png", dpi=400)

    pyplot.show()

if __name__ == "__main__":
    plot_graph()
