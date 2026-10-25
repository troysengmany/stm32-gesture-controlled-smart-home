# UART Command Protocol

The Python controller sends newline-terminated text commands at **38,400 baud**. The STM32 firmware receives one character at a time, builds a command string, and handles it when a newline or carriage return arrives.

| Command | Firmware action |
|---|---|
| `UP` | Move the main OLED menu selection up |
| `DOWN` | Move the main OLED menu selection down |
| `SELECT` | Open the selected OLED menu screen |
| `BACK` | Return to the main menu |
| `LIGHT:1` / `LIGHT:2` / `LIGHT:3` | Select the active light in the lights screen |
| `L1:0` ... `L1:999` | Set Light 1 PWM compare value |
| `L2:0` ... `L2:999` | Set Light 2 PWM compare value |
| `L3:0` ... `L3:999` | Set Light 3 PWM compare value |
| `FAN:0` ... `FAN:999` | Set fan speed value |
| `DOOR:1` | Close the servo door |
| `DOOR:0` | Open the servo door |

The LED values are clamped to the 0-999 range in firmware before the timer compare value is updated.
