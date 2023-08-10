#include "listener.hpp"

checksum256 listener::get_tx_id()
{
    auto size = transaction_size();
    char buf[size];
    uint32_t read = read_transaction(buf, size);
    check(size == read, "read_transaction failed");
    return sha256(buf, read);
}

void listener::checkPass(int32_t template_id, name new_asset_owner)
{
    if (template_id == 716286) {
        assets_t aa_assets = get_assets(new_asset_owner);
        auto ownedassets = collassets.get_index<name("assbyowner")>();
        auto lower = ownedassets.lower_bound(new_asset_owner.value);
        auto upper = ownedassets.upper_bound(new_asset_owner.value);
        bool egible = false;
        for (auto asset = lower; asset != upper; ++asset) {
            auto aa_asset = aa_assets.find(asset->asset_id);
            if (aa_asset != aa_assets.end() && (aa_asset->template_id == 671853 || aa_asset->template_id == 671855 || aa_asset->template_id == 672037)) {
                // Check the attribute
                schemas_t collection_schemas = get_schemas(aa_asset->collection_name);
                auto schema_itr = collection_schemas.find(aa_asset->schema_name.value);
                
                ATTRIBUTE_MAP deserialized_mutable_data = deserialize(
                    aa_asset->mutable_serialized_data,
                    schema_itr->format
                );
                uint16_t uses = get<uint16_t>(deserialized_mutable_data["uses"]);
                if (uses < 1) {
                    egible = true;
                    // Set attribute to 1
                    uint16_t new_val = 1;
                    deserialized_mutable_data["uses"] = new_val;
                    action(
                        permission_level{get_self(), name("active")},
                        name("atomicassets"),
                        name("setassetdata"),
                        make_tuple(
                            get_self(),
                            new_asset_owner,
                            aa_asset->asset_id,
                            deserialized_mutable_data
                        )
                    ).send();
                    break;
                }
            }
        }
        check(egible, "You can't claim this drops, no valid OG/Early pass found");
    }
}

void listener::save_asset(uint64_t asset_id, name owner)
{
    auto asset_itr = collassets.find(asset_id);
    if (asset_itr == collassets.end()) {
        collassets.emplace(get_self(), [&](auto &_asset) {
            _asset.asset_id = asset_id;
            _asset.owner = owner;
        });
    }
    else {
        collassets.modify(asset_itr, get_self(), [&](auto &_asset) {
            _asset.owner = owner;
        });
    }
}

void listener::saveTransfer(name collection_name, name from, name to, vector <uint64_t> asset_ids, string memo)
{
    // Save into the logs
    auto itr = transferlogs.emplace(get_self(), [&](auto& row) {
        row.key = transferlogs.available_primary_key();
        row.tx = get_tx_id();
        row.collection_name = collection_name;
        row.from = from;
        row.to = to;
        row.assets_ids = asset_ids;
        row.memo = memo;
        row.created_at = eosio::current_time_point();
    });
    
    assets_t userassets = get_assets(to);
    
    // Save into the assets table
    for (auto asset_id : asset_ids) {
        if (from == name("neftyblocksd")) {
            auto asset_itr = userassets.require_find(asset_id);
            checkPass(asset_itr->template_id, to);
        }
        save_asset(asset_id, to);
    }
}

void listener::saveMint (
        uint64_t asset_id,
        name authorized_minter,
        name collection_name,
        name schema_name,
        int32_t template_id,
        name new_asset_owner
)
{
    checkPass(template_id, new_asset_owner);
    auto itr = mintlogs.emplace(get_self(), [&](auto& row) {
        row.key = mintlogs.available_primary_key();
        row.asset_id = asset_id;
        row.authorized_minter = authorized_minter;
        row.collection_name = collection_name;
        row.schema_name = schema_name;
        row.template_id = template_id;
        row.new_asset_owner = new_asset_owner;
        row.created_at = eosio::current_time_point();
        row.tx = get_tx_id();
    });
    
    save_asset(asset_id, new_asset_owner);
}

void listener::saveBurn (
        name asset_owner,
        uint64_t asset_id,
        name collection_name,
        name schema_name,
        int32_t template_id,
        name asset_ram_payer
)
{
    auto itr = burnlogs.emplace(get_self(), [&](auto& row) {
        row.key = burnlogs.available_primary_key();
        row.asset_owner = asset_owner;
        row.asset_id = asset_id;
        row.collection_name = collection_name;
        row.schema_name = schema_name;
        row.template_id = template_id;
        row.asset_ram_payer = asset_ram_payer;
        row.created_at = eosio::current_time_point();
        row.tx = get_tx_id();
    });
    
    // Delete the record
    auto asset_itr = collassets.find(asset_id);
    if (asset_itr != collassets.end()) {
        collassets.erase(asset_itr);
    }
}

