#pragma once

#include <Arduino.h>

#include "Config.h"

namespace CrsfProtocol {

static constexpr size_t RcFrameSize = 26;

bool buildRcChannelsFrame(const uint16_t channelsUs[Config::ChannelCount],
                          uint8_t frameAddress,
                          uint8_t *out,
                          size_t outCapacity,
                          size_t &written);

uint16_t microsecondsToCrsf(uint16_t microseconds);

}  // namespace CrsfProtocol
