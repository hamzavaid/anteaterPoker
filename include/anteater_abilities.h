#ifndef ANTEATER_ABILITIES_H
#define ANTEATER_ABILITIES_H

#include "game_state.h"

int anteater_apply_ability(GameState *game, int seat, int target_seat,
                           int param, char *result, int result_size);

#endif
