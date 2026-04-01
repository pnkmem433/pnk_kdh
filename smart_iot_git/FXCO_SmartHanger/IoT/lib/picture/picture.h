#ifndef PICTURE_H
#define PICTURE_H

struct Picture {
  int width;
  int height;
  uint16_t data[];
};

#include "logo/pnksolution.h"
#include "logo/inst.h"

#endif