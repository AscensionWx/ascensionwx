

#include <TinyGPS++.h>
#include "time.h"

const unsigned long GPS_WaitAck_mS = 2000;        //number of mS to wait for an ACK response from GPS
uint8_t GPS_Reply[12];                //byte array for storing GPS reply to UBX commands (12 bytes)

uint32_t LatitudeBinary;
uint32_t LongitudeBinary;
uint16_t altitudeGps;
uint8_t hdopGps;
uint8_t sats;
char t[32]; // used to sprintf for Serial output

TinyGPSPlus _gps;

void gps_time(char * buffer, uint8_t size) {
    snprintf(buffer, size, "%02d:%02d:%02d", _gps.time.hour(), _gps.time.minute(), _gps.time.second());
}

uint16_t gps_year() {
  return _gps.date.year();
}

uint8_t gps_month() {
  return _gps.date.month();
}

uint8_t gps_day() {
  return _gps.date.day();
}

uint8_t gps_hour() {
  return _gps.time.hour();
}

uint8_t gps_minute() {
  return _gps.time.minute();
}

uint8_t gps_second() {
  return _gps.time.second();
}

uint64_t gps_time_age() {
    return _gps.time.age();
}

uint64_t gps_location_age() {
    return _gps.location.age();
}

float gps_latitude() {
    return _gps.location.lat();
}

float gps_longitude() {
    return _gps.location.lng();
}

float gps_altitude() {
    return _gps.altitude.meters();
}

float gps_hdop() {
    return _gps.hdop.hdop();
}

uint8_t gps_sats() {
    return _gps.satellites.value();
}

void gps_setup() {
    ss.begin(GPS_BAUDRATE, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN);           //format is baud, mode, UART RX data, UART TX data
    //_serial_gps.begin(GPS_BAUDRATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

boolean gps_available() {
  //return _serial_gps.available();
  return ss.available();
}

static void gps_read() {
    while (gps_available()) {
        //_gps.encode(_serial_gps.read());
        _gps.encode( ss.read() );
    }
}

boolean if_good_gps_quality()
{
  if (gps_hdop() > 2.0)
    return false;
  else
    return true;
}

void gps_power_off() {
  if (AXP192_FOUND)
  {
    if(axp.setPowerOutPut(GPS_POWER_PIN, AXP202_OFF) == AXP_PASS) {
      Serial.println(F("turned off GPS module"));
    } else {
      Serial.println(F("failed to turn off GPS module"));
    }
  } // end check for axp192
}

void gps_power_on() {
  if (AXP192_FOUND)
  {
    if(axp.setPowerOutPut(GPS_POWER_PIN, AXP202_ON) == AXP_PASS) {
      Serial.println("turned on GPS module");
    } else {
      Serial.println("failed to turn on GPS module");
    }
  } // end check for axp192
}

bool if_gps_on() {
  // NOTE: Assumes T-beam where GPS is on AXP's LDO3 pin
  if (AXP192_FOUND) 
    return axp.isLDO3Enable();
  else 
    return false;
}

bool wait_for_gps_time()
{
  if( !if_gps_on() ) {
    Serial.println("Error. Attempting to get GPS time while GPS is off.");
    return false;
  }
  
  // Cycles through sleep waiting for gps time to update
  // Returns true when gps's time has been updated

  uint64_t tmp = millis();
  while( !time_loop_check(tmp, 500) ) gps_read(); // Populate gps library with Serial data
  if (_gps.time.age() < 1500 ) return true;

  uint64_t begin_gps_wait = millis();
  while( !time_loop_check(begin_gps_wait, 30*1000) ) // Wait maximum of 30 seconds
  {
    // Sleep for as we wait to get partial fix
    sleep_light_seconds(5);

    tmp = millis();
    while( !time_loop_check(tmp, 500) ) gps_read();
    
    if ( _gps.time.age() < 1500 ) // GPS time is present
    { 
      return true;
    }
  }

  // Reach this point if max time wait has finished
  return false;
  
}

bool wait_for_gps_location()
{

  if( !if_gps_on() ) {
    Serial.println("Error. Attempting to get GPS location while GPS is off.");
    return false;
  }

  // We turn the GPS on for 5 minutes so that the button battery can be recharged
  sleep_light_seconds(5*60);

  // Now we can pull the most recent GPS data
  uint64_t tmp = millis();
  while( !time_loop_check(tmp, 500) ) gps_read(); // Populate gps library with Serial data
  
  if (gps_location_age()<1500) return true;
  else return false;
  
}
