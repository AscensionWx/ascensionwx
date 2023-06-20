// This sketch supports EU868 and US915

// The Arduino-LMIC library by MCCI Catena is set to US915,
// these settings have to be copied over the ones in the
// lmic_project_config.h file in the library,
// inside the project_config folder.

// Make sure only one of the following is defined (CFG_us915 or CFG_eu868)
#define CFG_us915 1
//#define CFG_eu868 1
//#define CFG_as923 1 // Southeast asia
//#define CFG_au921 1 // Australia
//#define CFG_in866 1 // India

// DO NOT modify this
#define CFG_sx1276_radio 1
