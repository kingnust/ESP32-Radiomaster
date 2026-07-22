#include "CommandProcessor.h"

#include <algorithm>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace {

char *trim(char *text) {
  while (*text && isspace(static_cast<unsigned char>(*text))) ++text;
  if (*text == '\0') return text;

  char *end = text + strlen(text) - 1;
  while (end > text && isspace(static_cast<unsigned char>(*end))) {
    *end = '\0';
    --end;
  }
  return text;
}

void upperInPlace(char *text) {
  while (*text) {
    *text = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
    ++text;
  }
}

bool parseUint(const char *text, uint32_t &value, uint8_t base = 10) {
  if (text == nullptr || *text == '\0') return false;
  char *end = nullptr;
  const unsigned long parsed = strtoul(text, &end, base);
  if (end == text || *end != '\0') return false;
  value = static_cast<uint32_t>(parsed);
  return true;
}

bool parseOnOff(const char *text, bool &enabled) {
  if (text == nullptr) return false;
  char temp[12];
  strncpy(temp, text, sizeof(temp));
  temp[sizeof(temp) - 1] = '\0';
  upperInPlace(temp);

  if (strcmp(temp, "1") == 0 || strcmp(temp, "ON") == 0 || strcmp(temp, "YES") == 0) {
    enabled = true;
    return true;
  }
  if (strcmp(temp, "0") == 0 || strcmp(temp, "OFF") == 0 || strcmp(temp, "NO") == 0) {
    enabled = false;
    return true;
  }
  return false;
}

const char *channelSetResultText(ChannelSetResult result) {
  switch (result) {
    case ChannelSetResult::Ok:
      return "OK";
    case ChannelSetResult::BadChannel:
      return "ERR bad channel";
    case ChannelSetResult::PrimaryLocked:
      return "ERR primary channels locked";
    case ChannelSetResult::BadValue:
      return "ERR value outside 988..2012";
    default:
      return "ERR channel";
  }
}

}  // namespace

CommandProcessor::CommandProcessor(ChannelState &channels) : channels_(channels) {}

