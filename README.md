# Trading Watch (ESP32-S3)

Trading Watch is an ESP32-S3 market watcher for Bybit options, with a local dashboard and command channel.

## What v1 does
- Connects ESP32-S3 to Wi-Fi
- Pulls BTC option contracts from Bybit REST (`https://api.bybit.com`)
- Auto-selects a default BTC option contract (nearest expiry, closest strike to underlying)
- Streams selected contract ticker via Bybit public option WebSocket
- Publishes normalized ticker data to dashboard at 1 Hz
- Hosts dashboard directly on ESP32:
  - Contract list
  - Expiry (date) filter
  - Side filter (`C` or `P`)
  - Strike min/max filters
  - Symbol selection and live resubscribe
- Supports dashboard command channel (`help` command)

## Dependencies (Arduino)
Install these libraries in Arduino IDE / PlatformIO:
- `ArduinoJson`
- `WebSockets by Markus Sattler (Links2004)` (required for `WebSocketsClient/WebSocketsServer`)
- ESP32 core libraries (`WiFi`, `WebServer`, `HTTPClient`, `WiFiClientSecure`)

## Setup
1. Open `Config.h` in this project root
2. Set:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
3. Build and flash to ESP32-S3.
4. Open Serial Monitor at `115200` for boot logs (if added later).
5. On same LAN, open:
   - `http://<esp32_ip>/`
   - `http://tradingwatch.local/` (mDNS, if your network/client supports `.local`)

On boot, Serial prints:
- device IP
- dashboard URL by IP
- dashboard URL by mDNS hostname

## Dashboard WebSocket Message Contract

### Device -> UI
- `watch_state`
- `contracts_list`
- `ticker_1hz`
- `error_event`
- `command_response`

### UI -> Device
- `select_contract`
- `set_filters`
- `refresh_contracts`
- `command_request`

## Dashboard Command Channel

### Request
```json
{"type":"command_request","command":"help"}
```

### Success Response
```json
{
  "type":"command_response",
  "ok":true,
  "command":"help",
  "docs_ref":"README.md#dashboard-command-channel",
  "commands":[
    {"name":"help","description":"Show available dashboard commands"}
  ]
}
```

### Unknown Command Response
```json
{
  "type":"command_response",
  "ok":false,
  "command":"<input>",
  "error":"Unknown command",
  "suggestion":"Run help",
  "docs_ref":"README.md#dashboard-command-channel"
}
```

## Bybit References
- WS Connect docs: [Bybit V5 WS Connect](https://bybit-exchange.github.io/docs/v5/ws/connect)
- Public ticker topic: `tickers.{symbol}` on `wss://stream.bybit.com/v5/public/option`
- REST tickers endpoint used for BTC option discovery:
  - `GET /v5/market/tickers?category=option&baseCoin=BTC`
