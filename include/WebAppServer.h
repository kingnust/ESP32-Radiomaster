#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "ChannelState.h"
#include "CommandProcessor.h"
#include "DirectRcLink.h"

class WebAppServer {
 public:
  WebAppServer(CommandProcessor &commands, ChannelState &channels, DirectRcLink &directRc);

  void begin(Print &log);
  void poll(Print &log);

 private:
  CommandProcessor &commands_;
  ChannelState &channels_;
  DirectRcLink &directRc_;
  WiFiServer httpServer_;
  WiFiServer wsServer_;
  WiFiClient wsClient_;
  bool wsReady_ = false;
  uint32_t lastStatusMs_ = 0;
  uint32_t phoneFrames_ = 0;
  uint32_t phoneErrors_ = 0;

  void pollHttp(Print &log);
  void pollWebSocket(Print &log);
  void acceptWebSocket(Print &log);
  void closeWebSocket();

  bool readHttpRequest(WiFiClient &client, char *request, size_t capacity, uint32_t timeoutMs);
  void serveIndex(WiFiClient &client);
  void serveManifest(WiFiClient &client);
  void serveNotFound(WiFiClient &client);

  bool handleWebSocketHandshake(WiFiClient &client, const char *request, Print &log);
  bool readWebSocketText(char *message, size_t capacity, uint8_t &opcode);
  bool sendWebSocketText(const char *message);
  bool sendWebSocketControl(uint8_t opcode, const uint8_t *payload, size_t length);

  void handleWebSocketMessage(const char *message, Print &log);
  bool parsePhoneFrame(const char *message,
                       uint16_t channelsUs[Config::ChannelCount],
                       bool &trainerEnabled,
                       bool &directEnabled,
                       bool &directConfirmed);
  void sendStatus();
};
