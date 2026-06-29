#pragma once

#include <Arduino.h>

#include "Config.h"

namespace SbusProtocol {

static constexpr size_t FrameSize = 25;

bool buildChannelsFrame(const uint16_t channelsUs[Config::ChannelCount],
                        uint8_t *out,
                        size_t outCapacity,
                        size_t &written);

uint16_t microsecondsToSbus(uint16_t microseconds);

}  // namespace SbusProtocol
