import serial
import time
import struct

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200

def read_from_esp32():
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)

    print("Reading from ESP32...")

    try:
        with open("leitura.csv", 'w') as leitura:
            leitura.write("timestamp,voltage,ampere,power\n")
            while True:
                try:
                    line = ser.readline().decode().rstrip()
                    while line != "start":
                        if line != "": print(line)
                        line = ser.readline().decode().rstrip()
                    break
                except:
                    pass    

            print("Started Reading")
            while True:
                data = ser.read(16)
                if data[:3] == b'end': break
                try:
                    data = struct.unpack('Ifff', data)
                    line_to_write = f"{data[0]},{data[1]},{data[2]},{data[3]}\n"
                    leitura.write(line_to_write)
                    print("Read:", line_to_write)
                except:
                    print("Ignored input", data)
            print("Finished Reading")
            try:
                while True:
                    line = ser.readline().decode().rstrip()
                    if line != "": print(line)
            except:
                pass

        
    except KeyboardInterrupt:
        print("Exiting...")

    finally:
        ser.close()

if __name__ == "__main__":
    read_from_esp32()
