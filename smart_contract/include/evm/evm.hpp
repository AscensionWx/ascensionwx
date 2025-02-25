#include "base.hpp"

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <eosio/symbol.hpp>

#include "namespaces.hpp"

using namespace std;
using namespace eosio;

struct [[eosio::table, eosio::contract("eosio.evm")]] Account {
    uint64_t index;
    eosio::checksum160 address;
    eosio::name account;
    uint64_t nonce;
    std::vector<uint8_t> code;
    eosio::checksum256 balance;

    Account () = default;
    Account (uint256_t _address): address(eosio_evm::addressToChecksum160(_address)) {}
    uint64_t primary_key() const { return index; };

    uint64_t get_account_value() const { return account.value; };
    uint256_t get_address() const { return eosio_evm::checksum160ToAddress(address); };

    // TODO: make this work if we need to lookup EVM balances, which we don't for this contract
    //uint256_t get_balance() const { return balance; };
    //bool is_empty() const { return nonce == 0 && balance == 0 && code.size() == 0; };
    uint64_t get_nonce() const { return nonce; };
    std::vector<uint8_t> get_code() const { return code; };

    eosio::checksum256 by_address() const { return eosio_evm::pad160(address); };

    EOSLIB_SERIALIZE(Account, (index)(address)(account)(nonce)(code)(balance));
};

typedef multi_index<"account"_n, 
        Account,
        indexed_by<name("byaddress"), const_mem_fun<Account, checksum256, &Account::by_address>>,
        indexed_by<name("byaccount"), const_mem_fun<Account, uint64_t, &Account::get_account_value>>
> evm_table_t;


struct [[eosio::table, eosio::contract("eosio.evm")]] Config {
        uint32_t trx_index;
        uint32_t last_block;

        // Actual EVM table uses bigint:checksum256 for lines below
        //   but eosio:checksum256 seems to work fine
        checksum256 gas_used_block; 
        checksum256 gas_price;

        uint64_t primary_key() const { return (uint64_t)trx_index; };

        EOSLIB_SERIALIZE(Config, (trx_index)(last_block)(gas_used_block)(gas_price));
};
typedef eosio::singleton<"config"_n, Config> evm_config_table_t;



