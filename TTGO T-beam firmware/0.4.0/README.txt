Changes in this version:

Reduce observation payload size from 31 bytes to 9 bytes
Every 3 hours, do location message and ESP time refresh
New send_gps() message is 10 bytes
Combining 3am GPS logic into 3-hourly loop
Remove Authentication sections
Place TX_COMPLETE back in library and output TX output
Rename ttn.ino to lorawan.ino
Remove credentials.h
Remove elevation calculation
Use new 6 byte latlon encoder instead of 8 byte
Replaced if_P_sensor() bitflag with if_gps_on()
Added if_wifi_on()
Changed the first 4 bit flags
Updated webpage to show devname
