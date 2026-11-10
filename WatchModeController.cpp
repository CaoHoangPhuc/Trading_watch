#include "WatchModeController.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <esp_system.h>
#include <WiFi.h>
#include <math.h>
#include <time.h>

#include "Config.h"

namespace {
bool isTodayOrFuture(const OptionContract& c, tm* nowUtc) {
  if (c.year > (nowUtc->tm_year + 1900)) return true;
  if (c.year < (nowUtc->tm_year + 1900)) return false;
  if (c.month > (nowUtc->tm_mon + 1)) return true;
  if (c.month < (nowUtc->tm_mon + 1)) return false;
  return c.day >= nowUtc->tm_mday;
}
}  // namespace

void WatchModeController::begin() {
  latestTicker_.valid = false;
  bootMs_ = millis();
  cpuWindowStartMs_ = millis();

  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    if (MDNS.begin(Config::MDNS_HOSTNAME)) {
      Serial.printf("[mdns] Registered: http://%s.local/\n", Config::MDNS_HOSTNAME);
    } else {
      publishError("mdns", "Failed to start mDNS service");
    }
  }
  syncClock();

  ws_.begin();
  dashboard_.setMessageHandler([this](const String& raw) { onDashboardMessage(raw); });
  dashboard_.begin();

  refreshContracts(true);
  publishWatchState();
}

void WatchModeController::loop() {
  unsigned long loopStartUs = micros();
  dashboard_.loop();
  ws_.loop();

  String wsErr = ws_.takeLastError();
  if (wsErr.length()) {
    publishError("ws", wsErr);
  }

  TickerState fresh;
  if (ws_.consumeTicker(fresh)) {
    latestTicker_ = fresh;
  }

  unsigned long now = millis();

  if (latestTicker_.valid && now - lastTickerPublishMs_ >= Config::DASHBOARD_TICKER_INTERVAL_MS) {
    lastTickerPublishMs_ = now;
    publishTicker(latestTicker_);
  }

  if (now - lastMetricsPublishMs_ >= Config::DASHBOARD_TICKER_INTERVAL_MS) {
    lastMetricsPublishMs_ = now;
    publishSystemMetrics();
  }

  if (now - lastContractsRefreshMs_ >= Config::CONTRACT_REFRESH_INTERVAL_MS) {
    refreshContracts(false);
  }

  static unsigned long lastStateMs = 0;
  if (now - lastStateMs >= 2000) {
    lastStateMs = now;
    publishWatchState();
  }

  cpuBusyMicrosAccum_ += (micros() - loopStartUs);

  unsigned long windowMs = now - cpuWindowStartMs_;
  if (windowMs >= 1000) {
    const unsigned long windowMicros = windowMs * 1000UL;
    if (windowMicros > 0) {
      lastCpuUsagePercent_ = (100.0f * static_cast<float>(cpuBusyMicrosAccum_)) / static_cast<float>(windowMicros);
      if (lastCpuUsagePercent_ > 100.0f) lastCpuUsagePercent_ = 100.0f;
      if (lastCpuUsagePercent_ < 0.0f) lastCpuUsagePercent_ = 0.0f;
    } else {
      lastCpuUsagePercent_ = 0.0f;
    }
    cpuWindowStartMs_ = now;
    cpuBusyMicrosAccum_ = 0;
  }
}

void WatchModeController::connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < Config::WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    publishError("wifi", "Wi-Fi not connected; check SSID/PASSWORD in Config.h");
    Serial.println("[wifi] connection failed");
    return;
  }

  Serial.printf("[wifi] connected, IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[dashboard] http://%s/\n", WiFi.localIP().toString().c_str());
}

bool WatchModeController::syncClock() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");

  time_t now = time(nullptr);
  unsigned long start = millis();
  while (now < 100000 && millis() - start < 5000) {
    delay(200);
    now = time(nullptr);
  }

  return now >= 100000;
}

