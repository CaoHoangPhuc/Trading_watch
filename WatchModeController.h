#pragma once

#include <Arduino.h>

#include "BybitRestClient.h"
#include "BybitWsClient.h"
#include "CommandProcessor.h"
#include "Config.h"
#include "DashboardServer.h"
#include "Types.h"

class WatchModeController {
 public:
  void begin();
  void loop();

 private:
  void connectWiFi();
  bool syncClock();

  void refreshContracts(bool forcePublish);
  String chooseDefaultSymbol(float underlying) const;
  void selectSymbol(const String& symbol, bool announce);

  void onDashboardMessage(const String& raw);

  void publishWatchState();
  void publishContracts();
  void publishTicker(const TickerState& ticker);
  void publishSystemMetrics();
  void publishError(const String& source, const String& message);

  void collectExpiries();
  bool isSymbolKnown(const String& symbol) const;

  static int compareContract(const OptionContract& a, const OptionContract& b);

  BybitRestClient rest_;
  BybitWsClient ws_;
  DashboardServer dashboard_;
  CommandProcessor commands_;

  OptionContract contracts_[Config::MAX_CONTRACTS];
  size_t contractCount_ = 0;

  String expiries_[Config::MAX_EXPIRES];
  size_t expiryCount_ = 0;

  String selectedSymbol_;

  String filterExpiry_ = "ALL";
  String filterSide_ = "ALL";
  float filterMinStrike_ = -1.0f;
  float filterMaxStrike_ = -1.0f;

  TickerState latestTicker_;

  unsigned long lastTickerPublishMs_ = 0;
  unsigned long lastMetricsPublishMs_ = 0;
  unsigned long lastContractsRefreshMs_ = 0;
  unsigned long bootMs_ = 0;
  unsigned long cpuWindowStartMs_ = 0;
  unsigned long cpuBusyMicrosAccum_ = 0;
  float lastCpuUsagePercent_ = 0.0f;
};
