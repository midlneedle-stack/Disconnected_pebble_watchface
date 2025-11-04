#include <pebble.h>

// Grid configuration: 24 columns x 28 rows, each cell 6x6 pixels.
#define GRID_COLS 24
#define GRID_ROWS 28
#define CELL_SIZE 6
#define DIGIT_WIDTH 4
#define DIGIT_HEIGHT 9
#define GLYPH_COLON 10
#define GLYPH_COUNT 11
#define DIGIT_START_COL 1
#define DIGIT_START_ROW 10
#define DIGIT_GAP 1
#define DIGIT_COLON_WIDTH 2

static Window *s_main_window;
static Layer *s_canvas_layer;
static int s_time_digits[4] = { -1, -1, -1, -1 };

// clang-format off
// Digits encode per-pixel colors: '1' -> #000000, '0' -> #555555.
static const char *s_background_cell_pattern[CELL_SIZE] = {
  "100000",
  "010000",
  "001000",
  "000100",
  "000010",
  "000001",
};

// Digit cell pattern: '1' -> #FFFFFF, '0' -> #555555.
static const char *s_digit_cell_pattern[CELL_SIZE] = {
  "000001",
  "000010",
  "000100",
  "001000",
  "010000",
  "100000",
};

// Digit glyphs (0-9) and colon: digits occupy 4 columns, colon occupies 2 columns, strings go top to bottom.
static const char *s_glyph_layouts[11][DIGIT_HEIGHT] = {
  { // 0
    "1111",
    "1001",
    "1001",
    "1001",
    "1001",
    "1001",
    "1001",
    "1001",
    "1111",
  },
  { // 1
    "0010",
    "0010",
    "1110",
    "0010",
    "0010",
    "0010",
    "0010",
    "0010",
    "1111",
  },
  { // 2
    "1111",
    "0001",
    "0001",
    "0001",
    "1111",
    "1000",
    "1000",
    "1000",
    "1111",
  },
  { // 3
    "1111",
    "0001",
    "0001",
    "0001",
    "1111",
    "0001",
    "0001",
    "0001",
    "1111",
  },
  { // 4
    "1001",
    "1001",
    "1001",
    "1001",
    "1111",
    "0001",
    "0001",
    "0001",
    "0001",
  },
  { // 5
    "1111",
    "1000",
    "1000",
    "1000",
    "1111",
    "0001",
    "0001",
    "0001",
    "1111",
  },
  { // 6
    "1111",
    "1000",
    "1000",
    "1000",
    "1111",
    "1001",
    "1001",
    "1001",
    "1111",
  },
  { // 7
    "1111",
    "0001",
    "0001",
    "0001",
    "0001",
    "0001",
    "0001",
    "0001",
    "0001",
  },
  { // 8
    "1111",
    "1001",
    "1001",
    "1001",
    "1111",
    "1001",
    "1001",
    "1001",
    "1111",
  },
  { // 9
    "1111",
    "1001",
    "1001",
    "1001",
    "1111",
    "0001",
    "0001",
    "0001",
    "1111",
  },
  // colon
  {
    "00",
    "00",
    "11",
    "11",
    "00",
    "11",
    "11",
    "00",
    "00",
  },
};
// clang-format on

static bool prv_background_color_for_symbol(char symbol, GColor *color_out) {
  switch (symbol) {
    case '1':
      *color_out = GColorBlack;
      return true;
    case '0':
      *color_out = PBL_IF_COLOR_ELSE(GColorFromRGB(0x55, 0x55, 0x55), GColorWhite);
      return true;
    default:
      return false;
  }
}

static bool prv_digit_color_for_symbol(char symbol, GColor *color_out) {
  switch (symbol) {
    case '1':
      *color_out = GColorWhite;
      return true;
    case '0':
      *color_out = PBL_IF_COLOR_ELSE(GColorFromRGB(0x55, 0x55, 0x55), GColorWhite);
      return true;
    default:
      return false;
  }
}

typedef bool (*ColorResolver)(char symbol, GColor *color_out);

