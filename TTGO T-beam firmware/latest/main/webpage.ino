

const char* miner_input = "miner_input";


int _first_page_visited_time = 0;
bool _new_keys_entered = false;
int _webpage_timeout_ms = 60000; // Default, but will be updated below if client connects

String webpage;
String newpage;


bool if_new_keys_entered()
{
  return _new_keys_entered;
}

int get_webpage_timeout_ms()
{
  return _webpage_timeout_ms;
}

String get_base_html()
{
  String frequency;
  #if defined(CFG_eu868)
    frequency = "868MHz";
  #elif defined(CFG_us915)
    frequency = "915MHz";
  #endif

  // HTML web page to handle 3 input fields (input1, input2, input3)
String html PROGMEM = R"(
  <!DOCTYPE HTML><html><head>
  <title>AscensionWx Weather Miner</title>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}   body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}
  p {font-size: 16px;color: #444444;margin-bottom: 10px;} form {font-size: 24px;color: #444444;margin-bottom: 10px;}
  </style>
  </head>
  <body>
  <h1>AscensionWx Weather Miner</h1>)" + 

  String("<p>Firmware: " + APP_VERSION_STR + "</p>") +
  String("<p>Frequency: "+ frequency + "</p>") +
  String("<p>Device: <b style='color:red'>"+preferences.getString("devname", "")+"</b></p>") +
  
  String("<p>Temperature: " + String(get_temperature()) + " &deg;C</p>") +
  String("<p>Humidity: " + String(get_humidity()) + "%</p>") +
  String("<p>Pressure: " + String(get_pressure_hpa()) + " hPa</p>");

  return html;
}

String get_miner_html_enabled()
{
  String html PROGMEM = R"(
  <p> </p>
  <form action="/get">
    Telos Miner Account: <input type="text" name="miner_input" style="height:24px">
    <input type="submit" value="Submit" style="height:24px">
  </form><br>
</body></html>)";

  return html;
}

String get_miner_html_disabled()
{
  String html PROGMEM = R"(
  <p> </p>
  <form action="/get">
    Telos Miner Account: <input type="text" name="miner_input" style="height:24px" disabled>
    <input type="submit" value="Submit" style="height:24px" disabled>
  </form><br>
</body></html>)";

  return html;
}

String get_verify_html()
{
  if ( DOWNLINK_RESPONSE.length() > 0 )
    return String("<p>" + String(DOWNLINK_RESPONSE) + "</p");
  else
    return String("<p>No downlink. Node-RED server error.</p>");
}

String get_save_and_reboot_html()
{
  return String("<p>Keys saved. Device rebooting.</p>");
}

String user_key_error_html() {}

String get_lorawan_keys_html()
{
  String html PROGMEM = R"(
  <p> </p>
  <form action="/get">
    Devname: <input type="text" name="devname" style="height:24px">
    Deveui (MSB): <input type="text" name="deveui" style="height:24px">
    Appkey (MSB): <input type="text" name="appkey" style="height:24px">
    <input type="submit" value="Save and Reboot" style="height:24px">
  </form><br>
</body></html>)";

  return html;
}

bool if_all_hex_chars( String input, int len )
{
  bool isHex;
  for( int i=0; i<len; i++) {
    char c = input[i];
    isHex = ((c >= '0' && c <= '9') || 
                 (c >= 'a' && c <= 'f') || 
                 (c >= 'A' && c <= 'F'));
    if( !isHex )
      return false;
  }

  // If none of the characters were non-hex return true;
  return true;
}


