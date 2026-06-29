#pragma once

#include <Arduino.h>

enum class RadioProtocol {
  Crsf,
  Sbus,
};

const char *radioProtocolName(RadioProtocol protocol);
