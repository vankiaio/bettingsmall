// #include "../bettingsmall.hpp"
#include "./fsm.hpp"

using namespace fsm;

void automaton::p1_deposit() {
  eosio::check(data.state == CREATED || data.state == P1_DEPOSITED || data.state == P2_DEPOSITED ||
               data.state == ALL_DEPOSITED || data.state == WHITE_BOX_PERIOD || data.state == BLACK_BOX_PERIOD , 
               "team1 funds already deposited");

  switch (data.state)
  {
  case CREATED:
    data.state = P1_DEPOSITED;
    break;
  case P2_DEPOSITED:
    data.state = ALL_DEPOSITED;
    break;
  case ALL_DEPOSITED:
    data.state = WHITE_BOX_PERIOD;
    break;
  default:
    break;
  }
  
}

void automaton::p2_deposit() {
  eosio::check(data.state == CREATED || data.state == P1_DEPOSITED || data.state == P2_DEPOSITED ||
               data.state == ALL_DEPOSITED || data.state == WHITE_BOX_PERIOD || data.state == BLACK_BOX_PERIOD , 
               "team2 funds already deposited");
  switch (data.state)
  {
  case CREATED:
    data.state = P2_DEPOSITED;
    break;
  case P1_DEPOSITED:
    data.state = ALL_DEPOSITED;
    break;
  case ALL_DEPOSITED:
    data.state = WHITE_BOX_PERIOD;
    break;
  default:
    break;
  }
}

void automaton::close(const eosio::checksum256 &commitment) {
  eosio::check(data.state == ALL_DEPOSITED || data.state == WHITE_BOX_PERIOD || data.state == BLACK_BOX_PERIOD ,
               "team2 needs to deposit first or game already started");
  // data.board2.commitment = commitment;
  data.state = REVEALED_PERIOD;
}

void automaton::attack(bool is_team1, const std::vector<uint8_t> &attacks) {
  // if (is_team1) {
  //   eosio::check(data.state == P2_ATTACKED, "P2 must attack first");
  //   data.state = P1_ATTACKED;
  // } else {
  //   eosio::check(data.state == P2_REVEALED, "P2 must reveal first");
  //   bool game_over = !data.board1.has_ships() || !data.board2.has_ships();
  //   eosio::check(!game_over,
  //                "The game is already in an end state. You must decommit");
  //   data.state = P2_ATTACKED;
  // }

  // // team1 attacks are marked on board2 and vice versa
  // const auto &attacker_board = is_team1 ? data.board1 : data.board2;
  // auto &attackee_board = is_team1 ? data.board2 : data.board1;
  // attackee_board.attack(attacks, attacker_board);
}

void automaton::reveal(bool is_team1, const state bet_state) {
  eosio::check(bet_state == P1_WIN || bet_state == P2_WIN || bet_state == DRAW, 
              "competition result state is not in range");

  // (is_team1 ? data.board1 : data.board2).reveal(attack_responses);
  eosio::check(!is_in_end_state(), "P1 must attack first");
  
  data.state = bet_state;
}

void automaton::decommit(bool is_team1,
                         const eosio::checksum256 &decommitment) {
  // if (is_team1) {
  //   eosio::check(data.state == P2_VERIFIED, "P2 must verify first");
  //   data.board1.decommit(decommitment);
  //   bool p1_has_ships = data.board1.has_ships();
  //   bool p2_has_ships = data.board2.has_ships();
  //   data.state = (!p1_has_ships && !p2_has_ships)
  //                    ? DRAW
  //                    : (p1_has_ships ? P1_WIN : P2_WIN);
  // } else {
  //   eosio::check(data.state == P2_REVEALED, "P2 must reveal first");
  //   bool game_over = !data.board1.has_ships() || !data.board2.has_ships();
  //   eosio::check(game_over, "The game is not over yet");
  //   data.state = P2_VERIFIED;
  //   data.board2.decommit(decommitment);
  // }
  data.state = (data.state != P1_WIN  && data.state != P2_WIN)
                  ? NEVER_STARTED
                  : (data.state == P1_WIN  ? P1_WIN_EXPIRED : P2_WIN_EXPIRED);
}