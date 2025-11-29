import serial
import time

SERIAL_PORT = '/dev/ttyACM2'
BAUD_RATE = 115200

from python_modules.states import StateWriter

states = ["preinferencia", "inferencia", "posinferencia"]

def read_from_esp32():
    esp32 = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    writer = StateWriter(esp32, states, 512)
    time.sleep(2)
    print(">>> Reading from ESP32...")

    try:
        writer.read_setup()

        while not writer.ended():
            writer.read_measurement()

    except KeyboardInterrupt:
        print("\nExiting...")

    finally:
        esp32.close()
        print("\nEsp32 connection closed.")

if __name__ == "__main__":
    read_from_esp32()