void CommandProcessor::handleLine(const char *line, Print &out) {
  char editable[Config::MaxCommandLine + 1];
  strncpy(editable, line, sizeof(editable));
  editable[sizeof(editable) - 1] = '\0';

  char *cursor = trim(editable);
  if (*cursor == '\0') return;

  char *save = nullptr;
  char *command = strtok_r(cursor, " \t", &save);
  if (command == nullptr) return;
  upperInPlace(command);

  if (strcmp(command, "HELP") == 0) {
    printHelp(out);
    return;
  }

  if (strcmp(command, "PING") == 0 || strcmp(command, "KEEPALIVE") == 0) {
    noteHostActivity();
    out.println(F("PONG"));
    return;
  }

  if (strcmp(command, "STATUS") == 0) {
    printStatus(out);
    return;
  }

  if (strcmp(command, "ARM") == 0 || strcmp(command, "ENABLE") == 0) {
    bool enabled = false;
    if (!parseOnOff(strtok_r(nullptr, " \t", &save), enabled)) {
      out.println(F("ERR usage: ARM 1|0"));
      return;
    }

    if (enabled) {
      outputEnabled_ = true;
      testActive_ = false;
      safeBurstUntilMs_ = 0;
      noteHostActivity();
      out.println(F("OK output enabled"));
    } else {
      stopWithSafeBurst(millis());
      out.println(F("OK output disabled, safe frame burst active"));
    }
    return;
  }

  if (strcmp(command, "LINK") == 0) {
    bool enabled = false;
    if (!parseOnOff(strtok_r(nullptr, " \t", &save), enabled)) {
      out.println(F("ERR usage: LINK 1|0"));
      return;
    }

    linkEnabled_ = enabled;
    if (enabled) {
      safeBurstUntilMs_ = 0;
      out.println(F("OK trainer link enabled with safe frames"));
    } else {
      stopWithSafeBurst(millis());
      out.println(F("OK trainer link disabled"));
    }
    return;
  }

  if (strcmp(command, "STOP") == 0) {
    stopWithSafeBurst(millis());
    out.println(F("OK stopped"));
    return;
  }

  if (strcmp(command, "NEUTRAL") == 0 || strcmp(command, "SAFE") == 0) {
    channels_.resetToSafe();
    noteHostActivity();
    out.println(F("OK safe channel values loaded"));
    return;
  }

  if (strcmp(command, "CH") == 0) {
    uint32_t channel = 0;
    uint32_t value = 0;
    if (!parseUint(strtok_r(nullptr, " \t", &save), channel) ||
        !parseUint(strtok_r(nullptr, " \t", &save), value)) {
      out.println(F("ERR usage: CH <11..26> <988..2012>"));
      return;
    }

    const ChannelSetResult result = setHostChannelUs(channel, value);
    if (result == ChannelSetResult::Ok) {
      noteHostActivity();
      out.println(F("OK channel updated"));
    } else {
      out.println(channelSetResultText(result));
    }
    return;
  }

  if (strcmp(command, "TEST") == 0) {
    uint32_t channel = 0;
    uint32_t value = 0;
    uint32_t durationMs = 0;
    if (!parseUint(strtok_r(nullptr, " \t", &save), channel) ||
        !parseUint(strtok_r(nullptr, " \t", &save), value) ||
        !parseUint(strtok_r(nullptr, " \t", &save), durationMs) ||
        durationMs < 100 ||
        durationMs > 10000) {
      out.println(F("ERR usage: TEST <11..26> <988..2012> <100..10000 ms>"));
      return;
    }

    const ChannelSetResult result = setHostChannelUs(channel, value);
    if (result != ChannelSetResult::Ok) {
      out.println(channelSetResultText(result));
      return;
    }

    outputEnabled_ = true;
    testActive_ = true;
    safeBurstUntilMs_ = 0;
    testUntilMs_ = millis() + durationMs;
    noteHostActivity();
    out.println(F("OK test output active"));
    return;
  }

  if (strcmp(command, "TESTRAW") == 0) {
    uint32_t trainerChannel = 0;
    uint32_t value = 0;
    uint32_t durationMs = 0;
    if (!parseUint(strtok_r(nullptr, " \t", &save), trainerChannel) ||
        !parseUint(strtok_r(nullptr, " \t", &save), value) ||
        !parseUint(strtok_r(nullptr, " \t", &save), durationMs) ||
        trainerChannel < 1 ||
        trainerChannel > Config::ChannelCount ||
        durationMs < 100 ||
        durationMs > 10000) {
      out.println(F("ERR usage: TESTRAW <TR 1..16> <988..2012> <100..10000 ms>"));
      return;
    }

    const ChannelSetResult result =
        value < Config::ChannelMinUs || value > Config::ChannelMaxUs
            ? ChannelSetResult::BadValue
            : channels_.setTransportUs(static_cast<uint8_t>(trainerChannel),
                                       static_cast<uint16_t>(value));
    if (result != ChannelSetResult::Ok) {
      out.println(channelSetResultText(result));
      return;
    }

    outputEnabled_ = true;
    testActive_ = true;
    safeBurstUntilMs_ = 0;
    testUntilMs_ = millis() + durationMs;
    noteHostActivity();
    out.println(F("OK raw trainer test output active"));
    return;
  }

  if (strcmp(command, "FRAME") == 0) {
    uint32_t pending[Config::ChannelCount];
    for (size_t i = 0; i < Config::ChannelCount; ++i) pending[i] = channels_.getUs(i);

    for (size_t i = 0; i < Config::ChannelCount; ++i) {
      uint32_t value = 0;
      if (!parseUint(strtok_r(nullptr, " ,\t", &save), value)) {
        out.println(F("ERR usage: FRAME <16 values for CH11..CH26>"));
        return;
      }
      if (value < Config::ChannelMinUs || value > Config::ChannelMaxUs) {
        out.print(F("ERR frame rejected at CH"));
        out.print(static_cast<uint8_t>(Config::FirstHostWritableChannel + i));
        out.print(F(": "));
        out.println(channelSetResultText(ChannelSetResult::BadValue));
        return;
      }
      pending[i] = static_cast<uint16_t>(value);
    }

    for (size_t i = 0; i < Config::ChannelCount; ++i) {
      const uint8_t channel = static_cast<uint8_t>(Config::FirstHostWritableChannel + i);

      const ChannelSetResult result = setHostChannelUs(channel, pending[i]);
      if (result != ChannelSetResult::Ok) {
        out.print(F("ERR frame rejected at CH"));
        out.print(channel);
        out.print(F(": "));
        out.println(channelSetResultText(result));
        return;
      }
    }

    noteHostActivity();
    out.println(F("OK frame updated"));
    return;
  }

  if (strcmp(command, "RATE") == 0) {
    uint32_t rate = 0;
    if (!parseUint(strtok_r(nullptr, " \t", &save), rate) ||
        rate < Config::MinOutputRateHz ||
        rate > Config::MaxOutputRateHz) {
      out.println(F("ERR usage: RATE <10..100>"));
      return;
    }
    outputRateHz_ = static_cast<uint16_t>(rate);
    out.println(F("OK rate updated"));
    return;
  }

  if (strcmp(command, "SBUSINV") == 0) {
    bool inverted = false;
    if (!parseOnOff(strtok_r(nullptr, " \t", &save), inverted)) {
      out.println(F("ERR usage: SBUSINV 1|0"));
      return;
    }
    sbusInverted_ = inverted;
    sbusInversionChanged_ = true;
    stopWithSafeBurst(millis());
    out.print(F("OK SBUS inverted="));
    out.println(sbusInverted_ ? 1 : 0);
    return;
  }

  if (strcmp(command, "UNLOCK_PRIMARY") == 0) {
    char *token = strtok_r(nullptr, " \t", &save);
    if (token == nullptr || strcmp(token, "I_UNDERSTAND") != 0) {
      out.println(F("ERR usage: UNLOCK_PRIMARY I_UNDERSTAND"));
      return;
    }
    channels_.setPrimaryWritesAllowed(true);
    out.println(F("OK primary channel writes unlocked"));
    return;
  }

  if (strcmp(command, "LOCK_PRIMARY") == 0) {
    channels_.setPrimaryWritesAllowed(false);
    channels_.resetToSafe();
    stopWithSafeBurst(millis());
    out.println(F("OK primary channel writes locked"));
    return;
  }

  out.println(F("ERR unknown command; send HELP"));
}

