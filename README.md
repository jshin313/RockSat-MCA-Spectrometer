# CapeMCA + Spectrometer Code

This hosts the Arduino and native code for communicating with the MCA

## Structure
* `capemca_example/`: This holds the example code provided by the CapeMCA v1.3.5 software.
    * Windows UART example
    * Windows and Linux USB examples
* `USB_Arduino/`: This holds the code for communicating with the MCA via an Arduino using USB
* `Serial_Arduino/`: This holds the code for communicating with the MCA via an Arduino using Serial UART
* `Spectrometer_Breakout_Board/`: Holds the KiCAD files for the UART connector breakout board

## TODO
* Requires reconnecting MCA when starting up
* After a long time, CRC error
