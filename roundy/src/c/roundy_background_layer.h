#pragma once

#include <pebble.h>

#include "roundy_animation.h"

typedef struct RoundyBackgroundLayer RoundyBackgroundLayer;

RoundyBackgroundLayer *roundy_background_layer_create(GRect frame);
void roundy_background_layer_destroy(RoundyBackgroundLayer *layer);
Layer *roundy_background_layer_get_layer(RoundyBackgroundLayer *layer);
void roundy_background_layer_mark_dirty(RoundyBackgroundLayer *layer);
void roundy_background_layer_start_diag_flip(RoundyBackgroundLayer *layer,
                                             RoundyAnimDirection direction);
