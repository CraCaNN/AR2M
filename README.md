# AR2M/Quantonium

The AR2M(Arduino Ribbon to MIDI, the code base name)/Quantonium(Quantized-Trautonium, the full instrument name) is a cheaper DIY alternative to the [Modulin](https://www.youtube.com/watch?v=QaW5K85UDR0) made by [Wintergatan](https://www.youtube.com/@Wintergatan).

This project is made up of the following:
* 1x wood plank, or something else to put the Arduino, ribbon and pressure pads on.
* 1x 500mm soft pot
* 1x500 or 600mm FSR ribbon
* 2x Force Sensitive Resistors (Rated for 0-10kg)
* 1x Tiny 2040 (the Tiny 2350 should work and I can confirm that the program compiles when using the `Raspberry Pi Pico 2` board, will confirm if it actually works when I've built another instrument using the Tiny 2350)

Initially I had trouble with getting the force and position ribbons working together but they now both work so you can play this instrument with one hand
There are 2 additional force pads at the end of the instrument and control MIDI CC (control channels) 1&2
The FSR (force sensitive resistor) sends MIDI AT (aftertouch) messages
The total cost of parts is less than £100 although I plan on making a sellable version at some point, I have designed my own PCB for this
I may sell these as kits or as completed units

To play without creating ghost notes you need to put your finger right on the middle of the position ribbon as lying it across a section will result in ghost notes

This project is designed around [Pimoronis Tiny 2040](https://shop.pimoroni.com/products/tiny-2040?variant=39560012300371) since they're well priced, very compact and have USB-C unlike the official Picos

# Updating/Uploading the Controllers Firmware
If you want to update your controller to the latest firmware, follow [this](https://github.com/CraCaNN/AR2M/wiki/Updating-the-controller) guide in the wiki

# Compiling the Code
For people who want to modify the code and upload this to the controller and are familiar with the Arduino language
Follow [this](https://github.com/CraCaNN/AR2M/wiki/Compiling) guide in the Wiki
