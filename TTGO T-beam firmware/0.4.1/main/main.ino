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

#include "config_hardware.h"

#include <Arduino.h>
#include <lmic.h>
#include <Preferences.h>

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

// Include esp32 version of WiFi.h
#include <WiFi.h>

const char * APP_VERSION = "0.4.1";
String APP_VERSION_STR = "0.4.1";

Preferences preferences;

// Set web server port number to 80
WiFiServer SERVER(80);
WiFiClient CLIENT;

// Used in power management
//  Will be used to turn off GPS, check if charging, battery etc
AXP20X_Class axp;

bool SSD1306_FOUND = false;
bool BME280_FOUND = false;
bool SHT21_FOUND = false;
bool SPL06_FOUND = false;
bool AXP192_FOUND = false;
bool RELAIS_ON = false;

// Message _counter, stored in RTC memory, survives deep sleep
RTC_DATA_ATTR uint32_t COUNT = 0;

bool GATEWAY_IN_RANGE;
String DOWNLINK_RESPONSE = "";
bool AUTHENTICATED = false;
bool BATT_CHARGING = false;
int8_t APPROX_UTC_HR_OFFSET = 0;

uint64_t last_seen_charging = 0;
uint64_t last_gps_message = 0; // Most straight forward to store this as millis()
float time_drift = 0;
uint64_t last_gps_refresh = 0;
uint64_t last_gps_send = 0;

bool overcharge_mode = false;
bool undercharge_mode = false;
bool if_deployed = false;

// Define how often to do time sync of esp32 clock with GPS
//   ... increase occurence to get better unix time stamps
//   ... decrease to save battery ; lower risk of GPS module wearing out with 1000s of on/off commands on monthly time frame
const int hr_milliseconds = 60*60*1000;
const int gps_send_interval = hr_milliseconds*3;

// Launch-specific values
float SFC_PRESSURE = 0.0;
bool IF_WEB_OPENED = false;

// Stores all the measured temperatures in the past 24 hours
int8_t temperatures[100];

