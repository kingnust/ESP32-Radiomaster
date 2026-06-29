#include "CrsfProtocol.h"

#include <string.h>

namespace {

constexpr uint8_t kFrameTypeRcChannelsPacked = 0x16;
constexpr uint8_t kRcPayloadSize = 22;
constexpr uint8_t kRcFrameLengthField = 24;  // type + payload + crc
constexpr uint16_t kCrsfMin = 172;
constexpr uint16_t kCrsfMid = 992;
constexpr uint16_t kCrsfMax = 1811;

uint8_t crc8DvbS2(const uint8_t *data, size_t length) {
  uint8_t crc = 0;
  while (length--) {
    crc ^= *data++;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0xD5)
                         : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

void packChannels(const uint16_t channelsUs[Config::ChannelCount], uint8_t payload[kRcPayloadSize]) {
  memset(payload, 0, kRcPayloadSize);

  uint16_t bitIndex = 0;
  for (size_t channel = 0; channel < Config::ChannelCount; ++channel) {
    const uint16_t value = CrsfProtocol::microsecondsToCrsf(channelsUs[channel]);
    for (uint8_t bit = 0; bit < 11; ++bit) {
      if (value & (1U << bit)) {
        payload[bitIndex >> 3] |= static_cast<uint8_t>(1U << (bitIndex & 0x07));
      }
      ++bitIndex;
    }
  }
}

}  // namespace

namespace CrsfProtocol {

uint16_t microsecondsToCrsf(uint16_t microseconds) {
  if (microseconds <= Config::ChannelMinUs) return kCrsfMin;
  if (microseconds >= Config::ChannelMaxUs) return kCrsfMax;

  const int32_t centeredUs = static_cast<int32_t>(microseconds) - Config::ChannelMidUs;
  const int32_t crsf = kCrsfMid + ((centeredUs * 8) / 5);
  if (crsf < kCrsfMin) return kCrsfMin;
  if (crsf > kCrsfMax) return kCrsfMax;
  return static_cast<uint16_t>(crsf);
}

bool buildRcChannelsFrame(const uint16_t channelsUs[Config::ChannelCount],
                          uint8_t frameAddress,
                          uint8_t *out,
                          size_t outCapacity,
                          size_t &written) {
  written = 0;
  if (out == nullptr || outCapacity < RcFrameSize) return false;

  out[0] = frameAddress;
  out[1] = kRcFrameLengthField;
  out[2] = kFrameTypeRcChannelsPacked;
  packChannels(channelsUs, &out[3]);
  out[RcFrameSize - 1] = crc8DvbS2(&out[2], 1 + kRcPayloadSize);
  written = RcFrameSize;
  return true;
}

}  // namespace CrsfProtocol
