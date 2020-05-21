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


void automaton::reveal(bool is_team1, const state bet_state) {
  eosio::check(bet_state == P1_WIN || bet_state == P2_WIN || bet_state == DRAW, 
              "competition result state is not in range");

  // (is_team1 ? data.board1 : data.board2).reveal(attack_responses);
  eosio::check(!is_in_end_state(), "P1 must attack first");
  
  data.state = bet_state;
}

void automaton::decommit(bool is_team1,
                         const eosio::checksum256 &decommitment) {
  data.state = (data.state != P1_WIN && data.state != P2_WIN)
                  ? NEVER_STARTED
                  : (data.state == P1_WIN  ? P1_WIN_EXPIRED : P2_WIN_EXPIRED);
}