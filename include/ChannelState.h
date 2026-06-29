#pragma once

#include <Arduino.h>

#include "Config.h"

enum class ChannelSetResult {
  Ok,
  BadChannel,
  PrimaryLocked,
  BadValue,
};

class ChannelState {
 public:
  ChannelState();

  void resetToSafe();
  ChannelSetResult setUs(uint8_t oneBasedChannel, uint16_t microseconds);
  ChannelSetResult setTransportUs(uint8_t oneBasedChannel, uint16_t microseconds);

  uint16_t getUs(uint8_t zeroBasedChannel) const;
  const uint16_t *values() const;

  void setPrimaryWritesAllowed(bool allowed);
  bool primaryWritesAllowed() const;
  bool canWriteChannel(uint8_t oneBasedChannel) const;

 private:
  uint16_t channels_[Config::ChannelCount];
  bool primaryWritesAllowed_ = false;
};
