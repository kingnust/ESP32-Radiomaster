#pragma once

#include <Arduino.h>

namespace Config {

static constexpr const char *FirmwareVersion = "sbus-link-2026-06-26";

// Wi-Fi AP is used instead of Bluetooth because TCP over Wi-Fi is easier to
// script from a laptop and gives predictable line-oriented delivery.
static constexpr const char *WifiSsid = "RM-Pocket-Bridge";
static constexpr const char *WifiPassword = "12345678";
static constexpr uint16_t WifiCommandPort = 7777;

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

static constexpr uint32_t HostTimeoutMs = 1000;
static constexpr uint32_t SafeBurstMs = 300;

static constexpr size_t ChannelCount = 16;
static constexpr uint16_t ChannelMinUs = 988;
static constexpr uint16_t ChannelMidUs = 1500;
static constexpr uint16_t ChannelMaxUs = 2012;
static constexpr uint16_t ArmSafeLowUs = 1000;

// Default lockout protects existing radio controls. Commands can only write
// channels 11-16 unless UNLOCK_PRIMARY I_UNDERSTAND is sent.
static constexpr uint8_t FirstHostWritableChannel = 11;

static constexpr size_t MaxCommandLine = 192;

}  // namespace Config
