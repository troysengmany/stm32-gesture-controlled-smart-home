# STM32 Gesture-Controlled Smart Home

Built by **Troy Sengmany**

This project is a computer-vision controlled smart home prototype. A Python program uses a webcam, OpenCV, and MediaPipe to track hand gestures, then sends UART commands to an STM32 microcontroller. The STM32 controls an OLED menu, three dimmable LEDs, a servo-powered door, and a transistor-driven fan.

I built this project to combine the software side of computer vision with the hardware side of embedded systems. The goal was not just to blink LEDs, but to make a complete working system where hand gestures on a laptop create real physical output on a microcontroller.

## What the system does

The project has three main control modes:

| Mode | Gesture behavior | STM32 output |
|---|---|---|
| Lights | Left hand selects Light 1, 2, or 3. Right-hand pinch controls brightness. | PWM dimming for three LEDs |
| Fan | Right-hand pinch controls fan speed. Left hand can unlock, lock, or turn the fan off. | GPIO-based fan speed control through a transistor circuit |
| Door | Finger-count command opens or closes the door. | Servo motor position control |
| Menu | Swipe gestures move through the OLED menu and select a mode. | OLED menu navigation |

## System overview

```text
Webcam
  -> Python gesture tracking with OpenCV and MediaPipe
  -> Gesture classification
  -> UART command string
  -> STM32 firmware
  -> OLED, LEDs, fan, and servo door
```

The Python side handles the webcam and gesture detection because that work is easier to do on a laptop. The STM32 side handles the hardware because it is better suited for PWM, GPIO, timers, UART, and I2C output.

## Features I implemented

- Real-time hand tracking using OpenCV and MediaPipe
- Automatic STM32 serial-port detection on macOS
- UART communication between Python and STM32
- OLED menu system with Lights, Fan, and Door screens
- PWM brightness control for three separate LEDs
- Servo control for a small door mechanism
- Fan control using a transistor driver circuit
- Gesture cooldown timing to prevent accidental repeated commands
- Back gesture support to return to the main menu
- Clean separation between laptop-side control and embedded firmware

## Hardware used

- STM32 Nucleo board using STM32 HAL firmware
- SSD1306 I2C OLED display
- Three LEDs with current-limiting resistors
- Micro servo for the door mechanism
- DC fan driven through a transistor circuit
- External power where needed for motor/fan loads
- USB serial connection between laptop and STM32

The STM32 firmware is written in C with STM32 HAL. The Python controller is written with OpenCV, MediaPipe, and PySerial.

## Serial command protocol

The Python script sends simple newline-terminated commands to the STM32.

| Command | Meaning |
|---|---|
| `UP` | Move OLED menu selection up |
| `DOWN` | Move OLED menu selection down |
| `SELECT` | Enter the selected menu item |
| `BACK` | Return to the main OLED menu |
| `LIGHT:1` | Select Light 1 on the OLED lights screen |
| `LIGHT:2` | Select Light 2 on the OLED lights screen |
| `LIGHT:3` | Select Light 3 on the OLED lights screen |
| `L1:0` to `L1:999` | Set Light 1 brightness |
| `L2:0` to `L2:999` | Set Light 2 brightness |
| `L3:0` to `L3:999` | Set Light 3 brightness |
| `FAN:0` to `FAN:999` | Set fan speed |
| `DOOR:1` | Close the servo door |
| `DOOR:0` | Open the servo door |

I used plain-text commands because they are easy to debug in a serial monitor and easy to extend with new features.

## How to run the Python controller

Install the Python dependencies:

```bash
python3 -m pip install -r requirements.txt
```

Run the gesture controller:

```bash
python3 src/python/gesture_control.py
```

The script automatically looks for the STM32 serial port. On macOS, the board normally appears as a `/dev/cu.usbmodem...` or `/dev/tty.usbmodem...` device. Press `q` in the OpenCV window to quit.

## STM32 firmware

The firmware in `src/stm32/main.c` handles:

- UART command parsing
- OLED screen updates
- LED PWM compare updates
- Servo PWM positioning
- Fan software PWM output
- Menu state tracking

This repository includes the main project firmware file. The full STM32CubeIDE workspace can be recreated by making a CubeIDE project with the same peripherals and copying this `main.c` into `Core/Src/main.c`.

The firmware expects SSD1306 OLED driver files such as `ssd1306.c`, `ssd1306.h`, `fonts.c`, and `fonts.h` to be included in the STM32CubeIDE project.

## Pin mapping

| Component | STM32 connection | Purpose |
|---|---|---|
| OLED SCL | PB6 / I2C1_SCL | OLED clock |
| OLED SDA | PB7 / I2C1_SDA | OLED data |
| Light 1 LED | PB4 / TIM3_CH1 | PWM brightness |
| Light 2 LED | PB5 / TIM3_CH2 | PWM brightness |
| Light 3 LED | PB3 / TIM2_CH2 | PWM brightness |
| Servo signal | PA8 / TIM1_CH1 | Door position |
| Fan driver input | PA7 | Fan control signal |
| Laptop connection | USART2 over USB serial | Gesture commands |

## What I learned

This project helped me practice system integration across software, firmware, and hardware. I had to tune gesture thresholds, avoid accidental gesture triggers, parse serial commands reliably, configure STM32 timers, update an OLED display, and make the physical hardware respond consistently during a live demo.

The most useful part was connecting everything end to end: webcam input, gesture logic, UART transmission, embedded C parsing, PWM/GPIO output, and real hardware behavior.

## Files in this repo

```text
src/python/gesture_control.py   Laptop-side OpenCV/MediaPipe gesture controller
src/stm32/main.c                STM32 firmware logic
hardware/wiring-notes.md        Hardware connections and safety notes
docs/demo-walkthrough.md        Short explanation for presenting the project
docs/system-design.md           Engineering notes about the architecture
docs/troubleshooting.md         Common issues and fixes
```
