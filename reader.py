import serial
import time

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200

from python_modules.states import StateWriter
from sys import argv

states = ["preinferencia", "inferencia", "posinferencia"]

def read_from_esp32():
    if len(argv) > 1:
        if argv[1] == "whisper":
            models = ("tiny", "base", "small", "medium", "turbo")
        elif argv[1] == "yolo":
            models = ('yolov8n', 'yolov8s', 'yolov8m', 'yolov8l', 'yolov8x')
        else:
            print(f"Models {argv[1]} not available")
            exit()
    else:
        print("Models not specified")
        exit()

    esp32 = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)
    esp32.setDTR(False)
    time.sleep(1)
    esp32.setDTR(True)

    writer = StateWriter(esp32, states, models, 100)
    
    print(">>> Reading from ESP32...")

    try:
        writer.read_setup()
        writer.read_measurements()
        
    except KeyboardInterrupt:
        print("\nExiting...")

    finally:
        esp32.close()
        print("\nEsp32 connection closed.")

if __name__ == "__main__":
    read_from_esp32()
