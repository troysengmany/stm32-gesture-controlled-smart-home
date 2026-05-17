# Serial Protocol

The Python controller sends newline-terminated text commands over UART at 38400 baud. The STM32 reads characters into a buffer until it receives a newline or carriage return, then it compares the received command to the supported command list.

| Command | STM32 action |
|---|---|
| `UP` | Move the OLED menu selection up |
| `DOWN` | Move the OLED menu selection down |
| `SELECT` | Enter the highlighted OLED menu item |
| `BACK` | Return to the main menu |
| `LIGHT:1` | Select Light 1 on the OLED lights screen |
| `LIGHT:2` | Select Light 2 on the OLED lights screen |
| `LIGHT:3` | Select Light 3 on the OLED lights screen |
| `L1:<0-999>` | Set Light 1 brightness |
| `L2:<0-999>` | Set Light 2 brightness |
| `L3:<0-999>` | Set Light 3 brightness |
| `FAN:<0-999>` | Set fan speed |
| `DOOR:1` | Close the door |
| `DOOR:0` | Open the door |

Using readable commands made it easier to test and debug the system while building it. It also made the interface easy to extend with new outputs.
