# esp-tylo
 Esphome controller for your Tylö sauna.

**WORK IN PROGRESS, DO NOT USE**

Features:

- [X] **Shows current temperature.**\
*Initial value is 0°. Relies on updates that are sent by the heater.*\
*My heater/panel is configured for celsius and therefor only tested for celsius.* 
- [X] **Enable/disable heater.**\
*Status is unknown until the first toggle of heater or lights.*
- [X] **Enable/disable lights.**\
*Status is unknown until the first toggle of heater or lights.*
- [ ] **Find a way to update status without toggling heater/lamps.**
- [ ] **Control of temperature setting.**\
*Low priority.*


## Services
Following services can be called from Home Assistant:

**esp_tylo_heater_on** - Heater on.\
**esp_tylo_heater_off** - Heater off.\
**esp_tylo_light_on** - Lights on.\
**esp_tylo_light_off** - Lights off.\


## Setup
My setup ("man-in-the-middle"):
* LOLIN S2 Mini.
* Generic (2$) MAX485-module.
* 2x RJ10-ports.

I've wired the RJ10-ports together, pin to pin.\
Each pin should correspond with the same pin with the ports oriented in the same way.\
1 to 1, 2 to 2, 3 to 3, 4 to 4.

Connections between MAX485 ports.
* A from port to A on module.
* B from port to B on module.

Connections between MAX485 and ESP32.

* RO to GPIO35.
* DE/RE to GPIO33.
* DI to GPIO36.
* VCC to VBUS.
* GND to GND.

**BE CAREFUL AS 2 OF THE PINS PROVIDE 12V/GND AND WILL FRY YOUR ESP32 OR IN WORST CASE DAMAGE YOUR TYLÖ-HEATER!**

![Pinout provided by Tylö](https://github.com/vkoppelo/esp-tylo/blob/main/Images/pinout.jpg)

