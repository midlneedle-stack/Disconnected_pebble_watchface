#include <pebble.h>

static Window *s_main_window;
static Layer *s_grid_layer;
static TextLayer *s_time_layer;

static void update_time(void) {
  if (!s_time_layer) {
    return;
  }

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  static char s_buffer[] = "00:00";
  strftime(s_buffer, sizeof(s_buffer), "%H:%M", tick_time);

  text_layer_set_text(s_time_layer, s_buffer);
}

static void grid_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  const GColor grid_color = GColorFromHEX(0xAAAAAA);
  graphics_context_set_fill_color(ctx, grid_color);

  const int cell_size = 4;
  const int dot_size = 2;

  for (int y = 0; y < bounds.size.h; y += cell_size) {
    for (int x = 0; x < bounds.size.w; x += cell_size) {
      const GRect dot_rect = GRect(x, y, dot_size, dot_size);
      graphics_fill_rect(ctx, dot_rect, 0, GCornerNone);
    }
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorWhite);

  s_grid_layer = layer_create(bounds);
  layer_set_update_proc(s_grid_layer, grid_update_proc);
  layer_add_child(window_layer, s_grid_layer);

  const int text_height = 50;
  GRect time_bounds = GRect(0, (bounds.size.h - text_height) / 2, bounds.size.w, text_height);

  s_time_layer = text_layer_create(time_bounds);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  update_time();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  layer_destroy(s_grid_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init(void) {
  s_main_window = window_create();

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });

  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
