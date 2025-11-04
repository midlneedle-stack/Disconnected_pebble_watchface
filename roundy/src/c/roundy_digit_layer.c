#include "roundy_digit_layer.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "roundy_glyphs.h"
#include "roundy_layout.h"
#include "roundy_palette.h"
#include <math.h>

typedef struct {
  int16_t digits[ROUNDY_DIGIT_COUNT];
  bool use_24h_time;
  /* animation state */
  AppTimer *anim_timer;
  float diag_progress;
} RoundyDigitLayerState;

struct RoundyDigitLayer {
  Layer *layer;
  RoundyDigitLayerState *state;
};

static inline RoundyDigitLayerState *prv_get_state(RoundyDigitLayer *layer) {
  return layer ? layer->state : NULL;
}

/* Draw a single cell. The diagonal inside the cell is interpolated between
 * the original (\) and the opposite (/) based on layer->diag_progress.
 * progress == 0.0 -> original (x = origin_x + idx)
 * progress == 1.0 -> flipped (x = origin_x + (cell_size-1-idx))
 */
static void prv_draw_digit_cell(GContext *ctx, int cell_col, int cell_row,
                               float progress) {
  const GRect frame = roundy_cell_frame(cell_col, cell_row);
  graphics_fill_rect(ctx, frame, 0, GCornerNone);

  const int origin_x = frame.origin.x;
  const int origin_y = frame.origin.y;
  const float p = progress;
  for (int idx = 0; idx < ROUNDY_CELL_SIZE; ++idx) {
    const int from_x = idx;
    const int to_x = (ROUNDY_CELL_SIZE - 1 - idx);
    const int x = origin_x + (int)roundf((1.0f - p) * from_x + p * to_x);
    const int y = origin_y + idx;
    graphics_draw_pixel(ctx, GPoint(x, y));
  }
}

static void prv_draw_glyph(GContext *ctx, const RoundyGlyph *glyph, int cell_col,
                           int cell_row, float progress) {
  if (!glyph) {
    return;
  }

  for (int row = 0; row < ROUNDY_DIGIT_HEIGHT; ++row) {
    const uint8_t mask = glyph->rows[row];
    if (!mask) {
      continue;
    }

    for (int col = 0; col < glyph->width; ++col) {
      if (mask & (1 << (glyph->width - 1 - col))) {
        prv_draw_digit_cell(ctx, cell_col + col, cell_row + row, progress);
      }
    }
  }
}

static void prv_draw_digit(GContext *ctx, int16_t digit, int cell_col, int cell_row, float progress) {
  if (digit < ROUNDY_GLYPH_ZERO || digit > ROUNDY_GLYPH_NINE) {
    return;
  }

  prv_draw_glyph(ctx, &ROUNDY_GLYPHS[digit], cell_col, cell_row, progress);
}

static void prv_draw_colon(GContext *ctx, int cell_col, int cell_row, float progress) {
  prv_draw_glyph(ctx, &ROUNDY_GLYPHS[ROUNDY_GLYPH_COLON], cell_col, cell_row, progress);
}

static void prv_digit_layer_update_proc(Layer *layer, GContext *ctx) {
  RoundyDigitLayerState *state = layer_get_data(layer);
  if (!state) {
    return;
  }

  graphics_context_set_fill_color(ctx, roundy_palette_digit_fill());
  graphics_context_set_stroke_color(ctx, roundy_palette_digit_stroke());

  int cell_col = ROUNDY_DIGIT_START_COL;
  const int cell_row = ROUNDY_DIGIT_START_ROW;

  prv_draw_digit(ctx, state->digits[0], cell_col, cell_row, state->diag_progress);
  cell_col += ROUNDY_DIGIT_WIDTH + ROUNDY_DIGIT_GAP;

  prv_draw_digit(ctx, state->digits[1], cell_col, cell_row, state->diag_progress);
  /* pass animation progress into drawing to interpolate diagonals */
  cell_col += ROUNDY_DIGIT_WIDTH + ROUNDY_DIGIT_GAP;

  prv_draw_colon(ctx, cell_col, cell_row, state->diag_progress);
  cell_col += ROUNDY_DIGIT_COLON_WIDTH + ROUNDY_DIGIT_GAP;

  prv_draw_digit(ctx, state->digits[2], cell_col, cell_row, state->diag_progress);
  cell_col += ROUNDY_DIGIT_WIDTH + ROUNDY_DIGIT_GAP;

  prv_draw_digit(ctx, state->digits[3], cell_col, cell_row, state->diag_progress);
}

