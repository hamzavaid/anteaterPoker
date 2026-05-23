#include <stdio.h>
#include <string.h>

#include "game_state.h"

void init_server_config(ServerConfig *config)
{
    if (config == NULL) {
        return;
    }

    config->port = 22022;
    snprintf(config->table_name, MAX_TABLE_NAME_LEN, "ZotHouse");
    config->starting_points = 1000;
    config->bot_count = 0;
    snprintf(config->log_path, MAX_LOG_PATH_LEN, "logs/game.log");
}

void init_game_state(GameState *game, const ServerConfig *config, int server_fd)
{
    if (game == NULL || config == NULL) {
        return;
    }

    game->server_fd = server_fd;
    game->config = *config;
    game->phase = PHASE_LOBBY;
    game->player_count = 0;
    game->current_turn = -1;
    game->dealer_seat = 0;
    game->pot = 0;
    game->current_bet = 0;
    game->community_count = 0;

    init_deck(&game->deck);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        game->players[i].seat = i;
        game->players[i].socket_fd = -1;
        game->players[i].name[0] = '\0';
        game->players[i].points = config->starting_points;
        game->players[i].current_bet = 0;
        game->players[i].status = PLAYER_EMPTY;

        game->players[i].ability.type = ABILITY_NONE;
        game->players[i].ability.used = 0;
        game->players[i].ability.owner_seat = i;
        game->players[i].ability.target_seat = -1;
        game->players[i].ability.param = 0;
    }

    for (int i = 0; i < COMMUNITY_CARD_SIZE; i++) {
        game->community_cards[i] = create_card(RANK_INVALID, SUIT_INVALID);
    }
}

int find_empty_seat(const GameState *game)
{
    if (game == NULL) {
        return -1;
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == PLAYER_EMPTY) {
            return i;
        }
    }

    return -1;
}

int add_player(GameState *game, int socket_fd, const char *name)
{
    if (game == NULL || name == NULL) {
        return -1;
    }

    int seat = find_empty_seat(game);

    if (seat < 0) {
        return -1;
    }

    Player *player = &game->players[seat];

    player->seat = seat;
    player->socket_fd = socket_fd;
    snprintf(player->name, MAX_NAME_LEN, "%s", name);
    player->points = game->config.starting_points;
    player->current_bet = 0;
    player->status = PLAYER_CONNECTED;

    player->ability.type = ABILITY_NONE;
    player->ability.used = 0;
    player->ability.owner_seat = seat;
    player->ability.target_seat = -1;
    player->ability.param = 0;

    game->player_count++;

    return seat;
}

void remove_player(GameState *game, int seat)
{
    if (game == NULL || seat < 0 || seat >= MAX_PLAYERS) {
        return;
    }

    Player *player = &game->players[seat];

    if (player->status != PLAYER_EMPTY) {
        player->socket_fd = -1;
        player->name[0] = '\0';
        player->points = game->config.starting_points;
        player->current_bet = 0;
        player->status = PLAYER_EMPTY;
        player->ability.type = ABILITY_NONE;

        if (game->player_count > 0) {
            game->player_count--;
        }
    }
}

void start_new_hand(GameState *game)
{
    if (game == NULL) {
        return;
    }

    init_deck(&game->deck);
    shuffle_deck(&game->deck);

    game->phase = PHASE_PREFLOP;
    game->pot = 0;
    game->current_bet = 0;
    game->community_count = 0;

    for (int i = 0; i < COMMUNITY_CARD_SIZE; i++) {
        game->community_cards[i] = create_card(RANK_INVALID, SUIT_INVALID);
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == PLAYER_CONNECTED ||
            game->players[i].status == PLAYER_ACTIVE) {
            game->players[i].status = PLAYER_ACTIVE;
            game->players[i].current_bet = 0;
            game->players[i].ability.used = 0;
            game->players[i].ability.owner_seat = i;
            game->players[i].ability.target_seat = -1;
        }
    }

    deal_private_cards(game);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == PLAYER_ACTIVE) {
            game->current_turn = i;
            break;
        }
    }
}

