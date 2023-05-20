
set "firmware_bin=ascensionwx_0-4-4_868mhz.bin"

set "bootload_bin=..\main.ino.bootloader.bin"
set "partitian_bin=..\main.ino.partitions.bin"
set "app_bin=..\boot_app0.bin"


..\..\esptool.exe --chip esp32 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 %bootload_bin% 0x8000 %partitian_bin% 0xe000 %app_bin% 0x10000 %firmware_bin%

rem If more than one COM ports open on PC, need to specify the COM port of the device
set port=COM7
rem ..\..\esptool.exe --chip esp32 --port %port% --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x1000 %bootload_bin% 0x8000 %partitian_bin% 0xe000 %app_bin% 0x10000 %firmware_bin%
