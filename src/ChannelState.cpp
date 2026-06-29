#include "ChannelState.h"

ChannelState::ChannelState() { resetToSafe(); }

void ChannelState::resetToSafe() {
  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    channels_[i] = Config::ChannelMidUs;
  }

  // ELRS commonly uses AUX1/CH5 as the arm channel, so keep it low by default.
  channels_[4] = Config::ArmSafeLowUs;
}

ChannelSetResult ChannelState::setUs(uint8_t oneBasedChannel, uint16_t microseconds) {
  if (oneBasedChannel < 1 || oneBasedChannel > Config::ChannelCount) {
    return ChannelSetResult::BadChannel;
  }
  if (!canWriteChannel(oneBasedChannel)) {
    return ChannelSetResult::PrimaryLocked;
  }
  return setTransportUs(oneBasedChannel, microseconds);
}

ChannelSetResult ChannelState::setTransportUs(uint8_t oneBasedChannel, uint16_t microseconds) {
  if (oneBasedChannel < 1 || oneBasedChannel > Config::ChannelCount) {
    return ChannelSetResult::BadChannel;
  }
  if (microseconds < Config::ChannelMinUs || microseconds > Config::ChannelMaxUs) {
    return ChannelSetResult::BadValue;
  }

  channels_[oneBasedChannel - 1] = microseconds;
  return ChannelSetResult::Ok;
}

uint16_t ChannelState::getUs(uint8_t zeroBasedChannel) const {
  if (zeroBasedChannel >= Config::ChannelCount) return Config::ChannelMidUs;
  return channels_[zeroBasedChannel];
}

const uint16_t *ChannelState::values() const { return channels_; }

void ChannelState::setPrimaryWritesAllowed(bool allowed) { primaryWritesAllowed_ = allowed; }

bool ChannelState::primaryWritesAllowed() const { return primaryWritesAllowed_; }

bool ChannelState::canWriteChannel(uint8_t oneBasedChannel) const {
  return primaryWritesAllowed_ || oneBasedChannel >= Config::FirstHostWritableChannel;
}
