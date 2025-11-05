#include "roundy_background_layer.h"

#include <stdlib.h>

#include "roundy_animation.h"
#include "roundy_layout.h"
#include "roundy_palette.h"

typedef struct {
  AppTimer *progress_timer;
  AppTimer *return_timer;
  RoundyAnimDirection direction;
  int16_t next_index;
  int16_t active_index;
  int16_t max_index;
  bool active_index_flipped;
} RoundyBackgroundLayerState;

struct RoundyBackgroundLayer {
  Layer *layer;
  RoundyBackgroundLayerState *state;
};

static inline RoundyBackgroundLayerState *prv_get_state(RoundyBackgroundLayer *layer) {
  return layer ? layer->state : NULL;
}

static inline bool prv_direction_is_vertical(RoundyAnimDirection direction) {
  return (direction == ROUNDY_ANIM_DIR_TOP_DOWN || direction == ROUNDY_ANIM_DIR_BOTTOM_UP);
}

static inline int16_t prv_direction_max_index(RoundyAnimDirection direction) {
  return prv_direction_is_vertical(direction) ? (ROUNDY_GRID_ROWS - 1) : (ROUNDY_GRID_COLS - 1);
}

static inline int16_t prv_direction_index_for_cell(RoundyAnimDirection direction, int col, int row) {
  switch (direction) {
    case ROUNDY_ANIM_DIR_TOP_DOWN:
      return row;
    case ROUNDY_ANIM_DIR_BOTTOM_UP:
      return (ROUNDY_GRID_ROWS - 1 - row);
    case ROUNDY_ANIM_DIR_LEFT_RIGHT:
      return col;
    case ROUNDY_ANIM_DIR_RIGHT_LEFT:
      return (ROUNDY_GRID_COLS - 1 - col);
    default:
      return row;
  }
}

static void prv_draw_background_cell(GContext *ctx, int cell_col, int cell_row, bool flipped) {
  const GPoint origin = roundy_cell_origin(cell_col, cell_row);

  /* leave top/bottom rows empty for the trimmed diagonal look */
  const int start_idx = 1;
  const int end_idx = ROUNDY_CELL_SIZE - 2;
  for (int idx = start_idx; idx <= end_idx; ++idx) {
    const int x_idx = flipped ? (ROUNDY_CELL_SIZE - 1 - idx) : idx;
    graphics_draw_pixel(ctx, GPoint(origin.x + x_idx, origin.y + idx));
  }
}

static void prv_background_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  RoundyBackgroundLayerState *state = layer_get_data(layer);

  graphics_context_set_fill_color(ctx, roundy_palette_background_fill());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  for (int row = 0; row < ROUNDY_GRID_ROWS; ++row) {
    for (int col = 0; col < ROUNDY_GRID_COLS; ++col) {
      bool flipped = false;
      if (state && state->active_index_flipped && state->active_index >= 0) {
        const int16_t cell_index = prv_direction_index_for_cell(state->direction, col, row);
        flipped = (cell_index == state->active_index);
      }
      graphics_context_set_stroke_color(ctx,
                                        flipped ? roundy_anim_bright_stroke()
                                                : roundy_anim_dim_stroke());
      prv_draw_background_cell(ctx, col, row, flipped);
    }
  }
}

static void prv_background_return_timer(void *ctx) {
  Layer *layer = (Layer *)ctx;
  if (!layer) {
    return;
  }

  RoundyBackgroundLayerState *state = layer_get_data(layer);
  if (!state) {
    return;
  }

  state->active_index_flipped = false;
  state->active_index = -1;
  state->return_timer = NULL;

  layer_mark_dirty(layer);
}

static void prv_background_progress_timer(void *ctx) {
  Layer *layer = (Layer *)ctx;
  if (!layer) {
    return;
  }

  RoundyBackgroundLayerState *state = layer_get_data(layer);
  if (!state) {
    return;
  }

  if (state->next_index > state->max_index) {
    state->progress_timer = NULL;
    if (!state->return_timer) {
      state->active_index_flipped = false;
      state->active_index = -1;
      layer_mark_dirty(layer);
    }
    return;
  }

  state->active_index = state->next_index;
  state->active_index_flipped = true;
  state->next_index++;

  if (state->return_timer) {
    app_timer_cancel(state->return_timer);
  }
  state->return_timer =
      app_timer_register(ROUNDY_DIAG_ANIM_RETURN_DELAY_MS, prv_background_return_timer, layer);

  if (state->next_index <= state->max_index) {
    state->progress_timer =
        app_timer_register(ROUNDY_DIAG_ANIM_RETURN_DELAY_MS, prv_background_progress_timer, layer);
  } else {
    state->progress_timer = NULL;
  }

  layer_mark_dirty(layer);
}

RoundyBackgroundLayer *roundy_background_layer_create(GRect frame) {
  RoundyBackgroundLayer *layer = calloc(1, sizeof(*layer));
  if (!layer) {
    return NULL;
  }

  layer->layer = layer_create_with_data(frame, sizeof(RoundyBackgroundLayerState));
  if (!layer->layer) {
    free(layer);
    return NULL;
  }
  layer->state = layer_get_data(layer->layer);
  layer->state->progress_timer = NULL;
  layer->state->return_timer = NULL;
  layer->state->direction = ROUNDY_ANIM_DIR_TOP_DOWN;
  layer->state->max_index = prv_direction_max_index(layer->state->direction);
  layer->state->next_index = 0;
  layer->state->active_index = -1;
  layer->state->active_index_flipped = false;

  layer_set_update_proc(layer->layer, prv_background_update_proc);
  return layer;
}

void roundy_background_layer_destroy(RoundyBackgroundLayer *layer) {
  if (!layer) {
    return;
  }

  if (layer->layer) {
    RoundyBackgroundLayerState *state = layer_get_data(layer->layer);
    if (state) {
      if (state->progress_timer) {
        app_timer_cancel(state->progress_timer);
        state->progress_timer = NULL;
      }
      if (state->return_timer) {
        app_timer_cancel(state->return_timer);
        state->return_timer = NULL;
      }
    }
    layer_destroy(layer->layer);
  }
  free(layer);
}

Layer *roundy_background_layer_get_layer(RoundyBackgroundLayer *layer) {
  return layer ? layer->layer : NULL;
}

void roundy_background_layer_mark_dirty(RoundyBackgroundLayer *layer) {
  if (layer && layer->layer) {
    layer_mark_dirty(layer->layer);
  }
}

void roundy_background_layer_start_diag_flip(RoundyBackgroundLayer *layer,
                                             RoundyAnimDirection direction) {
  if (!layer || !layer->layer) {
    return;
  }

  RoundyBackgroundLayerState *state = prv_get_state(layer);
  if (!state) {
    return;
  }

  if (state->progress_timer) {
    app_timer_cancel(state->progress_timer);
    state->progress_timer = NULL;
  }
  if (state->return_timer) {
    app_timer_cancel(state->return_timer);
    state->return_timer = NULL;
  }

  state->direction = direction;
  state->max_index = prv_direction_max_index(direction);
  state->next_index = 0;
  state->active_index = -1;
  state->active_index_flipped = false;

  layer_mark_dirty(layer->layer);
  state->progress_timer = app_timer_register(ROUNDY_DIAG_ANIM_INITIAL_DELAY_MS,
                                             prv_background_progress_timer, layer->layer);
}
