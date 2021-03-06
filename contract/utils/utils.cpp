#include "./utils.hpp"
#include <eosio/transaction.hpp>
#include "../bettingsmall.hpp"

void claim_deferred(eosio::name _self, uint64_t game_id, const bettingsmall::player &player) {
  eosio::transaction t{};
  t.actions.emplace_back(permission_level{_self, "active"_n}, _self, "claim"_n,
                         std::make_tuple(game_id, player));

  // set delay in seconds
  t.delay_sec = 0;

  uint128_t sender_id = (((uint128_t)game_id) << 64) + player.account.value;
  // first argument is a unique sender id
  // second argument is account paying for RAM
  t.send(sender_id, _self);
}