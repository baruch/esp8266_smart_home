# ESP8266 based Smart Home System

## The goal
Make fully automated OTA for ESP8266 under NodeMCU.

The software upload and update to the ESP8266 is trivial via UART. This way is not applicable for mass-deployed modules.

There are geek-driven solutions for OTA: the human (engineer or skilled person) should initiate file upload one-by-one to every particular module. This is not applicable too: the human is unable to do that for hundreds and thousands of modules, tens of files per module.

Therefore, module should initiate itself on the server (register) and request updates on periodic basis. Once developer releases the update to the server, every single ESP8266 module should update itself asynchronously.

This is what this project about.

## Implementation
**ESP8266 side:** Lua files to be uploaded to the module during "manufacturing" process.

**Server side:** The server files to be installed (copied) to the local server during initial installation process.

## Initial configuration and use
On the server run `smart-home-server.py file_repository`.

The ESP8266 module once powered up will run as AP (access point named SHMxxxxxx, where x-es stand for ID number of the module). Connect ot the module via any browser to address 192.168.4.1. Fill mandatory (and optional if needed) fields. Click Save button. That's it.

The ESP8266 module will restart and connect the server looking for the update. Once new files placed into the file repository the module will download them and run the code.
