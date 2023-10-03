# AR2M
The AR2M (Arduino Ribon to MIDI) controller is a cheaper DIY alternative to the [Modulin](https://www.youtube.com/watch?v=QaW5K85UDR0) made by [Wintergatan](https://www.youtube.com/@Wintergatan).

This project is made up of the following:
* 1x wood plank, or something else to put the Arduino, ribbon and pressure pads on.
* 1x 500mm soft pot
* 2x Force Sensetive Resistors (Rated for 0-10kg)
* 1x Arduino Uno R3
* 1x Sparkfun MIDI Shield
* For the Sparkfun MIDI Shield I would recommend getting passthough headers instead of the standard ones.

I have designed this project around the useage of VCV Rack 2, but since MIDI is used, you can use this with a keyboard or whatever you want that supports MIDI.

Unlike Wintergatans Modulin you will need 2 hands to play the controller 1 for the positional ribbon and 1 to activate the controller.

![AR2M Layout](https://raw.githubusercontent.com/CraCaNN/AR2M/main/AR2M%20diagram.png)

This is because the soft pot ribbon when lightly pressing on the ribbon the values can be read furhter up the ribbon than the actual position of your finger.
To get around this I have put a pressure pad on the end of the instrument to activate the controller. 
![Pressure end](https://github.com/CraCaNN/AR2M/blob/main/pressure%20close%20up.jpg)
Initially I did have another pressure ribbon ontop of the position ribbon but this increaced the amount of pressure required to play.
There is probably a way around this but this was an easier way of doing it.

There are 2 pressure pads used, the left one for activating the controller AND sending the aftertouch MIDI command.
The right one sends the modulation MIDI command.

You can change the use case but I have the aftertouch hooked up to the vibroto and the modulation hooked up to the ADSR module to change the time required to increase and decrease volume.

Using the buttons onboard the MIDI shield I have also connected them to VCV rack thorugh the Start, Stop and Continue MIDI commands.
![Physical to VCV](https://github.com/CraCaNN/AR2M/blob/main/AR2M%20physc%20to%20vcv.drawio.png)


I have included the patch that I have created to work with the AR2M controller

# Arduino Uploading
1. Download the AR2M folder and open in Arduino.
2. Make sure you have the [MIDI library](https://github.com/FortySevenEffects/arduino_midi_library) downloaded via the Arduino library manager, not the MIDI USB library.
3. Make sure that the switch on the MIDI shield is set to program otherwise the upload will fail!
4. Select your Arduino and upload.