void deal_private_cards(GameState *game)
{
    if (game == NULL) {
        return;
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == PLAYER_ACTIVE) {
            game->players[i].hand[0] = deal_card(&game->deck);
            game->players[i].hand[1] = deal_card(&game->deck);

            /* Temporary ability assignment.
             * Later, you can replace this with a real ability deck.
             */
            game->players[i].ability.type = (AbilityType)((i % 5) + 1);
            game->players[i].ability.used = 0;
            game->players[i].ability.owner_seat = i;
        }
    }
}

void deal_flop(GameState *game)
{
    if (game == NULL) {
        return;
    }

    if (game->community_count == 0) {
        game->community_cards[0] = deal_card(&game->deck);
        game->community_cards[1] = deal_card(&game->deck);
        game->community_cards[2] = deal_card(&game->deck);
        game->community_count = 3;
        game->phase = PHASE_FLOP;
    }
}

void deal_turn(GameState *game)
{
    if (game == NULL) {
        return;
    }

    if (game->community_count == 3) {
        game->community_cards[3] = deal_card(&game->deck);
        game->community_count = 4;
        game->phase = PHASE_TURN;
    }
}

void deal_river(GameState *game)
{
    if (game == NULL) {
        return;
    }

    if (game->community_count == 4) {
        game->community_cards[4] = deal_card(&game->deck);
        game->community_count = 5;
        game->phase = PHASE_RIVER;
    }
}

Player *get_player_by_seat(GameState *game, int seat)
{
    if (game == NULL || seat < 0 || seat >= MAX_PLAYERS) {
        return NULL;
    }

    if (game->players[seat].status == PLAYER_EMPTY) {
        return NULL;
    }

    return &game->players[seat];
}

const char *game_phase_to_string(GamePhase phase)
{
    switch (phase) {
        case PHASE_LOBBY:
            return "LOBBY";
        case PHASE_PREFLOP:
            return "PREFLOP";
        case PHASE_FLOP:
            return "FLOP";
        case PHASE_TURN:
            return "TURN";
        case PHASE_RIVER:
            return "RIVER";
        case PHASE_SHOWDOWN:
            return "SHOWDOWN";
        case PHASE_GAME_OVER:
            return "GAME_OVER";
        default:
            return "UNKNOWN";
    }
}

const char *ability_to_string(AbilityType ability)
{
    switch (ability) {
        case ABILITY_NONE:
            return "NONE";
        case ABILITY_SNIFF:
            return "SNIFF";
        case ABILITY_ANT_TRAIL:
            return "ANT_TRAIL";
        case ABILITY_POSE:
            return "POSE";
        case ABILITY_LONG_TONGUE:
            return "LONG_TONGUE";
        case ABILITY_WILD_GRAB:
            return "WILD_GRAB";
        default:
            return "UNKNOWN";
    }
}

void build_public_game_state(const GameState *game, char *buffer, int buffer_size)
{
    if (game == NULL || buffer == NULL || buffer_size <= 0) {
        return;
    }

    snprintf(
        buffer,
        buffer_size,
        "STAT:-1:phase=%s;players=%d;pot=%d;turn=%d;community=%d\n",
        game_phase_to_string(game->phase),
        game->player_count,
        game->pot,
        game->current_turn,
        game->community_count
    );
}

void build_private_hand_message(const GameState *game, int seat, char *buffer, int buffer_size)
{
    if (game == NULL || buffer == NULL || buffer_size <= 0 ||
        seat < 0 || seat >= MAX_PLAYERS) {
        return;
    }

    const Player *player = &game->players[seat];

    char card1[64];
    char card2[64];

    card_to_string(player->hand[0], card1, sizeof(card1));
    card_to_string(player->hand[1], card2, sizeof(card2));

    snprintf(
        buffer,
        buffer_size,
        "HAND:%d:%s,%s,ability=%s\n",
        seat,
        card1,
        card2,
        ability_to_string(player->ability.type)
    );
}