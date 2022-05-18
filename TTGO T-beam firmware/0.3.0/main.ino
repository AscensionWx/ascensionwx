/*

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Unless otherwise stated, all other files under this Arduino program
fall under the GPL license.

This code requires the MCCI LoRaWAN LMIC library
by IBM, Matthis Kooijman, Terry Moore, ChaeHee Won, Frank Rose
https://github.com/mcci-catena/arduino-lmic

*/

#pragma once

const char * APP_NAME = "dClimateIot";
const char * APP_VERSION = "0.3.0";

#include "config_hardware.h"

#include <Arduino.h>
#include <lmic.h>
void lorawan_register(void (*callback)(uint8_t message));

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "config_user.h"
#include "rom/rtc.h"
#include <TinyGPS++.h>
#include <Wire.h>
#include <Math.h>
#include <axp20x.h>

#include <LoraMessage.h>
#include <LoraEncoder.h>

#include <WiFi.h>

bool SSD1306_FOUND = false;
bool BME280_FOUND = false;
bool SHT21_FOUND = false;
bool SPL06_FOUND = false;
bool AXP192_FOUND = false;
bool RELAIS_ON = false;

// Message _counter, stored in RTC memory, survives deep sleep
RTC_DATA_ATTR uint32_t COUNT = 0;

int SEND_INTERVAL;
bool GATEWAY_IN_RANGE;
String DOWNLINK_RESPONSE = "";
bool AUTHENTICATED = false;
bool BATT_CHARGING = false;
int8_t APPROX_UTC_HR_OFFSET = 0;

uint64_t last_seen_charging = 0;
uint64_t last_time_refresh = 0; // Most straight forward to store this as millis()
float time_drift = 0;
uint64_t last_gps_refresh = 0;

// Define how often to do time sync of esp32 clock with GPS
//   ... increase occurence to get better unix time stamps
//   ... decrease to save battery ; lower risk of GPS module wearing out with 1000s of on/off commands on monthly time frame
const int hr_milliseconds = 60*60*1000;
const int time_refresh_rate = hr_milliseconds*3; // How often we attempt time sync
const int wifi_on_millis = 3*60*1000;

// Launch-specific values
float SFC_PRESSURE = 0.0;
bool IF_WEB_OPENED = false;
bool IF_LAUNCHED = false;
uint8_t LAUNCH_ID[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } ; // For launch_id downlink

// Stores all the measured temperatures in the past 24 hours
int8_t temperatures[100];

// Set web server port number to 80
WiFiServer SERVER(80);
WiFiClient CLIENT;

// Used in power management
//  Will be used to turn off GPS, check if charging, battery etc
AXP20X_Class axp;


// -----------------------------------------------------------------------------
// Application
// -----------------------------------------------------------------------------



