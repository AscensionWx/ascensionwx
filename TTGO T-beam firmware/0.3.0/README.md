These files are a stable release of the Arduino-based firmware for the Kanda Weather Sensors. It includes:

- Miner authentication over the ESP32's web server
- Weather observations every 15 minutes over Helium/TheThingsNetwork
- Real time clock adjustments periodically from the GPS unit
- GPS geolocation messages during the night

Not supported:

- On-device signatures

Note that if you wish to load firmware using these files, you must adjust the lmic_config file located in the library to match the correct frequency. 

Also note, that the correct LoRaWAN keys need to be loaded into the credentials.h file. For OTAA, DevEUI and AppEUI need to be little-endian (LSB).This can be interpretted from Helium or TheThingsNetwork consoles. The AppKEY can be big-endian. 

