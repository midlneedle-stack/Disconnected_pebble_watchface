#pragma once

#include <pebble.h>

typedef struct RoundyDigitLayer RoundyDigitLayer;

RoundyDigitLayer *roundy_digit_layer_create(GRect frame);
void roundy_digit_layer_destroy(RoundyDigitLayer *layer);
Layer *roundy_digit_layer_get_layer(RoundyDigitLayer *layer);
void roundy_digit_layer_set_time(RoundyDigitLayer *layer, const struct tm *time);
void roundy_digit_layer_refresh_time(RoundyDigitLayer *layer);
void roundy_digit_layer_force_redraw(RoundyDigitLayer *layer);
/**
 * Start a short diagonal flip animation when the watchface appears.
 * The animation quickly flips the cell diagonals to the opposite angle.
 */
void roundy_digit_layer_start_diag_flip(RoundyDigitLayer *layer);
