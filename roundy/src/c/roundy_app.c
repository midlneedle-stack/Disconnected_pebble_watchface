#include <pebble.h>

#include "roundy_background_layer.h"
#include "roundy_digit_layer.h"
#include "roundy_palette.h"

static Window *s_main_window;
static RoundyBackgroundLayer *s_background_layer;
static RoundyDigitLayer *s_digit_layer;

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  (void)units_changed;
  roundy_digit_layer_set_time(s_digit_layer, tick_time);
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(root);

  s_background_layer = roundy_background_layer_create(bounds);
  if (s_background_layer) {
    layer_add_child(root, roundy_background_layer_get_layer(s_background_layer));
  }

  s_digit_layer = roundy_digit_layer_create(bounds);
  if (s_digit_layer) {
    layer_add_child(root, roundy_digit_layer_get_layer(s_digit_layer));
    roundy_digit_layer_refresh_time(s_digit_layer);
    /* start a quick diagonal flip animation when the watchface appears */
    roundy_digit_layer_start_diag_flip(s_digit_layer);
  }
}

static void prv_window_unload(Window *window) {
  (void)window;

  roundy_digit_layer_destroy(s_digit_layer);
  s_digit_layer = NULL;

  roundy_background_layer_destroy(s_background_layer);
  s_background_layer = NULL;
}

static void prv_init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, roundy_palette_window_background());
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                            .load = prv_window_load,
                                            .unload = prv_window_unload,
                                          });

  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
  s_main_window = NULL;
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
