#include "./bettingsmall.hpp"

// https://eosio.stackexchange.com/a/1349/118
#include "./cleanup/cleanup.cpp"
#include "./fsm/fsm.cpp"
#include "./utils/utils.cpp"

using namespace eosio;
using namespace std;

// create just "opens" the game by allocating and paying for RAM
void bettingsmall::create(name creator, uint32_t nonce,
                        const eosio::checksum256 &commitment) {
  require_auth(creator);
  // any step between 0.1 and 100 VKT
  // eosio::check(quantity.symbol == VKT_SYMBOL, "only VKT tokens allowed");
  // eosio::check(quantity.amount == 1E3 || quantity.amount == 1E4 ||
  //                  quantity.amount == 1E5 || quantity.amount == 1E6,
  //              "Must pay any of 0.1 / 1.0 / 10.0 / 100.0 VKT");

  fsm::automaton machine(commitment);

  // make creator pay for RAM
  games.emplace(creator, [&](game &g) {
    // auto-increment key
    g.id = games.available_primary_key();
    g.creator = creator;
    g.creator_nonce = nonce;
    g.team1_can_claim = false;
    g.team2_can_claim = false;
    // g.bet_amount_per_team = quantity;
    g.expires_at = eosio::current_time_point() + EXPIRE_OPEN;
    g.game_data = machine.data;
  });
}

void bettingsmall::close(eosio::name creator, uint32_t nonce, uint64_t game_id,
                      const eosio::checksum256 &commitment) {
  require_auth(creator);

  auto game_itr = get_game(game_id);

  eosio::check(game_itr != games.end(), "must create a game first");
  eosio::check(game_itr->creator == creator, "can only end your own game");

  fsm::automaton machine(game_itr->game_data);
  machine.close(commitment);

  games.modify(game_itr, game_itr->creator, [&](auto &g) {
    // g.expires_at = eosio::current_time_point() + EXPIRE_TURN;
    // g.team2_nonce = nonce;
    g.game_data = machine.data;
  });
}

void bettingsmall::p1_deposit(name team1player, uint64_t game_id,
                      const asset &quantity) {
  require_auth(team1player);
  // this action should be called in a transaction after the "create" action
  // only then we can guarantee that the last created game is the opened game
  auto game_itr = get_game(game_id);

  eosio::check(game_itr != games.end(), "must create a game first");
  eosio::check(game_itr->creator != team1player, "cannot join for your own game");
  eosio::check(quantity.amount > 0, "only positive quantity allowed");

  fsm::automaton machine(game_itr->game_data);
  machine.p1_deposit();

  games.modify(game_itr, game_itr->creator, [&](game &g) {

    // join new player for one team
    g.team1players.reserve(g.team1players.size()+1);
    player p1;
    p1.id = g.team1players.size();
    p1.account = team1player;
    p1.player_can_claim = false;
    p1.bet_amount_per_player = quantity;

    g.team1players.emplace_back(p1);

    // g.expires_at = eosio::current_time_point() + EXPIRE_OPEN;
    g.game_data = machine.data;
  });
}

void bettingsmall::p2_deposit(name team2player, uint64_t game_id,
                      const asset &quantity) {
  require_auth(team2player);
  // this action should be called in a transaction after the "create" action
  // only then we can guarantee that the last created game is the opened game
  auto game_itr = get_game(game_id);

print("step----4\n",game_id);

  eosio::check(game_itr != games.end(), "must create a game first");
  eosio::check(game_itr->creator != team2player, "cannot join for your own game");
  eosio::check(quantity.amount > 0, "only positive quantity allowed");
print("step----5\n",game_id);
  fsm::automaton machine(game_itr->game_data);
  machine.p2_deposit();
print("step----6\n",game_id);
  games.modify(game_itr, game_itr->creator, [&](game &g) {
print("step----7\n",game_id);
    // join new player for one team
    g.team2players.reserve(g.team2players.size()+1);
    player p2;
    p2.id = g.team2players.size();
    p2.account = team2player;
    p2.player_can_claim = false;
    p2.bet_amount_per_player = quantity;

    g.team2players.emplace_back(p2);
print("step----8\n",game_id);
    // g.expires_at = eosio::current_time_point() + EXPIRE_OPEN;
    g.game_data = machine.data;
  });
}

void bettingsmall::transfer(name from, name to, const asset &quantity,
                          string memo) {
                            print("step----1");
  if (from == _self) {
    // we're sending money, do nothing additional
    return;
  }

  auto latest_game = games.end();
  latest_game--;

  print("step----2");

  eosio::check(latest_game != games.end(), "must create a game first");
  eosio::check(to == _self, "contract is not involved in this transfer");
  eosio::check(quantity.symbol.is_valid(), "invalid quantity");
  eosio::check(quantity.amount > 0, "only positive quantity allowed");
  eosio::check(quantity.symbol == VKT_SYMBOL, "only VKT tokens allowed");

  print("step----3");

  uint64_t game_id = latest_game->id;
  if (memo == "bettingteam1") {
    p1_deposit(from, game_id, quantity);
  } else {
    p2_deposit(from, game_id, quantity);
  }
}

