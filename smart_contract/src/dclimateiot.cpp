#include <dclimateiot.hpp>

#include <eosio/asset.hpp>
#include <eosio/system.hpp>

#include <delphioracle.hpp>
#include <evm/evm.hpp>

using namespace std;
using namespace eosio;

ACTION dclimateiot::addsensor( name devname ) {

  // Only dclimateiot can add a new sensor
  require_auth( get_self() );

  uint64_t unix_time_s = current_time_point().sec_since_epoch();
  name default_token = "eosio.token"_n;

  // Scope is self
  sensors_table_t _sensors(get_self(), get_first_receiver().value);

  // Add the row to the observation set
  _sensors.emplace(get_self(), [&](auto& sensor) {
    sensor.devname = devname;
    sensor.time_created = unix_time_s;
    sensor.last_location_update = 0;
    sensor.message_type = "p/t/rh";
    sensor.firmware_version = "0.3.1";
  });

  // Create the weather table with the miner's scope
  weather_table_t _weather(get_self(), get_first_receiver().value );
  _weather.emplace(get_self(), [&](auto& wthr) {
    wthr.devname = devname;
    wthr.latitude_deg = 0.0;
    wthr.longitude_deg = 0.0;
    wthr.elevation_gps_m = 0;
    wthr.loc_accuracy = "None ";
  });

  parameters_table_t _parameters(get_self(), get_self().value );
  auto parameters_itr = _parameters.find( default_token.value );

  // Set rewards 
  rewards_table_t _rewards( get_self(), get_first_receiver().value );
  _rewards.emplace( get_self(), [&](auto& reward) {
    reward.devname = devname;
    reward.last_round = 0;
    reward.token_contract = default_token;
    reward.consecutive_rounds = 0;
    reward.loc_multiplier = 1;
  });

}

ACTION dclimateiot::addminer( name devname,
                               name miner ) {

  // To allow this action to be called using the "iot"_n permission, 
  //   make sure eosio linkauth action assigns iot permission to this action
  //
  // The benefit is that the "active" key (which traditionally transfers token balances)
  //   doesn't need to be on the same server as the one with inbound internet traffic

  require_auth(get_self());

  // First check that device and miner accounts exist
  check( is_account(devname) , "Account doesn't exist.");
  check( is_account(miner) , "Account doesn't exist.");

  // To confirm the miner can receive tokens, we send a small amount of Telos to the account
  uint8_t precision = 4;
  string memo = "Miner account enabled!";
  uint32_t amt_number = (uint32_t)(pow( 10, precision ) * 
                                        0.0001);
  eosio::asset reward = eosio::asset( 
                          amt_number,
                          symbol(symbol_code( "TLOS" ), precision));
  
  action(
      permission_level{ get_self(), "active"_n },
      "eosio.token"_n , "transfer"_n,
      std::make_tuple( get_self(), miner, reward, memo )
  ).send();

  // Add the miner to the sensors table
  sensors_table_t _sensors(get_self(), get_first_receiver().value);
  auto sensor_itr = _sensors.find( devname.value );

  _sensors.modify(sensor_itr, get_self(), [&](auto& sensor) {
      sensor.miner = miner;
  });
                               
}

ACTION dclimateiot::addnoaa( string id_string,
                              float latitude_deg,
                              float longitude_deg,
                              float elevation_m ) {

  // Require auth from self
  require_auth( get_self() );

  // Scope is self
  noaa_table_t _noaasensors( get_self(), get_first_receiver().value );

  // Add the row to the observation set
  _noaasensors.emplace(get_self(), [&](auto& sensor) {
    sensor.id_string = id_string;
    sensor.latitude_deg = latitude_deg;
    sensor.longitude_deg = longitude_deg;
    sensor.elevation_m = elevation_m;
  });

}

