#include "DirectRcLink.h"

#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

namespace {

constexpr uint8_t kBroadcastPeer[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct __attribute__((packed)) DirectRcPacket {
  uint32_t magic;
  uint32_t linkId;
  uint8_t version;
  uint8_t channelCount;
  uint8_t flags;
  uint8_t reserved;
  uint16_t sequence;
  uint32_t timeMs;
  uint16_t channels[Config::ChannelCount];
  uint16_t crc;
};

static_assert(sizeof(DirectRcPacket) == 52, "Direct RC packet layout changed");

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
                                   bool trainerEnabled,
                                   bool directEnabled,
                                   bool confirmed,
                                   uint32_t nowMs) {
  Mode requestedMode = Mode::None;
  if (channelsUs != nullptr) {
    if (directEnabled && confirmed) {
      requestedMode = Mode::Direct;
    } else if (trainerEnabled && !directEnabled) {
      requestedMode = Mode::TrainerSideband;
    }
  }

  if (requestedMode == Mode::None) {
    if (mode_ != Mode::None) {
      stopBurstUntilMs_ = nowMs + Config::DirectRcStopBurstMs;
    }
    mode_ = Mode::None;
    confirmed_ = false;
    return;
  }

  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    channels_[i] = channelsUs[i];
  }
  mode_ = requestedMode;
  confirmed_ = requestedMode == Mode::Direct;
  lastPhoneMs_ = nowMs;
}

void DirectRcLink::poll(uint32_t nowMs, Print &log) {
  (void)log;
  if (!ready_) return;

  const Mode activeMode = active(nowMs)
      ? Mode::Direct
      : (trainerActive(nowMs) ? Mode::TrainerSideband : Mode::None);
  const bool stopping = static_cast<int32_t>(nowMs - stopBurstUntilMs_) < 0;
  if (activeMode == Mode::None && !stopping) return;

  const uint32_t intervalMs = 1000UL / Config::DirectRcSendRateHz;
  if (static_cast<int32_t>(nowMs - nextSendMs_) < 0) return;
  nextSendMs_ = nowMs + intervalMs;

  if (!sendPacket(nowMs, activeMode)) {
    ++sendErrors_;
  } else {
    ++sentFrames_;
  }
}

bool DirectRcLink::sendPacket(uint32_t nowMs, Mode mode) {
  DirectRcPacket packet = {};
  packet.magic = Config::DirectRcMagic;
  packet.linkId = Config::DirectRcLinkId;
  packet.version = Config::DirectRcVersion;
  packet.channelCount = Config::ChannelCount;
  // 0x03 remains the explicitly confirmed direct takeover. 0x04 is a
  // trainer-only AUX sideband and is never allowed to replace RC sticks.
  packet.flags = mode == Mode::Direct ? 0x03 :
      (mode == Mode::TrainerSideband ? 0x04 : 0x00);
  packet.sequence = sequence_++;
  packet.timeMs = nowMs;

  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    packet.channels[i] = mode != Mode::None ? channels_[i] : Config::ChannelMidUs;
  }
  if (mode == Mode::None) packet.channels[2] = Config::ChannelMinUs;

  packet.crc = crc16Ccitt(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet) - sizeof(packet.crc));
  return esp_now_send(kBroadcastPeer, reinterpret_cast<const uint8_t *>(&packet), sizeof(packet)) == ESP_OK;
}

bool DirectRcLink::ready() const { return ready_; }

bool DirectRcLink::active(uint32_t nowMs) const {
  return mode_ == Mode::Direct && confirmed_ &&
      (nowMs - lastPhoneMs_ <= Config::PhoneFrameHoldMs);
}

bool DirectRcLink::trainerActive(uint32_t nowMs) const {
  return mode_ == Mode::TrainerSideband &&
      (nowMs - lastPhoneMs_ <= Config::PhoneFrameHoldMs);
}

bool DirectRcLink::confirmed() const { return confirmed_; }

uint32_t DirectRcLink::ageMs(uint32_t nowMs) const {
  return lastPhoneMs_ ? nowMs - lastPhoneMs_ : 0;
}

uint32_t DirectRcLink::sentFrames() const { return sentFrames_; }
uint32_t DirectRcLink::sendErrors() const { return sendErrors_; }
uint16_t DirectRcLink::sequence() const { return sequence_; }
