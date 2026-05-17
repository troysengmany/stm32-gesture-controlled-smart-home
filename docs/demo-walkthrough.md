# Demo Walkthrough

This is the way I explain the project during a demo.

This is my STM32 gesture-controlled smart home prototype. The laptop runs a Python program that uses OpenCV and MediaPipe to track my hands through the webcam. The Python program converts the gestures into UART commands and sends them to the STM32. The STM32 then updates the OLED display and controls the physical outputs.

The OLED has a main menu with Lights, Fan, and Door. I can swipe to move through the menu and select the mode I want.

In Lights mode, my left hand selects which LED I am controlling. One finger selects Light 1, two fingers selects Light 2, and three fingers selects Light 3. My right hand controls brightness using pinch distance. A small pinch sends a low brightness value, and a wider pinch sends a higher brightness value. The STM32 receives commands like `L1:500` and updates the PWM duty cycle.

In Fan mode, the right-hand pinch controls the fan speed. The left hand can unlock the fan control, lock the current speed, or turn the fan off. The STM32 uses a GPIO output to control the fan through a transistor driver circuit.

In Door mode, finger-count commands control the servo. One command closes the door and another opens it. The STM32 changes the timer compare value for the servo signal to move the door.

The main engineering part of this project is the full pipeline: camera input, gesture recognition, serial communication, STM32 command parsing, OLED updates, PWM control, GPIO output, and real hardware movement.
