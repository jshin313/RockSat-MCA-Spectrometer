import serial
ser = serial.Serial(port="COM3", baudrate=115200, bytesize=8, stopbits=serial.STOPBITS_ONE)  # open serial port
print(ser.name)         # check which port was really used

cmd = bytearray([0, 1])
ser.write(cmd)
line = ser.read(1024)
print(line)
