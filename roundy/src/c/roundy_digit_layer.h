#pragma once

#include <pebble.h>

#include "roundy_animation.h"

typedef struct RoundyDigitLayer RoundyDigitLayer;

RoundyDigitLayer *roundy_digit_layer_create(GRect frame);
void roundy_digit_layer_destroy(RoundyDigitLayer *layer);
Layer *roundy_digit_layer_get_layer(RoundyDigitLayer *layer);
void roundy_digit_layer_set_time(RoundyDigitLayer *layer, const struct tm *time);
void roundy_digit_layer_refresh_time(RoundyDigitLayer *layer);
void roundy_digit_layer_force_redraw(RoundyDigitLayer *layer);
void roundy_digit_layer_start_diag_flip(RoundyDigitLayer *layer, RoundyAnimDirection direction);
