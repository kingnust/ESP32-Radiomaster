#pragma once

#include <Arduino.h>

#include "Config.h"

enum class LineReadResult {
  None,
  Complete,
  Overflow,
};

class LineReader {
 public:
  LineReadResult poll(Stream &input, char *out, size_t outCapacity);

 private:
  char buffer_[Config::MaxCommandLine + 1] = {};
  size_t length_ = 0;
  bool discarding_ = false;
};
