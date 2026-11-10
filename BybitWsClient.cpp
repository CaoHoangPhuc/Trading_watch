#include "BybitWsClient.h"

#include <ArduinoJson.h>

#include "Config.h"

BybitWsClient::BybitWsClient()
    : connected_(false), hasFreshTicker_(false), lastPingMs_(0) {
  latestTicker_.valid = false;
  latestTicker_.updatedAtMs = 0;
}

void BybitWsClient::begin() {
  ws_.beginSSL(Config::BYBIT_WS_OPTION, Config::BYBIT_WS_PORT, Config::BYBIT_WS_PATH);
  ws_.setReconnectInterval(5000);
  ws_.onEvent([this](WStype_t type, uint8_t* payload, size_t length) { onEvent(type, payload, length); });
}

void BybitWsClient::loop() {
  ws_.loop();
  sendPingIfDue();
}

void BybitWsClient::setSymbol(const String& symbol) {
  if (symbol == selectedSymbol_) {
    return;
  }

  if (selectedSymbol_.length() > 0 && connected_) {
    unsubscribe(selectedSymbol_);
  }

  selectedSymbol_ = symbol;
  latestTicker_.valid = false;

  if (connected_) {
    subscribe();
  }
}

bool BybitWsClient::isConnected() const { return connected_; }

const String& BybitWsClient::selectedSymbol() const { return selectedSymbol_; }

bool BybitWsClient::consumeTicker(TickerState& outTicker) {
  if (!hasFreshTicker_) {
    return false;
  }
  outTicker = latestTicker_;
  hasFreshTicker_ = false;
  return true;
}

String BybitWsClient::takeLastError() {
  String out = lastError_;
  lastError_ = "";
  return out;
}

void BybitWsClient::onEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      connected_ = false;
      break;

    case WStype_CONNECTED:
      connected_ = true;
      lastPingMs_ = millis();
      subscribe();
      break;

    case WStype_TEXT: {
      DynamicJsonDocument doc(Config::MAX_WS_PAYLOAD);
      DeserializationError err = deserializeJson(doc, payload, length);
      if (err) {
        markError(String("WS JSON parse error: ") + err.c_str());
        break;
      }

      const char* op = doc["op"] | "";
      if (String(op) == "ping" || String(op) == "pong") {
        break;
      }

      String topic = doc["topic"] | "";
      if (!topic.startsWith("tickers.")) {
        break;
      }

      JsonVariant data = doc["data"];
      if (data.isNull()) {
        break;
      }

      JsonObject obj;
      if (data.is<JsonArray>()) {
        JsonArray arr = data.as<JsonArray>();
        if (arr.size() == 0) break;
        obj = arr[0].as<JsonObject>();
      } else {
        obj = data.as<JsonObject>();
      }

      latestTicker_.symbol = obj["symbol"] | selectedSymbol_;
      latestTicker_.lastPrice = obj["lastPrice"] | "";
      latestTicker_.bid1Price = obj["bid1Price"] | obj["bidPrice"] | "";
      latestTicker_.ask1Price = obj["ask1Price"] | obj["askPrice"] | "";
      latestTicker_.markPrice = obj["markPrice"] | "";
      latestTicker_.indexPrice = obj["indexPrice"] | "";
      latestTicker_.underlyingPrice = obj["underlyingPrice"] | "";
      latestTicker_.delta = obj["delta"] | "";
      latestTicker_.gamma = obj["gamma"] | "";
      latestTicker_.vega = obj["vega"] | "";
      latestTicker_.theta = obj["theta"] | "";
      latestTicker_.updatedAtMs = millis();
      latestTicker_.valid = true;
      hasFreshTicker_ = true;
      break;
    }

    default:
      break;
  }
}

void BybitWsClient::subscribe() {
  if (!connected_ || selectedSymbol_.length() == 0) {
    return;
  }

  DynamicJsonDocument doc(256);
  doc["op"] = "subscribe";
  JsonArray args = doc.createNestedArray("args");
  args.add("tickers." + selectedSymbol_);

  String msg;
  serializeJson(doc, msg);
  ws_.sendTXT(msg);
}

void BybitWsClient::unsubscribe(const String& symbol) {
  if (!connected_ || symbol.length() == 0) {
    return;
  }

  DynamicJsonDocument doc(256);
  doc["op"] = "unsubscribe";
  JsonArray args = doc.createNestedArray("args");
  args.add("tickers." + symbol);

  String msg;
  serializeJson(doc, msg);
  ws_.sendTXT(msg);
}

void BybitWsClient::sendPingIfDue() {
  if (!connected_) {
    return;
  }

  unsigned long now = millis();
  if (now - lastPingMs_ < Config::HEARTBEAT_INTERVAL_MS) {
    return;
  }

  lastPingMs_ = now;
  DynamicJsonDocument doc(128);
  doc["op"] = "ping";
  String msg;
  serializeJson(doc, msg);
  ws_.sendTXT(msg);
}

void BybitWsClient::markError(const String& err) { lastError_ = err; }
