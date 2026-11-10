#pragma once

#include <Arduino.h>

namespace Config {
static const char* WIFI_SSID = "PhucWifi";
static const char* WIFI_PASSWORD = "000000001";

static const char* BYBIT_REST = "https://api.bybit.com";
static const char* BYBIT_WS_OPTION = "stream.bybit.com";
static const uint16_t BYBIT_WS_PORT = 443;
static const char* BYBIT_WS_PATH = "/v5/public/option";

static const uint16_t DASHBOARD_HTTP_PORT = 80;
static const uint16_t DASHBOARD_WS_PORT = 81;
static const char* MDNS_HOSTNAME = "tradingwatch";

static const uint32_t HEARTBEAT_INTERVAL_MS = 20000;
static const uint32_t DASHBOARD_TICKER_INTERVAL_MS = 1000;
static const uint32_t CONTRACT_REFRESH_INTERVAL_MS = 60000;
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
static const uint32_t HTTP_TIMEOUT_MS = 4000;
static const uint32_t HTTP_CONNECT_TIMEOUT_MS = 3000;

static const size_t MAX_CONTRACTS = 300;
static const size_t MAX_EXPIRES = 80;
static const size_t MAX_WS_PAYLOAD = 8192;

static const uint32_t APP_TASK_STACK_WORDS = 12288;
static const UBaseType_t APP_TASK_PRIORITY = 1;
static const BaseType_t APP_TASK_CORE = 1;
}  // namespace Config