void CommandProcessor::serviceSafety(uint32_t nowMs, Print &log) {
  if (phoneActive_) {
    if (timeReached(nowMs, phoneHoldUntilMs_)) {
      stopWithSafeBurst(nowMs);
      log.println(F("Phone hold timeout: output returned to safe values."));
    }
    return;
  }

  if (testActive_) {
    if (timeReached(nowMs, testUntilMs_)) {
      stopWithSafeBurst(nowMs);
      log.println(F("Test complete: output returned to safe values."));
    }
    return;
  }

  if (outputEnabled_ && !linkFresh(nowMs)) {
    stopWithSafeBurst(nowMs);
    log.println(F("Safety timeout: host link stale, output disabled."));
  }
}

void CommandProcessor::applyPhoneFrame(const uint16_t channelsUs[Config::ChannelCount],
                                       bool sendEnabled,
                                       bool deadmanHeld,
                                       uint32_t nowMs) {
  if (channelsUs == nullptr || !sendEnabled || !deadmanHeld) {
    if (outputEnabled_ || testActive_) {
      stopWithSafeBurst(nowMs);
    } else {
      channels_.resetToSafe();
    }
    return;
  }

  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    uint16_t value = channelsUs[i];
    if (i + 1 == Config::TrainerHeartbeatChannel) continue;
    if (i + 1 == Config::TrainerTakeoverChannel) value = Config::ChannelMaxUs;
    channels_.setTransportUs(static_cast<uint8_t>(i + 1), value);
  }

  outputEnabled_ = true;
  phoneActive_ = true;
  testActive_ = false;
  linkEnabled_ = false;
  safeBurstUntilMs_ = 0;
  lastHostMs_ = nowMs;
  phoneHoldUntilMs_ = nowMs + Config::PhoneFrameHoldMs;
}

