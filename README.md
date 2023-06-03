# -ESP8266_SolarBatteryController
SolarBatteryController based on ESP8266 (NodeMCU 1.0 ESP-12E) Module\
\
**universal modules**\
modLogger: local ring log\
modNTPClient: remote ntp time client\
modTimer: local timer and runtime\

**hardware support modules**\
modPVClient: query solar system stecagrid 2500\
modEMeter: query electricity meter from vzlogger\
modIO: application specivic hardware control \
\
**further code**
modStatic_WebInterface: web interface\
modStatic_Wifi: network (require WLANZUGANGSDATEN.h which defines STASSID and STAPSK for your network) \
prgController: automatic solar battery controller\
SolarBatteryController: program initialization\

# License

This Source Code Form is subject to the terms of the Mozilla Public \
License, v. 2.0. If a copy of the MPL was not distributed with this \
file, You can obtain one at https://mozilla.org/MPL/2.0/. \
THE SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY \
Author: Peter Lorenz 

# You found the code useful? Donate!

Paypal webmaster@peter-ebe.de \
[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=DZUZXE2WCJU4U) 

# Hardware
No specific hardware building instructions are published. Everything you do is on your own risk.\
Make sure to use safe hardware components and cabling!\
\
A schematic representation of the hardware responding to that software can be found here.\
<img src="https://raw.githubusercontent.com/Delphi-FPC-Lazarus/ESP8266_SolarBatteryController/main/Hardwareschema/Hardware_Grundschema.png">