static void prv_draw_cell(GContext *ctx, int col, int row, const char *const *pattern, ColorResolver resolver) {
  const int origin_x = col * CELL_SIZE;
  const int origin_y = row * CELL_SIZE;

  for (int pixel_row = 0; pixel_row < CELL_SIZE; ++pixel_row) {
    const char *pattern_row = pattern[pixel_row];
    if (!pattern_row) {
      continue;
    }

    for (int pixel_col = 0; pixel_col < CELL_SIZE; ++pixel_col) {
      char symbol = pattern_row[pixel_col];
      if (symbol == '\0') {
        break;
      }

      if (symbol == ' ' || symbol == '.') {
        continue;
      }

      GColor color;
      if (!resolver(symbol, &color)) {
        continue;
      }

      graphics_context_set_stroke_color(ctx, color);
      graphics_draw_pixel(ctx, GPoint(origin_x + pixel_col, origin_y + pixel_row));
    }
  }
}

static void prv_draw_background_cell(GContext *ctx, int col, int row) {
  prv_draw_cell(ctx, col, row, s_background_cell_pattern, prv_background_color_for_symbol);
}

static void prv_draw_digit_cell(GContext *ctx, int col, int row) {
  prv_draw_cell(ctx, col, row, s_digit_cell_pattern, prv_digit_color_for_symbol);
}

static void prv_draw_glyph(GContext *ctx, int glyph_index, int glyph_width, int cell_col, int cell_row) {
  if (glyph_index < 0 || glyph_index >= GLYPH_COUNT) {
    return;
  }

  for (int row = 0; row < DIGIT_HEIGHT; ++row) {
    const char *row_pattern = s_glyph_layouts[glyph_index][row];
    if (!row_pattern) {
      continue;
    }

    for (int col = 0; col < glyph_width; ++col) {
      char symbol = row_pattern[col];
      if (symbol == '\0') {
        break;
      }

      if (symbol != '1') {
        continue;
      }

      prv_draw_digit_cell(ctx, cell_col + col, cell_row + row);
    }
  }
}

static void prv_draw_digit(GContext *ctx, int digit, int cell_col, int cell_row) {
  if (digit < 0 || digit >= GLYPH_COLON) {
    return;
  }

  prv_draw_glyph(ctx, digit, DIGIT_WIDTH, cell_col, cell_row);
}

static void prv_draw_colon(GContext *ctx, int cell_col, int cell_row) {
  prv_draw_glyph(ctx, GLYPH_COLON, DIGIT_COLON_WIDTH, cell_col, cell_row);
}

static void prv_draw_time(GContext *ctx) {
  const int base_row = DIGIT_START_ROW;
  int col_cursor = DIGIT_START_COL;

  prv_draw_digit(ctx, s_time_digits[0], col_cursor, base_row);
  col_cursor += DIGIT_WIDTH + DIGIT_GAP;

  prv_draw_digit(ctx, s_time_digits[1], col_cursor, base_row);
  col_cursor += DIGIT_WIDTH + DIGIT_GAP;

  prv_draw_colon(ctx, col_cursor, base_row);
  col_cursor += DIGIT_COLON_WIDTH + DIGIT_GAP;

  prv_draw_digit(ctx, s_time_digits[2], col_cursor, base_row);
  col_cursor += DIGIT_WIDTH + DIGIT_GAP;

  prv_draw_digit(ctx, s_time_digits[3], col_cursor, base_row);
}

static void prv_update_time(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (!tick_time) {
    return;
  }

  int hour = tick_time->tm_hour;
  if (!clock_is_24h_style()) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  s_time_digits[0] = hour / 10;
  if (!clock_is_24h_style() && hour < 10) {
    s_time_digits[0] = -1;
  }
  s_time_digits[1] = hour % 10;
  s_time_digits[2] = tick_time->tm_min / 10;
  s_time_digits[3] = tick_time->tm_min % 10;
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  (void)tick_time;
  (void)units_changed;

  prv_update_time();

  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void prv_canvas_update_proc(Layer *layer, GContext *ctx) {
  (void)layer;
  for (int row = 0; row < GRID_ROWS; ++row) {
    for (int col = 0; col < GRID_COLS; ++col) {
      prv_draw_background_cell(ctx, col, row);
    }
  }

  prv_draw_time(ctx);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, prv_canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  prv_update_time();
  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_unload(Window *window) {
  (void)window;
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

static void prv_init(void) {
  s_main_window = window_create();
#ifdef PBL_COLOR
  window_set_background_color(s_main_window, GColorBlack);
#else
  window_set_background_color(s_main_window, GColorWhite);
#endif
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  window_stack_push(s_main_window, true);

  prv_update_time();
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
