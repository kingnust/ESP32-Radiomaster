#include "RadioProtocol.h"

const char *radioProtocolName(RadioProtocol protocol) {
  switch (protocol) {
    case RadioProtocol::Crsf:
      return "CRSF";
    case RadioProtocol::Sbus:
      return "SBUS";
    default:
      return "UNKNOWN";
  }
}
