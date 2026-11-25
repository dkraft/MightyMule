# MightyMule

ESP32 MQTT controller for a Mighty Mule gate: encoder position, endstop home, and FWD/REV outputs.

Copy `esp32_MQTT_MightyMule1/secrets.h.example` to `esp32_MQTT_MightyMule1/secrets.h` and fill in Wi-Fi plus MQTT. `secrets.h` is gitignored.

## Pins

| GPIO | Function |
|---|---|
| 16 | Encoder (interrupt, falling) |
| 17 | Endstop (interrupt, falling) — first pulse homes and zeros the counter |
| 18 | FWD |
| 19 | REV |

FWD/REV are compiled out when `DEBUG` is 1 so USB serial is safe. Set `#define DEBUG 0` to drive the motor pins.

## MQTT (`AIO_USERNAME`, default `Mule1`)

| Topic | Dir | Meaning |
|---|---|---|
| `Mule1/feeds/positionset` | sub | Target encoder count |
| `Mule1/feeds/position` | pub | Current count |
| `Mule1/feeds/endstop` | pub | Homed flag |
| `Mule1/feeds/direction` | pub | Direction |
| `Mule1/feeds/MQTTConnections` | pub | Reconnect count |

Broker defaults to LAN MQTT on port 1883.

## Libraries

Adafruit MQTT, GOTStateMachine.

## Flash

Board: ESP32. Sketch folder: `esp32_MQTT_MightyMule1`.
