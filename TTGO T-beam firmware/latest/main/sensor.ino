/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor
  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2650
  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface. The device's I2C address is either 0x76 or 0x77.
  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!
  First written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
  See the LICENSE file for details.

  Regarding calculation of elevation:
    - It's only possible with a rising sensor
    - The approximation is made under the assumption of 
         hydrostatic balance (almost always) in meteorology
    - Written by Nicolas Lopez - Kanda Weather Group LLC
    - kandaweather@gmail.com
  
 ***************************************************************************/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SHT21.h>
#include <SPL06-007.h>


uint32_t humidity_bme;
uint32_t pressure_bme;
uint32_t temperature_bme;

char bme_char[32]; // used to sprintf for Serial output

Adafruit_BME280 bme; // I2C
SHT21 sht;

// BME 280 environment
float temperature_now; // celcius
float humidity_now; // percent (range 0 to 100)
float pressure_now; // hpa
float v_temperature_now; // kelvin
float ELEVATION_NOW; // meters
  
float v_temperature_last;
float elevation_last;
float pressure_last;

void bme_setup() {
  
    bool status;

    if (BME280_FOUND) {
       status = bme.begin( BME280_ADDRESS_0x76 );
       if (!status) // if still not found, try 0x77
          status = bme.begin( BME280_ADDRESS_0x77 );
    }
    else if (SPL06_FOUND) {
      SPL_init( ); // Has hardcoded 0x77 into the library function
      status = true;
    }
    
    if (!status ) {
        Serial.println("Could not find a valid weather sensor, check wiring!");
    }

    // Wake up sensor
    delay(20);
    get_pressure_hpa();
    delay(500);

}

float get_elevation()
{
  return ELEVATION_NOW;
}

void cycle_elevation() {
  
      // Store the last variables
      pressure_last = pressure_now;
      v_temperature_last = v_temperature_now;
      elevation_last = ELEVATION_NOW;
  
      // Update environment state
      temperature_now = get_temperature();
      humidity_now = get_humidity();
      pressure_now = get_pressure_hpa(); // Store as hpa
      v_temperature_now = calc_v_temperature( temperature_now, 
                                              humidity_now, 
                                              pressure_now );
      ELEVATION_NOW = calc_elevation();
}

float get_humidity() {
    float rh = MISSING_FLOAT;
    if ( BME280_FOUND ) rh = bme.readHumidity();
    else if ( SHT21_FOUND ) rh = sht.getHumidity();
    
    return rh;
}

float get_pressure_hpa() {
    float p = MISSING_FLOAT;
    if ( BME280_FOUND ) p = bme.readPressure() / 100.0; // convert to hpa
    // be careful... get_pressure referenced below is spl06 library function
    else if ( SPL06_FOUND ) p = get_pressure(); // returns already in hpa

    return p;
}

float get_temperature() {
    float t = MISSING_FLOAT;
    if ( BME280_FOUND ) t = bme.readTemperature();
    else if ( SHT21_FOUND ) t = sht.getTemperature();
    
    return t;
}

bool if_bme280_damaged(float temperature, float pressure, float humidity) {
  return ( pressure < 700 || pressure > 1100 ||
           temperature < -55 || temperature > 55 ||
           humidity < 0 || humidity > 100 );
}

float calc_24hr_stdev() {

  float total;
  uint16_t num_obs = sizeof(temperatures);

  // First get mean
  for (int i = 0; i < num_obs; i++) {
    total = total + temperatures[i];
  }
  float avg = total/num_obs;

  // Calculate variance
  total=0;
  for (int i = 0; i < num_obs; i++) {
    total = total + (temperatures[i] - avg) * (temperatures[i] - avg);
  }
  float variance = total/num_obs;

  // Return standard deviaton
  return sqrt(variance);

}

float calc_v_temperature(float t, float rh, float p) {
  
  // calc saturation vapor pressure es(T)
  // https://journals.ametsoc.org/jamc/article/57/6/1265/68235/A-Simple-Accurate-Formula-for-Calculating
  float es;
  if ( t>0 )
  {
    es = exp( 34.494 - (4924.99/(t+237.1)) ) /  pow(t+105, 1.57);
  } else {
    es = exp( 43.494 - (6545.8/(t+278)) ) /  pow(t+868, 2);
  }
  
  // calc actual vapor pressure using  rh = e/es
  float e = (rh/100)*es;
  e = e/100.0; // Convert to hpa
  
  // calc mixing ratio
  float w = 0.62197 * (e / (p-e));

  // calc virtual temperature in Kelvin
  float tv = (1+(0.61*w))*(t+273.15);

  return tv;
  
}

