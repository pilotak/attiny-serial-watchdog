# ATtiny412 Serial Watchdog

3-relay controller with watchdog, fan/temperature, and manual modes. Receive-only serial interface at 9600 baud.

## Pin Assignment

| Pin | Function |
|-----|----------|
| PA1 | Relay 2 (active HIGH) |
| PA2 | Relay 3 (active HIGH) |
| PA3 | Relay 1 (active HIGH) |
| PA6 | Temperature sensor (NTC ADC or DS18B20 OneWire) |
| PA7 | USART0 RX (serial input) |
| PA0 | UPDI (programming) |

## Serial Protocol

9600 baud, 8N1, ASCII, `\n` terminated, receive-only.

Relay numbering: 1=Relay1(PA3), 2=Relay2(PA1), 3=Relay3(PA2)

| Command | Description | Example |
|---------|-------------|---------|
| `P` | Ping - reset watchdog timer, turn watchdog relay back ON | `P` |
| `T=nn` | Set target temperature in C | `T=25` |
| `H=nn` | Set hysteresis in C | `H=3` |
| `W=nnnn` | Set watchdog timeout in seconds (1–65535) | `W=30` |
| `D=nnnn` | Set watchdog restart delay in seconds (0–65535, relay OFF duration after timeout) | `D=5` |
| `Rn=X` | Set relay n (1-3) mode: W=watchdog, I=inverse watchdog, F=fan, M=manual | `R1=W` |
| `Rn=ON` | Relay n ON (manual mode only) | `R1=ON` |
| `Rn=OFF` | Relay n OFF (manual mode only) | `R1=OFF` |
| `Rn=nnnn` | Relay n ON for nnnn seconds then auto-off | `R2=60` |

Settings (T, H, W, relay modes) are saved to EEPROM automatically on change.

## Relay Modes

- **Manual (M):** ON/OFF via command, or ON with auto-off timeout. Default mode.
- **Watchdog (W):** Stays ON. Turns OFF if no ping received within timeout, then automatically turns back ON after the restart delay (`D`). Also turns back ON immediately on next ping. Starts ON when mode is set or on boot.
- **Inverse Watchdog (I):** Stays OFF. Turns ON if no ping received within timeout. Turns back OFF on next ping. Starts OFF on boot or mode set.
- **Fan (F):** ON when temp >= target+hysteresis, OFF when temp <= target-hysteresis.

Default assignment: Relay1=Watchdog, Relay2=Manual, Relay3=Fan.

Watchdog relays start ON at boot; all others start OFF. Watchdog timer starts immediately on boot.

## NTC Circuit

- 10k NTC between PA6 and GND
- 10k fixed resistor between PA6 and VCC
- Hot = low ADC, Cold = high ADC

> A DS18B20 OneWire sensor can be used instead of NTC (mutually exclusive, selected at build time).
