
// -----------------------------------------------------------------------------
// User Configuration
// -----------------------------------------------------------------------------

#define DEBUG_MODE 1  // Begins flight observations 2 minutes after WiFi
//#define IGNORE_TPRH_CHECK 1 // Uncomment to allow no I2C solder for T,P, RH

// If using a single-channel gateway, uncomment this next option and set to your gateway's channel
//#define SINGLE_CHANNEL_GATEWAY  0

// Helium only uses OTAA
#define USE_OTAA 1

#define MESSAGE_TO_SLEEP_DELAY  5000        // Time after message before going to sleep
#define JOIN_WAIT              7000        // Time to wait for LoRaWAN join
#define QRCODE_DELAY            10000        // Time to show logo on first boot

#define OBSERVATION_PORT        1          // Port the obs messages will be sent to
#define GPS_LORAWAN_PORT        2
#define AUTHENTICATION_PORT     3          // Port for authentication message
#define STATUS_PORT             4          // Port the status messages will be sent to

#define LORAWAN_CONFIRMED_EVERY 0           // Send confirmed message every these many messages (0 means never)
#define LORAWAN_SF              DR_SF10     // Spreading factor
#define LORAWAN_ADR             0           // Enable ADR

#define MISSING_UINT            0
#define MISSING_FLOAT           0

//Wifi defines
const char* SSID     = "AscensionWx";
const char* PASSWORD = "weather123";
