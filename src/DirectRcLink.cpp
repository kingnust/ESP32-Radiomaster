#include "DirectRcLink.h"

#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

namespace {

constexpr uint8_t kBroadcastPeer[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct __attribute__((packed)) DirectRcPacket {
  uint32_t magic;
  uint8_t version;
  uint8_t channelCount;
  uint8_t flags;
  uint8_t reserved;
  uint16_t sequence;
  uint32_t timeMs;
  uint16_t channels[Config::ChannelCount];
  uint16_t crc;
};

uint16_t crc16Ccitt(const uint8_t *data, size_t len) {
  uint16_t crc = 0xffff;
  while (len--) {
    crc ^= static_cast<uint16_t>(*data++) << 8;
    for (uint8_t i = 0; i < 8; ++i) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

}  // namespace

bool DirectRcLink::begin(Print &log) {
  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    channels_[i] = Config::ChannelMidUs;
  }
  channels_[2] = Config::ChannelMinUs;

  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(Config::DirectRcWifiChannel, WIFI_SECOND_CHAN_NONE);

  const esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK) {
    log.print(F("Direct RC ESP-NOW init failed: "));
    log.println(static_cast<int>(initResult));
    ready_ = false;
    return false;
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcastPeer, sizeof(kBroadcastPeer));
  peer.channel = Config::DirectRcWifiChannel;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = false;

  if (!esp_now_is_peer_exist(kBroadcastPeer)) {
    const esp_err_t peerResult = esp_now_add_peer(&peer);
    if (peerResult != ESP_OK) {
      log.print(F("Direct RC broadcast peer failed: "));
      log.println(static_cast<int>(peerResult));
      ready_ = false;
      return false;
    }
  }

  ready_ = true;
  log.print(F("Direct RC ESP-NOW TX ready on Wi-Fi channel "));
  log.println(Config::DirectRcWifiChannel);
  return true;
}

void DirectRcLink::applyPhoneFrame(const uint16_t channelsUs[Config::ChannelCount],
                                   bool directEnabled,
                                   bool confirmed,
                                   uint32_t nowMs) {
  if (!channelsUs || !directEnabled || !confirmed) {
    if (directEnabled_) {
      stopBurstUntilMs_ = nowMs + Config::DirectRcStopBurstMs;
    }
    directEnabled_ = false;
    confirmed_ = false;
    return;
  }

  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    channels_[i] = channelsUs[i];
  }
  directEnabled_ = true;
  confirmed_ = true;
  lastPhoneMs_ = nowMs;
}

void DirectRcLink::poll(uint32_t nowMs, Print &log) {
  (void)log;
  if (!ready_) return;

  const bool activeNow = active(nowMs);
  const bool stopping = static_cast<int32_t>(nowMs - stopBurstUntilMs_) < 0;
  if (!activeNow && !stopping) return;

  const uint32_t intervalMs = 1000UL / Config::DirectRcSendRateHz;
  if (static_cast<int32_t>(nowMs - nextSendMs_) < 0) return;
  nextSendMs_ = nowMs + intervalMs;

  if (!sendPacket(nowMs, activeNow)) {
    ++sendErrors_;
  } else {
    ++sentFrames_;
  }
}

bool DirectRcLink::sendPacket(uint32_t nowMs, bool enabled) {
  DirectRcPacket packet = {};
  packet.magic = Config::DirectRcMagic;
  packet.version = Config::DirectRcVersion;
  packet.channelCount = Config::ChannelCount;
  packet.flags = enabled ? 0x03 : 0x00;  // bit0=enabled, bit1=confirmed
  packet.sequence = sequence_++;
  packet.timeMs = nowMs;

  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    packet.channels[i] = enabled ? channels_[i] : Config::ChannelMidUs;
  }
  if (!enabled) packet.channels[2] = Config::ChannelMinUs;

  packet.crc = crc16Ccitt(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet) - sizeof(packet.crc));
  return esp_now_send(kBroadcastPeer, reinterpret_cast<const uint8_t *>(&packet), sizeof(packet)) == ESP_OK;
}

bool DirectRcLink::ready() const { return ready_; }

bool DirectRcLink::active(uint32_t nowMs) const {
  return directEnabled_ && confirmed_ && (nowMs - lastPhoneMs_ <= Config::PhoneFrameHoldMs);
}

bool DirectRcLink::confirmed() const { return confirmed_; }

uint32_t DirectRcLink::ageMs(uint32_t nowMs) const {
  return lastPhoneMs_ ? nowMs - lastPhoneMs_ : 0;
}

uint32_t DirectRcLink::sentFrames() const { return sentFrames_; }
uint32_t DirectRcLink::sendErrors() const { return sendErrors_; }
uint16_t DirectRcLink::sequence() const { return sequence_; }