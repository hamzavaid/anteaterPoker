#include <stdio.h>
#include <string.h>

#include "poker_rules.h"
#include "game_state.h"
#include "cards.h"

int poker_action_is_legal(const GameState *game, int seat, const char *action, int raise_amount)
{
    // Check if the game state and action are valid, and if it's the player's turn.
    if (game == NULL || action == NULL)
    {
        return 0;
    }

    // Check if the seat number is valid and corresponds to an active player.
    if (seat < 0 || seat >= game->player_count)
    {
        return 0;
    }

    // Check if it's the player's turn.
    if (game->current_turn != seat)
    {
        return 0;
    }

    const Player *player = &game->players[seat];

    if (strcmp(action, "FOLD") == 0)
    {
        return 1;
        // CHECK: If the player has already matched the current bet, they can check.
    }
    else if (strcmp(action, "CHECK") == 0)
    {
        return player->current_bet >= game->current_bet;
        // CALL: If the player has not yet matched the current bet, they can call if they have enough points.
    }
    else if (strcmp(action, "CALL") == 0)
    {
        return player->current_bet < game->current_bet &&
               player->points >= (game->current_bet - player->current_bet);
        // RAISE: If the player has not yet matched the current bet, they can raise if they have enough points and the raise amount is valid.
    }
    else if (strcmp(action, "RAISE") == 0)
    {
        int min_raise = game->current_bet * 2 - player->current_bet;
        return raise_amount >= min_raise && raise_amount <= player->points;
    }
    return 0;
}

/*Gonna implement these function later, just wanted to get the header file done first.
Lowkey hella work so some help would be appreciated. Some function are set to return 0 for now
so nothing breaks (hopefully)
-jim
*/
int poker_apply_action(GameState *game, int seat, const char *action, int raise_amount)
{
    return 0;
}

int poker_is_betting_round_finished(const GameState *game)
{
    return 0;
}

void poker_advance_phase(GameState *game);

void poker_resolve_showdown(GameState *game);

int poker_find_showdown_winners(const GameState *game, int winners[], int max_winners)
{
    return 0;
}

int poker_count_active_players(const GameState *game)
{
    return 0;
}

/*
returns the seat index of the first active player, or -1 if none.
*/
int poker_get_first_active_player(const GameState *game)
{
    if (game == NULL) {
        return -1;
    }
    for (int i = 0; i < game->player_count; ++i) {
        if (game->players[i].status == PLAYER_ACTIVE) {
            return i;
        }
    }
    return -1;
}

/*
returns the next active player after a given seat (wrapping around), or -1 if none.
*/
int poker_get_next_active_player(const GameState *game, int seat)
{
    if (game == NULL || game->player_count == 0) {
        return -1;
    }
    int start = (seat + 1) % game->player_count;
    int i = start;
    do {
        if (game->players[i].status == PLAYER_ACTIVE) {
            return i;
        }
        i = (i + 1) % game->player_count;
    } while (i != start);
    return -1;
}

/*
Compares two poker hands
0: Complete Tie
1: First hand is better then second
-1: Second hand is better then second
*/
int poker_compare_hands(const PokerHandValue *a, const PokerHandValue *b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    // Compare hand ranks first
    if (a->rank > b->rank) {
        return 1;
    } else if (a->rank < b->rank) {
        return -1;
    }
    // If ranks are equal, compare tiebreaker values
    for (int i = 0; i < 5; ++i) {
        if (a->values[i] > b->values[i]) {
            return 1;
        } else if (a->values[i] < b->values[i]) {
            return -1;
        }
    }
    // Completely tied
    return 0;
}

