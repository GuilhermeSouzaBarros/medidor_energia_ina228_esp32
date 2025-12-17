from datetime import date
from pathlib import Path
import struct

class StateWriter:
    def __init__(self, states:list[str], print_count:int=1024):
        self.states = states
        self.num_states = len(self.states)
        self.cur = -1

        self.date = date.today()
        print(">>> Current date:", self.date)
        
        self.cur_file = None
        self.path_folder = Path("measurements")
        if not self.path_folder.is_dir(): Path.mkdir(self.path_folder)
        self.path_folder = self.path_folder.joinpath(str(self.date))
        if not self.path_folder.is_dir(): Path.mkdir(self.path_folder)
        
        self.measurement_print_count = print_count
        
    def __del__(self):
        if self.cur_file is not None: self.cur_file.close()
    
    @property
    def state_current(self):
        return self.states[self.cur]

    def state_set_next(self):
        if self.cur_file is not None: self.cur_file.close()
        
        self.cur += 1
        if self.cur >= self.num_states: return

        state_current = self.state_current
        path_file = self.path_folder.joinpath(state_current + ".csv")

        print("\n========================================================")
        print(">>> State changed to", state_current)
        print(">>> Writing to", path_file)
        self.cur_file = open(path_file, "w")
        self.cur_file.write("timestamp,voltage,ampere,power\n")

    def write(self, string:str):
        if self.cur_file is None: raise FileNotFoundError
        self.cur_file.write(string)

    def waiting(self):
        return self.cur == -1

    def ended(self):
        return self.cur >= self.num_states
    
    def read_setup(self, serial):
        line = ""    
        while line != "finished_setup":
            try:
                if line != "": print(line)
                line = serial.readline().decode().rstrip()
            except: continue 

    def read_measurement(self, serial):
        measurement_count = 0
        time_first = None
        time_last = None
        while True:
            data = serial.read(20)
            if b'state swap' in data: break
            if self.waiting(): continue

            try:
                data = struct.unpack('Ifff', data)
                line_to_write = f"{data[0]},{data[1]},{data[2]},{data[3]}\n"
                if time_first is None: time_first = data[0]
                time_last = data[0]
                measurement_count += 1
                if measurement_count % self.measurement_print_count == 0: print(">>> Measurements at:", measurement_count)
                self.write(line_to_write)
            except Exception as error:
                print("Ignored input:", error.args[0])

        if not self.waiting():
            time_delta = (time_last - time_first) / 1000000
        
            print(f"\n>>> MEDIÇÃO {self.state_current} FINALIZADA!")
            print(f"Total de medições: {measurement_count}")
            print(f"Tempo medido: {time_delta:.2f} segundos")
            print(f"Taxa de amostragem: {(measurement_count / time_delta):.2f} medições/segundo")
            print("\n========================================================")

        self.state_set_next()
