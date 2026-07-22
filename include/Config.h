#pragma once

#include <Arduino.h>

namespace Config {

static constexpr const char *FirmwareVersion = "phone-rc-2026-07-22-sbus-espnow-trainer-v4";

// Wi-Fi AP is used instead of Bluetooth because TCP over Wi-Fi is easier to
// script from a laptop and gives predictable line-oriented delivery.
static constexpr const char *WifiSsid = "RM-Pocket-Bridge";
static constexpr const char *WifiPassword = "12345678";
static constexpr uint16_t WifiHttpPort = 80;
static constexpr uint16_t WifiCommandPort = 7777;
static constexpr uint16_t WifiWebSocketPort = 7778;

static constexpr uint8_t DirectRcWifiChannel = 6;
static constexpr uint16_t DirectRcSendRateHz = 100;
static constexpr uint32_t DirectRcStopBurstMs = 140;
static constexpr uint32_t DirectRcDeliveryFreshMs = 500;
// Paired FC station MAC (ESP32-S3 ec:da:3b:4e:39:14). Unicast ESP-NOW gets
// link-layer acknowledgement and retries; broadcast does not.
static constexpr uint8_t DirectRcPeerMac[6] = {0xec, 0xda, 0x3b, 0x4e, 0x39, 0x14};
static constexpr uint32_t DirectRcMagic = 0x31524344UL;  // "DRC1" little-endian
// Must match ESPFC_DRONE_PROTO_DIRECT_RC_LINK_ID in the FC receiver.
static constexpr uint32_t DirectRcLinkId = 0x6D5A31C7UL;
static constexpr uint8_t DirectRcVersion = 2;

static constexpr uint32_t UsbSerialBaud = 115200;

// ESP32-S3 SuperMini defaults. These use commonly exposed pins on SuperMini
// boards that only break out GPIO1-GPIO13. Keep all signals at 3.3 V logic.
static constexpr int RadioRxPin = 4;
static constexpr int RadioTxPin = 5;
static constexpr int RadioUartNumber = 1;

static constexpr uint32_t CrsfBaud = 420000;
static constexpr uint32_t SbusBaud = 100000;
static constexpr bool SbusInverted = false;

static constexpr uint8_t DefaultCrsfAddress = 0xEE;
static constexpr uint16_t DefaultOutputRateHz = 100;
static constexpr uint16_t MinOutputRateHz = 10;
static constexpr uint16_t MaxOutputRateHz = 100;

static constexpr uint16_t PhoneSendRateHz = 50;
static constexpr uint32_t PhoneStatusIntervalMs = 200;
static constexpr uint32_t PhoneFrameHoldMs = 300;
static constexpr uint8_t TrainerTakeoverChannel = 13;
static constexpr uint8_t TrainerHeartbeatChannel = 16;
static constexpr uint16_t TrainerHeartbeatBaseUs = 1250;
static constexpr uint16_t TrainerHeartbeatDeltaUs = 20;

static constexpr uint32_t HostTimeoutMs = 1000;
static constexpr uint32_t SafeBurstMs = 300;

static constexpr size_t ChannelCount = 16;
static constexpr uint16_t ChannelMinUs = 988;
static constexpr uint16_t ChannelMidUs = 1500;
static constexpr uint16_t ChannelMaxUs = 2012;

// Default lockout protects existing radio controls. The host-facing channel
// numbers are logical receiver outputs: CH11-CH26 map onto trainer inputs
// TR1-TR16 in the 16-channel SBUS/CRSF frame.
static constexpr uint8_t FirstHostWritableChannel = 11;
static constexpr uint8_t LastHostWritableChannel = 26;

static constexpr size_t MaxCommandLine = 192;

}  // namespace Config
