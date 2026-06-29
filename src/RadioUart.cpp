#include "RadioUart.h"

#include "Config.h"
#include "CrsfProtocol.h"
#include "SbusProtocol.h"

RadioUart::RadioUart(int uartNumber) : serial_(uartNumber) {}

void RadioUart::begin(RadioProtocol protocol) {
  protocol_ = protocol;
  if (protocol_ == RadioProtocol::Sbus) {
    serial_.begin(Config::SbusBaud, SERIAL_8E2, Config::RadioRxPin, Config::RadioTxPin,
                  sbusInverted_);
  } else {
    serial_.begin(Config::CrsfBaud, SERIAL_8N1, Config::RadioRxPin, Config::RadioTxPin, false);
  }
}

void RadioUart::setProtocol(RadioProtocol protocol) {
  if (protocol == protocol_) return;
  serial_.end();
  delay(20);
  begin(protocol);
}

void RadioUart::setSbusInverted(bool inverted) {
  if (sbusInverted_ == inverted) return;
  sbusInverted_ = inverted;
  if (protocol_ == RadioProtocol::Sbus) {
    serial_.end();
    delay(20);
    begin(protocol_);
  }
}

RadioProtocol RadioUart::protocol() const { return protocol_; }

bool RadioUart::sendChannels(const ChannelState &channels, uint8_t crsfAddress) {
  uint8_t frame[32];
  size_t written = 0;

  const bool ok = (protocol_ == RadioProtocol::Sbus)
                      ? SbusProtocol::buildChannelsFrame(channels.values(), frame, sizeof(frame), written)
                      : CrsfProtocol::buildRcChannelsFrame(channels.values(), crsfAddress, frame,
                                                           sizeof(frame), written);
  if (!ok) return false;

  return serial_.write(frame, written) == written;
}

void RadioUart::drainIncomingTo(Print &debugOut) {
  static uint32_t lastPrefixMs = 0;

  while (serial_.available() > 0) {
    const int value = serial_.read();
    if (value < 0) return;

    const uint32_t now = millis();
    if (now - lastPrefixMs > 250) {
      debugOut.println();
      debugOut.print(F("RADIO RX:"));
      lastPrefixMs = now;
    }
    debugOut.print(' ');
    if (value < 0x10) debugOut.print('0');
    debugOut.print(value, HEX);
  }
}
