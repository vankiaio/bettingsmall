#pragma once

#include <string>

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/time.hpp>

#include "../logic/board.hpp"

namespace fsm {
// finite state machine states
enum state : uint8_t {
  // running game states
  WHITE_BOX_PERIOD,
  BLACK_BOX_PERIOD,
  REVEALED_PERIOD,

  // game over game states
  CREATED,
  P1_DEPOSITED,
  P2_DEPOSITED,
  ALL_DEPOSITED,
  // winning states
  P1_WIN,
  P2_WIN,
  DRAW,
  // won by opponent taking no action in time
  P1_WIN_EXPIRED,
  P2_WIN_EXPIRED,
  NEVER_STARTED,
};

struct game_data {
  // default constructor needed for multi_index default initialization
  game_data() : state(CREATED) {}
  game_data(const eosio::checksum256 &commitment)
      : state(CREATED) {}
  // is used as state struct, but cannot be serialized by EOS
  uint8_t state;

  EOSLIB_SERIALIZE(game_data, (state))
};

// struct game_data {
//   // default constructor needed for multi_index default initialization
//   game_data() : board1(), board2(), state(CREATED) {}
//   game_data(const eosio::checksum256 &commitment)
//       : board1(commitment), board2(), state(CREATED) {}
//   // is used as state struct, but cannot be serialized by EOS
//   uint8_t state;
//   logic::board board1;
//   logic::board board2;

//   EOSLIB_SERIALIZE(game_data, (state)(board1)(board2))
// };

class automaton {
 public:
  automaton(const eosio::checksum256 &commitment) : data(commitment) {}
  automaton(const game_data &gd) : data(gd) {}

  state get_winner() {
    eosio::check(
        data.state == P1_WIN || data.state == P2_WIN || data.state == DRAW,
        "not in a regular winning state");
    return (state)data.state;
  };

  bool is_in_end_state() {
    switch (data.state) {
      case P1_WIN:
      case P2_WIN:
      case DRAW:
      case P1_WIN_EXPIRED:
      case P2_WIN_EXPIRED:
      case NEVER_STARTED: {
        return true;
      }
      default:
        return false;
    }
  };

  void expire_game(bool *p1_can_claim, bool *p2_can_claim) {
    switch (data.state) {
      // draw scene
      case DRAW: {
        data.state = NEVER_STARTED;
        *p1_can_claim = false;
        *p2_can_claim = false;
        break;
      }

      // only P1 win
      case P1_WIN: {
        data.state = P1_WIN_EXPIRED;
        *p1_can_claim = false;
        *p2_can_claim = false;
        break;
      }

      // only P2 win
      case P2_WIN: {
        data.state = P2_WIN_EXPIRED;
        *p1_can_claim = false;
        *p2_can_claim = false;
        break;
      }

      // all other states are already end states
      default: {
        eosio::check(false, "game already in an end state");
      }
    }
  }

  uint32_t get_payout_multiplier() {
    switch (data.state) {
      case NEVER_STARTED: {
        // note that we get to this state also from CREATED
        // where P1 did not transfer the funds yet
        // however then p1_can_claim is set to false
        return 1;
      }

      case P1_WIN:
      case P1_WIN_EXPIRED: {
        return 2;
      }

      case P2_WIN:
      case P2_WIN_EXPIRED: {
        return 2;
      }

      case DRAW: {
        return 1;
      }
      // all other states are not end states
      default: {
        eosio::check(false,
                     "game is not in an end state - no claims possible yet");
        return 0;  // make compiler happy
      }
    }
  }

  void expire_game(bool *p1_won, uint32_t *multiplier);
  void p1_deposit();
  void p2_deposit();
  void close(const eosio::checksum256 &commitment);
  void reveal(bool is_player1, const state bet_state);
  void decommit(bool is_player1, const eosio::checksum256 &decommitment);

  game_data data;
};
}  // namespace fsm