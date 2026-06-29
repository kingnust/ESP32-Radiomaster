#include "LineReader.h"

#include <string.h>

LineReadResult LineReader::poll(Stream &input, char *out, size_t outCapacity) {
  while (input.available() > 0) {
    const int raw = input.read();
    if (raw < 0) break;

    const char c = static_cast<char>(raw);
    if (c == '\r') continue;

    if (c == '\n') {
      if (discarding_) {
        discarding_ = false;
        length_ = 0;
        return LineReadResult::Overflow;
      }
      if (length_ == 0) continue;

      buffer_[length_] = '\0';
      if (out != nullptr && outCapacity > 0) {
        strncpy(out, buffer_, outCapacity);
        out[outCapacity - 1] = '\0';
      }
      length_ = 0;
      return LineReadResult::Complete;
    }

    if (discarding_) continue;

    if (length_ >= Config::MaxCommandLine) {
      discarding_ = true;
      length_ = 0;
      continue;
    }

    if (c >= 32 && c <= 126) {
      buffer_[length_++] = c;
    }
  }

  return LineReadResult::None;
}