ACTION dclimateiot::addflag( uint64_t bit_value,
                            string processing_step,
                            string issue,
                            string explanation ) {
  
  flags_table_t _flags(get_self(), get_first_receiver().value);
  auto flag_itr = _flags.find( bit_value );

  // If flag doesn't exist, add it
  if (flag_itr==_flags.cend()) {
    _flags.emplace( get_self(), [&](auto& flag) {
      flag.bit_value = bit_value;
      flag.processing_step = processing_step;
      flag.issue = issue;
      flag.explanation = explanation;
    });
  } else { // else modify it
    _flags.modify(flag_itr, get_self(), [&](auto& flag) {
      flag.bit_value = bit_value;
      flag.processing_step = processing_step;
      flag.issue = issue;
      flag.explanation = explanation;
    });
  }

}

ACTION dclimateiot::submitdata(name devname,
                                float pressure_hpa,
                                float temperature_c, 
                                float humidity_percent,
                                uint8_t flags) {

  uint64_t unix_time_s = current_time_point().sec_since_epoch();

  // This flag is to be used to tell the user the data 
  //     was signed by the calling server instead of the device
  bool signature_flag = false;

  // Check that permissions are met
  if( !has_auth(devname) ) { // check if device signed transaction
    require_auth( get_self() ); // if device didn't sign it, then dclimateiot should have
    signature_flag = true; // Flag the data as signed by the server
  }

  // Find the sensor in the sensors table
  sensors_table_t _sensors(get_self(), get_first_receiver().value);
  auto sensor_itr = _sensors.find( devname.value );

  // Since any account can technically call this action, we first check that the device exists
  //    in the sensors table
  check( sensor_itr != _sensors.cend() , "Device not in table.");

  // Get device row in weather table
  weather_table_t _weather(get_self(), get_first_receiver().value );
  auto weather_itr = _weather.find( devname.value ); // In case the owner has multiple devices

  // Update the weather table
  _weather.modify(weather_itr, get_self(), [&](auto& wthr) {
      wthr.unix_time_s = unix_time_s;
      wthr.pressure_hpa = pressure_hpa;
      wthr.temperature_c = temperature_c;
      wthr.humidity_percent = humidity_percent;
  });

  // Update flags variable
  _weather.modify(weather_itr, get_self(), [&](auto& wthr) {
      wthr.flags = flags 
                    + ( signature_flag ? 1 : 0 )
                    + ( (unix_time_s > sensor_itr->last_location_update + 14*3600*24) ? 2 : 0 )
                    + 0
                    + 0 ; // + TODO: all other flags
  });

  // Set rewards 
  rewards_table_t _rewards( get_self(), get_first_receiver().value );
  auto rewards_itr = _rewards.find( devname.value );

  parameters_table_t _parameters(get_self(), get_first_receiver().value );
  auto parameters_itr = _parameters.find( rewards_itr->token_contract.value );
  
  uint64_t last_round = rewards_itr->last_round;

  // Check if it has been 1 hour since we paid out the reward
  //if ( unix_time_s > last_round + parameters_itr->reward_hours*3600)
  if ( unix_time_s > last_round + 3600)
  {

    //if ( unix_time_s > last_round + (parameters_itr->1+1)*3600 ) // Missed the last round
    if ( unix_time_s > last_round + 2*3600 )
    {
      _rewards.modify( rewards_itr, get_self(), [&](auto& reward) {
        reward.consecutive_rounds = 0;
        reward.last_round = unix_time_s;
      });
    } else {
      _rewards.modify( rewards_itr, get_self(), [&](auto& reward) {
        reward.consecutive_rounds += 1;
        reward.last_round = last_round + 3600;
      });
    }

    // Send the reward if miner account present
    name miner = sensor_itr->miner;
    if ( is_account(miner) )
    {
      sendReward( miner, devname );
    }

  }

}