// Also called geopotential height
float calc_elevation() {

  float p1 = pressure_last;
  float tv1 = v_temperature_last;
  float z1 = elevation_last;

  float p2 = pressure_now;
  float tv2 = v_temperature_now;

  
  // Uses COUNT and last values to get the elevation
  //   p2 and tv2 are the most recent pressure and virtual temp values
  if (COUNT == 0)
  {
     // Turning on device at about 1 meter above surface
     return 1;
  }
  
  float R = 287.058; // specific gas constant for dry air
  float g = calc_gravity(); // gravitational acceleration

  // Approx average virtual temperature of layer
  float tv_avg = tv1 + (tv2 - tv1)/2.0;

  // Use natural logarithm
  float elevation = (R*tv_avg/g)*log(p1/p2) + z1;

  return elevation;

}

float calc_gravity(){

  // Do this to offset gravitational changes. For example,
  //   in West Africa, gravity is a little less because being closer to the equator
  //   means centrifigal force is stronger
  // difference can be as much as 0.5% ... which adds up when integrating up the column

  float g = 978031.85; // gravity at elev=0 and equator
  
  if ( gps_available() )
  {
    float lat = (gps_latitude() * 71.0) / 4068.0; // Get lat and convert to radians
    float elev = gps_altitude(); // in meters
    // Latitude correction
    g = g * (1.0 + 0.005278895*sin(lat)*sin(lat));
    // Altitude correction
    g = g + 0.3086*elev;
  }

  return g / 100000.0; // convert to m/s^2 units
}

bool if_T_and_RH_sensor()
{
  if (BME280_FOUND) return true;
  else if (SHT21_FOUND) return true;
  else return false;
}

bool if_P_sensor()
{
  if (BME280_FOUND) return true;
  else if (SPL06_FOUND) return true;
  else return false;
}

bool if_indoors(float temperature)
{
  if( (calc_24hr_stdev() < 1.4) )
    // Check if also between 66 and 78 farenheight
    if ( temperature > 18.9 && temperature < 25.5 )
      return true;
    else
      return false;

  return false;
 
}

// Adjusts all temperatures by one location and adds new temp
void update_temperature_list( int8_t t )
{
  for (int i = sizeof(temperatures); i > 0; i--){        
    temperatures[i]=temperatures[i-1];
  }
  temperatures[0] = t;
}

void scanI2Cdevice(void)
{
    byte err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        err = Wire.endTransmission();
        if (err == 0) {
            Serial.print("I2C device found at address 0x");
            if (addr < 16)
                Serial.print("0");
            Serial.print(addr, HEX);
            Serial.println(" !");
            nDevices++;

            if (addr == SSD1306_ADDRESS) {
                SSD1306_FOUND = true;
                Serial.println("ssd1306 display found");
            }
            if (addr == AXP192_SLAVE_ADDRESS) {
                AXP192_FOUND = true;
                Serial.println("axp192 PMU found");
            }
            if ( addr == SHT21_ADDRESS ) {
                SHT21_FOUND = true;
                Serial.println("SHT21 sensor found");
            }
            // This is a slight hack because SPL06 and some BME280 share 0x76 and 0x77 address
            //   Warning:  Only use if SPL06 SHT21 are guaranteed on the same board and working!
            if ( addr == SPL06_ADDRESS_0x76 && SHT21_FOUND ) {
                SPL06_FOUND = true;
                Serial.println("SPL06 sensor found");
            }
            if ( addr == SPL06_ADDRESS_0x77 && SHT21_FOUND ) {
                SPL06_FOUND = true;
                Serial.println("SPL06 sensor found");
            }
            if ( addr == BME280_ADDRESS_0x76 && !SHT21_FOUND  ) {
                BME280_FOUND = true;
                Serial.println("BME280 sensor found");
            }
            if ( addr == BME280_ADDRESS_0x77 && !SHT21_FOUND ) {
                BME280_FOUND = true;
                Serial.println("BME280 sensor found");
            }
     
        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16)
                Serial.print("0");
            Serial.println(addr, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("done\n");
}
