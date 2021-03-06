#include "volentixprir.hpp"

void volentixprir::txfds( name account ) {
    require_auth( txfds_treasury );
    require_auth( account );
    
    facilitators_index facilitators(_self, _self.value);
    auto iterator = facilitators.find(account.value);
    check(iterator != facilitators.end(), "facilitator doesn't exist");

    time_point_sec tps = time_point_sec();
    uint32_t sse = tps.sec_since_epoch();
    asset already_allocated = iterator->already_allocated;
    asset total_allocation = iterator->allocation;
    check(already_allocated.amount < total_allocation.amount, "all available VTX already allocated");
    asset allocation = calculate_allocation(sse, total_allocation);
    
    asset to_send = allocation - already_allocated;
    check(to_send.amount > 0, "liquid allocation is 0, try later");
    vector<permission_level> p;
    p.push_back(permission_level{ get_self(), "active"_n });
    p.push_back(permission_level{ txfds_treasury, "active"_n });
    
    action(
        p, 
        vtxsys_contract, 
        "transfer"_n, 
        std::make_tuple( get_self(), account, to_send, std::string("") )
    ).send();

    facilitators.modify(iterator, txfds_treasury, [&]( auto& row ) {
            row.already_allocated += to_send;
        });
}



void volentixprir::afacilitator(name account, asset allocation) {
    require_auth(facilitators_modify_treasury);
    check(allocation.symbol == vtx_symbol, "only VTX symbol allowed");
    check(allocation.amount > 0, "allocation should be greater than 0");

    facilitators_index facilitators(_self, _self.value);
    auto iterator = facilitators.find( account.value );
    if ( iterator == facilitators.end() ) {
        facilitators.emplace(facilitators_modify_treasury, [&] ( auto& row ) {
            row.key = account;
            row.allocation = allocation;
            row.already_allocated.symbol = vtx_symbol;
            row.already_allocated.amount = 0;
        });
    } else {
        facilitators.modify(iterator, facilitators_modify_treasury, [&]( auto& row ) {
            row.allocation = allocation;
        });
    }

}

void volentixprir::erase(name account) {
    require_auth(facilitators_modify_treasury);
    facilitators_index facilitators(_self, _self.value);
    auto iterator = facilitators.find( account.value );
    check(iterator != facilitators.end(), "facilitator does not exist");
    facilitators.erase(iterator);

}

EOSIO_DISPATCH(volentixprir, (afacilitator)(txfds)(erase))