void WatchModeController::refreshContracts(bool forcePublish) {
  lastContractsRefreshMs_ = millis();

  size_t count = 0;
  float underlying = 0.0f;
  String err;
  if (!rest_.fetchBtcOptionTickers(contracts_, Config::MAX_CONTRACTS, count, underlying, err)) {
    publishError("rest", err);
    return;
  }

  contractCount_ = count;

  for (size_t i = 0; i < contractCount_; ++i) {
    size_t j = i;
    while (j > 0 && compareContract(contracts_[j - 1], contracts_[j]) > 0) {
      OptionContract tmp = contracts_[j - 1];
      contracts_[j - 1] = contracts_[j];
      contracts_[j] = tmp;
      --j;
    }
  }

  collectExpiries();

  if (!isSymbolKnown(selectedSymbol_)) {
    String def = chooseDefaultSymbol(underlying);
    if (def.length()) {
      selectSymbol(def, false);
    }
  }

  (void)forcePublish;
  publishContracts();
}

String WatchModeController::chooseDefaultSymbol(float underlying) const {
  if (contractCount_ == 0) {
    return "";
  }

  time_t now = time(nullptr);
  tm* nowUtc = gmtime(&now);

  int bestDateKey = 0;
  float bestStrikeDelta = 999999999.0f;
  int bestIdx = -1;

  for (size_t i = 0; i < contractCount_; ++i) {
    const OptionContract& c = contracts_[i];

    if (nowUtc && !isTodayOrFuture(c, nowUtc)) {
      continue;
    }

    int dateKey = c.year * 10000 + c.month * 100 + c.day;
    float strikeDelta = (underlying > 0.0f) ? fabs(c.strike - underlying) : c.strike;

    if (bestIdx < 0 || dateKey < bestDateKey || (dateKey == bestDateKey && strikeDelta < bestStrikeDelta)) {
      bestIdx = static_cast<int>(i);
      bestDateKey = dateKey;
      bestStrikeDelta = strikeDelta;
    }
  }

  if (bestIdx < 0) {
    return contracts_[0].symbol;
  }
  return contracts_[bestIdx].symbol;
}

void WatchModeController::selectSymbol(const String& symbol, bool announce) {
  if (!isSymbolKnown(symbol)) {
    publishError("select_contract", "Symbol not found in BTC option list");
    return;
  }

  selectedSymbol_ = symbol;
  ws_.setSymbol(selectedSymbol_);

  if (announce) {
    publishWatchState();
  }
}

void WatchModeController::onDashboardMessage(const String& raw) {
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, raw);
  if (err) {
    publishError("dashboard", String("Invalid JSON from dashboard: ") + err.c_str());
    return;
  }

  String type = doc["type"] | "";

  if (type == "select_contract") {
    selectSymbol(doc["symbol"].as<String>(), true);
    return;
  }

  if (type == "refresh_contracts") {
    refreshContracts(true);
    return;
  }

  if (type == "set_filters") {
    filterExpiry_ = doc["expiry"] | "ALL";
    filterSide_ = doc["side"] | "ALL";

    filterMinStrike_ = doc["min_strike"].isNull() ? -1.0f : static_cast<float>(doc["min_strike"].as<double>());
    filterMaxStrike_ = doc["max_strike"].isNull() ? -1.0f : static_cast<float>(doc["max_strike"].as<double>());

    publishContracts();
    return;
  }

  if (type == "command_request") {
    String command = doc["command"] | "";
    String response = commands_.handle(command);
    dashboard_.broadcastJson(response);
    return;
  }

  publishError("dashboard", "Unsupported message type: " + type);
}

void WatchModeController::publishWatchState() {
  DynamicJsonDocument doc(512);
  String ip = WiFi.localIP().toString();

  doc["type"] = "watch_state";
  doc["uptime_ms"] = millis() - bootMs_;
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = ip;
  doc["dashboard_url"] = String("http://") + ip + "/";
  doc["dashboard_mdns"] = String("http://") + Config::MDNS_HOSTNAME + ".local/";
  doc["bybit_ws_connected"] = ws_.isConnected();
  doc["selected_symbol"] = selectedSymbol_;

  String out;
  serializeJson(doc, out);
  dashboard_.broadcastJson(out);
}

