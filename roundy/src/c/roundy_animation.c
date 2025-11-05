#include "roundy_animation.h"

#include <stdlib.h>

RoundyAnimDirection roundy_anim_random_direction(void) {
  return (RoundyAnimDirection)(rand() % ROUNDY_ANIM_DIR_COUNT);
}
