# SKILL.md

## Project Skill: ESP32 Bybit Watch Development

## Objective
Implement and maintain low-latency, resilient watch-mode firmware on ESP32-S3 with dashboard delivery.

## Required Practices
- Use non-blocking loops and interval-based tasks.
- Keep JSON contracts explicit and backward-compatible.
- Normalize Bybit payloads before broadcasting to UI.
- Separate transport logic (REST/WS) from orchestration logic.

## Coding Checklist
- Add/keep clear constants in `Config.h`.
- Guard against malformed dashboard and WS JSON input.
- Emit `error_event` instead of silently failing.
- Maintain 1 Hz dashboard publish cadence independent of Bybit message rate.

## Command Extension Pattern
When adding commands:
1. Parse new command in `CommandProcessor`.
2. Return `command_response` with `ok`, `command`, and `docs_ref`.
3. Add short docs entry in README command section.
4. Avoid command side effects that block ticker streaming.

## Testing Focus
- Reconnect behavior
- Contracts refresh correctness
- Command response schema stability
- Dashboard filter behavior

## Build Commands
- Default workflow after every code change:
  - `powershell -ExecutionPolicy Bypass -File .\scripts\build-and-upload.ps1 -Port COM3 -Jobs 0 -VerboseOutput`
  - This is your local CI/CD-style loop: **compile -> upload -> verify by success exit code**.
  - `-Jobs 0` means auto jobs = `4 x logical cores` (fast for mostly I/O-bound compile stages).
  - Verbose mode streams compile/upload output live and writes to `logs\`.

- Arduino CLI compile:
  - `powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Jobs 0 -VerboseOutput`

- Node one-liner compile:
```bash
node -e "const {execSync}=require('child_process'); execSync('powershell -ExecutionPolicy Bypass -File .\\\\scripts\\\\build.ps1 -Jobs 0',{stdio:'inherit'})"
```

## Upload Commands
- PowerShell pipeline script:
  - `powershell -ExecutionPolicy Bypass -File .\scripts\build-and-upload.ps1 -Port COM3 -Jobs 0 -VerboseOutput`

- Upload only:
  - `powershell -ExecutionPolicy Bypass -File .\scripts\upload.ps1 -Port COM3 -VerboseOutput`
  - `upload.ps1` does not support `-Jobs` (only `build.ps1` and `build-and-upload.ps1` do).
  - During real upload you should see esptool progress lines like `Connecting...`, `Writing at 0x...`, `Hash of data verified`.

- Direct upload should go through project scripts to avoid temp build path mismatches:
  - `powershell -ExecutionPolicy Bypass -File .\scripts\upload.ps1 -Port COM3 -VerboseOutput`

- Node one-liner upload:
```bash
node -e "const {execSync}=require('child_process'); execSync('powershell -ExecutionPolicy Bypass -File .\\\\scripts\\\\upload.ps1 -Port COM3',{stdio:'inherit'})"
```

- Node script (recommended):
```bash
node .\scripts\build-upload.mjs "esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashMode=qio120,PartitionScheme=min_spiffs,LoopCore=0,EventsCore=0" COM3 . 0
```

## Cleanup Commands
- Clean project build artifacts, binaries, and junk files:
  - `powershell -ExecutionPolicy Bypass -File .\scripts\clean.ps1`
- Dry-run preview (no delete):
  - `powershell -ExecutionPolicy Bypass -File .\scripts\clean.ps1 -DryRun`
- Keep logs while cleaning:
  - `powershell -ExecutionPolicy Bypass -File .\scripts\clean.ps1 -IncludeLogs:$false`

## If Upload Hangs At "Connecting...."
- Hold `BOOT`, tap `RST`, then release `BOOT` after upload starts.
- Try lower baud (`115200` or `460800`).
- Confirm correct serial port and close Serial Monitor before uploading.
- Use a data-capable USB cable and direct USB port (avoid weak hubs).
