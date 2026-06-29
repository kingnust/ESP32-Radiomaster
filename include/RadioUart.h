#pragma once

#include <Arduino.h>

#include "ChannelState.h"
#include "Config.h"
#include "RadioProtocol.h"

class RadioUart {
 public:
  explicit RadioUart(int uartNumber);

  void begin(RadioProtocol protocol);
  void setProtocol(RadioProtocol protocol);
  void setSbusInverted(bool inverted);
  RadioProtocol protocol() const;

  bool sendChannels(const ChannelState &channels, uint8_t crsfAddress);
  void drainIncomingTo(Print &debugOut);

 private:
  HardwareSerial serial_;
  RadioProtocol protocol_ = RadioProtocol::Sbus;
  bool sbusInverted_ = Config::SbusInverted;
};
