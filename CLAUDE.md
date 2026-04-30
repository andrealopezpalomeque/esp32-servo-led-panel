# CLAUDE.md

Context file for Claude (or any AI assistant) working on this project.

## Project Overview

ESP32-based control panel that exposes a servo motor and the onboard LED via a WiFi web interface. Single-page UI with sliders and buttons; the ESP32 hosts everything itself — no external server, no app, no cloud.

**Repo:** https://github.com/andrealopezpalomeque/esp32-servo-led-panel

## Hardware

- **Board:** ESP32 Dev Module
- **Servo:** standard hobby servo (SG90-class), wired to GPIO 13
  - Red → 5V (or VIN)
  - Brown/Black → GND
  - Orange/Yellow → GPIO 13
- **LED:** onboard LED on GPIO 2 (no external wiring)
- **Power:** USB from laptop. If servo jitters, the laptop port can't supply enough current — try a powered USB hub or different cable.

## Software Stack

- Arduino IDE 2.3.x
- ESP32 board package (Espressif)
- Libraries: `WiFi.h`, `WebServer.h` (built-in), `ESP32Servo` by Kevin Harrington (Library Manager)
- Serial Monitor baud: **115200**

## Project Structure

```
sketch_apr30a/
├── sketch_apr30a.ino    # main sketch (in repo)
├── secrets.h            # WiFi credentials (gitignored)
└── .gitignore
```

`secrets.h` defines `WIFI_SSID` and `WIFI_PASSWORD` and is included from the main sketch. Never commit it.

## Architecture

The sketch follows one consistent pattern that scales to any number of controls:

1. **State variables at the top** (`sweepSpeed`, `minAngle`, `blinkOn`, etc.) — single source of truth
2. **`handleRoot()`** — serves the HTML/CSS/JS as one big string
3. **One `handleX()` function per endpoint** — each updates a single state variable
4. **`setup()`** — connects WiFi, registers endpoints with `server.on(...)`
5. **`loop()`** — calls `server.handleClient()`, then runs non-blocking actions using the `millis()` pattern

### The `millis()` pattern (critical)

Never use `delay()` in `loop()` — it freezes the web server and the servo. Instead:

```cpp
if (millis() - lastAction > interval) {
    // do the thing
    lastAction = millis();
}
```

This is how multiple "concurrent" things (sweep + blink + serving requests) coexist on a single-threaded microcontroller.

### Web endpoints (current)

| Endpoint | Effect |
|---|---|
| `GET /` | Returns the full HTML page |
| `GET /speed?v=N` | Sets sweep speed (ms between steps) |
| `GET /min?v=N` | Sets sweep range minimum angle |
| `GET /max?v=N` | Sets sweep range maximum angle |
| `GET /angle?v=N` | Manually sets servo angle (auto-flips to Manual mode) |
| `GET /blink?v=N` | Sets LED blink rate (ms) |
| `GET /ledtoggle` | Toggles LED blinking on/off, returns "ON" or "OFF" |
| `GET /modetoggle` | Toggles servo Sweep/Manual, returns "Sweep" or "Manual" |

## User Context

The user is a software developer (frontend-leaning), comfortable with git, Node ecosystem, web tech. Treat them as such — no need to explain HTTP, JS, CSS basics. The hardware/embedded side is newer to them; explain microcontroller-specific concepts (PWM, GPIO, non-blocking patterns, flash memory) when they come up.

User language: English in code/docs, Spanish-speaking from Buenos Aires (some replies may be in Spanish).

## Development Workflow

```bash
# normal cycle
# 1. edit code in Arduino IDE
# 2. upload to ESP32
# 3. test in browser
git add .
git commit -m "what changed"
git push
```

For experiments that might break things:
```bash
git checkout -b feature/whatever
# experiment freely
# if it works → merge to main
# if not → delete the branch
```

Tag working versions: `git tag v1 && git push --tags`.

## Conventions for Future Changes

- **Keep the millis() pattern.** No `delay()` calls in `loop()` ever.
- **One state variable per controllable thing.** Don't pack multiple values into one.
- **One endpoint per action.** `handleSpeed`, `handleBlink`, etc. Stays grep-able.
- **HTML/CSS/JS goes in `handleRoot()` as concatenated strings.** Ugly but works. If it gets unwieldy, consider serving from SPIFFS/LittleFS.
- **Encoding:** always include `<meta charset='UTF-8'>` and use HTML entities (`&deg;`, `&ndash;`) or unicode escapes (`\u00B0`) — the ESP32 sometimes mangles raw UTF-8.
- **Confirm with the user before adding new physical hardware** (sensors, second servo, etc.). Most ideas can be explored in software first.

