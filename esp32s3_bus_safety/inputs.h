#pragma once
#include <Arduino.h>
struct DebouncedInput {
  bool rawActive;
  bool stableActive;
  uint32_t lastChangeMs;
};
