#include "WifiCommandServer.h"

#include "Config.h"

WifiCommandServer::WifiCommandServer(CommandProcessor &commands)
    : commands_(commands), server_(Config::WifiCommandPort) {}

void WifiCommandServer::begin(Print &log) {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  const bool started = WiFi.softAP(Config::WifiSsid, Config::WifiPassword, Config::DirectRcWifiChannel, false, 4);
  if (!started) {
    log.println(F("Wi-Fi AP start failed."));
    return;
  }

  server_.begin();
  server_.setNoDelay(true);

  log.print(F("Wi-Fi AP: "));
  log.println(Config::WifiSsid);
  log.print(F("Password: "));
  log.println(Config::WifiPassword);
  log.print(F("Wi-Fi channel: "));
  log.println(Config::DirectRcWifiChannel);
  log.print(F("Command endpoint: "));
  log.print(WiFi.softAPIP());
  log.print(':');
  log.println(Config::WifiCommandPort);
}

void WifiCommandServer::poll(Print &log) {
  acceptClient(log);
  if (!client_ || !client_.connected()) return;

  const LineReadResult result = reader_.poll(client_, line_, sizeof(line_));
  if (result == LineReadResult::Complete) {
    commands_.handleLine(line_, client_);
  } else if (result == LineReadResult::Overflow) {
    client_.println(F("ERR command line too long"));
  }
}

void WifiCommandServer::acceptClient(Print &log) {
  WiFiClient next = server_.available();
  if (!next) return;

  if (client_ && client_.connected()) {
    client_.println(F("ERR another client connected; closing old session"));
    client_.stop();
  }

  client_ = next;
  client_.setNoDelay(true);
  client_.println(F("OK RadioMaster bridge ready. Send HELP."));

  log.print(F("Wi-Fi client connected from "));
  log.println(client_.remoteIP());
}
