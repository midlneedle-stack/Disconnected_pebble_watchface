#pragma once

#include <pebble.h>

typedef enum {
  ROUNDY_ANIM_DIR_TOP_DOWN = 0,
  ROUNDY_ANIM_DIR_LEFT_RIGHT,
  ROUNDY_ANIM_DIR_BOTTOM_UP,
  ROUNDY_ANIM_DIR_RIGHT_LEFT,
  ROUNDY_ANIM_DIR_COUNT,
} RoundyAnimDirection;

enum {
  /* Delay before the very first row/column flip kicks off (in ms). */
  ROUNDY_DIAG_ANIM_INITIAL_DELAY_MS = 120,
  /* Delay between successive steps while the sweep is running (in ms). */
  ROUNDY_DIAG_ANIM_STEP_DELAY_MS = 60,
  /* How long the active background band stays highlighted (in ms). */
  ROUNDY_DIAG_ANIM_RETURN_DELAY_MS = 60,
};

RoundyAnimDirection roundy_anim_random_direction(void);

static inline GColor roundy_anim_dim_stroke(void) {
  return PBL_IF_COLOR_ELSE(GColorFromRGB(0x55, 0x55, 0x55), GColorBlack);
}

static inline GColor roundy_anim_bright_stroke(void) {
  return PBL_IF_COLOR_ELSE(GColorFromRGB(0xFF, 0xFF, 0xFF), GColorWhite);
}
