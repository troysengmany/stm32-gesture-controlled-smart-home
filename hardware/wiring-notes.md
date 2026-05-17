# Wiring Notes

This is the wiring used for the STM32 gesture-controlled smart home prototype.

| Part | Connection | Notes |
|---|---|---|
| OLED SCL | PB6 / I2C1_SCL | I2C clock line |
| OLED SDA | PB7 / I2C1_SDA | I2C data line |
| OLED VCC | 3.3V or 5V depending on OLED module | Use the voltage supported by the module |
| OLED GND | GND | Common ground |
| Light 1 LED | PB4 / TIM3_CH1 | PWM output with current-limiting resistor |
| Light 2 LED | PB5 / TIM3_CH2 | PWM output with current-limiting resistor |
| Light 3 LED | PB3 / TIM2_CH2 | PWM output with current-limiting resistor |
| Servo signal | PA8 / TIM1_CH1 | Servo door control |
| Fan driver input | PA7 | Goes to transistor driver input, not directly to fan power |
| USB serial | USART2 | Laptop sends commands to STM32 |

The LEDs should each use a current-limiting resistor. The fan should not be powered directly from an STM32 GPIO pin. I used the STM32 pin as a control signal for a transistor driver circuit, with the fan powered from a separate supply when needed. The fan circuit should share ground with the STM32, and a flyback diode should be placed across the fan terminals to protect the circuit from voltage spikes.

For the servo, the signal line goes to PA8. If the servo draws too much current from the board, it should use a separate 5V supply with a shared ground. The firmware uses timer compare values to move between the closed and open positions.