void checkServer() 
{
  // Constructed with:
  //https://randomnerdtutorials.com/esp32-web-server-arduino-ide/

  // client connection
  //   only lasts about 2 secs max, so shouldnt interfere with status messages
  const long timeoutTime = 2000; // millis
  long previousTime = 0;
  long currentTime;

  // Stores response from client
  String header;
  
  // Listen for incoming clients
  CLIENT = SERVER.available();

  if (CLIENT) // Init correctly
  {
    if ( !IF_WEB_OPENED ) // Only do this once on initial connection
    {
      Serial.println("Client connected.");
      screen_print("Smartphone connected!\n\n");
      screen_print("Please go to: \n");
      screen_print("  http://192.168.4.1\n");
      screen_print("\n"); // two lines
    
      IF_WEB_OPENED = true;      
    }

    // Store for timeout purposes
    currentTime = millis();
    previousTime = currentTime;

    // Stores incoming client data
    String currentLine = "";

    while( CLIENT.connected() && currentTime - previousTime <= timeoutTime )
    {
      currentTime = millis();
      if (CLIENT.available())
      {
        char c = CLIENT.read();
        header += c; // store most recent character

        if ( c== '\n' )
        {
          if ( currentLine.length() == 0 ) // two newline characters in a row... end of client HTTP request
          {

            webpage = get_base_html() + get_lorawan_keys_html();

            // TEMPORARY: Printing client's response so that we can read it later
            Serial.println("This is the full header of the client's response:\n");
            Serial.println(header);

            // Check what the client input
            //if (header.indexOf( "GET /
            String tmp = "/get?devname"; // Note: all page loads actually have /get? so need devname here

            // This will be true if Submit was already hit and three seconds have elapsed since the first page was loaded
            if ( index >= 0 && _first_page_visited_time!=0 && time_loop_check( _first_page_visited_time, 3000 ) )
            {

              Serial.println("/get? request happened so if user didn't hit Submit button, this is a bug or browser quirk.");

              uint16_t index1 = 9 + strlen("devname") + 1; // 9 comes from "GET /get?" and 1 comes from the equals sign
              uint16_t index2 = index1 + 12;  // Device names should always be 12 characters
              String devname = header.substring(index1, index2);
              Serial.println("Input devname string is "+ devname);
                            
              index1 = index2 + 1 + strlen("deveui") + 1; // ampersand + "deveui" + equals sign
              index2 = index1 + 8*2;  // Eight bytes which have 2 characters each
              String deveui = header.substring(index1, index2);
              Serial.println("Input deveui string is "+ deveui);

              index1 = index2 + 1 + strlen("appkey") + 1; // ampersand + "appkey" + equals sign
              index2 = index1 + 16*2; // Siexteen bytes with 2 characters each
              String appkey = header.substring(index1, index2);
              Serial.println("Input appkey string is "+ appkey);

              // Check that lengths of inputs are correct
              if ( if_all_hex_chars( deveui, 16 ) && if_all_hex_chars( appkey, 32 ) )
              {
                // Set devname
                preferences.putString("devname", devname);
                
                u1_t deveui_byte_msb[8];
                u1_t deveui_byte_lsb[8];
                lorawan_key_str2bytes( deveui_byte_msb, deveui );
                // flip byte order
                for (int i=7; i>=0; i--) {
                  deveui_byte_lsb[7-i] = deveui_byte_msb[i];
                }
                preferences.putBytes("deveui", deveui_byte_lsb, 8);

                // Set appeui to null bytes (temporary)
                preferences.putBytes("appeui", NULL_KEY, 8);

                u1_t appkey_byte_msb[16];
                lorawan_key_str2bytes( appkey_byte_msb, appkey );
                preferences.putBytes("appkey", appkey_byte_msb, 16);

                _new_keys_entered = true;

                Serial.println("User inputs passed. Keys saved and device will intentionally reboot.");
              
                // Construct new html page
                newpage = webpage + "<p>Keys saved. Device will reboot now.</p>";
                
              } 
              else // user input had an error
              {

                Serial.println("User's input had error. Loading new webpage.");
                
                _new_keys_entered = false; // Allow user to enter again.
                newpage = webpage + "<p>Key length error. Please try again.</p>";
              }

              //Serial.println("Stored the following as preferences:");
              //Serial.println("Deveui: " + deveui_byte_lsb);
              //Serial.println("Appeui: " + appeui);
              //Serial.println("Appkey: " + appkey_byte_msb);

              // HTTP response 
              CLIENT.println("HTTP/1.1 200 OK");
              //CLIENT.println("Location: http://192.168.4.1");
              CLIENT.println("Content-type:text/html");
              CLIENT.println("Connection: close");
              CLIENT.println();
              CLIENT.println( newpage.c_str() );
              CLIENT.println();
             
            }
            else // This means we're dealing with initial webpage
            {

              Serial.println("Loading initial webpage.");
              
              _first_page_visited_time = millis();

              Serial.println("Increasing webpage timeout to 5 minutes");
              _webpage_timeout_ms = 5*60*1000; // Keep webpage on for 5 minutes
              
              // Create newpage html string
              //webpage = get_base_html() + get_lorawan_keys_html();// + get_save_and_reboot_html(); // + String("<p>" + String(DOWNLINK_RESPONSE) + "</p");

              // Print the webpage to the client
                            // HTTP response 
              CLIENT.println("HTTP/1.1 200 OK");
              CLIENT.println("Content-type:text/html");
              CLIENT.println("Connection: close");
              CLIENT.println();
              CLIENT.println( webpage.c_str() );
              CLIENT.println();
            }

            break; // break out of while loop once we have printed the webpage to the client

          }
          else // newline received
          {
            currentLine = "";
          }
        }
        else if (c != '\r') // Received something other than carriage return
        {
          currentLine += c;
        } 
      } // end check for client.available
    } // end check for client connection

    CLIENT.stop();
    Serial.println("Client disconnected.");
    Serial.println("");

    
  } // end check that client was init correctly

}
