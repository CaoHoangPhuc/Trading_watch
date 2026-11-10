#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>

#include "Types.h"

class BybitWsClient {
 public:
  BybitWsClient();

  void begin();
  void loop();

  void setSymbol(const String& symbol);
  bool isConnected() const;
  const String& selectedSymbol() const;

  bool consumeTicker(TickerState& outTicker);
  String takeLastError();

 private:
  void onEvent(WStype_t type, uint8_t* payload, size_t length);
  void subscribe();
  void unsubscribe(const String& symbol);
  void sendPingIfDue();
  void markError(const String& err);

  WebSocketsClient ws_;
  String selectedSymbol_;
  bool connected_;

  TickerState latestTicker_;
  bool hasFreshTicker_;
  String lastError_;
  unsigned long lastPingMs_;
};
