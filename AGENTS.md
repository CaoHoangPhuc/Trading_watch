# AGENTS.md

## Purpose
This repository runs an ESP32-S3 watch-mode firmware for Bybit option market data and dashboard delivery.

## Collaboration Rules
- Keep v1 as watch-only (no order execution code).
- Preserve message contract stability for dashboard clients.
- Keep loop non-blocking; do not add long blocking delays.
- Treat Wi-Fi and network instability as normal and handle gracefully.

## Architecture Ownership
- `WatchModeController`: system orchestration and scheduling.
- `BybitRestClient`: BTC option contract discovery.
- `BybitWsClient`: live selected symbol ticker stream.
- `DashboardServer`: HTTP page + dashboard WebSocket.
- `CommandProcessor`: dashboard command handling.

## Security Notes
- Never commit real Wi-Fi credentials.
- Keep API auth/trading keys out of v1.

## Future Extensions
- Add private/authenticated channels for trading mode in a separate module.
- Add persistent config storage (NVS) for Wi-Fi and user preferences.
- Add richer command set via `CommandProcessor`.
