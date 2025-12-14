import serial
import csv

logging = open('logging.csv', mode='a', newline='')
writer = csv.writer(logging, delimiter=",", escapechar=' ', quoting=csv.QUOTE_NONE)

ser = serial.Serial('COM3', 115200)  # Port, Baudrate
ser.flushInput()

while True:
    # Zeile als Bytes lesen
    ser_bytes = ser.readline()
    print(ser_bytes)

    # Bytes -> String und Zeilenende (\r\n) abschneiden
    decoded = ser_bytes.decode("utf-8").strip()
    print(decoded)

    # Stop-Bedingung
    if decoded == "stop":
        break

    # Erwartetes Format: "temp,millis"
    # z.B. "201.532,12345"
    temp_str, ms_str = decoded.split(",")

    # Optional: in Zahlen umwandeln (für Plausibilitätscheck etc.)
    temp = float(temp_str)
    ms = int(ms_str)

    # In CSV schreiben (Reihenfolge wie du magst)
    writer.writerow([temp, ms])
    # oder: writer.writerow([temp, ms])

ser.close()
logging.close()
print("logging finished")
