import os
import struct
from multiprocessing import Process, Queue, Event

from datetime import date

class StateWriter:
    def __init__(self, serial, states:list[str],
                 models, inferences_per_model=100):
        self.serial = serial
        self.queue = Queue()
        self.queue_should_read = Event()
        self.queue_shutdown = Event()
        self.queue_process = Process(target=self.read_to_queue)
        self.queue_process.start()

        self.models = models
        self.inference_num = inferences_per_model
        self.states = states

        self.date = date.today()
        print(">>> Current date:", self.date)
        
        self.path_folder = "measurements/" + str(self.date)
        os.makedirs(self.path_folder, exist_ok=True)
        
        self.cur_file = None
        self.path_folder_model = None
        self.path_folder_model_inference = None

    def __del__(self):
        if self.cur_file is not None: self.cur_file.close()
        self.queue_shutdown.set()
    
    def read_to_queue(self):
        print("\t Process -> read_to_queue : started")
        while self.queue_should_read.wait() and not self.queue_shutdown.is_set():
            while self.serial.in_waiting >= 8:
                self.queue.put(self.serial.read(8))
        print("\t Process -> read_to_queue : ended")

    def write(self, string:str):
        if self.cur_file is None: raise FileNotFoundError
        self.cur_file.write(string)

    def read_setup(self):
        line = ""    
        while line != "setup_finished":
            try:
                if line != "": print(line)
                line = self.serial.readline().decode().rstrip()
            except: continue     
        self.queue_should_read.set()
        print("Pronto para realizar leituras")
    
    def read_measurement(self):
        measurement_count = 0
        time_first = None
        time_last = None
        while True:
            data = self.queue.get()
            if b'staswap' in data: break
            try:
                data = struct.unpack('IHH', data) # I: unsigned long | H: unsigned short
                line_to_write = f"{data[0]},{data[1]},{data[2]},{data[1]*data[2]/1000}\n"
                if time_first is None: time_first = data[0]
                measurement_count += 1
                
                self.write(line_to_write)
                time_last = data[0]
            except Exception as error:
                print("Ignored input:", error.args[0])

        time_delta = (time_last - time_first) / 1000000
        print(f"{measurement_count}/{time_delta:.2f}s = " +
            f"{(measurement_count / time_delta):.2f}/s | ", end="", flush=True)

    def read_measurements(self):
        for model in self.models:
            self.path_folder_model = self.path_folder + "/" + model
            os.makedirs(self.path_folder_model, exist_ok=True)

            print("\n========================================================")
            print(">>> Model changed to", model)

            for inference in range(self.inference_num):
                inference_path = "inference_" + str(inference)
                self.path_folder_model_inference = self.path_folder_model + "/" + inference_path
                os.makedirs(self.path_folder_model_inference, exist_ok=True)

                # waiting for start of measurements
                while True:
                    data = self.queue.get()
                    if b'staswap' in data: break

                print(f"\tInference {inference}: ", end="", flush=True)
                for state in self.states:
                    if self.cur_file is not None: self.cur_file.close()
                    
                    path_file = self.path_folder_model_inference + "/" + state + ".csv"

                    self.cur_file = open(path_file, "w")
                    self.cur_file.write("timestamp,volt,miliampere,watt\n")
                    self.read_measurement()
                print()
        print()
        