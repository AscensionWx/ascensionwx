rem May need to update COM port 

rem Need to copy lmic config file to arduino lmic library

rem C:\Users\nick_\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\3.3.0/esptool.exe --chip esp32 --port COM6 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 C:\Users\nick_\AppData\Local\Temp\arduino_build_558325/main.ino.bootloader.bin 0x8000 C:\Users\nick_\AppData\Local\Temp\arduino_build_558325/main.ino.partitions.bin 0xe000 C:\Users\nick_\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.4/tools/partitions/boot_app0.bin 0x10000 C:\Users\nick_\AppData\Local\Temp\arduino_build_558325/main.ino.bin 
C:\Users\nick_\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\3.3.0/esptool.exe --chip esp32 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 C:\Users\nick_\AppData\Local\Temp\arduino_build_558325/main.ino.bootloader.bin 0x8000 C:\Users\nick_\AppData\Local\Temp\arduino_build_558325/main.ino.partitions.bin 0xe000 C:\Users\nick_\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.4/tools/partitions/boot_app0.bin 0x10000 C:\Users\nick_\AppData\Local\Temp\arduino_build_558325/ascensionwx_0-4-0_915mhz.bin 