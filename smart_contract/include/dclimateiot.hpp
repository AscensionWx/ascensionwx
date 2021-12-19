#include <eosio/eosio.hpp>
#include <math.h> // pow() and trig functions

using namespace std;
using namespace eosio;

CONTRACT dclimateiot : public contract {
  public:
    using contract::contract;

    // Should be called by device

    ACTION addsensor( name dev_name,
                      name owner,
                      uint64_t unix_time_s);

    ACTION addnoaa( string id,
                    float latitude_deg,
                    float longitude_deg,
                    float elevation_m);

    ACTION submitdata(name devname,
                      uint64_t unix_time_s,
                      float pressure_hpa,
                      float temperature_c, 
                      float humidity_percent,
                      uint8_t flags);

    ACTION submitgps( name dev_name,
                      uint64_t unix_time_s, 
                      double latitude_deg,
                      double longitude_deg);
    
    ACTION submitelev( name dev_name,
                      float elevation);

    ACTION chngreward(name devname,
                        name token_contract);
    
    ACTION setrate(name token_contract,
                  float base_hourly_rate);

    ACTION addparam( name token_contract,
                        float max_distance_km,
                        float max_rounds,
                        string symbol_letters,
                        bool usd_denominated,
                        float base_hourly_rate,
                        uint8_t precision);

    ACTION removeparam( name token_contract );
    ACTION removereward(name devname);
    ACTION removeobs(name devname);
    ACTION removesensor( name devname );
    ACTION rmnoaasensor(uint16_t num);

  private:

    // Local functions (not actions)
    void sendReward( name miner, name devname );
    name getMiner( name devname ); // Gets the owner of a device, given its name

    float calcDistance( float lat1, float lon1, float lat2, float lon2 ); // Calculate distance between two points
    float degToRadians( float degrees );
    float calcLocMultiplier( name devname, float max_distance );
    string gen_ptrh_byte_buffer( float pressure_hpa,
                                float temperature_c,
                                float humidity_percent);
    
    static uint64_t noaa_to_uint64(string id) { 

        // Convert to lower case and convert remaining zeros to x
        for_each(id.begin(), id.end(), [](char & c){c = tolower(c);});

        // replace all numbers with a new letter to preserve uniqueness
        replace( id.begin(), id.end(), '0', 'a');
        replace( id.begin(), id.end(), '1', 'b');
        replace( id.begin(), id.end(), '2', 'c');
        replace( id.begin(), id.end(), '3', 'd');
        replace( id.begin(), id.end(), '4', 'e');
        replace( id.begin(), id.end(), '5', 'f');
        replace( id.begin(), id.end(), '6', 'g');
        replace( id.begin(), id.end(), '7', 'h');
        replace( id.begin(), id.end(), '8', 'i');
        replace( id.begin(), id.end(), '9', 'j');

        return name(id).value;

    }


    // Data Table List
    TABLE sensors {
        name devname;
        name miner;
        double latitude_deg;
        double longitude_deg;
        float elevation;
        uint64_t time_created;
        uint64_t last_location_update;
        string message_type;
        string firmware_version;

        auto  primary_key() const { return devname.value; }
        uint64_t by_miner() const { return miner.value; }
        double by_latitude() const { return latitude_deg; }
        double by_longitude() const { return longitude_deg; }
        uint64_t by_update() const { return last_location_update; } // Useful for filtering out unmaintained sensors
    };
    typedef multi_index<name("sensors"), 
                        sensors,
                        indexed_by<"miner"_n, const_mem_fun<sensors, uint64_t, &sensors::by_miner>>,
                        indexed_by<"latitude"_n, const_mem_fun<sensors, double, &sensors::by_latitude>>,
                        indexed_by<"longitude"_n, const_mem_fun<sensors, double, &sensors::by_longitude>>,
                        indexed_by<"update"_n, const_mem_fun<sensors, uint64_t, &sensors::by_update>>

    > sensors_table_t;

    TABLE parameters {
        name token_contract;
        float max_distance_km;
        float max_rounds;
        string symbol_letters;
        bool usd_denominated;
        float base_hourly_rate;
        uint8_t precision; // e.g. 4 for Telos , 8 for Kanda

        auto primary_key() const { return token_contract.value; }
    };
    typedef multi_index<name("parameters"), parameters> parameters_table_t;

    TABLE observations {
      name devname;
      uint64_t unix_time_s;
      float pressure_hpa;
      float temperature_c;
      float humidity_percent;
      float wind_dir_deg;
      float wind_spd_ms;
      float rain_1hr;
      float rain_6hr;
      float rain_24hr;
      float solar_wm2;
      uint8_t flags;

      auto  primary_key() const { return devname.value; }
    };
    //using observations_index = multi_index<"observations"_n, observations>;
    typedef multi_index<name("observations"), observations> observations_table_t;

    TABLE rewards {
      name devname;
      name token_contract;
      uint64_t last_round;
      uint16_t consecutive_rounds;
      float loc_multiplier;

      auto primary_key() const { return devname.value; }
    };
    typedef multi_index<name("rewards"), rewards> rewards_table_t;

    TABLE security {
      name devname;
      string pubkey;
      string identity_DID;
      string last_encoded_bytes;

      auto primary_key() const { return devname.value; }
    };
    typedef multi_index<name("security"), security> security_table_t;

    TABLE noaasensors {
      string id_string;
      double latitude_deg;
      double longitude_deg;
      float elevation_m;

      auto  primary_key() const { return noaa_to_uint64( id_string ); }
      double by_latitude() const { return latitude_deg; }
      double by_longitude() const { return longitude_deg; }
    };
    // Note: To conserve RAM we'll leave latitude out as index for this table
    //   (every new index has over 100 bytes overhead per row!)
    typedef multi_index<name("noaasensors"), 
                        noaasensors,
                        indexed_by<"latitude"_n, const_mem_fun<noaasensors, double, &noaasensors::by_latitude>>,
                        indexed_by<"longitude"_n, const_mem_fun<noaasensors, double, &noaasensors::by_longitude>>
    > noaa_table_t;

};
