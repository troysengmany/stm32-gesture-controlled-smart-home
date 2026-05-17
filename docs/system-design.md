# System Design

This project is split into two parts: a laptop-side vision controller and an STM32-side embedded controller.

The laptop is responsible for camera input and gesture recognition. It uses OpenCV to capture frames from the webcam and MediaPipe to detect hand landmarks. After the landmarks are detected, the Python code checks hand position, finger count, and pinch distance to decide what command should be sent.

The STM32 is responsible for hardware control. It receives UART commands from the laptop, parses the command strings, updates the OLED menu state, and changes the outputs for the LEDs, fan, and servo.

I kept the serial protocol simple on purpose. Each command is a short text string ending in a newline. This made the project easier to debug because I could print every command from Python and also test commands manually with a serial terminal.

The main states in the system are Menu, Lights, Fan, and Door. Menu mode handles navigation. Lights mode maps pinch distance to LED brightness. Fan mode maps pinch distance to fan speed and includes lock/off gestures. Door mode maps finger-count commands to servo positions.

The most important design challenge was preventing accidental inputs. I added cooldown timing for swipes, fan updates, and door commands. I also used a hold gesture for going back to the main menu so the system would not exit modes from a quick accidental finger count.