ACTION dclimateiot::submitgps( name devname,
                                float latitude_deg,
                                float longitude_deg,
                                float elev_gps_m,
                                float lat_deg_geo,
                                float lon_deg_geo) {
                                
  /*
    WARNING:  This action can be very intensive and may result in a CPU timeout.
              It is recommended this ACTION be attempted 2-3 times by the calling application,
                 because some block producers bill different CPU rates.

    Submitting new GPS coordinates means we have to recalculate the location
      multiplier. This requires looping over many noaa stations.
  */

  // To allow this action to be called using the "iot"_n permission, 
  //   make sure eosio linkauth action assigns iot permission to submitgps action
  //
  // The benefit is that the "active" key (which traditionally transfers token balances)
  //   doesn't need to be on the same server as the one with inbound internet traffic

  // Require auth from self
  require_auth( get_self() );

  uint64_t unix_time_s = current_time_point().sec_since_epoch();

  // Get sensors table
  sensors_table_t _sensors(get_self(), get_first_receiver().value);

  weather_table_t _weather(get_self(), get_first_receiver().value);
  auto weather_itr = _weather.find( devname.value );

  // If all fields are blank, exit the function
  if ( latitude_deg == 0 && longitude_deg == 0 && lat_deg_geo == 0 && lon_deg_geo == 0 ) return;

  if ( latitude_deg != 0 && longitude_deg != 0 && lat_deg_geo == 0 && lon_deg_geo == 0 )
  {
    // If we submit lat/lon without elevation data, we can assume it's based on "shipment city"
    if (  elev_gps_m == 0 )
    {
      _weather.modify(weather_itr, get_self(), [&](auto& wthr) {
        wthr.loc_accuracy = "Low (Shipment city name)";
        wthr.latitude_deg = latitude_deg;
        wthr.longitude_deg = longitude_deg;
      });
    } else {
    // We have gps elevation, so data came from GPS
      _weather.modify(weather_itr, get_self(), [&](auto& wthr) {
        wthr.loc_accuracy = "Medium (GPS only)";
        wthr.latitude_deg = latitude_deg;
        wthr.longitude_deg = longitude_deg;
        wthr.elevation_gps_m = elev_gps_m;
      });
    }
  }

  // If only geolocation lat/lons are supplied
  if ( latitude_deg == 0 && latitude_deg == 0 && lat_deg_geo != 0 && lon_deg_geo != 0 )
  {
    _weather.modify(weather_itr, get_self(), [&](auto& wthr) {
      wthr.loc_accuracy = "Medium (Geolocation only)";
      wthr.latitude_deg = lat_deg_geo;
      wthr.longitude_deg = lon_deg_geo;
    });
  }

  // Finally, if all 4 fields are filled out, we give high confidence
  if ( latitude_deg != 0 && latitude_deg != 0 && lat_deg_geo != 0 && lon_deg_geo != 0 )
  {
      float distance = calcDistance( latitude_deg, 
                                    longitude_deg, 
                                    lat_deg_geo, 
                                    lon_deg_geo );

      // LoRaWAN gateways can hear a maximum of about 15 kilometers away
      check( distance < 15.0 ,
          "Geolocation check fail. Geolocation and GPS "+
          to_string(uint32_t(distance))+
          " km apart.");
          

      _weather.modify(weather_itr, get_self(), [&](auto& wthr) {
        wthr.loc_accuracy = "High (Geolocation + GPS)";
        wthr.latitude_deg = latitude_deg;
        wthr.longitude_deg = longitude_deg;
        wthr.elevation_gps_m = elev_gps_m;
      });

  }


  // Find devname in table
  auto sensor_itr = _sensors.find( devname.value );

  _sensors.modify(sensor_itr, get_self(), [&](auto& sensor) {
      sensor.last_location_update = unix_time_s;
  });

  rewards_table_t _rewards( get_self(), get_first_receiver().value );
  auto rewards_itr = _rewards.find( devname.value );

  parameters_table_t _parameters(get_self(), get_first_receiver().value );
  auto parameters_itr = _parameters.find( rewards_itr->token_contract.value );

  // Recalculate the location multiplier for the sensor
  _rewards.modify( rewards_itr, get_self(), [&](auto& reward) {
      reward.loc_multiplier = calcLocMultiplier( devname, parameters_itr->max_distance_km );
  });

}

