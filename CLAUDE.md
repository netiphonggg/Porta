# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project
ESP-IDF v6.0.1 firmware for an ESP32-based "Porta1" data-logger (`porta1-v01`). Reads Modbus RTU slaves over RS-485, logs to SD card, publishes via MQTT, served behind a small HTTP config UI (HTML/JS hosted on the device's FAT partition).

Target chip: **ESP32** (factory app slot 1.6 MB, `storage` FAT partition 2 MB — see [partitions.csv](partitions.csv)).

## Build & flash

ESP-IDF env is sourced via the `get_idf` alias defined in `~/.zshrc` (points at `/Users/gg/.espressif/v6.0.1/esp-idf/export.sh`). All commands below assume `get_idf` was run in the shell first.

```bash
idf.py build                # compile (5-15 min cold, much faster incremental)
idf.py -p /dev/cu.usbserial-XXXX flash monitor
idf.py fullclean            # wipe build/ before re-cmake (needed after IDF path or sdkconfig changes)
```

**Web assets are baked into a FAT image at build time.** [main/CMakeLists.txt](main/CMakeLists.txt) runs `fatfs_create_rawflash_image(storage ../html FLASH_IN_PROJECT)` — so any edit under [html/](html/) (HTML, JS, CSS, the bundled `xlsx.full.min.js`) requires `idf.py flash` (not just `app-flash`) to take effect on the device. There is no live-reload path.

If `idf.py build` errors with `CMake Error: The source ... does not match the source ... used to generate cache` after switching IDF installs, `rm -rf build/` and rebuild — CMake cache hard-codes the IDF source dir.

## High-level architecture

### Component layout
All app code lives under [main/](main/), split into per-subsystem subdirs each prefixed `i_`:

| Subsystem | Subdir | Entry-point called from `main.c` |
|---|---|---|
| WiFi STA/AP + SNTP | [i_wifi_connect/](main/i_wifi_connect/) | `wifiInitTask` (task), `wifi_connect_init` |
| MQTT client | [i_mqtt_client/](main/i_mqtt_client/) | `init_mqtt_client` |
| HTTP server + mDNS | [i_server/](main/i_server/) | `init_esp_server`, `start_mdns_service`, `mount_fat_ro` |
| Modbus RTU master | [i_modbus/](main/i_modbus/) | `init_esp_modbus` (currently commented out at [main.c:234](main/main.c#L234)) |
| SD/MMC + JSON config loader | [i_sd_mmc/](main/i_sd_mmc/) | `init_esp_sd_mmc` |
| I²C | [i_i2c/](main/i_i2c/) | `init_esp_i2c` |
| HTTPS client | [i_https_client/](main/i_https_client/) | `init_esp_https` |

Each subsystem owns its `.c`/`.h` and exposes a small surface: an `init_*` plus a handful of task entry-points. Cross-subsystem coordination happens through globals and FreeRTOS primitives, **not function calls**.

### Shared global state contract
[main/i_define.h](main/i_define.h) declares `extern` globals; [main/main.c](main/main.c) (lines ~15-48) is the single point of definition. Notable shared state:

- `g_params`, `g_params_count`, `g_my_param_type`, `cfg_list`, `cfg_count` — Modbus runtime descriptors. Populated by `load_device_cfg_from_json()` in [i_sd_mmc.c:396](main/i_sd_mmc/i_sd_mmc.c#L396); consumed by `mb_master_init()` in [i_modbus.c:536](main/i_modbus/i_modbus.c#L536).
- `g_pool_holding/input/coil/discrete` — Modbus parameter value pools, dynamically sized from the JSON.
- `mqttClient` — esp-mqtt handle, used by every component that publishes.
- `binSem_initWifi`, `binSem_connectWifi`, `sdmmcBus` — synchronization between init tasks.
- `q_sd_cmd`, `q_sd_write` — work queues into the SD task.

If you add cross-subsystem state, follow this pattern (extern in `i_define.h`, defined once in `main.c`). Components routinely poke each other's state through these — there is no DI.

### Config persistence
Two channels:

**NVS** (small typed values) — namespaces defined in [i_define.h](main/i_define.h):
- `wifiCreds` (`ssid`, `pass`)
- `mqttCreds` (`host`, `port`, `client_id`, `username`, `password`)
- `sntp` (last synced epoch)
- `sdindex`, `sdreadfile` (SD log iteration state)

Pattern: `nvs_open(NVS_*_KEY, …)` → `nvs_get_str/u16/i32/i64` → `nvs_close`. Keys ≤15 chars. Type used on get must match type used on set, else `ESP_ERR_NVS_TYPE_MISMATCH`.

**SD card JSON** (large structured config) — `/sd/conf/mb.json` (`MB_CONF_PATH`). The Modbus device/point dictionary lives here. Schema and field mappings are documented at the bottom of [i_modbus.h](main/i_modbus/i_modbus.h) (the long comment block); `load_device_cfg_from_json` parses it.

### HTTP server routing
[i_server.c](main/i_server/i_server.c) runs a single `httpd_handle_t`. Routes register in two tiers:

1. Explicit `/api/*` handlers (POST/GET on specific URIs).
2. A wildcard `/*` GET handler ([on_default_url](main/i_server/i_server.c#L75)) that resolves paths against the FAT mount `/store/...` (= the flashed `html/` dir), with `index.html` as the SPA fallback.

**Two non-obvious rules:**
- Always register `/api/*` routes **before** the wildcard, otherwise `/*` swallows them.
- The default `max_uri_handlers` is 8. We bumped it to 16 in `esp_server_config.max_uri_handlers = 16;` before `httpd_start()` — adding new APIs beyond that requires bumping it again, or the last `httpd_register_uri_handler` calls silently fail with `no slots left for registering handler`.

### Web UI ↔ firmware contract
Frontend in [html/scripts.js](html/scripts.js) is plain ES modules-free JS — talks to firmware exclusively via the helpers `apiGet(path)` and `apiPostJson(path, obj)`. JSON field names in the JS payload must match `cJSON_GetObjectItem(payload, "…")` keys on the C side exactly (e.g., `datatype` vs `data_type` — easy to typo).

The Modbus Excel-upload flow worth understanding:
1. JS uses **SheetJS** (hosted locally at `/xlsx.full.min.js`, ~930 KB) to parse `mb.xlsx` into three sheets `serial`/`devices`/`points` → composes nested JSON → `POST /api/upload-mb`.
2. `on_upload_mb` chunk-reads the body, streams to `/sd/conf/mb.json.tmp`, validates with `cJSON`, then atomically `rename()`s to `MB_CONF_PATH`.
3. Reboot required to apply — there is no runtime hot-reload (would need to teardown the Modbus master handle and reload `g_params`; deemed too risky).

### MQTT specifics
[i_mqtt_client.c](main/i_mqtt_client/i_mqtt_client.c) keeps the config in a file-scope `static struct mqtt_cfg`. **Do not** put it on the stack: `esp_mqtt_client_config_t` stores pointers to the strings, not copies, so the buffers must outlive `esp_mqtt_client_init()`. Default values (`iot-mqtt.egat.co.th:1883`, client_id `porta1`) are set first, then `nvs_get_str` overlays them.

## Files that look orphan but matter
- [main/files/index.html](main/files/index.html) — embedded via `EMBED_TXTFILES` in `main/CMakeLists.txt`. **Not** the live web UI; that one is in `html/`. Likely legacy.
- [main/learn.c](main/learn.c) — scratch/learning code, not compiled (not in `SRCS`).
- [debug/](debug/), [site/](site/) — out-of-tree experiments, not part of the build.

## Conventions
- Tag strings for `ESP_LOGI/E/W` are `"* SUBSYSTEM"` (e.g., `"* WIFI"`, `"* MQTT"`, `"* SERVER"`).
- Comments are bilingual EN/TH; both are normal.
- Code style is unenforced — mixed indentation in places, follow surrounding style of the file you're editing rather than imposing a global rule.
