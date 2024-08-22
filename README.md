# AR2M
The AR2M (Arduino Ribbon to MIDI) controller is a cheaper DIY alternative to the [Modulin](https://www.youtube.com/watch?v=QaW5K85UDR0) made by [Wintergatan](https://www.youtube.com/@Wintergatan).

This project is made up of the following:
* 1x wood plank, or something else to put the Arduino, ribbon and pressure pads on.
* 1x 500mm soft pot
* 1x500 or 600mm FSR ribbon
* 2x Force Sensitive Resistors (Rated for 0-10kg)
* 1x Tiny 2040 (the Tiny 2350 should work when earlephilhower has updated the board library as the pinout has been updated, will confirmn this when I can)

Initially I had trouble with getting the force and position ribbons working together but they now both work so you can play this instrument with one hand
There are 2 additional force pads at the end of the instrument and control MIDI CC (control channels) 1&2
The FSR (force sensetive resistor) sends MIDI AT (aftertouch) messages
The total cost of parts is less than £100 although I plan on making a sellable version at somepoint, I have designed my own PCB for this
I may sell these as kits or as compleated units

To play without creating ghost notes you need to put your finger right on the middle of the position ribbon as lying it accross a section will result in ghost notes

This project is designed around [Pimoronis Tiny 2040](https://shop.pimoroni.com/products/tiny-2040?variant=39560012300371) since they're well priced and have USB-C unlike the official Picos

# Uploading to the Pico

1. From the [releases](https://github.com/CraCaNN/AR2M/releases) on the Latest release page (or pre-release if you want to try early but possibly unstable features) download the file called `AR2M.U2F`
2. Whilst holding the BOOT button on the controller plug the device into your computer, or whilst the controller is still on, hold the BOOT buttin then press RESET
3. On Windows File Explorer should automatically pop up a Window with the device `RPI-RP2`. On MacOS it should appear in the smae name in File Explorer
4. Drag the `AR2M.U2F` file into the `RPI-RP2`. The controller should then reboot and the new firmware should now be used by the controller.

# Compiling the Code
For people who want to modify the code and upload this to the controller. **No support will be provided if you modify and upload the code** 
### finish later
1. Download the AR2M folder and open in Arduino.
2. Make sure you have the [MIDI library](https://github.com/FortySevenEffects/arduino_midi_library) downloaded via the Arduino library manager, not the MIDI USB library.
3. Make sure that the switch on the MIDI shield is set to program otherwise the upload will fail!
4. Select your Arduino and upload.

# In the Future