ACTION dclimateiot::chngreward(name devname,
                                   name token_contract) {

  // Only self can run this Action
  require_auth(get_self());

  // Delete the fields currently in the rewards table
  rewards_table_t _rewards(get_self(), get_first_receiver().value);
  auto rewards_itr = _rewards.find( devname.value );
  
  _rewards.modify( rewards_itr, get_self(), [&](auto& reward) {
    reward.devname = devname;
    reward.token_contract = token_contract;
  });

}

ACTION dclimateiot::setrate(name token_contract,
                              float base_hourly_rate) {
                              //int reward_hours) {
                                  
  // Only self can run this Action
  require_auth(get_self());

  parameters_table_t _parameters(get_self(), get_self().value );

  auto parameters_itr = _parameters.find( token_contract.value );
  _parameters.modify(parameters_itr, get_self(), [&](auto& parameter) {
      parameter.base_hourly_rate = base_hourly_rate;
      //parameter.reward_hours = reward_hours;
  });
}

ACTION dclimateiot::addparam(name token_contract,
                                  float max_distance_km,
                                  float max_rounds,
                                  string symbol_letters,
                                  bool usd_denominated,
                                  float base_hourly_rate,
                                  //int reward_hours,
                                  uint8_t precision) {
                                  
  // Only self can run this Action
  require_auth(get_self());

  parameters_table_t _parameters(get_self(), get_self().value );


  _parameters.emplace(get_self(), [&](auto& parameter) {
      parameter.token_contract = token_contract;
      parameter.max_distance_km = max_distance_km;
      parameter.max_rounds = max_rounds;
      parameter.symbol_letters = symbol_letters;
      parameter.usd_denominated = usd_denominated;
      parameter.base_hourly_rate = base_hourly_rate;
      //parameter.reward_hours = reward_hours;
      parameter.precision = precision;
  });
  
}

ACTION dclimateiot::removeparam( name token_contract )
{
  // Only self can run this Action
  require_auth(get_self());

  parameters_table_t _parameters(get_self(), get_self().value );

  auto parameters_itr = _parameters.find( token_contract.value );
  _parameters.erase( parameters_itr );
}

ACTION dclimateiot::removereward( name devname )
{

  // Require auth from self
  require_auth( get_self() );

  rewards_table_t _rewards(get_self(), get_first_receiver().value);
  auto rewards_itr = _rewards.find( devname.value );

  _rewards.erase( rewards_itr );
}


ACTION dclimateiot::removeobs(name devname)
{
  // Require auth from self
  require_auth( get_self() );

  weather_table_t _weather(get_self(), get_first_receiver().value);
  auto itr = _weather.find( devname.value );

  _weather.erase( itr );
}

ACTION dclimateiot::removesensor(name devname)
{
  // Require auth from self
  require_auth( get_self() );

  sensors_table_t _sensors(get_self(), get_first_receiver().value);
  auto sensor_itr = _sensors.find( devname.value );

  name miner = sensor_itr->miner;

  // First erase observations
  weather_table_t _weather(get_self(), get_first_receiver().value);
  auto wthr_itr = _weather.find( devname.value );
  if ( wthr_itr != _weather.cend()) // if row was found
  {
      _weather.erase( wthr_itr ); // remove the row
  }

  // Erase rewards table
  rewards_table_t _rewards(get_self(), get_first_receiver().value);
  auto rewards_itr = _rewards.find( devname.value );
  if ( rewards_itr != _rewards.cend()) // if row was found
  {
    _rewards.erase( rewards_itr );
  }

  // Finally erase the sensor table
  _sensors.erase( sensor_itr );

}

