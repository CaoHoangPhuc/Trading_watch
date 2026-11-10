#pragma once

#include <Arduino.h>

#include "Types.h"

class BybitRestClient {
 public:
  bool fetchBtcOptionTickers(OptionContract* contracts, size_t maxContracts, size_t& outCount,
                             float& outUnderlyingPrice, String& err) const;

 private:
  bool fetchBtcUnderlyingPrice(float& outUnderlyingPrice) const;
  static bool parseOptionSymbol(const String& symbol, OptionContract& outContract);
  static int monthFromToken(const String& mon);
};