char* DEVNAME = "";
//u1_t DEVEUI[8];
u1_t NULL_KEY[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void lorawan_register(void (*callback)(uint8_t message));


// -----------------------------------------------------------------------------
// Application
// -----------------------------------------------------------------------------



//////////////////////////////////////////
///////////// ARDUINO SETUP //////////////
//////////////////////////////////////////
void setup() {

  /////// BEGIN HARDWARE CHECKS /////////
  
  // Debug
  #ifdef DEBUG_PORT
  DEBUG_PORT.begin(SERIAL_BAUD);
  #endif

  // Helps to briefly wait for serial baud
  while ( millis() < 500 );

  // Allow print output to Serial
  Serial.begin(115200);

  // Buttons & LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  scanI2Cdevice();

  // Begin looking at AXP on I2C
  if(axp.begin(Wire, AXP192_SLAVE_ADDRESS) == AXP_FAIL) {
      Serial.println(F("failed to initialize communication with AXP192"));
  }

  Serial.println();
  Serial.printf("Chip Temp: %f\n", axp.getTemp());
  Serial.printf("Chip TS Temp: %f\n", axp.getTSTemp());
  Serial.printf("Battery voltage: %d\n", axp.getBattVoltage());

  Serial.printf("DCDC1: %s\n", axp.isDCDC1Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("DCDC2: %s\n", axp.isDCDC2Enable() ? "ENABLE" : "DISABLE");
  Serial.printf("DCDC3: %s\n", axp.isDCDC3Enable() ? "ENABLE" : "DISABLE");
  
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

  // Init BME280
  bme_setup();

  // Init GPS
  gps_power_on();
  gps_setup();

  // LoRaWAN setup
  if (!lorawan_setup()) {
    screen_print("[ERR] Radio module not found!\n");
    Serial.println("[ERR] Radio module not found!");
    delay(MESSAGE_TO_SLEEP_DELAY);
    screen_off();
    sleep_forever();
  }

  // Init display
  screen_setup();
  screen_clear();

  // Print initial header at top
  char screen_buff[40];
  strncpy( screen_buff, APP_VERSION, 5 );
  strncat( screen_buff, "   ", 3 );
  strncat(screen_buff, get_devname(), 12);
  strncat( screen_buff, "   ", 3);
  strncat( screen_buff, get_freq(), 3);
  screen_print(screen_buff, 0, 0);

  /////// END HARDWARE CHECKS /////////

  /////// BEGIN LORAWAN INIT  /////////

  // Initialize namespace for LoRaWAN keys
  preferences.begin("lorawan", false);
  axp.setChgLEDMode(AXP20X_LED_BLINK_4HZ);

  if( rtc_get_reset_reason(0) == 1 ) // Button pressed or vbat on (non-software)
    if_deployed = false;
  else // reset by software
    if_deployed = true;

  if ( lorawan_keys_set() )
  {
    Serial.println("Lorawan keys found.");
    lorawan_register(callback);
    lorawan_sf(LORAWAN_SF);
    lorawan_adr(LORAWAN_ADR);

    Serial.println("Beginning Join attempt.");
    lorawan_join();
    if(!LORAWAN_ADR){
      LMIC_setLinkCheckMode(0); // Link check problematic if not using ADR. Must be set after join
    }
  } else {
    Serial.println("Lorawan credentials empty.");
  }

  Serial.println("Waiting in Join window.");
  screen_show_logo(); 
  screen_update();
  time_loop_wait( JOIN_WAIT ); // If Join_accept message received during this time, it will trigger LMIC callback function

  if (GATEWAY_IN_RANGE)
    axp.setChgLEDMode(AXP20X_LED_LOW_LEVEL);
  else
  {
    Serial.println("No OTAA Join received yet. Continuing to listen for ack.");
    screen_print("Please wait!\n\n");
    screen_print("Listening for LoRaWAN\n");
    screen_print("network coverage...\n");

    // Send status messages for 20 seconds trying to get a response
    if ( lorawan_keys_set() )
    {
      int begin_status_msg = millis();
      send_status(true);
      int last_status_msg = millis();

      time_loop_wait(5000);

      while ( !time_loop_check(begin_status_msg , 20000) && !GATEWAY_IN_RANGE )
      {
        // We continue to send statuses every 5 seconds in case we hear an OTAA Join, gateway ack, or downlink
        send_status(true); // Receive possible missed join message or downlinks
        time_loop_wait(5000);
      }
    } // end status message loop
  } // Join not immediately successful
  
  if( GATEWAY_IN_RANGE ) 
  {
    axp.setChgLEDMode(AXP20X_LED_LOW_LEVEL); // set LED constantly lit
    Serial.println("LoRaWAN connected. Will still enable WiFi for at least 1 minute");
    screen_print("LoRaWAN Connected!\n");
  }
  else
    axp.setChgLEDMode(AXP20X_LED_BLINK_1HZ);

  /////// LORAWAN CHECKS COMPLETE  ///////

  ////////// BEGIN WIFI INIT /////////////

  // If not deployed, 1 minute for WiFi or until keys are entered. 
  //  Note: If the webpage is opened, wifi will increase to 5 minutes
  if (!if_deployed) {
    
    // Start WiFi server
    Serial.println("Starting WiFi server.");

    // Init wifi access point
    WiFi.softAP(SSID, PASSWORD);
    //initServer();
    SERVER.begin();
    int wifi_time_start = millis();

    while ( !time_loop_check( wifi_time_start, get_webpage_timeout_ms() ) && !if_new_keys_entered() ) {

      // Helps in case there are lingering lmic messages
      os_runloop_once();

      checkServer();
      screen_loop();
    
    }// End WiFi wait loop

    Serial.println("WiFi window closed. Turning off WiFi.");

    // Close access to the server
    SERVER.end();
    // Ends wifi connection completely
    WiFi.softAPdisconnect(true);

    // Turn off LED flash to indicate WiFi window closed
    axp.setChgLEDMode(AXP20X_LED_OFF);
    
  }

  if ( if_new_keys_entered() ) {
    Serial.println("Device restarting.");
    Serial.println();
    ESP.restart();
  } 

  // Check that at least one confirmed message was received
  //if ( false ) // For debugging ... ignores gateway check
  if (!GATEWAY_IN_RANGE) // If gateway still isn't in range, we try again in 3 hours
  { 
    screen_print("[ERR]\n");
    screen_print("No LoRaWAN coverage!\n\n");
    screen_print("Please find a better\n");
    screen_print("place outdoors.\n");
    Serial.println("[ERR] LoRaWAN network not connected!");
    delay(MESSAGE_TO_SLEEP_DELAY);

    screen_off();
    gps_power_off();

    // Try again much later
    Serial.println("");
    Serial.printf("Going to sleep for %d seconds", 60*60);
    //sleep_light_seconds(10);
    sleep_light_seconds(60*60);

    // Restart
    preferences.end();
    ESP.restart();
  }

  //screen_show_qrcode();
  //screen_update();
  /*
  screen_print("LoRaWAN Connected!\n");
  screen_print("WiFi AP:  dClimateIot\n");
  screen_print("  password:  weather123\n");
  screen_print("\n");
  screen_print("Then go to  192.168.4.1\n");
  */

  screen_print("Entering observation mode. \n");
  screen_print("  Turning off display...\n\n");
  time_loop_wait( 5*1000 ); // Wait 5 seconds so user can see message

  // Do initial clock sync before we begin
  sync_rtc_with_gps();
  calc_utc_offset();

  // Closes access to the EEPROM
  preferences.end();

  // Turn off final bits
  axp.setChgLEDMode(AXP20X_LED_OFF);
  screen_off();
  gps_power_off(); // Cut power supply to GPS chip (saves about 20-35ma)

}// End Arduino setup() method

/////////////////////////////////////////
///////////// ARDUINO LOOP //////////////
/////////////////////////////////////////
void loop() {

  ////////////// Undervoltage Check  //////////////////
  if( axp.getBattVoltage() < 3300 ) {
    // Do bare minimum with 1-hour between observations
    undercharge_mode = true;
    send_observation();
    update_temperature_list( (uint8_t)get_temperature() );
    time_loop_wait( 3000 ); // Wait 3 seconds for observation message to complete
    
    sleep_light_seconds( 60*60 ); // Sleep for 1 hour
    return;  // Skip all other statements are restart loop.
  } else {
    undercharge_mode = false;
  }
  /////////////////////////////////////////////////////

  ///////////  Solar Overcharge Adjustment GPS  /////////////////
  // Note: This is a software fix to a hardware problem
  //   Currently, the 5V solar panel bypasses the regulator through 5V pin,
  //    so to prevent overcharging, we turn on GPS (20-45ma) when battery voltage is high.
  //   This should allow net outflow from the battery, since solar panel is 60ma only at peak sun.
  //////////////////////////////////////////////////////////
  Serial.println("Battery milli-voltage is: "); 
  Serial.println(axp.getBattVoltage());
  if( axp.getBattVoltage() > 3750 ) {
    Serial.println("Setting overcharge to true.");
    overcharge_mode = true;
  } 
  else if (axp.getBattVoltage() < 3600 ) {
    Serial.println("Setting overcharge to false.");
    overcharge_mode = false;
  }

  Serial.println("Sensor's unix time approximation:");
  Serial.println(get_unix_time());
  Serial.println("Approx local hour:" );
  Serial.println(get_approx_local_hour());
  Serial.println();

  uint64_t loop_start = millis();
  
  gps_read();  
  lorawan_loop();
  //screen_loop();

  // Restart every 45 days to avoid millis() overflow and get fresh LoRaWAN join
  if (loop_start > 3888000000) ESP.restart();
  
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

  ////////////// SEND OBSERVATION ////////////////

  send_observation();
  update_temperature_list( (uint8_t)get_temperature() );
  time_loop_wait( 5000 ); // Wait 2.5 seconds for observation message to complete

  /////////////////  GPS Send Message and Time Refresh  /////////////
  // Every 3 hours, we send a GPS message
  // Also, if it is 2, 3, or 4am we keep GPS on for additional 5 minutes
  //   - Checking 3am local time (instead of UTC) ensures it's middle of the night (better GPS reception).
  //   - Also allows for location reports being spaced out in time by all sensors on the network
  ///////////////////////////////////////////////////////////////////
  if( time_loop_check( last_gps_send , gps_send_interval ) )
  {
    Serial.println("Beginning scheduled GPS uplink and time time refresh");
    
    // Turn on GPS
    gps_power_on();
    gps_setup();

    // If local hour is 2,3,or 4am then wait 5 minutes to try and get GPS fix
    uint8_t local_hour = get_approx_local_hour();
    if ( local_hour==2 || local_hour==3 || local_hour==4 )
      sleep_light_seconds(60*5);
      
    // Wait to read lat/lon/time data from the GPS over serial.
    //    NOTE: This is important to populate latitude and longitude
    bool time_and_location_received = wait_for_gps_time_and_location();
    
    if ( time_and_location_received ) // returns true if GPS was able to get time
      Serial.println("GPS time and location grabbed.");
    else
      Serial.println("GPS time grab wait timed out.");

    // Send GPS mesage
    send_gps();
    time_loop_wait( 5000 ); // Wait for lmic and receive pending downlinks
    last_gps_send = millis();

    Serial.println("Syning with ESP32 RTC.");
    sync_rtc_with_gps();

    Serial.println("Calculating UTC offset in case it was changed.");
    calc_utc_offset();
    
  }

  // Turn on/off GPS based on overcharge mode before going to sleep
  if (overcharge_mode)
    gps_power_on();
  else
    gps_power_off();

  // Sleep for about 15 minutes
  Serial.println("Going to sleep for 15 minutes.");

  time_loop_wait(50);// Let Serial catch up to view messages
  uint16_t loop_time_total = (uint16_t)((millis()-loop_start)/1000);
  sleep_light_seconds( 900 - loop_time_total ); // 15 minutes
}
