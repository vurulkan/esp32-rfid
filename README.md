# RFID Access Control (ESP32)

## Overview
- Target: ESP32_Relay X2 Board with ESP32-WROOM-32E (N4)
- Framework: Arduino ESP32 core (2.0.17 tested, 3.3.x compatible)
- Architecture: FreeRTOS tasks + queues, modular components
- Web UI: Embedded gzip assets served via `WebServer`

## Requirements
- Arduino IDE with ESP32 core (2.0.17 recommended)
- Library: `MFRC522` (Miguel Balboa)
- Optional: DS3231 RTC module

## WiFi (AP mode)
- SSID: `RFID-ACCESS`
- Password: `rfid1234`
- Default AP IP: `192.168.4.1`

## WiFi (Client mode)
- Configure SSID/Password in Settings
- Optional static IP (IP/Gateway/Mask)
- IP is shown in Status and printed to Serial after connect

## GPIO / Wiring

### Relay outputs (fixed by board)
- Relay 1: GPIO16
- Relay 2: GPIO17

### RC522 #1 (RC522 1 -> Relay 1)
- SDA/SS: GPIO5
- RST: GPIO26

### RC522 #2 (RC522 2 -> Relay 2)
- SDA/SS: GPIO27
- RST: GPIO25

### Shared SPI bus (VSPI)
- SCK: GPIO18
- MISO: GPIO19
- MOSI: GPIO23

### DS3231 RTC (optional, I2C)
- SDA: GPIO21
- SCL: GPIO22

### IO0 (maintenance)
- Actions are decided **when the button is released** (press duration).
- Hold IO0 for 2-5 seconds to reset WiFi settings (AP mode)
- Hold IO0 for 5-10 seconds to disable authentication
- Hold IO0 for 10+ seconds to format LittleFS and reboot

### Power
- RC522 VCC: 3.3V
- RC522 GND: GND
- IRQ: not used

## Tasks
- `wifi_task`: starts AP, updates state flag only in WiFi event callback
- `web_task`: REST API + UI
- `rfid_task`: reads two RC522 devices
- `logic_task`: users, logs, relay decisions

## User Management
- Stored in LittleFS (`/users.txt`)
- Survives reboot/power loss
- Max users: 1000

## Log System
- RAM keeps last 50 entries (ring buffer)
- LittleFS keeps up to 10,000 entries (overwrites oldest)
- Persisted to LittleFS (`/logs.txt`)
- Only `granted` and `denied` entries are stored
- When RTC is enabled and set, logs include `DD/MM/YYYY,HH:MM:SS` (as extra columns)
- `logs.txt` uses comma-separated columns:
  - Without RTC: `<ts_ms>,<relay>,<status>,<uid>,<name>`
  - With RTC: `<ts_ms>,<DD/MM/YYYY>,<HH:MM:SS>,<relay>,<status>,<uid>,<name>`
- Clearable via API

## Settings (LittleFS)
- Stored in `/settings.txt`
- RTC default is disabled after format or clean install
- RTC set state is persisted (no need to re-set after reboot)
- WiFi mode and credentials are persisted until format
- Optional static IP for client mode is persisted
- Relay names are persisted (defaults: `Relay 1`, `Relay 2`)
- Relay manual on/off states are persisted
- Authentication settings are persisted (username, password, API key)

## Backup & Restore
- `GET /backup?type=users|settings` returns plain text
- `POST /restore` with plain text body (auto-detects settings/users sections)
- Logs can be downloaded via `/logs/export`

## Build & Upload (Arduino IDE)
1. Install ESP32 core (2.0.17 recommended).
2. Install the `MFRC522` library.
3. Open `src/esp32-rfid` as the sketch folder.
4. Board: `ESP32 Dev Module`, select your port.
5. Upload and open Serial Monitor at 115200.

## First Boot (LittleFS)
- Some new boards may ship with an unformatted LittleFS.
- If you see LittleFS mount errors on first boot, format it via Maintenance -> "Format LittleFS" or hold IO0 for 10+ seconds.

## REST API
- `GET /` UI (gzip)
- `GET /login` Login page (gzip)
- `GET /app.js`, `GET /style.css` (gzip)
- `GET /users`
- `POST /users` (uid, name, relay1, relay2)
- `DELETE /users` (uid)
- `GET /logs`
- `DELETE /logs?scope=ram|all`
- `GET /logs/export`
- `GET /rfid`
- `GET /status`
- `GET /backup?type=users|settings`
- `POST /restore`
- `POST /auth/login`
- `POST /auth/logout`
- `POST /maintenance/format`
- `POST /maintenance/relay` (relay=1|2, action=pulse|on|off, duration_ms=50..10000)
- `POST /maintenance/reboot`

## Authentication
- When enabled, the UI shows a login page.
- API requests accept `X-API-Key` or an authenticated session cookie.
- API key is shown **once** when enabling; store it safely.
- Session has a 5-minute inactivity timeout and can be ended via Logout.
- Logout is explicit; browsers cannot reliably distinguish refresh vs close.

## Notes
- If you edit `src/esp32-rfid/web/*`, regenerate `*.gz.h` assets.
- Maintenance tasks can be done via UI or IO0 button (2-5s WiFi reset, 5-10s auth disable, 10s+ format).
