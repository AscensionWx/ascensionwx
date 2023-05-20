rem Sets sim link to correct lmic frequency config file

rem Need to copy lmic config file to arduino lmic library
copy /y lmic_configs\915.h "C:\Users\nick_\Documents\Arduino_Beta\libraries\arduino-lmic\project_config\lmic_project_config.h"