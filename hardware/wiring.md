# Hardware Wiring

The recovered application source uses the following STM32 connections.

| Function | STM32 pin | Peripheral / role |
|---|---|---|
| OLED SCL | PB6 | I2C1 SCL |
| OLED SDA | PB7 | I2C1 SDA |
| Light 1 | PB4 | TIM3 CH1 PWM |
| Light 2 | PB5 | TIM3 CH2 PWM |
| Light 3 | PB3 | TIM2 CH2 PWM |
| Door servo | PA8 | TIM1 CH1 PWM |
| Fan driver | PA7 | GPIO software PWM to transistor driver |
| Computer link | USART2 | 38,400 baud UART over the board's USB/ST-Link serial interface |

## Power notes

The LEDs use current-limiting resistors. The servo and fan/motor should not be powered directly from a microcontroller GPIO pin. A separate suitable supply and a common ground are recommended. The fan is driven through a transistor stage, with a flyback diode across an inductive load where applicable.
