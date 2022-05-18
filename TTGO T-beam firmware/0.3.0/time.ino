

#include "time.h"


bool time_loop_check( int time_loop_start , int wait_millis )
{
  if ( millis() > time_loop_start + wait_millis )
  {
    return true;
  } else {
    return false;
  }
}

void time_loop_wait( int milliseconds )
{
  int start = millis();
  while ( !time_loop_check( start, milliseconds ) )
  {
    os_runloop_once();
  }
}

struct tm tm_construct( int yr, int month, int mday, int hr, int minute, int sec, int isDst )
{
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0

  return tm;
}

int seconds_since( struct tm tm )
{
  time_t now;
  time(&now);

  uint64_t num_seconds = difftime(now, mktime(&tm));
  return num_seconds;
}

void setTime(struct tm tm){
  // Sets the ESP32 time using struct tm
  time_t t = mktime(&tm);
  
  Serial.printf("Setting time on ESP32: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void calc_utc_offset() {

  if( !if_gps_on() ) {
    Serial.println("Error. Cannot calculate UTC offset while GPS is off.");
  }
  
  // Local timezone is changes roughly every 15 degrees
  // UTC (offset=0) is also prime meridian, so this is relatively easy.
  //   - offset is negative value for degrees West , positive for degrees east
  float longitude = gps_longitude();
  APPROX_UTC_HR_OFFSET = int( longitude/15.0 );
  
}

int8_t get_approx_local_hour() {

  struct tm tm;
  int hour;

  // Gets the ESP32 local time
  getLocalTime(&tm);

  hour = tm.tm_hour + APPROX_UTC_HR_OFFSET;
  
  if (hour < 0) {// Prevent negative value for hour
    hour = hour + 24;
  }
  else if (hour > 23) { // Prevent hour going over 23
    hour = hour - 24;
  }

  return hour;
  // return APPROX_UTC_HR_OFFSET; // for testing

}

void sync_rtc_with_gps()
{
  if( !if_gps_on() ) {
    Serial.println("Error. Attempting to do RTC sync while GPS is off.");
  }
  
  // WARNING: This function should only be called if the GPS is on

  // Initialize tm struct
  struct tm gps_time = tm_construct( gps_year(), gps_month(), gps_day(), gps_hour(), gps_minute(), gps_second(), 0 );

  // Calculate drift in X num of seconds per one esp32 milli. This will help adjust the unix timestamp
  time_drift = seconds_since(gps_time) / (millis() - last_time_refresh);
  Serial.println("Drift calculated to be:");
  Serial.println(time_drift);
  Serial.println();
  
    
  // Set ESP 32 clock to match GPS's UTC time
  setTime(gps_time);

  // Reset last_time_refresh
  last_time_refresh = millis();

}

uint64_t get_unix_time() {
  // Uses last time_refresh, esp32's time, and time_drift to get unix_time.

  time_t now;
  struct tm timeinfo;

  if( !getLocalTime(&timeinfo) ) {
    Serial.println("getLocalTime failed.");
    return 0;
  };

  time(&now);

  return now;

  // Approx time based on last_time_refresh and time_drift
  //uint32_t millis_since_refresh = millis() - last_time_refresh;
  //uint64_t unix_time = ( now + (millis_since_refresh*time_drift) );

  //return unix_time;

}
