# AR2M
The AR2M (Arduino Ribbon to MIDI) controller is a cheaper DIY alternative to the [Modulin](https://www.youtube.com/watch?v=QaW5K85UDR0) made by [Wintergatan](https://www.youtube.com/@Wintergatan).

This project is made up of the following:
* 1x wood plank, or something else to put the Arduino, ribbon and pressure pads on.
* 1x 500mm soft pot
* 2x Force Sensitive Resistors (Rated for 0-10kg)
* 1x Arduino Uno R3
* 1x Sparkfun MIDI Shield
* For the Sparkfun MIDI Shield I would recommend getting passthrough headers instead of the standard ones.

You can play this instrument using one hand on just the ribbon like Wintergatans Modulin (Mode 2) but I would recommend using the pads at the end of the MIDI stick (Mode 1) since the ribbon can get false readings when you touch the ribbon.

Mode 1 requires both the ribbon and the left pressure pad to be touched in order to play notes. I would choose the note on the ribbon then activate the ribbon using the left pad since this prevents false readings. This is the default mode.
Mode 2 only needs the ribbon to be touched in order to play, the left pad does still work in this mode and just acts as the MIDI channel pressure. This does work but I would recommend applying pressure quickly to the ribbon and quickly off since this prevents a lot of false readings.
You can toggle this behaviour with the middle button on the MIDI shield or you can set the default mode in the options section of the code.

![AR2M Layout](https://raw.githubusercontent.com/CraCaNN/AR2M/main/AR2M%20diagram.png)

This is because the soft pot ribbon when lightly pressing on the ribbon the values can be read further up the ribbon than the actual position of your finger.
To get around this I have put a pressure pad on the end of the instrument to activate the controller. 
![Pressure end](https://github.com/CraCaNN/AR2M/blob/main/pressure%20close%20up.jpg)
Initially I did have another pressure ribbon on top of the position ribbon, but this increased the amount of pressure required to play.
There is probably a way around this, but this was an easier way of doing it.

There are 2 pressure pads used, the left one for activating the controller AND sending the aftertouch MIDI command.
The right one sends the modulation MIDI command.

You can change the use case, but I have the aftertouch hooked up to the vibrato and the modulation hooked up to the ADSR of the volume to change the time required to increase and decrease volume, a bit like a sustain pedal.

# Arduino Uploading
1. Download the AR2M folder and open in Arduino.
2. Make sure you have the [MIDI library](https://github.com/FortySevenEffects/arduino_midi_library) downloaded via the Arduino library manager, not the MIDI USB library.
3. Make sure that the switch on the MIDI shield is set to program otherwise the upload will fail!
4. Select your Arduino and upload.
