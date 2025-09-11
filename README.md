# REDESIGN IN PROGRESS!
**SIGNIFICANT CHANGES TO THE CODE WILL BE MADE!**\
As such I will be working towards a v3 release for the program\
v3 will not be backwards compatible with v2 hardware without significant changes to either the hardware or software!\
\
As I write this I've got the first PCBs back today, In the fairly near future this will add a touch screen interface and customisable buttons. \
\
In the nearish future this will allow options to be saved to the controller (small iteration of the PCB)\
\
In the distant future this may work its way towards wireless play, CV/gate & direct audio out, multi-ribbon designs and maybe more (major iteration(s) of the PSB)\
\
If there is enough interest I **may** release the diagram/PCB files, some things I need to work out before then.\
I also plan on getting the main body machined from a CNC, so those files may find their way here as well.
# Quantonium/AR2M
![The full controller](https://github.com/CraCaNN/AR2M/blob/main/media/Quantonium.jpg)
The Quantonium (Quantized-Trautonium) is a MIDI based ribbon controller, inspired by the [Modulin](https://www.youtube.com/watch?v=QaW5K85UDR0) made by [Wintergatan](https://www.youtube.com/@Wintergatan).\
It's also a partial alternative to [Doepfers R2M](https://doepfer.de/R2M.htm) (Ribbon to MIDI system which is no longer in available), with the main difference being that this project is MIDI only and no CV\
\
Quantonium is the full instrument name\
AR2M (Arduino Ribbon to MIDI) is the codebase name

This project is made up of the following:
* 1x wood plank, or something else to put the Arduino, ribbon and pressure pads on.
* 1x 500mm soft pot
* 1x 600mm FSR ribbon
* 2x Force Sensitive Resistors (Rated for 0-10kg)
* 1x Tiny 2040 (the Tiny 2350 should work and I can confirm that the program compiles when using the `Raspberry Pi Pico 2` board, will confirm if it actually works when I've built another instrument using the Tiny 2350)

The codebase is made up of 2 main sections, the music logic code, and the user interface\
The music logic handles what note to play, when to change note, and anything that sends MIDI. This makes up the core functionality of the controller.\
The user interface code allows the user to interact with the instrument, setting the key, and changing ribbon behaviour. Technically this aspect of the code & hardware isn't required to operate the controller but makes it much easier to interface with and see what is happening with the instrument.

The softpot (main ribbon/gets position) controls which note you play, by default this is Quantized to the key of C
- The key can be changed at any point
- There is also a pitch bend only mode where only pitch bend after the first note (Requies a patch that works around this feature in your choice of synth)

An FSR (Force Sensitive Resistor) ribbon is positioned underneath the softpot and sends aftertouch depending on how hard you press down on the ribbon, it also helps to prevent **ghost notes**\
\
2 smaller FSR pads are positioned at the end of the stick where you hold the instrument and by default send MIDI CC on CC 1&2.
- These can be changed to send pitch bend instead where pressing on the left pad pitches down, whilst the right pad pitches up
- The FSR pads are not nessasery to play the instrument but **the FSR ribbon IS**
![The head of the controller with the pressure pads](https://github.com/CraCaNN/AR2M/blob/main/media/quantonium%20head%20and%20pads.jpg)

The total cost of parts is ~£100 if you build it from scratch, although I plan on making a sellable version at some point\
I may sell these as kits or as completed units or both

**Ghost Notes** are a result of not playing the instrument correctly, where the instrument thinks you are playing a note further down the ribbon than you actually are.\
To play without creating ghost notes you need to put your finger right on the middle of the position ribbon as lying it across a section will result in ghost notes

This project is designed around [Pimoronis Tiny 2040](https://shop.pimoroni.com/products/tiny-2040?variant=39560012300371) since they're well priced, very compact and have USB-C unlike the official Picos
![The head of the controller with an active note](https://github.com/CraCaNN/AR2M/blob/main/media/quantonium%20head.jpg)

# Updating/Uploading the Controllers Firmware
If you want to update your controller to the latest firmware\
Follow [this](https://github.com/CraCaNN/AR2M/wiki/Updating-the-controller) guide in the wiki

# Compiling the Code
For people who want to modify the code and upload this to the controller and are familiar with the Arduino (C++) language\
Follow [this](https://github.com/CraCaNN/AR2M/wiki/Compiling) guide in the Wiki
