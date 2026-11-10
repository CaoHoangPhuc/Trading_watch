# WORKLOG.md

## 2026-04-08

### Implemented
- Created modular ESP32-S3 firmware architecture:
  - `BybitRestClient`
  - `BybitWsClient`
  - `DashboardServer`
  - `WatchModeController`
  - `CommandProcessor`
- Added watch mode data flow:
  - REST fetch BTC option contracts
  - Auto-pick default symbol (nearest expiry ATM)
  - WS subscribe to selected option ticker
  - 1 Hz dashboard ticker publishing
- Added dashboard UX:
  - Expiry/side/strike filters
  - Contract selection + resubscribe
  - Command input and command response pane
- Added dashboard command support:
  - `help` command
  - Unknown command error with suggestion

### Documents Added/Updated
- `README.md`
- `AGENTS.md`
- `SKILL.md`
- `TODO.md`
- `WORKLOG.md`

### Notes
- v1 remains watch-only by design.
- Real Wi-Fi credentials must be set in `Config.h` before flashing.
- Required third-party Arduino libraries should be installed in IDE.
