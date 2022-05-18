These files are a stable release of the Arduino-based firmware for the Kanda Weather Sensors. It includes:

- Miner authentication over the ESP32's web server
- Weather observations every 15 minutes over Helium/TheThingsNetwork
- Real time clock adjustments periodically from the GPS unit
- GPS geolocation messages during the night

Not supported:

- On-device signatures

It is recommended the user have a blank Arduino environment. And then unzip the dclimateiot_arduino_libs file in the relevant Arduino "libraries" folder.

Note that when you are ready to upload the firmware, you must first adjust the lmic_config file located in the library to match the correct frequency. 

Also note, that the correct LoRaWAN keys need to be loaded into the credentials.h file. For OTAA, DevEUI and AppEUI need to be little-endian (LSB).This can be interpretted from Helium or TheThingsNetwork consoles. The AppKEY can be big-endian. 

