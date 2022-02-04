### M5Atom_GPS_FreqCount

a simple test for frequency counting, GPS and aModBus with an ESP32

- frequency counting works up to 100.000 Hz, beyond this value the calculation is struggling...
- tested with an oszilloscope the frequencies in the lower regions are very accurate
- modbus is also working (because of threading there might be an issue - but in the moment everything is OK)
- RemoteXY is nice - but only a quick solution - used only if AP is available

- shows, that Serial(2), ModBus-Serial and GPIO33 are working together on an M5Atom Lite device.
