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
  void applyPhoneFrame(const uint16_t channelsUs[Config::ChannelCount],
                       bool sendEnabled,
                       bool deadmanHeld,
                       uint32_t nowMs);

  void stopPhoneControl(uint32_t nowMs);
  void prepareTrainerFrame(ChannelState &frameChannels);
  bool shouldSendFrame(uint32_t nowMs) const;
  bool shouldSendSafeFrame(uint32_t nowMs) const;
  bool outputEnabled() const;
  bool linkEnabled() const;
  bool testActive() const;
  bool linkFresh(uint32_t nowMs) const;
  uint32_t lastHostMs() const;
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
  bool phoneActive_ = false;
  bool linkEnabled_ = false;
  uint32_t lastHostMs_ = 0;
  uint32_t phoneHoldUntilMs_ = 0;
  uint32_t safeBurstUntilMs_ = 0;
  uint32_t testUntilMs_ = 0;
  uint16_t outputRateHz_ = Config::DefaultOutputRateHz;
  uint8_t crsfAddress_ = Config::DefaultCrsfAddress;
  bool sbusInverted_ = Config::SbusInverted;
  RadioProtocol protocol_ = RadioProtocol::Sbus;
  bool protocolChanged_ = false;
  bool sbusInversionChanged_ = false;
  bool trainerHeartbeatHigh_ = false;

  void stopWithSafeBurst(uint32_t nowMs);
  void noteHostActivity();
  bool timeReached(uint32_t nowMs, uint32_t targetMs) const;
  ChannelSetResult setHostChannelUs(uint32_t hostChannel, uint32_t microseconds);
  bool canWriteHostChannel(uint32_t hostChannel) const;
  uint8_t hostChannelToTrainerChannel(uint32_t hostChannel) const;
  void printHelp(Print &out) const;
  void printStatus(Print &out) const;
};