RoundyDigitLayer *roundy_digit_layer_create(GRect frame) {
  RoundyDigitLayer *layer = calloc(1, sizeof(*layer));
  if (!layer) {
    return NULL;
  }

  layer->layer = layer_create_with_data(frame, sizeof(RoundyDigitLayerState));
  if (!layer->layer) {
    free(layer);
    return NULL;
  }

  layer->state = layer_get_data(layer->layer);
  layer->state->use_24h_time = clock_is_24h_style();
  for (int i = 0; i < ROUNDY_DIGIT_COUNT; ++i) {
    layer->state->digits[i] = -1;
  }

  layer_set_update_proc(layer->layer, prv_digit_layer_update_proc);
  return layer;
}

void roundy_digit_layer_destroy(RoundyDigitLayer *layer) {
  if (!layer) {
    return;
  }
  if (layer->layer) {
    /* cancel any running animation timer stored in layer data */
    RoundyDigitLayerState *state = layer_get_data(layer->layer);
    if (state && state->anim_timer) {
      app_timer_cancel(state->anim_timer);
      state->anim_timer = NULL;
    }
    layer_destroy(layer->layer);
  }
  free(layer);
}

Layer *roundy_digit_layer_get_layer(RoundyDigitLayer *layer) {
  return layer ? layer->layer : NULL;
}

/* Animation timer callback: ctx is the Layer* whose data is RoundyDigitLayerState */
static void prv_diag_anim_timer(void *ctx) {
  Layer *layer = (Layer *)ctx;
  if (!layer) {
    return;
  }
  RoundyDigitLayerState *state = layer_get_data(layer);
  if (!state) {
    return;
  }

  const int FRAME_MS = 30;
  const int DURATION_MS = 200; /* quick and smooth */
  const float delta = (float)FRAME_MS / (float)DURATION_MS;

  state->diag_progress += delta;
  if (state->diag_progress >= 1.0f) {
    state->diag_progress = 1.0f;
    /* animation finished */
    state->anim_timer = NULL;
  } else {
    /* re-register next frame */
    state->anim_timer = app_timer_register(FRAME_MS, prv_diag_anim_timer, layer);
  }

  layer_mark_dirty(layer);
}

void roundy_digit_layer_start_diag_flip(RoundyDigitLayer *rdl) {
  if (!rdl || !rdl->layer) {
    return;
  }
  RoundyDigitLayerState *state = layer_get_data(rdl->layer);
  if (!state) {
    return;
  }
  /* Cancel existing timer if any */
  if (state->anim_timer) {
    app_timer_cancel(state->anim_timer);
    state->anim_timer = NULL;
  }
  state->diag_progress = 0.0f;
  state->anim_timer = app_timer_register(30, prv_diag_anim_timer, rdl->layer);
}

void roundy_digit_layer_set_time(RoundyDigitLayer *layer, const struct tm *time_info) {
  RoundyDigitLayerState *state = prv_get_state(layer);
  if (!state || !time_info) {
    return;
  }

  const bool use_24h = clock_is_24h_style();
  int hour = time_info->tm_hour;
  if (!use_24h) {
    hour %= 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  int16_t new_digits[ROUNDY_DIGIT_COUNT] = {
    hour / 10,
    hour % 10,
    time_info->tm_min / 10,
    time_info->tm_min % 10,
  };

  if (!use_24h && hour < 10) {
    new_digits[0] = -1;
  }

  bool changed = (state->use_24h_time != use_24h);
  for (int i = 0; i < ROUNDY_DIGIT_COUNT; ++i) {
    if (state->digits[i] != new_digits[i]) {
      state->digits[i] = new_digits[i];
      changed = true;
    }
  }

  if (state->use_24h_time != use_24h) {
    state->use_24h_time = use_24h;
  }

  if (changed && layer->layer) {
    layer_mark_dirty(layer->layer);
  }
}

void roundy_digit_layer_refresh_time(RoundyDigitLayer *layer) {
  time_t now = time(NULL);
  struct tm *time_info = localtime(&now);
  if (!time_info) {
    return;
  }
  roundy_digit_layer_set_time(layer, time_info);
}

void roundy_digit_layer_force_redraw(RoundyDigitLayer *layer) {
  if (layer && layer->layer) {
    layer_mark_dirty(layer->layer);
  }
}
