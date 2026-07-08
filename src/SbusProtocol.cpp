#include "SbusProtocol.h"

#include <string.h>

namespace {

constexpr uint8_t kStartByte = 0x0F;
constexpr uint8_t kEndByte = 0x00;
constexpr uint16_t kSbusMin = 172;
constexpr uint16_t kSbusMid = 992;
// EdgeTX SBUS trainer display reaches +100 at 1812. 1811 can show as +99.
constexpr uint16_t kSbusMax = 1812;

void packChannels(const uint16_t channelsUs[Config::ChannelCount], uint8_t payload[22]) {
  memset(payload, 0, 22);

  uint16_t bitIndex = 0;
  for (size_t channel = 0; channel < Config::ChannelCount; ++channel) {
    const uint16_t value = SbusProtocol::microsecondsToSbus(channelsUs[channel]);
    for (uint8_t bit = 0; bit < 11; ++bit) {
      if (value & (1U << bit)) {
        payload[bitIndex >> 3] |= static_cast<uint8_t>(1U << (bitIndex & 0x07));
      }
      ++bitIndex;
    }
  }
}

}  // namespace

namespace SbusProtocol {

uint16_t microsecondsToSbus(uint16_t microseconds) {
  if (microseconds <= Config::ChannelMinUs) return kSbusMin;
  if (microseconds >= Config::ChannelMaxUs) return kSbusMax;

  const int32_t centeredUs = static_cast<int32_t>(microseconds) - Config::ChannelMidUs;
  const int32_t sbus = kSbusMid + ((centeredUs * 8) / 5);
  if (sbus < kSbusMin) return kSbusMin;
  if (sbus > kSbusMax) return kSbusMax;
  return static_cast<uint16_t>(sbus);
}

bool buildChannelsFrame(const uint16_t channelsUs[Config::ChannelCount],
                        uint8_t *out,
                        size_t outCapacity,
                        size_t &written) {
  written = 0;
  if (out == nullptr || outCapacity < FrameSize) return false;

  out[0] = kStartByte;
  packChannels(channelsUs, &out[1]);
  out[23] = 0x00;  // flags: no frame lost, no failsafe
  out[24] = kEndByte;
  written = FrameSize;
  return true;
}

}  // namespace SbusProtocol