void listener::saveEquip (
    name player, 
    string act, 
    name schema_name, 
    int32_t template_id, 
    uint64_t asset_id
)
{
    equiplogs.emplace(get_self(), [&](auto& _row) {
        _row.key = equiplogs.available_primary_key();
        _row.player = player;
        _row.act = act;
        _row.schema_name = schema_name;
        _row.template_id = template_id;
        _row.asset_id = asset_id;
        _row.created_at = eosio::current_time_point();
        _row.tx = get_tx_id();
    });
}

void listener::saveClaim (
    name player,
    string entity,
    uint64_t quantity
)
{
    claimlogs.emplace(get_self(), [&](auto &_row) {
        _row.id = claimlogs.available_primary_key();
        _row.player = player;
        _row.entity = entity;
        _row.quantity = quantity;
        _row.created_at = eosio::current_time_point();
        _row.tx = get_tx_id();
    });
}

void listener::savePlace (
    uint64_t land_id, 
    string act, 
    name schema_name, 
    int32_t template_id, 
    uint64_t asset_id
)
{
    placelogs.emplace(get_self(), [&](auto& _row) {
        _row.key = placelogs.available_primary_key();
        _row.land_id = land_id;
        _row.act = act;
        _row.schema_name = schema_name;
        _row.template_id = template_id;
        _row.asset_id = asset_id;
        _row.created_at = eosio::current_time_point();
        _row.tx = get_tx_id();
    });
}

void listener::saveasset(uint64_t asset_id, name owner)
{
    require_auth(get_self());
    save_asset(asset_id, owner);
}

void listener::deltransrow(uint64_t lower, uint64_t upper)
{
    check(has_auth(get_self()), "Not authorized");
        
    auto itr_lower = transferlogs.lower_bound(lower);
    auto itr_upper = transferlogs.upper_bound(upper);
    
    // Iterate and delete rows until we reach the end_key
    while (itr_lower != itr_upper) {
        itr_lower = transferlogs.erase(itr_lower);
    }
}

void listener::delmintrow(uint64_t lower, uint64_t upper)
{
    check(has_auth(get_self()), "Not authorized");
    
    auto itr_lower = mintlogs.lower_bound(lower);
    auto itr_upper = mintlogs.upper_bound(upper);
    
    // Iterate and delete rows until we reach the end_key
    while (itr_lower != itr_upper) {
        itr_lower = mintlogs.erase(itr_lower);
    }
}

void listener::delburnrow(uint64_t lower, uint64_t upper)
{
    check(has_auth(get_self()), "Not authorized");
    
    auto itr_lower = burnlogs.lower_bound(lower);
    auto itr_upper = burnlogs.upper_bound(upper);
    
    // Iterate and delete rows until we reach the end_key
    while (itr_lower != itr_upper) {
        itr_lower = burnlogs.erase(itr_lower);
    }
}

void listener::delclaimrow(uint64_t lower, uint64_t upper)
{
    check(has_auth(get_self()), "Not authorized");
    
    auto itr_lower = claimlogs.lower_bound(lower);
    auto itr_upper = claimlogs.upper_bound(upper);
    
    // Iterate and delete rows until we reach the end_key
    while (itr_lower != itr_upper) {
        itr_lower = claimlogs.erase(itr_lower);
    }
}

void listener::delequiprows(uint64_t lower, uint64_t upper)
{
    check(has_auth(get_self()), "Not authorized");
    
    auto itr_lower = equiplogs.lower_bound(lower);
    auto itr_upper = equiplogs.upper_bound(upper);
    
    // Iterate and delete rows until we reach the end_key
    while (itr_lower != itr_upper) {
        itr_lower = equiplogs.erase(itr_lower);
    }
}

listener::assets_t listener::get_assets(name acc) {
    return assets_t(name("atomicassets"), acc.value);
}
listener::schemas_t listener::get_schemas(name collection_name) {
    return schemas_t(name("atomicassets"), collection_name.value);
}
listener::templates_t listener::get_templates(name collection_name) {
    return templates_t(name("atomicassets"), collection_name.value);
}