
void send_status( bool want_confirmed )
{
  MESSAGE_PENDING = true;
  uint8_t txBuffer[1];
  LoraEncoder encoder(txBuffer);

  encoder.writeBitmap( !if_T_and_RH_sensor(),
                       !if_P_sensor(),
                       !SSD1306_FOUND,
                       !AXP192_FOUND,
                       if_indoors(get_temperature()),
                       !time_loop_check( last_seen_charging , hr_milliseconds ), // AXP module flag
                       (get_humidity() > 95.0),
                       false ); // Last flag needs implementing

  // Battery / solar voltage
  //encoder.writeUint8(0);

  lorawan_cnt(COUNT);
  
  Serial.println("Sending status message.");
  lorawan_send(txBuffer, sizeof(txBuffer), STATUS_PORT, want_confirmed);
  COUNT++;
}

void send_status() {
  send_status( false );
}

void send_authentication( String account ) {

  MESSAGE_PENDING = true;
  uint8_t txBuffer[13];  // 8 for uint64   ... 13 for string (12 characters plus \n)

  boolean confirmed = false;
  lorawan_cnt(COUNT);
  Serial.println("Sending balloon authentication message.");

  account.getBytes(txBuffer, 13);
  
  lorawan_send(txBuffer, sizeof(txBuffer), AUTHENTICATION_PORT, false);

}

void send_observation() {
  MESSAGE_PENDING = true;

  uint8_t txBuffer[9];
  LoraEncoder encoder(txBuffer);
  
  float temperature = get_temperature();
  float humidity = get_humidity();
  float pressure = get_pressure_hpa();
  float battery_voltage = axp.getBattVoltage();
  
  char buffer[40];
  
  snprintf(buffer, sizeof(buffer), "Temperature: %10.1f\n", temperature);
  Serial.println(buffer);
  screen_print(buffer);
  snprintf(buffer, sizeof(buffer), "Humidity: %10.1f\n", humidity);
  Serial.println(buffer);  
  screen_print(buffer);
  snprintf(buffer, sizeof(buffer), "Pressure: %10.2f\n", pressure);
  Serial.println(buffer);
  snprintf(buffer, sizeof(buffer), "Battery voltage: %10.1f\n", battery_voltage );
  Serial.println(buffer);

  encoder.writeUint16( (uint16_t)(pressure*10.0) ); // Convert hpa to deci-paschals
  encoder.writeTemperature(temperature);
  encoder.writeHumidity(humidity);
  encoder.writeBitmap( if_T_and_RH_sensor(),
                       SSD1306_FOUND,
                       if_gps_on(),
                       !(undercharge_mode || overcharge_mode), // Returns true if normal energy operation
                       if_indoors(temperature),
                       !time_loop_check( last_seen_charging , hr_milliseconds ), // AXP module flag
                       (humidity > 95.0),
                       if_bme280_damaged( temperature, pressure, humidity ) ); // Last flag needs implementing
 encoder.writeUint16( battery_voltage );

 Serial.println("Built message successfully");

#if LORAWAN_CONFIRMED_EVERY > 0
  bool confirmed = (COUNT % LORAWAN_CONFIRMED_EVERY == 0);
#else
  bool confirmed = false;
#endif

  lorawan_cnt(COUNT);
  
  Serial.println("Sending balloon observation message.");
  lorawan_send(txBuffer, sizeof(txBuffer), OBSERVATION_PORT, confirmed);
  //lorawan_send(message.getBytes(),message.getLength(), OBSERVATION_PORT, confirmed);

  COUNT++;
}

void send_empty_gps()
{
  MESSAGE_PENDING = true;
  
  uint8_t txBuffer[14];
  LoraEncoder encoder(txBuffer);

  encoder.writeUnixtime( get_unix_time() );
  encoder.writeLatLng( 0.0 , 0.0 ); // If the location hasn't been updated, send zeros
  encoder.writeUint16( 0.0 );

  bool confirmed = false;
  lorawan_cnt(COUNT);
  Serial.println("Sending GPS data message.");
  lorawan_send(txBuffer, sizeof(txBuffer), GPS_LORAWAN_PORT, confirmed);

  COUNT++;
  
}

void send_gps()
{
  MESSAGE_PENDING = true;
  
  uint8_t txBuffer[10];
  LoraEncoder encoder(txBuffer);

  // Send lmic messages with unixtime, lat, lon, elev
  //encoder.writeLatLng_6( gps_latitude(), gps_longitude() );
  encoder.writeLatLng_6( gps_latitude(), gps_longitude() );
  encoder.writeUint16( gps_altitude() ); // stored as meters
  encoder.writeUint16( (uint16_t)(gps_location_age() / (1000*60*60)) ); // num hours since last fix

  bool confirmed = false;
  lorawan_cnt(COUNT);
  Serial.println("Sending GPS data message.");
  lorawan_send(txBuffer, sizeof(txBuffer), GPS_LORAWAN_PORT, confirmed);

  COUNT++;
}

uint32_t get_count() {
  return COUNT;
}

void callback(uint8_t message) {

/*
  if (EV_JOINING == message) screen_print("Joining TTN...\n");
  if (EV_JOINED == message) screen_print("TTN joined!\n");
  if (EV_JOIN_FAILED == message) screen_print("TTN join failed\n");
  if (EV_REJOIN_FAILED == message) screen_print("TTN rejoin failed\n");
  if (EV_RESET == message) screen_print("Reset TTN connection\n");
  if (EV_LINK_DEAD == message) screen_print("TTN link dead\n");
  if (EV_ACK == message) screen_print("ACK received\n");
  if (EV_PENDING == message) screen_print("Message discarded\n");
  if (EV_QUEUED == message) screen_print("Message queued\n");
*/

  if (EV_JOINED == message && if_otaa()) {
    // OTAA Join request successful
    GATEWAY_IN_RANGE = true;
  }

  if (EV_TXCOMPLETE == message) {
    //screen_print("Message sent\n");

    MESSAGE_PENDING = false;
    if (LMIC.txrxFlags & TXRX_ACK)
    {
        GATEWAY_IN_RANGE = true;
    }

  }

  if (EV_RESPONSE == message) {

    if (LMIC.dataLen) 
    {
      GATEWAY_IN_RANGE = true; // if we receive downlink, we know this is true

      int auth_size = 1;
      char* msg_start = (char*)(LMIC.frame+LMIC.dataBeg);

      char auth_status[ auth_size ];
      strncpy( auth_status, msg_start, 1 ); // Get first byte
      auth_status[1] = '\0'; // null terminate to make it a string
      if ( strcmp( auth_status , "1" ) == 0 ) // strcmp equals 0 if the two strings are the same
      {
        AUTHENTICATED=true;
      } else {
        AUTHENTICATED=false;
      }
        
      char tmp[25]; // set to size of smartphone screen, etc
      int data_len = LMIC.dataLen - 2;
      strncpy( tmp, msg_start + 2, data_len );
      tmp[ data_len ] = '\0'; // null terminate to make it a string

      DOWNLINK_RESPONSE = tmp; // Store the response as a string

      String respText = "Blockchain response:\n" + DOWNLINK_RESPONSE + "\n";
      screen_print(respText.c_str());
      Serial.println();
      Serial.println("Authenticated: ");
      Serial.println(AUTHENTICATED);
      Serial.println("LoRaWAN response: " + DOWNLINK_RESPONSE);   
    }

    /*if (!gps_available()) {
      screen_print("Warning:\n");
      screen_print("  GPS not yet locked.\n");
    } else 
    {
      screen_print(" \n \n");
    }
    */

    screen_print(" \n \n");
  } // end of check for response
}