void WatchModeController::publishContracts() {
  DynamicJsonDocument doc(64 * 1024);
  doc["type"] = "contracts_list";

  JsonArray expiries = doc.createNestedArray("expiries");
  for (size_t i = 0; i < expiryCount_; ++i) {
    expiries.add(expiries_[i]);
  }

  JsonArray contracts = doc.createNestedArray("contracts");
  for (size_t i = 0; i < contractCount_; ++i) {
    const OptionContract& c = contracts_[i];

    if (filterExpiry_ != "ALL" && c.expiry != filterExpiry_) {
      continue;
    }
    if (filterSide_ != "ALL" && String(c.side) != filterSide_) {
      continue;
    }
    if (filterMinStrike_ >= 0.0f && c.strike < filterMinStrike_) {
      continue;
    }
    if (filterMaxStrike_ >= 0.0f && c.strike > filterMaxStrike_) {
      continue;
    }

    JsonObject row = contracts.createNestedObject();
    row["symbol"] = c.symbol;
    row["expiry"] = c.expiry;
    row["strike"] = c.strike;
    row["side"] = String(c.side);
  }

  String out;
  serializeJson(doc, out);
  dashboard_.broadcastJson(out);
}

void WatchModeController::publishTicker(const TickerState& ticker) {
  DynamicJsonDocument doc(1024);
  doc["type"] = "ticker_1hz";
  doc["symbol"] = ticker.symbol;
  doc["last_price"] = ticker.lastPrice;
  doc["bid_1"] = ticker.bid1Price;
  doc["ask_1"] = ticker.ask1Price;
  doc["mark_price"] = ticker.markPrice;
  doc["index_price"] = ticker.indexPrice;
  doc["underlying_price"] = ticker.underlyingPrice;
  doc["delta"] = ticker.delta;
  doc["gamma"] = ticker.gamma;
  doc["vega"] = ticker.vega;
  doc["theta"] = ticker.theta;
  doc["updated_at_ms"] = ticker.updatedAtMs;

  String out;
  serializeJson(doc, out);
  dashboard_.broadcastJson(out);
}

void WatchModeController::publishSystemMetrics() {
  DynamicJsonDocument doc(1024);
  doc["type"] = "system_metrics_1hz";
  doc["uptime_ms"] = millis() - bootMs_;
  doc["cpu_usage_pct"] = lastCpuUsagePercent_;
  doc["heap_free_bytes"] = ESP.getFreeHeap();
  doc["heap_total_bytes"] = ESP.getHeapSize();
  doc["heap_min_free_bytes"] = ESP.getMinFreeHeap();
  doc["mem_usage_pct"] = 100.0f * (1.0f - (static_cast<float>(ESP.getFreeHeap()) / static_cast<float>(ESP.getHeapSize())));

  float tempC = temperatureRead();
  doc["cpu_temp_c"] = tempC;

  time_t nowTs = time(nullptr);
  doc["unix_time"] = static_cast<uint32_t>(nowTs);
  char iso[32];
  tm* t = gmtime(&nowTs);
  if (t && strftime(iso, sizeof(iso), "%Y-%m-%dT%H:%M:%SZ", t) > 0) {
    doc["time_utc"] = iso;
  } else {
    doc["time_utc"] = "";
  }

  String out;
  serializeJson(doc, out);
  dashboard_.broadcastJson(out);
}

void WatchModeController::publishError(const String& source, const String& message) {
  DynamicJsonDocument doc(512);
  doc["type"] = "error_event";
  doc["source"] = source;
  doc["message"] = message;

  String out;
  serializeJson(doc, out);
  dashboard_.broadcastJson(out);
}

void WatchModeController::collectExpiries() {
  expiryCount_ = 0;

  for (size_t i = 0; i < contractCount_; ++i) {
    const String& e = contracts_[i].expiry;
    bool exists = false;
    for (size_t j = 0; j < expiryCount_; ++j) {
      if (expiries_[j] == e) {
        exists = true;
        break;
      }
    }

    if (!exists && expiryCount_ < Config::MAX_EXPIRES) {
      expiries_[expiryCount_++] = e;
    }
  }
}

bool WatchModeController::isSymbolKnown(const String& symbol) const {
  if (symbol.length() == 0) return false;
  for (size_t i = 0; i < contractCount_; ++i) {
    if (contracts_[i].symbol == symbol) {
      return true;
    }
  }
  return false;
}

int WatchModeController::compareContract(const OptionContract& a, const OptionContract& b) {
  if (a.year != b.year) return a.year - b.year;
  if (a.month != b.month) return a.month - b.month;
  if (a.day != b.day) return a.day - b.day;
  if (a.strike < b.strike) return -1;
  if (a.strike > b.strike) return 1;
  if (a.side < b.side) return -1;
  if (a.side > b.side) return 1;
  return 0;
}