void bettingsmall::reveal(uint64_t game_id, eosio::name creator) {
  require_auth(creator);
  auto game_itr = get_game(game_id);

  eosio::check(game_itr->creator == creator, "can only end your own game");

  float bet_amount_t1 = 0.0;
  float bet_amount_t2 = 0.0;
  fsm::state bet_state = DRAW;

  //calculate who is the winner or DRAW
  for (auto player_itr = game_itr->team1players.begin(); player_itr != game_itr->team1players.end(); ++player_itr) {
    bet_amount_t1 += player_itr->bet_amount_per_player.amount;
  }
  for (auto player_itr = game_itr->team2players.begin(); player_itr != game_itr->team2players.end(); ++player_itr) {
    bet_amount_t2 += player_itr->bet_amount_per_player.amount;
  }

  // the team1 win
  if(bet_amount_t1 < bet_amount_t2)
  {
    bet_state = P1_WIN;
  }else if(bet_amount_t1 > bet_amount_t2)
  {
    bet_state = P2_WIN;
  }
  
  

  fsm::automaton machine(game_itr->game_data);
  machine.reveal(creator == game_itr->creator, bet_state);

  games.modify(game_itr, game_itr->creator, [&](auto &g) {
    g.expires_at = eosio::current_time_point() + EXPIRE_GAME_OVER;
    g.game_data = machine.data;
  });
}

void bettingsmall::decommit(uint64_t game_id, eosio::name player,
                          const eosio::checksum256 &decommitment) {
  require_auth(player);
  auto game_itr = get_game(game_id);
  //assert_player_in_game(*game_itr, player);

  bool is_creator = player == game_itr->creator;
  fsm::automaton machine(game_itr->game_data);

  // team1 decommits means that the game is over and a winner was determined
  if (is_creator) {
    switch (machine.get_winner()) {
      case P1_WIN: {
        for (auto player_itr = game_itr->team1players.begin(); player_itr != game_itr->team1players.end(); ++player_itr) {
          action(permission_level{_self, "active"_n}, "eosio.token"_n,
                "transfer"_n,
                make_tuple(_self, player_itr->account,
                            player_itr->bet_amount_per_player * 2,
                            "BettingSmall Winner refund back ID:" +std::to_string(game_itr->id)))
              .send();
        }
        break;
      }
      case P2_WIN: {
        for (auto player_itr = game_itr->team2players.begin(); player_itr != game_itr->team2players.end(); ++player_itr) {
          action(permission_level{_self, "active"_n}, "eosio.token"_n,
                "transfer"_n,
                make_tuple(_self, player_itr->account,
                            player_itr->bet_amount_per_player * 2,
                            "BettingSmall Winner refund back ID:" +std::to_string(game_itr->id)))
              .send();
        }
      }
      case DRAW: {
        game_itr = get_game(game_id);
        games.modify(game_itr, game_itr->creator, [&](auto &g) {
          g.team1_can_claim = true;
          g.team2_can_claim = true;
        });
        // send deferred because P2 has incentive to block transfers
        // as he gets 2*stake if this action fails
        for (auto player_itr = game_itr->team1players.begin(); player_itr != game_itr->team1players.end(); ++player_itr) {
          claim_deferred(_self, game_itr->id, *player_itr);
        }
        for (auto player_itr = game_itr->team2players.begin(); player_itr != game_itr->team2players.end(); ++player_itr) {
          claim_deferred(_self, game_itr->id, *player_itr);
        }
        break;
      }
      default: {
        eosio::check(false, "FSM is in a broken state");
      }
    }

    machine.decommit(is_creator, decommitment);
    games.modify(game_itr, game_itr->creator, [&](auto &g) {
    // team1 decommits means that the game is over and a winner was determined
    g.expires_at =
        eosio::current_time_point() + (is_creator ? EXPIRE_GAME_OVER : EXPIRE_TURN);
    g.game_data = machine.data;
  });
  }
}

void bettingsmall::claim(uint64_t game_id, const player &player) {
  auto game_itr = get_game(game_id);
  assert_player_in_game(*game_itr, player.account);

  bool is_creator = player.account == game_itr->creator;
  bool can_claim =
      is_creator ? game_itr->team1_can_claim : game_itr->team2_can_claim;
  eosio::check(can_claim, "not allowed to claim");

  fsm::automaton machine(game_itr->game_data);
  auto multiplier = machine.get_payout_multiplier();

  games.modify(game_itr, game_itr->creator, [&](auto &g) {
    if (is_creator) {
      g.team1_can_claim = false;
    } else {
      g.team2_can_claim = false;
    }
  });

  action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n,
         make_tuple(_self, player.account, player.bet_amount_per_player * multiplier,
                    std::to_string(game_itr->id)))
      .send();
}

#ifndef PRODUCTION
void bettingsmall::testreset(uint16_t max_games) {
  require_auth(_self);
  uint16_t count = 0;
  max_games = max_games == 0 ? -1 : max_games;
  auto itr = games.begin();
  while (itr != games.end() && count < max_games) {
    itr = games.erase(itr);
    count++;
  }
}
#endif
