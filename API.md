> **AUTHORITATIVE COPY** — This is the canonical `API.md` for the LISYclock project.
> A mirrored read-only copy exists in `../config_editor/API.md`.
> When updating the API, edit this file first, then run `../sync_api.ps1` to sync both repos.

# LISYclock HTTP API Contract

**API Version:** 2
**Firmware Version:** v2.43
**Server Port:** 8080

This file is the **Single Source of Truth** for the HTTP API between the LISYclock firmware and the Config Editor. Both projects reference this document.

When a breaking change is made:
1. Increment `API_VERSION` here
2. Increment `HTTP_API_VERSION` in `LISYclock/main/httpserver.h`
3. Update the Config Editor to handle the new version
4. Update this document

---

## API Version History

| Version | Change |
|---------|--------|
| 1 | Initial versioned API. `/status` response includes `api_version` field. |
| 2 | Added `/reboot`, `/update`, `GET /files`, `GET /files/<name>`, `/rename`. Config Editor: "Connect Clock" button, Reboot/Update/SD-Files UI. `GET /files` response includes `mtime` (Unix timestamp). Added `POST /time`, `DELETE /files/<name>`. Added USB serial commands for config/file transfer with data framing. Unified IP/USB communication in Config Editor Clock tab. |

---

## Endpoints

### GET /status

Connection test and firmware info.

**Response:** `200 OK`, `application/json`

```json
{
  "status": "ok",
  "version": "v2.42",
  "api_version": 2
}
```

- `version`: Firmware version string (hardware-version-dependent, set in `gpiodefs.h`)
- `api_version`: Integer. Incremented only on breaking API changes.

---

### GET /config

Download the current `config.txt` from the SD card.

**Response:**
- `200 OK`, `text/plain` — file contents (chunked transfer)
- `404 Not Found` — if `config.txt` does not exist on the SD card

---

### POST /config

Upload a new `config.txt` to the SD card. Overwrites the existing file.

**Request body:** `text/plain` — full config file contents

**Response:**
- `200 OK`, `application/json`: `{"status":"ok"}`
- `500 Internal Server Error` — if the file cannot be written

---

### PUT /files/\<filename\>

Upload an arbitrary file to the SD card root. Used for audio files etc.

**URL parameter:** `<filename>` — percent-encoded filename (no path separators, no `..`)

**Request body:** raw file bytes

**Response:**
- `200 OK`, `application/json`: `{"status":"ok"}`
- `400 Bad Request` — invalid or empty filename
- `500 Internal Server Error` — if the file cannot be written

---

### DELETE /files/\<filename\>

Delete a file from the SD card root.

**URL parameter:** `<filename>` — percent-encoded filename (no path separators, no `..`)

**Response:**
- `200 OK`, `application/json`: `{"status":"ok"}`
- `400 Bad Request` — invalid or empty filename
- `404 Not Found` — file does not exist
- `500 Internal Server Error` — delete operation failed

---

### POST /reboot

Reboot the clock. Requires confirmation to prevent accidental reboots.

**Request body:** `application/json`: `{"confirm":"reboot"}`

**Response:**
- `200 OK`, `application/json`: `{"status":"ok"}` — clock reboots immediately after response
- `400 Bad Request` — missing or wrong confirmation value

---

### POST /update

Upload a firmware binary to the SD card as `update.bin`, then reboot. `CheckFWUpdate()` applies the update on next boot.

**Request body:** raw binary (`.bin` file)

**Response:**
- `200 OK`, `application/json`: `{"status":"ok"}` — clock reboots immediately after response
- `500 Internal Server Error` — if the file cannot be written

---

### POST /time

Set the system clock and RTC time from a Unix timestamp.

**Request body:** `application/json`

```json
{"unix_timestamp": 1234567890}
```

**Response:**
- `200 OK`, `application/json`: `{"status":"ok"}`
- `400 Bad Request` — missing or invalid `unix_timestamp`

---

### GET /files

List all files in the SD card root directory.

**Response:** `200 OK`, `application/json`

```json
{
  "files": [
    {"name": "config.txt", "size": 1234, "mtime": 1741430400},
    {"name": "update.bin", "size": 512000, "mtime": 0}
  ]
}
```

- `mtime`: Unix timestamp (seconds since epoch) of last modification. `0` if unavailable (RTC not set or filesystem limitation).

---

### GET /files/\<filename\>

Download a file from the SD card root.

**URL parameter:** `<filename>` — percent-encoded filename (no path separators, no `..`)

**Response:**
- `200 OK`, `application/octet-stream` — file contents (chunked transfer)
- `400 Bad Request` — invalid or empty filename
- `404 Not Found` — file does not exist

---

### POST /rename

Rename a file on the SD card.

**Request body:** `application/json`