## Known Issues / Quirks

- Web page doesn't auto-sync across multiple devices. Open it on phone + laptop, change something on one, the other shows stale values until refresh. Fix with polling (`setInterval(fetch, 200)`) or WebSockets.
- Settings reset on power cycle. To persist, use the `Preferences` library (writes to flash).
- ESP32 only sees 2.4GHz WiFi, never 5GHz.
- If `Connecting...` hangs forever during upload, hold the BOOT button on the board.

## Roadmap / Ideas Backlog

Brainstormed but not yet built. Grouped by effort.

### Software-only (no new hardware)

**Quick wins**
- [ ] **Polling for live state** — page auto-refreshes angle/speed every 200ms; multiple devices stay in sync
- [ ] **Stop All button** — master kill that freezes servo + LED but keeps server running
- [ ] **Persist settings to flash** via `Preferences` library
- [ ] **mDNS** — access at `esp32.local` instead of typing IP. ~3 lines of code, huge UX win.
- [ ] **Status dashboard** — uptime, free RAM, WiFi RSSI, IP, current state. Looks professional.
- [ ] **PWM brightness for LED** — fade instead of just on/off; brightness slider
- [ ] **Breathing LED effect** — slow sine-wave fade in/out
- [ ] **Action log** — last 50 actions in memory, displayed as a feed in the UI

**Servo as data display**
- [ ] **Clock hand** — NTP fetches real time; servo points to current hour. Most "complete" small project on this list.
- [ ] **WiFi signal strength meter** — `WiFi.RSSI()` → servo angle. Walk around with phone, watch needle move. ~5 lines.
- [ ] **Stock/crypto price gauge** — fetch from free API, map to angle
- [ ] **Weather vane** — fetch wind direction, point servo at it

**Sequences and choreography**
- [ ] **Wave hello / nervous tic / sleep** preset buttons — choreographed motion via state machine
- [ ] **Macro recorder** — record manual movements, replay them
- [ ] **Pomodoro mode** — servo sweeps slowly across 25 minutes as visual progress; LED celebrates at end
- [ ] **Morse code transmitter** — type message, LED blinks it
- [ ] **Schedule** — "wiggle every hour on the hour" via NTP

**Web/API improvements**
- [ ] **JSON API endpoints** — `/api/state` returns full JSON; enables external scripts (Python, Node) to control it
- [ ] **WebSocket instead of polling** — instant bidirectional updates
- [ ] **Captive portal for WiFi setup** — first-boot AP mode lets user pick network without hardcoding credentials
- [ ] **OTA updates** — push new firmware over WiFi, no USB needed

### Mechanical (servo as actuator)

- [ ] Page-turner arm
- [ ] Pet feeder (rotating chute)
- [ ] Light switch flipper (strap to wall switch)
- [ ] Door knocker / bell ringer (audible notification system)
- [ ] "In a meeting" indicator flag

### Combined / Bigger projects

- [ ] **Two ESP32s talking** — slider on one controls servo on other across WiFi
- [ ] **Webhook trigger** — IFTTT/Discord/email pings ESP32, servo waves
- [ ] **Reaction time game** — LED at random intervals, web button measures reaction time, servo points at best score
- [ ] **Two-player matching game** — Player 1 sets angle, Player 2 must match within 5° before timer

### Developer experience

- [ ] Switch to **VS Code + PlatformIO** — proper editor, IntelliSense, real debugger, cleaner project structure
- [ ] Add a **README.md** — wiring diagram, endpoint list, screenshots
- [ ] **GitHub Actions** to compile-check the sketch on push (using `arduino-cli`)

## Anti-Goals

Things explicitly *not* on the roadmap, to avoid scope creep:

- Cloud services (AWS IoT, Firebase, etc.) — defeats the point of the local-first design
- Mobile native app — the web UI works on mobile already
- Voice control / Alexa integration — not interesting yet
- Mesh networking, multiple boards beyond a 2-device experiment
- 3D-printed enclosures — not until the project is "done"

## When in Doubt

- Default to extending the existing pattern, not introducing a new one
- Prefer one more endpoint over restructuring the architecture
- Ask before recommending hardware purchases
- Show the smallest possible diff that demonstrates the idea