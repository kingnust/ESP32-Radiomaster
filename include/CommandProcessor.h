#pragma once

#include <Arduino.h>

#include "ChannelState.h"
#include "Config.h"
#include "RadioProtocol.h"

class CommandProcessor {
 public:
  explicit CommandProcessor(ChannelState &channels);

  void handleLine(const char *line, Print &out);
  void serviceSafety(uint32_t nowMs, Print &log);

  bool shouldSendFrame(uint32_t nowMs) const;
  bool shouldSendSafeFrame(uint32_t nowMs) const;
  uint16_t outputRateHz() const;
  uint8_t crsfAddress() const;
  bool sbusInverted() const;
  RadioProtocol protocol() const;
  bool protocolChanged();
  bool sbusInversionChanged();
  uint32_t testRemainingMs(uint32_t nowMs) const;

 private:
  ChannelState &channels_;
  bool outputEnabled_ = false;
  bool testActive_ = false;
  bool linkEnabled_ = false;
  uint32_t lastHostMs_ = 0;
  uint32_t safeBurstUntilMs_ = 0;
  uint32_t testUntilMs_ = 0;
  uint16_t outputRateHz_ = Config::DefaultOutputRateHz;
  uint8_t crsfAddress_ = Config::DefaultCrsfAddress;
  bool sbusInverted_ = Config::SbusInverted;
  RadioProtocol protocol_ = RadioProtocol::Sbus;
  bool protocolChanged_ = false;
  bool sbusInversionChanged_ = false;

  void stopWithSafeBurst(uint32_t nowMs);
  void noteHostActivity();
  bool linkFresh(uint32_t nowMs) const;
  bool timeReached(uint32_t nowMs, uint32_t targetMs) const;
  ChannelSetResult setHostChannelUs(uint8_t hostChannel, uint16_t microseconds);
  bool canWriteHostChannel(uint8_t hostChannel) const;
  uint8_t hostChannelToTrainerChannel(uint8_t hostChannel) const;
  void printHelp(Print &out) const;
  void printStatus(Print &out) const;
};
