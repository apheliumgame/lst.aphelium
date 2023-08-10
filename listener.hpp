#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>
#include "includes/checkformat.hpp"
#include "includes/atomicdata.hpp"

using namespace std;
using namespace eosio;
using namespace atomicdata;

CONTRACT listener : public contract {
    public:
        using contract::contract;

        checksum256 get_tx_id();
        void save_asset(uint64_t asset_id, name owner);
        void checkPass(int32_t template_id, name new_asset_owner);

        [[eosio::on_notify("atomicassets::logtransfer")]] void saveTransfer (
            name collection_name,
            name from,
            name to,
            vector <uint64_t> asset_ids,
            string memo
        );

        [[eosio::on_notify("atomicassets::logmint")]] void saveMint (
            uint64_t asset_id,
            name authorized_minter,
            name collection_name,
            name schema_name,
            int32_t template_id,
            name new_asset_owner
        );

        [[eosio::on_notify("atomicassets::logburnasset")]] void saveBurn (
            name asset_owner,
            uint64_t asset_id,
            name collection_name,
            name schema_name,
            int32_t template_id,
            name asset_ram_payer
        );
        
        [[eosio::on_notify("hnl.aphelium::logequip")]] void saveEquip (
            name player, 
            string act, 
            name schema_name, 
            int32_t template_id, 
            uint64_t asset_id
        );
        
        [[eosio::on_notify("hnl.aphelium::logclaim")]] void saveClaim (
            name player,
            string entity,
            uint64_t quantity
        );
        
        [[eosio::on_notify("hnl.aphelium::logplace")]] void savePlace (
            uint64_t land_id, 
            string act, 
            name schema_name, 
            int32_t template_id, 
            uint64_t asset_id
        );

        [[eosio::action]] void saveasset(uint64_t asset_id, name owner);
        [[eosio::action]] void deltransrow (uint64_t lower, uint64_t upper);
        [[eosio::action]] void delmintrow (uint64_t lower, uint64_t upper);
        [[eosio::action]] void delburnrow (uint64_t lower, uint64_t upper);
        [[eosio::action]] void delclaimrow (uint64_t lower, uint64_t upper);
        [[eosio::action]] void delequiprows (uint64_t lower, uint64_t upper);

    private:
        struct [[eosio::table]] collassets_s {
            uint64_t asset_id;
            name owner;
            
            uint64_t primary_key() const { return asset_id; }
            uint64_t byowner() const { return owner.value; }
        };
        typedef multi_index<name("collassets"), collassets_s, indexed_by<name("assbyowner"), const_mem_fun<collassets_s, uint64_t, &collassets_s::byowner>>> collassets_t;
        collassets_t collassets = collassets_t(get_self(), get_self().value);
    
        struct [[eosio::table]] transferlog {
            uint64_t key;
            name collection_name;
            name from;
            name to;
            vector <uint64_t> assets_ids;
            string memo;
            time_point_sec created_at;
            checksum256 tx;

            uint64_t primary_key() const { return key;}
        };
        typedef multi_index<name("transferlogs"), transferlog> transferlog_s;
        transferlog_s transferlogs = transferlog_s(get_self(), get_self().value);

        struct [[eosio::table]] mintlog {
            uint64_t key;
            uint64_t asset_id;
            name authorized_minter;
            name collection_name;
            name schema_name;
            int32_t template_id;
            name new_asset_owner;
            time_point_sec created_at;
            checksum256 tx;

            uint64_t primary_key() const { return key;}
        };
        typedef multi_index<name("mintlogs"), mintlog> mintlog_s;
        mintlog_s mintlogs = mintlog_s(get_self(), get_self().value);

        struct [[eosio::table]] burnlog {
            uint64_t key;
            name asset_owner;
            uint64_t asset_id;
            name collection_name;
            name schema_name;
            int32_t template_id;
            name asset_ram_payer;
            time_point_sec created_at;
            checksum256 tx;

            uint64_t primary_key() const { return key;}
        };
        typedef multi_index<name("burnlogs"), burnlog> burnlog_s;
        burnlog_s burnlogs = burnlog_s(get_self(), get_self().value);
        
        struct [[eosio::table]] equiplog {
            uint64_t key;
            name player;
            string act;
            name schema_name;
            int32_t template_id;
            uint64_t asset_id;
            time_point_sec created_at;
            checksum256 tx;

            uint64_t primary_key() const { return key;}
        };
        typedef multi_index<name("equiplogs"), equiplog> equiplog_s;
        equiplog_s equiplogs = equiplog_s(get_self(), get_self().value);
        
        struct [[eosio::table]] claimlogs_s {
            uint64_t id;
            name player;
            string entity;
            uint64_t quantity;
            time_point_sec created_at;
            checksum256 tx;
            
            uint64_t primary_key() const { return id; }
        };
        typedef multi_index<name("claimlogs"), claimlogs_s> claimlogs_t;
        claimlogs_t claimlogs = claimlogs_t(get_self(), get_self().value);
        
        struct [[eosio::table]] placelogs_s {
            uint64_t key;
            uint64_t land_id;
            string act;
            name schema_name;
            int32_t template_id;
            uint64_t asset_id;
            time_point_sec created_at;
            checksum256 tx;
            
            uint64_t primary_key() const { return key; }
        };
        typedef multi_index<name("placelogs"), placelogs_s> placelogs_t;
        placelogs_t placelogs = placelogs_t(get_self(), get_self().value);
        
        struct assets_s {
            uint64_t         asset_id;
            name             collection_name;
            name             schema_name;
            int32_t          template_id;
            name             ram_payer;
            vector <asset>   backed_tokens;
            vector <uint8_t> immutable_serialized_data;
            vector <uint8_t> mutable_serialized_data;
            
            uint64_t primary_key() const { return asset_id; };
        };
        typedef multi_index<name("assets"), assets_s> assets_t;
        assets_t get_assets(name acc);
        
        struct schemas_s {
            name            schema_name;
            vector <FORMAT> format;

            uint64_t primary_key() const { return schema_name.value; }
        };
        typedef multi_index<name("schemas"), schemas_s> schemas_t;
        schemas_t get_schemas(name collection_name);
        
        struct templates_s {
            int32_t          template_id;
            name             schema_name;
            bool             transferable;
            bool             burnable;
            uint32_t         max_supply;
            uint32_t         issued_supply;
            vector <uint8_t> immutable_serialized_data;
    
            uint64_t primary_key() const { return (uint64_t) template_id; }
        };
        typedef multi_index <name("templates"), templates_s> templates_t;
        templates_t get_templates(name collection_name);
};