/*
Takes the 7 cards from the total community cards and the 2 private and find the best 5-card hand rank and tiebreakers according to poker rules.
Fills PokerHandValue with the best hand.
*/
void poker_evaluate_hand(const Card cards[7], PokerHandValue *value)
{
    if (!value) return;
    // Count occurrences of each rank and suit
    int rank_count[15] = {0}; // RANK_TWO=2 ... RANK_ACE=14
    int suit_count[4] = {0};
    for (int i = 0; i < 7; ++i) {
        if (cards[i].rank >= RANK_TWO && cards[i].rank <= RANK_ACE)
            rank_count[cards[i].rank]++;
        if (cards[i].suit >= SUIT_HEARTS && cards[i].suit <= SUIT_SPADES)
            suit_count[cards[i].suit]++;
    }

    // Find flush
    int flush_suit = -1;
    for (int s = 0; s < 4; ++s) {
        if (suit_count[s] >= 5) {
            flush_suit = s;
            break;
        }
    }

    // Find straight (and straight flush)
    int straight_high = 0, straight_flush_high = 0;
    for (int start = 14; start >= 5; --start) {
        int found = 1;
        for (int j = 0; j < 5; ++j) {
            if (rank_count[start - j] == 0) {
                found = 0;
                break;
            }
        }
        if (found) {
            straight_high = start;
            // Check for straight flush
            if (flush_suit != -1) {
                int count = 0;
                for (int i = 0; i < 7; ++i) {
                    if (cards[i].suit == flush_suit && cards[i].rank <= start && cards[i].rank >= start - 4)
                        count++;
                }
                if (count >= 5) straight_flush_high = start;
            }
            break;
        }
    }
    // Special case: wheel straight (A-2-3-4-5)
    if (!straight_high && rank_count[RANK_ACE] && rank_count[RANK_TWO] && rank_count[RANK_THREE] && rank_count[RANK_FOUR] && rank_count[RANK_FIVE]) {
        straight_high = 5;
        if (flush_suit != -1) {
            int count = 0;
            for (int i = 0; i < 7; ++i) {
                if (cards[i].suit == flush_suit &&
                    (cards[i].rank == RANK_ACE || (cards[i].rank >= RANK_TWO && cards[i].rank <= RANK_FIVE)))
                    count++;
            }
            if (count >= 5) straight_flush_high = 5;
        }
    }

    // Count multiples (pairs, trips, quads)
    int pairs[3] = {0}, npairs = 0, trips[2] = {0}, ntrips = 0, quad = 0;
    for (int r = 14; r >= 2; --r) {
        if (rank_count[r] == 4) quad = r;
        else if (rank_count[r] == 3 && ntrips < 2) trips[ntrips++] = r;
        else if (rank_count[r] == 2 && npairs < 3) pairs[npairs++] = r;
    }

    // Fill PokerHandValue
    if (straight_flush_high) {
        value->rank = HAND_RANK_STRAIGHT_FLUSH;
        value->values[0] = straight_flush_high;
        for (int i = 1; i < 5; ++i) value->values[i] = 0;
        return;
    }
    if (quad) {
        value->rank = HAND_RANK_FOUR_OF_A_KIND;
        value->values[0] = quad;
        // Kicker
        for (int r = 14; r >= 2; --r) {
            if (r != quad && rank_count[r]) { value->values[1] = r; break; }
        }
        for (int i = 2; i < 5; ++i) value->values[i] = 0;
        return;
    }
    if (ntrips && npairs) {
        value->rank = HAND_RANK_FULL_HOUSE;
        value->values[0] = trips[0];
        value->values[1] = pairs[0];
        for (int i = 2; i < 5; ++i) value->values[i] = 0;
        return;
    }
    if (ntrips > 1) { // Double trips: use highest as trips, next as pair
        value->rank = HAND_RANK_FULL_HOUSE;
        value->values[0] = trips[0];
        value->values[1] = trips[1];
        for (int i = 2; i < 5; ++i) value->values[i] = 0;
        return;
    }
    if (flush_suit != -1) {
        value->rank = HAND_RANK_FLUSH;
        int count = 0;
        for (int r = 14; r >= 2 && count < 5; --r) {
            for (int i = 0; i < 7 && count < 5; ++i) {
                if (cards[i].suit == flush_suit && cards[i].rank == r) {
                    value->values[count++] = r;
                }
            }
        }
        while (count < 5) value->values[count++] = 0;
        return;
    }
    if (straight_high) {
        value->rank = HAND_RANK_STRAIGHT;
        value->values[0] = straight_high;
        for (int i = 1; i < 5; ++i) value->values[i] = 0;
        return;
    }
    if (ntrips) {
        value->rank = HAND_RANK_THREE_OF_A_KIND;
        value->values[0] = trips[0];
        int count = 1;
        for (int r = 14; r >= 2 && count < 5; --r) {
            if (r != trips[0] && rank_count[r]) value->values[count++] = r;
        }
        while (count < 5) value->values[count++] = 0;
        return;
    }
    if (npairs >= 2) {
        value->rank = HAND_RANK_TWO_PAIR;
        value->values[0] = pairs[0];
        value->values[1] = pairs[1];
        int count = 2;
        for (int r = 14; r >= 2 && count < 5; --r) {
            if (r != pairs[0] && r != pairs[1] && rank_count[r]) value->values[count++] = r;
        }
        while (count < 5) value->values[count++] = 0;
        return;
    }
    if (npairs == 1) {
        value->rank = HAND_RANK_PAIR;
        value->values[0] = pairs[0];
        int count = 1;
        for (int r = 14; r >= 2 && count < 5; --r) {
            if (r != pairs[0] && rank_count[r]) value->values[count++] = r;
        }
        while (count < 5) value->values[count++] = 0;
        return;
    }
    // High card
    value->rank = HAND_RANK_HIGH_CARD;
    int count = 0;
    for (int r = 14; r >= 2 && count < 5; --r) {
        if (rank_count[r]) value->values[count++] = r;
    }
    while (count < 5) value->values[count++] = 0;
    return;
}

const char *poker_hand_rank_to_string(PokerHandRank rank)
{
    return "Not implemented yet";
}