```json
{"old_name": "foo.txt", "new_name": "bar.txt"}
```

**Response:**
- `200 OK`, `application/json`: `{"status":"ok"}`
- `400 Bad Request` — missing fields or invalid filename (path separators, `..`)
- `500 Internal Server Error` — rename operation failed

---

### OPTIONS /\*

CORS preflight handler. Returns `204 No Content` with appropriate `Access-Control-Allow-*` headers.

All endpoints include CORS headers to allow access from `file://` browser origins.

---

## USB Serial Protocol

### Handshake

After USB connect, the editor sends `0x55` and waits for `OK:READY\r\n`.

### Simple Commands

| Command | Response |
|---------|----------|
| `WIFI:SET_SSID=<ssid>` | `OK:SSID_SET` |
| `WIFI:SET_PWD=<pwd>` | `OK:PWD_SET` |
| `WIFI:ENABLE` | `OK:WIFI_ENABLED` |
| `WIFI:DISABLE` | `OK:WIFI_DISABLED` |
| `WIFI:TEST` | `OK:WIFI_CONNECTED=<ip>` or `ERR:WIFI_CONNECT_FAILED` |
| `WIFI:STATUS` | `OK:WIFI_STATUS=<yes\|no>,<ssid>,<ip\|none>` |
| `TIME:SET=<unix_timestamp>` | `OK:TIME_SET` |
| `SYS:STATUS` | `OK:STATUS=<version>,<api_version>` |
| `SYS:REBOOT` | `OK:REBOOTING` (then ESP restarts) |
| `FILE:RENAME=<old>,<new>` | `OK:FILE_RENAMED` or `ERR:RENAME_FAILED` |
| `FILE:DELETE=<name>` | `OK:FILE_DELETED` or `ERR:DELETE_FAILED` |
| anything else | `ERR:UNKNOWN_CMD` |

### Data Framing Commands

These commands use a chunked framing protocol for multi-line/binary data transfer:

| Command | Response |
|---------|----------|
| `CONFIG:DOWNLOAD` | `DATA:BEGIN=<size>` ... lines ... `DATA:END` |
| `CONFIG:UPLOAD` | Expects `DATA:BEGIN=<size>` ... lines ... `DATA:END` from editor, then `OK:CONFIG_SAVED` or `ERR:CONFIG_SAVE_FAILED` |
| `FILE:LIST` | `DATA:BEGIN=<size>` ... JSON ... `DATA:END` |
| `FILE:DOWNLOAD=<name>` | `BINDATA:BEGIN=<size>` ... base64 lines ... `BINDATA:END` or `ERR:FILE_NOT_FOUND` |
| `FILE:UPLOAD=<name>` | Expects `BINDATA:BEGIN=<size>` ... base64 lines ... `BINDATA:END` from editor, then `OK:FILE_SAVED` or `ERR:FILE_SAVE_FAILED` |

### Data Framing Protocol

**Text data** (config.txt, file listings):

```
Sender:     DATA:BEGIN=<total_bytes>\r\n
            <line of up to 192 bytes>\r\n    (repeated)
            DATA:END\r\n
Receiver:   OK:DATA_RECEIVED\r\n
```

**Binary data** (SD card files):

```
Sender:     BINDATA:BEGIN=<total_bytes>\r\n
            <base64 line, up to 192 chars>\r\n  (repeated, each ~144 raw bytes)
            BINDATA:END\r\n
Receiver:   OK:BINDATA_RECEIVED\r\n
```

**Flow control:** After every 8 lines, the receiver sends `OK:CHUNK_ACK\r\n`. The sender waits for this ACK before continuing (~1.5 KB per cycle, safe for ESP32 RAM).

**Base64 encoding** uses mbedtls (firmware) and `btoa`/`atob` (browser).

### Transfer Speed Estimates (at 115200 baud)

- Config.txt (~3 KB): < 1 second
- Small files (<50 KB): ~10 seconds
- Large files (>100 KB): Warning shown in UI, suggest IP mode instead

---

## Notes

- wifi_manager is no longer used; port 80 is now free. The REST API remains on **port 8080** for compatibility.
- WiFi credentials are configured in the General tab of the Config Editor and stored in `config.txt` as `WIFI_ENABLE`, `WIFI_SSID`, `WIFI_PWD`. The "Test WiFi Connection" button in the Clock tab (USB mode) reads these values from `state.wifi` and sends them via USB.
- All HTTP responses include CORS headers (`Access-Control-Allow-Origin: *`).
- The Config Editor warns the user if the connected firmware's `api_version` does not match the expected version.
- `unix_timestamp`: Seconds since 1970-01-01 00:00:00 UTC. Sets both the ESP32 system clock (`settimeofday`) and the DS3231 RTC (if present).
