#pragma once

#include <string>
#include <vector>

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/time.hpp>

#include "fsm/fsm.hpp"

#define VKT_SYMBOL symbol("VKT", 4)

// #define PRODUCTION

static const eosio::microseconds EXPIRE_OPEN = eosio::days(7);
static const eosio::microseconds EXPIRE_TURN = eosio::days(1);
static const eosio::microseconds EXPIRE_GAME_OVER = eosio::days(3);

CONTRACT bettingsmall : public eosio::contract {
 public:
  bettingsmall(eosio::name receiver, eosio::name code,
             eosio::datastream<const char *> ds)
      : contract(receiver, code, ds), games(receiver, receiver.value) {}

  TABLE player {
    uint64_t id;
    eosio::name account;
    bool player_can_claim;
    eosio::asset bet_amount_per_player;

    auto primary_key() const { return id; }

    EOSLIB_SERIALIZE(player, (id)(account)(player_can_claim)
                            (bet_amount_per_player))
  };

  TABLE game {
    // game meta information
    uint64_t id;
    eosio::name creator;
    // eosio::name team1;
    // eosio::name team2;
    // needed for hashing from the frontend when hiding ships
    // (pk id is not a nonce, it can repeat when erasing games)
    uint32_t creator_nonce;
    // uint32_t team1_nonce;
    // uint32_t team2_nonce;
    // team 1 players
    // typedef eosio::multi_index< "team1players"_n, player > team1players_t;
    // team1players_t team1players;
    std::vector<player> team1players;
    // team 2 players
    // typedef eosio::multi_index< "team2players"_n, player > team2players_t;
    // team2players_t team2players;
    std::vector<player> team2players;
    
    // when payouts are done with deferred transactions and they fail
    // this flag handles the alternative manual payout
    bool team1_can_claim;
    bool team2_can_claim;
    // eosio::asset bet_amount_per_team;
    eosio::time_point_sec expires_at;
    // actual game data like ships, hits, etc.
    fsm::game_data game_data;

    auto primary_key() const { return id; }
    uint64_t by_expires_at() const { return expires_at.sec_since_epoch(); }
    // auto by_team1players() const { return team1players; }
    // auto by_team2players() const { return team2players; }
    uint64_t by_game_state() const { return game_data.state; }

    EOSLIB_SERIALIZE(game, (id)(creator)(creator_nonce)(team1players)(team2players)
                               (team1_can_claim)(team2_can_claim)
                               (expires_at)(game_data))
  };

  typedef eosio::multi_index<
      "games"_n, game,
      // eosio::indexed_by<
      //     "team1players"_n, eosio::const_mem_fun<game, team1players_t, &game::by_team1players>>,
      // eosio::indexed_by<
      //     "team2players"_n, eosio::const_mem_fun<game, team2players_t, &game::by_team2players>>,
      eosio::indexed_by<
          "expiresat"_n,
          eosio::const_mem_fun<game, uint64_t, &game::by_expires_at>>,
      eosio::indexed_by<"state"_n, eosio::const_mem_fun<game, uint64_t,
                                                        &game::by_game_state>>>
      games_t;

  auto get_game(uint64_t game_id) {
    const auto game = games.find(game_id);
    eosio::check(game != games.end(), "Game not found");
    return game;
  }

  void assert_player_in_game(const game &game, eosio::name player) {
    bool is_part_of_team1 = false;
    bool is_part_of_team2 = false;
    // eosio::check( game.team1players.size() > 0, "there is no player in the team 1 of this game" );
    // eosio::check( game.team2players.size() > 0, "there is no player in the team 2 of this game" );
    // for( size_t i = 0; i < game.team1players.size(); i++ ) {
    //   if(game.team1players[i].account == player){
    //     is_part_of_team1 = true;
    //     break;
    //   }
    // }
    for (auto player_itr = game.team1players.begin(); player_itr != game.team1players.end(); ++player_itr) {
      if(player_itr->account == player){
        is_part_of_team1 = true;
        break;
      }
    }
    // for( size_t i = 0; i < game.team2players.size(); i++ ) {
    //   if(game.team2players[i].account == player){
    //     is_part_of_team2 = true;
    //     break;
    //   }
    // }
    for (auto player_itr = game.team2players.begin(); player_itr != game.team2players.end(); ++player_itr) {
      if(player_itr->account == player){
        is_part_of_team2 = true;
        break;
      }
    }

    eosio::check(is_part_of_team1 || is_part_of_team2,
                    "You are not part of this game");
  }

#ifndef PRODUCTION
  ACTION testreset(uint16_t max_games);
#endif
  // implemented in cleanup.cpp
  ACTION cleanup();
  ACTION create(eosio::name creator, uint32_t nonce, const eosio::checksum256 &commitment);
  ACTION close(eosio::name creator, uint32_t nonce, uint64_t game_id,
              const eosio::checksum256 &commitment);
  void attack(uint64_t game_id, eosio::name team,
                const std::vector<uint8_t> &attacks);
  ACTION reveal(uint64_t game_id, eosio::name team);
  ACTION decommit(uint64_t game_id, eosio::name team,
                  const eosio::checksum256 &decommitment);
  ACTION claim(uint64_t game_id, const player &player);

  [[eosio::on_notify("eosio.token::transfer")]] 
  void transfer(eosio::name from, eosio::name to, const eosio::asset &quantity,
                std::string memo);
  // [[eosio::on_notify("*::transfer")]] 
  // ACTION transfer(eosio::name from, eosio::name to, const eosio::asset &quantity, std::string memo);

  // [[eosio::on_notify("eosio.token::transfer")]]
  // void dummytansfer(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo){transfer(from,to,quantity,memo);} 

  void p1_deposit(eosio::name team1player, uint64_t game_id,
                  const eosio::asset &quantity);
  void p2_deposit(eosio::name team2player, uint64_t game_id,
                  const eosio::asset &quantity);

  games_t games;
};
