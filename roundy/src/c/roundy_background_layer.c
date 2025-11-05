#include "roundy_background_layer.h"

#include <stdlib.h>

#include "roundy_layout.h"
#include "roundy_palette.h"

struct RoundyBackgroundLayer {
  Layer *layer;
};

static void prv_draw_background_cell(GContext *ctx, int cell_col, int cell_row) {
  const GPoint origin = roundy_cell_origin(cell_col, cell_row);

  /* leave top/bottom rows empty for the trimmed diagonal look */
  const int start_idx = 1;
  const int end_idx = ROUNDY_CELL_SIZE - 2;
  for (int idx = start_idx; idx <= end_idx; ++idx) {
    graphics_draw_pixel(ctx, GPoint(origin.x + idx, origin.y + idx));
  }
}

static void prv_background_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, roundy_palette_background_fill());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, roundy_palette_background_stroke());
  for (int row = 0; row < ROUNDY_GRID_ROWS; ++row) {
    for (int col = 0; col < ROUNDY_GRID_COLS; ++col) {
      prv_draw_background_cell(ctx, col, row);
    }
  }
}

RoundyBackgroundLayer *roundy_background_layer_create(GRect frame) {
  RoundyBackgroundLayer *layer = calloc(1, sizeof(*layer));
  if (!layer) {
    return NULL;
  }

  layer->layer = layer_create(frame);
  if (!layer->layer) {
    free(layer);
    return NULL;
  }

  layer_set_update_proc(layer->layer, prv_background_update_proc);
  return layer;
}

void roundy_background_layer_destroy(RoundyBackgroundLayer *layer) {
  if (!layer) {
    return;
  }

  if (layer->layer) {
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