void CommandProcessor::stopPhoneControl(uint32_t nowMs) {
  if (phoneActive_ || outputEnabled_ || testActive_) {
    stopWithSafeBurst(nowMs);
  }
}

void CommandProcessor::prepareTrainerFrame(ChannelState &frameChannels) {
  const int32_t heartbeat = static_cast<int32_t>(Config::TrainerHeartbeatBaseUs) +
      (trainerHeartbeatHigh_ ? Config::TrainerHeartbeatDeltaUs
                             : -static_cast<int32_t>(Config::TrainerHeartbeatDeltaUs));
  frameChannels.setTransportUs(Config::TrainerHeartbeatChannel,
                               static_cast<uint16_t>(heartbeat));
  trainerHeartbeatHigh_ = !trainerHeartbeatHigh_;
}

bool CommandProcessor::shouldSendFrame(uint32_t nowMs) const {
  (void)nowMs;
  return true;
}

bool CommandProcessor::shouldSendSafeFrame(uint32_t nowMs) const {
  if (phoneActive_) return timeReached(nowMs, phoneHoldUntilMs_);
  const bool live = (outputEnabled_ && (linkFresh(nowMs) ||
                                        (testActive_ && !timeReached(nowMs, testUntilMs_)))) ||
                    linkEnabled_;
  return !live;
}

bool CommandProcessor::outputEnabled() const { return outputEnabled_; }

bool CommandProcessor::linkEnabled() const { return linkEnabled_; }

bool CommandProcessor::testActive() const { return testActive_; }

uint32_t CommandProcessor::lastHostMs() const { return lastHostMs_; }

uint16_t CommandProcessor::outputRateHz() const { return outputRateHz_; }

uint8_t CommandProcessor::crsfAddress() const { return crsfAddress_; }

bool CommandProcessor::sbusInverted() const { return sbusInverted_; }

RadioProtocol CommandProcessor::protocol() const { return protocol_; }

bool CommandProcessor::protocolChanged() {
  const bool changed = protocolChanged_;
  protocolChanged_ = false;
  return changed;
}

bool CommandProcessor::sbusInversionChanged() {
  const bool changed = sbusInversionChanged_;
  sbusInversionChanged_ = false;
  return changed;
}

uint32_t CommandProcessor::testRemainingMs(uint32_t nowMs) const {
  if (!testActive_ || timeReached(nowMs, testUntilMs_)) return 0;
  return testUntilMs_ - nowMs;
}

void CommandProcessor::stopWithSafeBurst(uint32_t nowMs) {
  outputEnabled_ = false;
  phoneActive_ = false;
  testActive_ = false;
  testUntilMs_ = 0;
  phoneHoldUntilMs_ = 0;
  channels_.resetToSafe();
  safeBurstUntilMs_ = nowMs + Config::SafeBurstMs;
}

void CommandProcessor::noteHostActivity() { lastHostMs_ = millis(); }

bool CommandProcessor::linkFresh(uint32_t nowMs) const {
  return nowMs - lastHostMs_ <= Config::HostTimeoutMs;
}

bool CommandProcessor::timeReached(uint32_t nowMs, uint32_t targetMs) const {
  return static_cast<int32_t>(nowMs - targetMs) >= 0;
}

