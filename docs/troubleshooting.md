# Troubleshooting

If the Python program cannot find the STM32, first check that the board is connected over USB. On macOS, run:

```bash
ls /dev/cu.*
ls /dev/tty.*
```

The STM32 usually appears as a device with `usbmodem`, `STLink`, or `STM` in the name.

If the camera does not open, macOS may be blocking camera access. Go to System Settings, then Privacy & Security, then Camera, and allow camera access for Terminal, VS Code, or whichever app is running the script.

If the OLED does not display anything, check VCC, GND, SCL, SDA, and the OLED I2C address. The firmware expects the SSD1306 driver files to be included in the CubeIDE project.

If the LEDs do not dim, confirm that the LED pins match the timer channels in the firmware and that each LED has a resistor. Also check that the Python script is sending commands like `L1:500`.

If the servo moves incorrectly, the pulse values may need to be tuned for the specific servo. The servo should also have enough power, and the servo power supply must share ground with the STM32.

If the fan does not spin, check the transistor wiring, the fan power supply, the flyback diode direction, and the shared ground between the fan supply and STM32.
