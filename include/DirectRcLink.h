#pragma once

#include <Arduino.h>

#include "Config.h"

class DirectRcLink {
 public:
  bool begin(Print &log);
  void applyPhoneFrame(const uint16_t channelsUs[Config::ChannelCount],
                       bool directEnabled,
                       bool confirmed,
                       uint32_t nowMs);
  void poll(uint32_t nowMs, Print &log);

  bool ready() const;
  bool active(uint32_t nowMs) const;
  bool confirmed() const;
  uint32_t ageMs(uint32_t nowMs) const;
  uint32_t sentFrames() const;
  uint32_t sendErrors() const;
  uint16_t sequence() const;

 private:
  bool sendPacket(uint32_t nowMs, bool enabled);

  bool ready_ = false;
  bool directEnabled_ = false;
  bool confirmed_ = false;
  uint16_t channels_[Config::ChannelCount] = {};
  uint32_t lastPhoneMs_ = 0;
  uint32_t nextSendMs_ = 0;
  uint32_t stopBurstUntilMs_ = 0;
  uint32_t sentFrames_ = 0;
  uint32_t sendErrors_ = 0;
  uint16_t sequence_ = 0;
};