#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "CommandProcessor.h"
#include "LineReader.h"

class WifiCommandServer {
 public:
  explicit WifiCommandServer(CommandProcessor &commands);

  void begin(Print &log);
  void poll(Print &log);

 private:
  CommandProcessor &commands_;
  WiFiServer server_;
  WiFiClient client_;
  LineReader reader_;
  char line_[Config::MaxCommandLine + 1] = {};

  void acceptClient(Print &log);
};