ACTION dclimateiot::rmnoaasensor(uint16_t num)
{
  // Erase "num" number of datapoints from noaa sensor list

  // Require auth from self
  require_auth( get_self() );

  // Get noaa sensor list
  noaa_table_t _noaasensors( get_self(), get_first_receiver().value );

  uint16_t count = 0;

  auto itr = _noaasensors.begin();
  while ( itr != _noaasensors.end() && count < num )
  {
    itr = _noaasensors.erase( itr );
    count++;
  }
    
}

float dclimateiot::calcDistance( float lat1, float lon1, float lat2, float lon2 )
{
  // This function uses the given lat/lon of the devices to deterimine
  //    the distance between them.
  // The calculation is made using the Haversine method and utilizes Math.h

  float R = 6372800; // Earth radius in meters

  float phi1 = degToRadians(lat1);
  float phi2 = degToRadians(lat2);
  float dphi = degToRadians( lat2 - lat1 );
  float dlambda = degToRadians( lon2 - lon1 );

  float a = pow(sin(dphi/2), 2) + pow( cos(phi1)*cos(phi2)*sin(dlambda/2.0) , 2);

  float distance_meters = 2*R*atan2( sqrt(a) , sqrt(1-a) );

  // return distance in km
  return distance_meters/1000.0;

}

float dclimateiot::degToRadians( float degrees )
{
  // M_PI comes from math.h
    return (degrees * M_PI) / 180.0;
}

float dclimateiot::calcLocMultiplier( name devname, float max_distance )
{
  // First get the latitude/longitude
  weather_table_t _weather(get_self(), get_first_receiver().value);
  auto devitr = _weather.find( devname.value );

  noaa_table_t _noaasensors( get_self(), get_first_receiver().value );

  // Get device's reported lat/lon
  float lat1 = devitr->latitude_deg;
  float lon1 = devitr->longitude_deg;

  /*
      Because the list of noaa sensors can be extremely long (up to 120,0000 lines), we cannot
        simply check the distance across each one. Doing so on public blockchain would mean CPU timeout.
      So first we will create a lat/lon "bounding box" around the sensor in question,
        and then only calculate distances to all the sensors that lie within this box.
  */

  // First determine the length of one degree of lat and one degree of lon
  float one_lat_degree_km = 111.0; // Distance between latitudes is essentially constant
  float one_lon_degree_km = (M_PI/180)*6372.8*cos(degToRadians(lat1)); // meridian distance varies by lat

  // Calculate all 4 "sides" of the box
  float lon_lower = lon1 - max_distance/one_lon_degree_km;
  float lon_upper = lon1 + max_distance/one_lon_degree_km;
  float lat_lower = lat1 - max_distance/one_lat_degree_km;
  float lat_upper = lat1 + max_distance/one_lat_degree_km;

  // Get SORTED list of sensors to the "right" of the leftern-most station in the box.
  // We do this to increase the speed and significantly reduce the number of noaa
  //   sensors we have to check in the next steps.
  // I'm choosing longitude, because global stations are spread out more evenly by lon than lat
  auto lon_index = _noaasensors.get_index<"longitude"_n>();
  auto lon_itr = lon_index.lower_bound( lon_lower );

  // Begin by assuming closest station is outside the box
  float distance_min = max_distance;

  // Remember this lon_itr list is sorted... so we can start with the left-most
  //   station in the box, and stop at the first station that falls outside the box
  for ( auto itr = lon_itr ; itr->longitude_deg < lon_upper ; itr++ )
  {
    // Check if latitude constraints are met as well.
    if ( itr->latitude_deg > lat_lower && itr->latitude_deg < lat_upper )
    {
      // At this point, noaa station is inside the box. Calculate the distance.
      float lat2 = itr->latitude_deg;
      float lon2 = itr->longitude_deg;
      float distance = calcDistance(lat1, lon1, lat2, lon2);

      // If this noaa sensor is the closest so far, set distance_min
      if ( distance < distance_min )
      {
        distance_min = distance;
      }

    }// end latitude check
  } // end longitude loop

  // Lowest allowable distance to receive reward.
  float lowest_distance = 50.0;

  if ( distance_min < lowest_distance )
    { return 1.0; } // Less than 50km means no multiplier (equal to 1)
  else if ( distance_min >= max_distance )
    { return 1.5; } // Max reward
  else
  {
    // If closes sensor is between 50km (lowest tolerance) and 100km (highest tolerance)
    float ratio = ( distance_min - lowest_distance ) / (max_distance - lowest_distance);
    return 1 + 0.5*ratio;
  }
  
}


