#include <Arduino.h>

#include "ChannelState.h"
#include "CommandProcessor.h"
#include "Config.h"
#include "LineReader.h"
#include "RadioUart.h"
#include "WebAppServer.h"
#include "WifiCommandServer.h"

ChannelState channels;
ChannelState safeChannels;
CommandProcessor commands(channels);
RadioUart radio(Config::RadioUartNumber);
WifiCommandServer wifiServer(commands);
WebAppServer webApp(commands, channels);
LineReader usbReader;

char usbLine[Config::MaxCommandLine + 1] = {};
uint32_t nextRadioFrameMs = 0;

void handleUsbCommands() {
  const LineReadResult result = usbReader.poll(Serial, usbLine, sizeof(usbLine));
  if (result == LineReadResult::Complete) {
    commands.handleLine(usbLine, Serial);
  } else if (result == LineReadResult::Overflow) {
    Serial.println(F("ERR command line too long"));
  }
}

void serviceRadioOutput(uint32_t nowMs) {
  commands.serviceSafety(nowMs, Serial);

  if (commands.protocolChanged()) {
    radio.setProtocol(commands.protocol());
    nextRadioFrameMs = 0;
    Serial.print(F("Radio UART changed to "));
    Serial.println(radioProtocolName(commands.protocol()));
  }

  if (commands.sbusInversionChanged()) {
    radio.setSbusInverted(commands.sbusInverted());
    nextRadioFrameMs = 0;
    Serial.print(F("SBUS inverted set to "));
    Serial.println(commands.sbusInverted() ? 1 : 0);
  }

  const uint32_t frameIntervalMs = 1000UL / commands.outputRateHz();
  if (nowMs < nextRadioFrameMs) return;
  nextRadioFrameMs = nowMs + frameIntervalMs;

  if (commands.shouldSendFrame(nowMs)) {
    const ChannelState &frameChannels = commands.shouldSendSafeFrame(nowMs) ? safeChannels : channels;
    if (!radio.sendChannels(frameChannels, commands.crsfAddress())) {
      Serial.println(F("ERR failed to write radio frame"));
    }
  }
}

void setup() {
  Serial.begin(Config::UsbSerialBaud);
  delay(1500);

  Serial.println();
  Serial.println(F("ESP32-S3 RadioMaster command bridge"));
  Serial.print(F("Firmware: "));
  Serial.println(Config::FirmwareVersion);
  Serial.println(F("Output is disabled on boot. Send HELP, then ARM 1 when ready."));

  safeChannels.resetToSafe();
  radio.begin(commands.protocol());
  wifiServer.begin(Serial);
  webApp.begin(Serial);
}

void loop() {
  const uint32_t nowMs = millis();

  wifiServer.poll(Serial);
  webApp.poll(Serial);
  handleUsbCommands();
  serviceRadioOutput(nowMs);

  // Leave this disabled unless you are actively debugging radio telemetry.
  // radio.drainIncomingTo(Serial);

  delay(1);
}