ChannelSetResult CommandProcessor::setHostChannelUs(uint32_t hostChannel, uint32_t microseconds) {
  if (hostChannel < 1 || hostChannel > Config::LastHostWritableChannel) {
    return ChannelSetResult::BadChannel;
  }
  if (microseconds < Config::ChannelMinUs || microseconds > Config::ChannelMaxUs) {
    return ChannelSetResult::BadValue;
  }
  if (!canWriteHostChannel(hostChannel)) {
    return ChannelSetResult::PrimaryLocked;
  }
  const uint8_t trainerChannel = hostChannelToTrainerChannel(hostChannel);
  if (trainerChannel < 1 || trainerChannel > Config::ChannelCount) {
    return ChannelSetResult::BadChannel;
  }
  return channels_.setTransportUs(trainerChannel, static_cast<uint16_t>(microseconds));
}

bool CommandProcessor::canWriteHostChannel(uint32_t hostChannel) const {
  if (hostChannel >= Config::FirstHostWritableChannel &&
      hostChannel <= Config::LastHostWritableChannel) {
    return true;
  }
  return channels_.primaryWritesAllowed() && hostChannel < Config::FirstHostWritableChannel;
}

uint8_t CommandProcessor::hostChannelToTrainerChannel(uint32_t hostChannel) const {
  if (hostChannel < Config::FirstHostWritableChannel) {
    return static_cast<uint8_t>(hostChannel);
  }
  return static_cast<uint8_t>(hostChannel - Config::FirstHostWritableChannel + 1);
}

void CommandProcessor::printHelp(Print &out) const {
  out.println(F("Commands:"));
  out.println(F("  PING"));
  out.println(F("  STATUS"));
  out.println(F("  ARM 1|0"));
  out.println(F("  LINK 1|0"));
  out.println(F("  STOP"));
  out.println(F("  SAFE"));
  out.println(F("  CH <11..26> <988..2012>"));
  out.println(F("  TEST <11..26> <988..2012> <100..10000 ms>"));
  out.println(F("  TESTRAW <TR 1..16> <988..2012> <100..10000 ms>"));
  out.println(F("  FRAME <16 values for CH11..CH26>"));
  out.println(F("  RATE <10..100>"));
  out.println(F("  SBUSINV 1|0"));
  out.println(F("  UNLOCK_PRIMARY I_UNDERSTAND"));
  out.println(F("  LOCK_PRIMARY"));
}

void CommandProcessor::printStatus(Print &out) const {
  const uint32_t now = millis();
  out.print(F("OK fw="));
  out.print(Config::FirmwareVersion);
  out.print(F(" proto="));
  out.print(radioProtocolName(protocol_));
  out.print(F(" enabled="));
  out.print(outputEnabled_ ? 1 : 0);
  out.print(F(" link="));
  out.print(linkEnabled_ ? 1 : 0);
  out.print(F(" test_active="));
  out.print(testActive_ ? 1 : 0);
  out.print(F(" test_remaining_ms="));
  out.print(testRemainingMs(now));
  out.print(F(" fresh="));
  out.print(linkFresh(now) ? 1 : 0);
  out.print(F(" rate="));
  out.print(outputRateHz_);
  out.print(F(" sbus_inv="));
  out.print(sbusInverted_ ? 1 : 0);
  out.print(F(" primary_unlocked="));
  out.print(channels_.primaryWritesAllowed() ? 1 : 0);
  out.print(F(" host_map=CH"));
  out.print(Config::FirstHostWritableChannel);
  out.print(F("-CH"));
  out.print(Config::LastHostWritableChannel);
  out.print(F("->TR1-TR"));
  out.print(static_cast<uint8_t>(Config::ChannelCount));
  out.print(F(" trainer="));
  for (size_t i = 0; i < Config::ChannelCount; ++i) {
    if (i) out.print(',');
    out.print(channels_.getUs(i));
  }
  out.println();
}