//////////////////////////////////////////
///////////// ARDUINO SETUP //////////////
//////////////////////////////////////////
void setup() {
  
  // Debug
  #ifdef DEBUG_PORT
  DEBUG_PORT.begin(SERIAL_BAUD);
  #endif

  // Helps to briefly wait for serial baud
  while ( millis() < 500 );
  
  Wire.begin(I2C_SDA, I2C_SCL);
  scanI2Cdevice();

  // Buttons & LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Disable brownout detector
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Display
  screen_setup();
  
  Serial.begin(115200);
  
  #ifndef IGNORE_TPRH_CHECK
  if (!if_T_and_RH_sensor() || !if_P_sensor())
  {
    screen_print("[ERR]\n");
    screen_print("No weather sensor \n");
    screen_print("connected! Check wiring.");
    Serial.println("[ERR] No weather sensor connected!");
    delay(MESSAGE_TO_SLEEP_DELAY);
    screen_off();
    sleep_forever();
  }
  #endif

  // Begin looking at AXP on I2C
  if(axp.begin(Wire, AXP192_SLAVE_ADDRESS) == AXP_FAIL) {
      Serial.println(F("failed to initialize communication with AXP192"));
  }

  // Init BME280
  bme_setup();

  // Init GPS
  gps_power_on();
  gps_setup();

  // TTN setup
  if (!lorawan_setup()) {
    screen_print("[ERR] Radio module not found!\n");
    Serial.println("[ERR] Radio module not found!");
    delay(MESSAGE_TO_SLEEP_DELAY);
    screen_off();
    sleep_forever();
  }

  lorawan_register(callback);
  lorawan_join();
  lorawan_sf(LORAWAN_SF);
  lorawan_adr(LORAWAN_ADR);
  if(!LORAWAN_ADR){
    LMIC_setLinkCheckMode(0); // Link check problematic if not using ADR. Must be set after join
  }

  // Show logo on first boot (if OLED attached)
  //screen_print(APP_NAME " " APP_VERSION " " DEVNAME " " get_freq(), 0, 0 ); // print firmware version in upper left corner
  screen_clear();

  // Print initial header at top
  char screen_buff[40];
  strncpy( screen_buff, APP_VERSION, 5 );
  strncat( screen_buff, "   ", 3 );
  strncat(screen_buff, get_devname(), 12);
  strncat( screen_buff, "   ", 3);
  strncat( screen_buff, get_freq(), 3);
  screen_print(screen_buff, 0, 0);
  
  screen_show_logo(); 
  screen_update();
  time_loop_wait( LOGO_DELAY ); // If Join_accept message received during this time, it will trigger LMIC callback function

  Serial.println();
  Serial.printf("Chip Temp: %f\n", axp.getTemp());
  Serial.printf("Chip TS Temp: %f\n", axp.getTSTemp());
  Serial.printf("Battery Percentage: %d\n", axp.getBattPercentage());

  Serial.printf("DCDC1: %s\n", axp.isDCDC1Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("DCDC2: %s\n", axp.isDCDC2Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("DCDC3: %s\n", axp.isDCDC3Enable() ? "ENABLE" : "DISABLE");

  if ( !GATEWAY_IN_RANGE )
  {
    // Listen for LoRaWAN response
    Serial.println("Waiting for OTAA Joined response.");
    screen_print("Please wait!\n\n");
    screen_print("Listening for LoRaWAN\n");
    screen_print("network coverage...\n");

    int time_check_start = millis();
    time_loop_wait( 5*1000 ); 
    while( !GATEWAY_IN_RANGE && !time_loop_check( time_check_start, 30*1000 ) )
    {
      send_status(true); // Receive possible missed join message or downlinks
      screen_update();
      time_loop_wait( 5*1000 );// Wait 5 seconds
    }
  }

  // Check that at least one confirmed message was received
  #if !defined(IGNORE_GATEWAY_CHECK)
  if (!GATEWAY_IN_RANGE) // If gateway still isn't in range, we cannot continue
  { 
    screen_print("[ERR]\n");
    screen_print("No LoRaWAN coverage!\n\n");
    screen_print("Please find a better\n");
    screen_print("place outdoors.\n");
    Serial.println("[ERR] LoRaWAN network not connected!");
    delay(MESSAGE_TO_SLEEP_DELAY);
    screen_off();
    gps_power_off();
    sleep_forever();
  }
  #endif

  Serial.println("LoRaWAN connected. Starting WiFi server.");
  //screen_show_qrcode();
  //screen_update();
  screen_print("LoRaWAN Connected!\n");
  screen_print("WiFi AP:  dClimateIot\n");
  screen_print("  password:  weather123\n");
  screen_print("\n");
  screen_print("Then go to  192.168.4.1\n");

  // Init wifi access point
  WiFi.softAP(SSID, PASSWORD);

  // Defines how async server handles GET requests
  //initServer();
  SERVER.begin();

  SEND_INTERVAL = 30000; // Status message this many millis
  uint32_t last = 0;

  bool enter_obs_mode = false;
  int time_check_start = millis();
  while ( !AUTHENTICATED && !enter_obs_mode ) {

    os_runloop_once();

    checkServer();
    gps_read(); // Do we need this to run so often?
    screen_loop();

    #ifdef DEBUG_MODE
      if (time_loop_check( time_check_start, wifi_on_millis )) {  // In debug mode, we start broadcasting observations after 3 minutes
         enter_obs_mode = true;
      }
    #endif
    
  }// End IF_AUTHENTICATED check

  // Close access to the server
  SERVER.end();
  // Ends wifi connection completely
  WiFi.softAPdisconnect(true);

  if (AUTHENTICATED) {
    screen_print("\nAuthentication success!\n\n");
  } else {
    screen_print("\nAuth Window closed.\n\n");
  }
  screen_print("Entering observation mode. \n");
  screen_print("  Turning off display...\n\n");
  time_loop_wait( 5*1000 ); // Wait 5 seconds so user can see message

  // Do initial clock sync before we begin
  sync_rtc_with_gps();
  calc_utc_offset();

  screen_off();
  gps_power_off(); // Cut power supply to GPS chip (saves about 20-35ma)

}// End Arduino setup() method

/////////////////////////////////////////
///////////// ARDUINO LOOP //////////////
/////////////////////////////////////////
void loop() {

  Serial.println("Sensor's unix time approximation:");
  Serial.println(get_unix_time());
  Serial.println("Approx local hour:" );
  Serial.println(get_approx_local_hour());
  Serial.println();

  uint64_t loop_start = millis();
  
  gps_read();  
  lorawan_loop();
  //screen_loop();
  
  //if (0 < gps_hdop() && gps_hdop() < 50 && gps_latitude() != 0 && gps_longitude() != 0) {

  // Warning: millis() is stored as uint32. This means integer overflow at 49 days.

  // Getting AXP temperature is best way to see if board is charging / charged recently
  // 50 Celcius is 122 degrees Farenheight
  //   -> May need toggling, particularly for cold outdoor temperatures
  if ( axp.getTemp() > 50)   
  {
    BATT_CHARGING = true;
    Serial.println("Battery charging: TRUE");
    last_seen_charging = millis();
  } else {
    BATT_CHARGING = false;
    Serial.println("Battery charging: FALSE");
  }

  if ( !time_loop_check( last_seen_charging , hr_milliseconds ) )
  {
    // TODO: Set some sort of flag here before we send the observation
    // Serial.println("AXP temperature indicates battery charging/charged recently. Flagging observation.");
  }

  send_observation();
  update_temperature_list( (uint8_t)get_temperature() );
  time_loop_wait( 2500 ); // Wait 2.5 seconds for observation message to complete

  // Note:  The subsequent send_observation() call will run asynchronously for the next two seconds.
  //   Next we update the ESP32 clock. To do whis, we turn it on, grab the time, and then turn it back off. 
  //   A full fix isn't necessarily needed to get the gps_time... the NEO6M has its
  //     own internal battery and clock that can store the GMT time for about 2 weeks.
  
  if( time_loop_check( last_time_refresh , time_refresh_rate ) )
  //if( time_loop_check( last_time_refresh , 0 ) ) // for testing
  {
    Serial.println("Beginning scheduled RTC time refresh.");
    
    // Turn on GPS
    gps_power_on();
    gps_setup();

    // Wait to get a partial fix and new timestamp from GPS
    if (wait_for_gps_time()) // returns true if GPS was able to get time
    {
      Serial.println("GPS time grabbed. Syning with ESP32 RTC.");
      sync_rtc_with_gps();
    } else {
      Serial.println("GPS time grab wait timed out.");
    }

  } else {
    Serial.println("RTC time sync not needed.");
  }

  // Finally if it's around 3am local, we do all-out GPS update
  //   - Checking 3am local time (instead of UTC) ensures it's middle of the night (better GPS reception).
  //     Also allows for location reports being spaced out in time by all sensors on the network
  //if( time_loop_check( last_gps_refresh , hr_milliseconds*3 ) ) // for testing
  if ( time_loop_check( last_gps_refresh , hr_milliseconds*2 ) && 
       ( get_approx_local_hour()==3 ) )
  {
    Serial.println("Beginning GPS refresh");

    time_loop_wait( 3000 ); // Wait more seconds to put some time between send_gps() and send_obs();
    
    // Turn on GPS
    gps_power_on();
    gps_setup();

    if (wait_for_gps_location()) // returns true if GPS was able to get location
    {
      Serial.println("GPS fix successful. Syning with ESP32 RTC.");
      sync_rtc_with_gps(); // if we got location, we can have high confidence, we also got time
      calc_utc_offset();
      send_gps();
    } else {
      Serial.println("GPS fix wait timed out.");
      send_empty_gps();
    }

    // Finally wait for lmic to finish sending GPS data
    time_loop_wait( 3000 );
    
    last_gps_refresh = millis();

  }

  // Turn off GPS in case it was on during RTC clock and GPS sync
  gps_power_off();

  // Sleep for about 15 minutes
  Serial.println("Going to sleep.");

  time_loop_wait(50);// Let Serial catch up to view messages
  uint16_t loop_time_total = (uint16_t)((millis()-loop_start)/1000);
  sleep_light_seconds( 900 - loop_time_total );
  //sleep_light_seconds( 60 - loop_time_total );
}
