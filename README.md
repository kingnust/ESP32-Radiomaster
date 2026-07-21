# ESP32-S3 RadioMaster Command Bridge

This firmware runs on an ESP32-S3 SuperMini and accepts control from a phone
web app, a laptop over Wi-Fi TCP, or USB serial. It outputs RC channel frames to
a RadioMaster/EdgeTX UART input or an ELRS/CRSF path. CRSF is the default for
the phone-controller build, with SBUS still selectable for trainer-port setups.

## Phone web app

The ESP32 creates an access point:

- SSID: `RM-Pocket-Bridge`
- Password: `12345678`
- App URL: `http://192.168.4.1/`
- WebSocket control endpoint: `ws://192.168.4.1:7778/rc`

Connect your phone to the ESP32 Wi-Fi network, open the app URL, then use the
two on-screen sticks and RadioMaster-style switch row. The app sends RC frames
at about 50 Hz. Firmware output becomes active when the on-screen `TRN PHONE`
or `PHONE CONTROL` switch is ON. Turning it OFF, closing the app, losing Wi-Fi,
or letting packets go stale returns the output to safe channel values.

Direct Wi-Fi channel layout:

- `CH1`-`CH4`: roll, pitch, throttle, yaw
- `CH5`: arm
- `CH6`: low acro, middle angle, high optical-flow hover
- `CH7`: airmode
- `CH8`: 0-180 degree positional servo
- `CH9`/`CH10`: task selector and momentary execute
- `CH11`-`CH16`: task parameters and reserved controls

Trainer packets are EdgeTX inputs rather than receiver outputs. `TR1`-`TR5`
carry roll, pitch, throttle, yaw, and arm. `TR6` is the phone heartbeat/task
pulse, `TR7` is the servo, `TR8` is the flight mode, and `TR13` is the EdgeTX
phone-takeover switch.

## Preventing radio-stick and phone-stick clash

EdgeTX must map trainer inputs into the reserved receiver outputs. Standard
CRSF carries 16 receiver channels, so logical trainer channels `CH17`-`CH26`
cannot be sent directly over a normal RC frame.

For SBUS trainer mode:

1. Select `SBUS` in the phone app.
2. Set the connected RadioMaster serial port to `SBUS Trainer`.
3. Create logical switch:

```text
L01 = TR13 > 1700
```

4. Add replacement mixer lines:

```text
CH6   REPLACE  TR8  switch=L01   ; acro / angle / hover
CH8   REPLACE  TR7  switch=L01   ; phone servo slider
CH11  REPLACE  TR1  switch=L01   ; phone roll
CH12  REPLACE  TR2  switch=L01   ; phone pitch
CH13  REPLACE  TR3  switch=L01   ; phone throttle
CH14  REPLACE  TR4  switch=L01   ; phone yaw
CH15  REPLACE  TR5  switch=L01   ; phone arm
CH16  REPLACE  TR6  switch=L01   ; heartbeat / task pulse
```

Map the RadioMaster Pocket rear roller (`S1`) to normal output `CH8`. With
phone control off, that roller drives the drone servo. During takeover, `TR7`
replaces `CH8`, so the app slider drives the same servo. Use a three-position
radio switch on `CH6`: low acro, middle angle, high optical-flow hover.

The FC recognizes trainer-phone control only while transmitted `CH16` carries
the reserved 1250 us heartbeat or a task pulse. A normal midpoint `CH16`, a
stopped app, stale Wi-Fi, or safe frames cannot activate phone takeover.

The momentary task codes are `1300` (forward preset), `1400` (right preset),
`1600` (home), `1800` (task 1), and `2000` (task 2). The FC accepts one task
edge only after seeing the 1250 us idle heartbeat again. Position requests
expire after 500 ms if MTF02P position hold is not active, so a request made
while disarmed or during a sensor fault cannot execute later without a new
button press.

Direct mode uses a versioned ESP-NOW packet with CRC, sequence tracking, a
shared link ID, 100 Hz updates, and a 180 ms FC timeout. The shared ID rejects
packets from a differently configured bridge before they can override the
RadioMaster. `DirectRcLinkId` in `include/Config.h` must match
`ESPFC_DRONE_PROTO_DIRECT_RC_LINK_ID` in the FC firmware if either value is
changed.

Keep props off for all setup and latency testing.

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

For SBUS trainer use, set the RadioMaster UART/serial-port function to `SBUS Trainer`. That is the
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

CRSF is selected by default and is also available with `PROTO CRSF`, but only
use it if the RadioMaster port or ELRS-side connection expects CRSF at 420 kbaud.
If your menu only lists `SBUS Trainer`, select `SBUS` in the phone app or use
`PROTO SBUS`.

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
output keeps roll, pitch, and yaw centered, throttle low, and every trainer
auxiliary low. This explicitly turns off `TR5` arm, `TR6` takeover marker, and
`TR13` phone takeover instead of leaving an old command latched.

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
