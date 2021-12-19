
using namespace std;
using namespace eosio;

// Define delphi oracle data structure   
TABLE delphi_data {
  uint64_t id;
  name owner; 
  uint64_t value;
  uint64_t median;
  time_point timestamp;

  uint64_t primary_key() const {return id;}
  uint64_t by_timestamp() const {return timestamp.elapsed.to_seconds();}
  uint64_t by_value() const {return value;}

  };
  typedef eosio::multi_index<"datapoints"_n, delphi_data,
    indexed_by<"value"_n, const_mem_fun<delphi_data, uint64_t, &delphi_data::by_value>>, 
    indexed_by<"timestamp"_n, const_mem_fun<delphi_data, uint64_t, &delphi_data::by_timestamp>>> datapointstable;
