# AR2M
The AR2M (Arduino Ribon to MIDI) cheaper alternative to the Modulin made by Wintergatan.

Pictures will follow. This project is made up of the following:
* 1x wood plank, or something else to put the ribbon and pressure pads on.
* 1x 500mm soft pot
* 2x Pressure pads
* 1x Arduino Uno R3
* 1x Sparkfun MIDI Shield


I have designed this project around the useage of VCV Rack 2 to emulate the hardware synths, but since MIDI is used you can use this with a keyboard or whatever you want that supports MIDI.

Unlike Wintergatans Modulin you will need 2 hands to play the controller 1 for the positional ribbon and 1 to activate the controller.

This is because the soft pot ribbon when lightly pressing on the ribbon the values can be read furhter up the ribbon than the actual position of your finger.
To get around this I have put a pressure pad on the end of the instrument to activate the controller. 

Initially I did have another pressure ribbon ontop of the position ribbon but this increaced the amount of pressure required to play.
There is probably a way around this but I did not take that path.

There 2 pressure pads used, 1 for activating the controller and sending the aftertouch MIDI command.
The other send the modulation MIDI command.

You can change the use case but I have the aftertouch hooked up to the vibroto and the modulation hooked up to the ADSR module to change the time required to get up to and lose volume.

Using the buttons onboard the MIDI shield I have also connected them to VCV rack thorugh the Start, Stop and Continue MIDI commands. See an explanation within the VCV patch.

I have included the patch that I have created to work with the AR2M controller



