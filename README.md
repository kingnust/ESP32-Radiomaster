# ESP32-S3 RadioMaster Command Bridge

This firmware runs on an ESP32-S3 SuperMini and accepts simple text commands
from a laptop over Wi-Fi TCP or USB serial. It outputs RC channel frames to a
RadioMaster/EdgeTX UART input using SBUS by default, with CRSF available as an
optional output protocol.

## Default wiring

- ESP32-S3 `GND` to RadioMaster `GND`
- ESP32-S3 `GPIO5` TX to RadioMaster UART RX/trainer input
- ESP32-S3 `GPIO4` RX to RadioMaster UART TX, optional
- Use 3.3 V logic only

If the RadioMaster connector is labeled as a 5V UART port, treat `5V` as a
power pin only. Do not connect `5V` to any ESP32 GPIO. For first tests, leave
the RadioMaster `5V` pin disconnected and power the ESP32 from USB. Later, you
may power the ESP32 from RadioMaster `5V` only by connecting it to the ESP32
board's `5V`/`VIN` pin, not `3V3`, and only if the UART TX/RX idle levels have
been confirmed safe with a meter or logic analyzer.

Change pins in `include/Config.h` if your SuperMini is wired differently.

## RadioMaster serial-port setting

Set the RadioMaster UART/serial-port function to `SBUS Trainer`. That is the
port mode that accepts external channel data and lets EdgeTX mix/send it onward
through the radio link.

In the model Trainer page, use `Master/Serial` for this UART path. If the radio
is set to a jack/buddy-box trainer mode, the UART input may be ignored.

SBUS is sent as non-inverted 100 kbaud 8E2 serial by default. If the RadioMaster
does not detect trainer input even though wiring and menus are correct, change
`SbusInverted` in `include/Config.h` to `true`, rebuild, and test again, or use
`SBUSINV 1` at runtime.

Do not use `Telemetry In`, `Telem Mirror`, `AUX1`, or `LUA` for the normal
channel-control path:

- `Telemetry In` is for receiving telemetry data, not RC channel input.
- `Telem Mirror` outputs/copies telemetry, not laptop commands.
- `AUX1` is a generic auxiliary serial function, not trainer channel input.
- `LUA` is for Lua-script serial access and would require a custom radio script.

CRSF is still available with `PROTO CRSF`, but only use it if the RadioMaster
port is configured for a CRSF trainer/input mode. If your menu only lists
`SBUS Trainer`, keep the firmware on SBUS.

## Wi-Fi command connection

The ESP32 creates an access point:

- SSID: `RM-Pocket-Bridge`
- Password: `12345678`
- TCP endpoint: `192.168.4.1:7777`

Example from a laptop:

```text
PING
PROTO SBUS
CH 11 2000
ARM 1
KEEPALIVE
STOP
```

Send `KEEPALIVE` or new channel commands at least once per second. If the host
goes quiet, output is disabled and safe channel values are sent briefly.

From Windows PowerShell, after connecting the laptop to the ESP32 Wi-Fi network:

```powershell
python tools\wifi_send.py
```

Then type commands such as:

```text
STATUS
LINK 1
TESTRAW 1 2000 10000
```

For active control with automatic keepalives:

```powershell
python tools\wifi_send.py --keepalive
```

## USB serial monitor

The firmware enables ESP32-S3 USB CDC serial on boot. Open PlatformIO's serial
monitor at `115200`. The board prints startup information once, then stays
quiet unless you send a command such as `HELP` or `STATUS`.

## Commands

- `HELP`
- `STATUS`
- `PING` or `KEEPALIVE`
- `ARM 1|0`
- `LINK 1|0`
- `STOP`
- `SAFE`
- `CH <11..26> <988..2012>`
- `TEST <11..26> <988..2012> <100..10000 ms>`
- `TESTRAW <TR 1..16> <988..2012> <100..10000 ms>`
- `FRAME <16 values for CH11..CH26>`
- `RATE <10..100>`
- `PROTO CRSF|SBUS`
- `SBUSINV 1|0`
- `ADDR <hex byte>` for CRSF only, for example `ADDR EE` or `ADDR C8`
- `UNLOCK_PRIMARY I_UNDERSTAND`
- `LOCK_PRIMARY`

By default, channels 1-10 are locked so laptop commands cannot overwrite existing
radio controls. Host commands can write logical receiver channels 11-26. Safe
output keeps all trainer inputs neutral, so the bridge will not silently pull
`TR5`/`CH15` low.

`TR1`, `TR2`, etc. are EdgeTX trainer-input channels, not receiver output
channels. The firmware maps laptop/output commands like this by default:

- `CH 11 ...` -> trainer input `TR1`
- `CH 12 ...` -> trainer input `TR2`
- `CH 13 ...` -> trainer input `TR3`
- `CH 14 ...` -> trainer input `TR4`
- `CH 15 ...` -> trainer input `TR5`
- `CH 16 ...` -> trainer input `TR6`
- `CH 17 ...` -> trainer input `TR7`
- `CH 18 ...` -> trainer input `TR8`
- `CH 19 ...` -> trainer input `TR9`
- `CH 20 ...` -> trainer input `TR10`
- `CH 21 ...` -> trainer input `TR11`
- `CH 22 ...` -> trainer input `TR12`
- `CH 23 ...` -> trainer input `TR13`
- `CH 24 ...` -> trainer input `TR14`
- `CH 25 ...` -> trainer input `TR15`
- `CH 26 ...` -> trainer input `TR16`

So to control receiver output channel 11, set the RadioMaster mix:

```text
CH11 source = TR1
```

For a simple CH11 check, send:

```text
TEST 11 2000 5000
```

This holds the SBUS channel value for 5 seconds, then returns to safe output.

For receiver channel 26, set the RadioMaster mix:

```text
CH26 source = TR16
```

Then test it with:

```text
TEST 26 2000 5000
```

If `CH11 source = TR1` does not move, test the raw trainer input directly:

```text
LINK 1
TESTRAW 1 2000 10000
```

If that still does not move `TR1`, flip SBUS polarity without rebuilding:

```text
SBUSINV 1
LINK 1
TESTRAW 1 2000 10000
```
