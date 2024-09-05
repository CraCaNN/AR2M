# AR2M
The AR2M (Arduino Ribbon to MIDI) controller is a cheaper DIY alternative to the [Modulin](https://www.youtube.com/watch?v=QaW5K85UDR0) made by [Wintergatan](https://www.youtube.com/@Wintergatan).

This project is made up of the following:
* 1x wood plank, or something else to put the Arduino, ribbon and pressure pads on.
* 1x 500mm soft pot
* 1x500 or 600mm FSR ribbon
* 2x Force Sensitive Resistors (Rated for 0-10kg)
* 1x Tiny 2040 (the Tiny 2350 should work when earlephilhower as the pinout is unchanged, will confirm this when I've built a new instrument using a Tiny 2350)

Initially I had trouble with getting the force and position ribbons working together but they now both work so you can play this instrument with one hand
There are 2 additional force pads at the end of the instrument and control MIDI CC (control channels) 1&2
The FSR (force sensitive resistor) sends MIDI AT (aftertouch) messages
The total cost of parts is less than £100 although I plan on making a sellable version at some point, I have designed my own PCB for this
I may sell these as kits or as completed units

To play without creating ghost notes you need to put your finger right on the middle of the position ribbon as lying it accross a section will result in ghost notes

This project is designed around [Pimoronis Tiny 2040](https://shop.pimoroni.com/products/tiny-2040?variant=39560012300371) since they're well priced, very compact and have USB-C unlike the official Picos

# Uploading to the Pico

1. From the [releases](https://github.com/CraCaNN/AR2M/releases) on the Latest release page (or pre-release if you want to try early but possibly unstable features) download the file called `AR2M.U2F`
2. Whilst holding the BOOT button on the controller plug the device into your computer, or whilst the controller is still on, hold the BOOT button then press RESET
3. On Windows File Explorer should automatically pop up a Window with the device `RPI-RP2`. On MacOS it should appear in the smae name in File Explorer
4. Drag the `AR2M.U2F` file into the `RPI-RP2`. The controller should then reboot and the new firmware should now be used by the controller.

# Compiling the Code
For people who want to modify the code and upload this to the controller.\
**No support will be provided for any modified code, even if the modifications are within the settings list of variables**\
**This guide assumes that you know the basics of Arduino and you also have Arduino IDE 2 installed**
1. Download the AR2M folder and open in Arduino IDE.
2. Make sure you have the following libraries installed. All can be found by searching the names in the Arduino library manager:
-  [MIDI library](https://github.com/FortySevenEffects/arduino_midi_library) not the MIDI USB library.
-  [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
-  [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
-  [Adafruit TinyUSB Library](https://github.com/adafruit/Adafruit_TinyUSB_Arduino)
3. Install earlephilhower's Pico board by following [this](https://arduino-pico.readthedocs.io/en/latest/install.html#installing-via-arduino-boards-manager) guide
4. Under Board Manager select `Raspberry Pi Pico/RP2040`, NOT THE ONE MADE ARDUINO
5. Select the Pico that's connected the computer, it will not show up properly unless you have put the Pico into BOOT mode
6. Under Tools->USB Stack: Select `Adafruit TinyUSB`
7. Make any changes you want to the code and upload!

## To Work On
At some point I want to add support for an EEPROM chip. This way settings can be stored and loaded from the chip. This also means that the user can change the defaults without having to compile the code.
