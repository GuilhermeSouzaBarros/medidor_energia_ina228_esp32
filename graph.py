import matplotlib.pyplot as pyplot
import pandas
from pathlib import Path

from sys import argv


files_path = ["preinferencia", "inferencia", "posinferencia"]
measurement = "voltage" 

def plot_graph():
    if len(argv) < 2:
        print("No date provided")
        exit()

    folder_path = Path("measurements/"+argv[1])
    if not folder_path.is_dir():
        print(str(folder_path), "is not a folder")
        exit()

    min_timestamp = None
    max_timestamp = None
    max_measurement = None

    last_timestamp = None
    deltas = []
    for path in files_path:
        file_path = folder_path.joinpath(path + ".csv")
        data = pandas.read_csv(file_path)
        min_timestamp_cur = min(data["timestamp"])
        max_timestamp_cur = max(data["timestamp"])
        max_measurement_cur = max(data[measurement])
        if not min_timestamp or min_timestamp_cur < min_timestamp: min_timestamp = min_timestamp_cur
        if not max_timestamp or max_timestamp_cur > max_timestamp: max_timestamp = max_timestamp_cur
        if not max_measurement or max_measurement_cur > max_measurement: max_measurement = max_measurement_cur
        pyplot.plot(data["timestamp"], data[measurement], linestyle="solid")

        for timestamp in data["timestamp"]:
            if last_timestamp: deltas.append(timestamp - last_timestamp)
            last_timestamp = timestamp
    
    
    pyplot.axis([
        min_timestamp * 0.9,
        max_timestamp * 1.05,
        0,
        max_measurement * 1.05            
    ])    
    pyplot.show()


    pyplot.axis([-1, len(deltas), 0, max(deltas) * 1.05])
    pyplot.plot([x for x in range(len(deltas))], deltas)
    pyplot.show()

if __name__ == "__main__":
    plot_graph()
