import serial
ser = serial.Serial(port="COM7", baudrate=115200, bytesize=8, stopbits=serial.STOPBITS_ONE)  # open serial port
print(ser.name)         # check which port was really used

with open("dump.txt", "w") as f:
    for i in range(2000):
        line = ser.readline().decode("utf-8")
        print(line, end="")
        f.write(line)
