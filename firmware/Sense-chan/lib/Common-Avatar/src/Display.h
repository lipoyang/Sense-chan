#pragma once
#include <LovyanGFX.h>

extern lgfx::LGFX_Device* const pDisplay;

#define Display (*pDisplay)

#define SET_DISPLAY_CLASS(class_name) \
  class_name _Display;  \
  extern lgfx::LGFX_Device* const pDisplay = &_Display;