void dclimateiot::sendReward( name miner, name devname ) 
{
  /* Determines how much reward to send based on:
   1. Base hourly reward for given currency type
   2. Location multiplier
   3. Uplink multiplier
   4. On-chain price oracle (for USD-denominated amounts)

   Once this is determined, actually send the reward
  */
  
  rewards_table_t _rewards(get_self(), get_first_receiver().value);
  auto rewards_itr = _rewards.find( devname.value );

  parameters_table_t _parameters(get_self(), get_first_receiver().value );
  auto parameters_itr = _parameters.find( rewards_itr->token_contract.value );

  float consecutive_rounds = (float)rewards_itr->consecutive_rounds;
  
  float rounds_mult;
  if ( consecutive_rounds > parameters_itr->max_rounds ) 
    rounds_mult = 2;
  else
    rounds_mult = 1 + (consecutive_rounds / parameters_itr->max_rounds);

  float token_amount;

  if ( parameters_itr->usd_denominated ) {
    // Get price oracle data
    datapointstable _delphi_prices( name("delphioracle"), "tlosusd"_n.value );
    auto delphi_itr = _delphi_prices.begin(); // gets the latest price
    float usd_price = delphi_itr->value / 10000.0;

    token_amount = (parameters_itr->base_hourly_rate
                    //* parameters_itr->reward_hours
                    * rewards_itr->loc_multiplier
                    * rounds_mult
                    ) / usd_price;
  } else {
    token_amount = (parameters_itr->base_hourly_rate
                  //* parameters_itr->reward_hours
                  * rewards_itr->loc_multiplier
                  * rounds_mult);
  }

  uint32_t amt_number = (uint32_t)(pow( 10, parameters_itr->precision ) * 
                                        token_amount);
    
  eosio::asset reward = eosio::asset( 
                          amt_number,
                          symbol(symbol_code( parameters_itr->symbol_letters ), parameters_itr->precision));

  if( rewards_itr->token_contract == name("eosio.evm") )
  {
    // Look up the EVM address from the evm contract
    evm_table_t _evmaccounts( name("eosio.evm"), name("eosio.evm").value );
    auto acct_index = _evmaccounts.get_index<"byaccount"_n>();
    auto evm_itr = acct_index.find( miner.value ); // Use miner's name to query

    checksum160 evmaddress = evm_itr->address;
    std::array<uint8_t, 32u> bytes = eosio_evm::fromChecksum160( evmaddress );

    // Convert to hex and chop off padded bytes
    string memo = "0x" + eosio_evm::bin2hex(bytes).substr(24);
    action(
      permission_level{ get_self(), "active"_n },
      "eosio.token"_n , "transfer"_n,
      std::make_tuple( get_self(), "eosio.evm"_n, reward, memo )
    ).send();

  } else {

    string memo = "Weather sensor payment";

    // Do token transfer using an inline function
    //   This works even with "iot" or another account's key being passed, because even though we're not passing
    //   the traditional "active" key, the "active" condition is still met with @eosio.code
    action(
      permission_level{ get_self(), "active"_n },
      rewards_itr->token_contract , "transfer"_n,
      std::make_tuple( get_self(), miner, reward, memo)
    ).send();

  }
    


}

// Dispatch the actions to the blockchain
EOSIO_DISPATCH(dclimateiot, (addsensor)(addminer)(addnoaa)(addflag)(submitdata)(submitgps)(chngreward)(setrate)(addparam)(removeparam)(removereward)(removeobs)(removesensor)(rmnoaasensor))
