#include "../helpers.cpp"

using namespace std;
using namespace eosio;

ACTION ascensionwx::migrewards() {

  // Only own contract can add a new sensor
  require_auth( get_self() );

/*
  ////////////// Temporary calculation of quality rates using v1 ///////////////

  rewardsv1_table_t _rewardsv1( get_self(), get_first_receiver().value );

  minersv1_table_t _minersv1( get_self(), get_first_receiver().value );

  tokensv1_table_t _tokensv1(get_self(), get_first_receiver().value);
  auto tokensv1_itr = _tokensv1.find("eosio.token"_n.value);

  rewardsv2_table_t _rewardsv2( get_self(), get_first_receiver().value );

  auto rewardsv1_itr = _rewardsv1.begin();
  while ( rewardsv1_itr != _rewardsv1.cend() ) {

    name miner = rewardsv1_itr->miner;
    auto minerv1_itr = _minersv1.find( miner.value );
    float multiplier = minerv1_itr->multiplier;

    _rewardsv2.emplace( get_self(), [&](auto& reward) {

        reward.devname = rewardsv1_itr->devname;
        reward.miner = rewardsv1_itr->miner;
        reward.builder = rewardsv1_itr->builder;
        reward.sponsor = rewardsv1_itr->sponsor;

        reward.last_miner_payout = rewardsv1_itr->last_miner_payout;
        reward.last_builder_payout = rewardsv1_itr->last_builder_payout;
        reward.last_sponsor_payout = rewardsv1_itr->last_sponsor_payout;

        reward.poor_quality_rate = tokensv1_itr->miner_amt_per_poor_obs*multiplier;
        reward.good_quality_rate = tokensv1_itr->miner_amt_per_good_obs*multiplier;
        reward.great_quality_rate = tokensv1_itr->miner_amt_per_great_obs*multiplier;

        reward.picture_sent = rewardsv1_itr->picture_sent;
        reward.unique_location = 1;
        reward.recommend_memo = rewardsv1_itr->recommend_memo;
    });

    // Finally increment the rewardsv1 iterator
    rewardsv1_itr++;
  }
  
*/
/*
  rewardsv1_table_t _rewards(get_self(), get_first_receiver().value);
  auto rewards_itr = _rewards.begin();
  while ( rewards_itr != _rewards.end()) // if row was found
  {
    rewards_itr = _rewards.erase( rewards_itr );
  }

  tokensv1_table_t _tokens(get_self(), get_first_receiver().value);
  auto tokens_itr = _tokens.begin();
  while ( tokens_itr != _tokens.end()) // if row was found
  {
    tokens_itr = _tokens.erase( tokens_itr );
  }
*/

}

ACTION ascensionwx::migsensors() {
  // Only own contract can add a new sensor
  require_auth( get_self() );

/*
  sensorsv2_table_t _sensorsv2(get_self(), get_first_receiver().value);
  sensorsv3_table_t _sensorsv3(get_self(), get_first_receiver().value);


  auto sensorsv2_itr = _sensorsv2.begin();
  while ( sensorsv2_itr != _sensorsv2.cend() ) {

    _sensorsv3.emplace( get_self(), [&](auto& sensor) {

        sensor.devname = sensorsv2_itr->devname;
        sensor.devname_uint64 = sensorsv2_itr->devname_uint64;
        sensor.station_type = sensorsv2_itr->station_type;
        sensor.time_created = sensorsv2_itr->time_created;
        sensor.message_type = sensorsv2_itr->message_type;

        sensor.firmware_version = sensorsv2_itr->firmware_version;

        sensor.voltage_mv = 0;
        sensor.num_uplinks_today = 0;
        sensor.num_uplinks_poor_today = 0;
        sensor.num_stations_nearby = 0;
        sensor.percent_uplinks_poor_previous = 0;
        sensor.sensor_pay_previous = 0;
        sensor.max_temp_today = 0;
        sensor.min_temp_today = 0;

        sensor.if_indoor = false;
        sensor.permanently_damaged = false;
        sensor.in_transit = false;
        sensor.one_hotspot = false;
        sensor.active_this_period = sensorsv2_itr->active_this_period;
        sensor.active_last_period = sensorsv2_itr->active_last_period;
        sensor.has_helium_miner = false;
        sensor.allow_new_memo = true;

    });

    // Increment the iterator
    sensorsv2_itr++;
  }
*/

/*
  sensorsv2_table_t _sensors2(get_self(), get_first_receiver().value);
  auto sensors_itr2 = _sensors2.begin();
  while ( sensors_itr2 != _sensors2.end()) // if row was found
  {
    sensors_itr2 = _sensors2.erase( sensors_itr2 );
  }
*/

}

ACTION ascensionwx::migminers() {
  // Only own contract can add a new sensor
  require_auth( get_self() );
/*
  minersv1_table_t _minersv1(get_self(), get_first_receiver().value);
  minersv2_table_t _minersv2(get_self(), get_first_receiver().value);

  auto minersv1_itr = _minersv1.begin();
  while ( minersv1_itr != _minersv1.cend() ) {

    if (minersv1_itr->evm_address != "" && minersv1_itr->evm_address != "0x0000000000000000000000000000000000000000" ) {
      _minersv2.emplace( get_self(), [&](auto& miners) {
        miners.miner = minersv1_itr->miner;
        miners.token_contract = minersv1_itr->token_contract;
        miners.evm_address_str = minersv1_itr->evm_address;
        //miners.evm_address256 = eosio_evm::pad160(evm_string_to_checksum160( minersv1_itr->evm_address ));
        miners.evm_send_enabled = minersv1_itr->evm_send_enabled;
        miners.balance = minersv1_itr->balance;
        miners.previous_day_pay_usd = 0;
      });
    } else 
      _minersv2.emplace( get_self(), [&](auto& miners) {
        miners.miner = minersv1_itr->miner;
        miners.token_contract = minersv1_itr->token_contract;
        miners.evm_address_str = "0x0000000000000000000000000000000000000000";
        //miners.evm_address256 = eosio_evm::pad160(evm_string_to_checksum160( minersv1_itr->evm_address ));
        miners.evm_send_enabled = minersv1_itr->evm_send_enabled;
        miners.balance = minersv1_itr->balance;
        miners.previous_day_pay_usd = 0;
      });

    // Increment the iterator
    minersv1_itr++;
  }
*/
/*
  minersv2_table_t _minersv2(get_self(), get_first_receiver().value);

  auto minersv2_itr = _minersv2.begin();
  while ( minersv2_itr != _minersv2.cend() ) {

    if (test != "" ) {
      _minersv2.modify( minersv2_itr, get_self(), [&](auto& miner) {
          miner.address = evm_string_to_checksum160( minersv1_itr->evm_address );
      });
    }

    minersv1_itr++;
  }
*/

  // Remove entire miners table
    // Erase rewards table
    /*
  minersv1_table_t _miners(get_self(), get_first_receiver().value);
  auto miners_itr = _miners.begin();
  while ( miners_itr != _miners.end()) // if row was found
  {
    miners_itr = _miners.erase( miners_itr );
  }
*/
}
