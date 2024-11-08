/*
<CraCaN, turns analogue readings from softpots and FSRs into MIDI notes.>
Copyright (C) <2024>  <CraCaN>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, 
or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

//Core libraries
#include <Arduino.h>
#include <Wire.h>

//OLED libraries
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>

//MIDI/MIDI related libraries
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>


//CONFIG OPTIONS//

//These options change how the controller behaves and you can modify to set the default behaviour on boot
//change the 'size' of the note sections on the ribbon. Increase the number to make the sections bigger or decrease the number to make the sections smaller. make sure you have enough notes in the note list if you make the section size smaller
int sectionSize = 32;
//Recommended sizes are:
//32 can fit 4 octaves 3 notes. (Default)
//28 can fit 5 octaves.
//23 can fit 6 octaves and a note
//The sectionSize does not take into account ribbonDeadZone so you may have a partial note at the start of the ribbon

//offset for in key notes, allows you to play in any key
int offSetKey;
//change to play in higher or lower octaves
int octaveOffSet;
//offset for chromatic scale
int offSetSharp = 27;

//Consider using only one of the 3 options below as they share variable data
//Disables MIDI CC on the left and right pressure pads, and changes it into a fake pitch bend wheel
bool padPitchBend = false;
//Setting to true allows pitch bend to work within each note. A special synth patch will be required
bool allowInNotePB = false;
//Enbale trautonium mode - send continuous pitch bend for a traditional trautonium glide. Must have a suitable patch set in your synthesizer
bool trautoniumMode = false;

//sends note velocity data depending on how hard the note was pressed down, if set to false a note velocity of 127 will always be send
bool dynamicVelocity = false;

//Control change pads
int lftPotControlChanel = 1;  //Chooses which MIDI CC channel to send information out of from the left pad.
int rhtPotControlChanel = 2;  //Chooses which MIDI CC channel to send information out of from the right pad.

//Poly Play. Designed around the diatonic scale
int polyCount = 1;
//Should be between 1-4. Absolute max is 16! Number of poly channels sent. Could be higher if you wanted to make a chord longer than 4 notes.
//Many things will probably break if this becomes more than 4

//DYNAMIC VARS//Variables that can change but not while playing the controller//
int polyChord[4] = { 0, 12, 24 };
//Some chords support either 3 or 4, this prevents the user from increasing the poly send count above the supported chord count
int maxPolyCount = 3;

//Set poly plays channel output mode
bool polyPlayMultiCh = true;
//if true each chord is sent on its own MIDI channel. Meant for allowing multiple instances of mono synthesizers so gliding can work correctly


//Debuging
bool debug = false;  //set to true to show more variables on the OLED

//determines what list is currently in use, 0 is diatonic, 1 is chromatic
bool noteListSelect = 0;
//END OF CONFIG OPTIONS//

//SENSOR CONFIG//
//If making your own instrument, you may need to adjust the following variables

//Position Ribbon
//The ribbons deadzone size. Since when the ribbon isn't in use the value read drops to a low number, usually less than 10. This is one of the ways how the controller determines if the ribbon isn't in use. this value works for my ribbon: (https://coolcomponents.co.uk/products/softpot-membrane-potentiometer-500mm)
const int ribbonDeadZone = 32;

//Aftertouch ribbon
const int lowerPrTr = 500;  //set the minimum amount of pressure needed for the pressure ribon to activate
const int prBuffer = 600;   //the amount of pressure required to send any aftertouch
const int upperPres = 900;  //the pressure amount where AT is at max

//CC Pads
//set the max MIDI CC values for the pads at that pad reading
const int upperPresPads = 900;
//END OF SENSOR OPTIONS//

//BOARD OPTIONS//
//Change these options to match how you have wired up your own instrument

const int softpotPin = A0;         //position ribbon pin
const int pressurePadPinLft = A1;  //pressure pad right pin
const int pressurePadPinRht = A2;  //pressure pad right pin
const int pressureRibbon = A3;     //aftertouch ribbon

//Button Pins
const int btn1 = 7;
const int btn2 = 6;
const int btn3 = 5;

//RGB LED for Tiny2040
const int rLed = 18;
const int gLed = 19;
const int bLed = 20;

//OLED SDA/SCL pins
const int oledSDApin = 0;
const int oledSCLpin = 1;
//END OF BOARD OPTIONS//


//CODE BEGIN//
//Mostly Global variables required for performing music logic and the interface logic

//MUSIC VARS//Variables that are expected to change under normal use
int rawPosRead;       //rawPosRead is the reading used before it is processed to be sent as a MIDI message, I need both to be able to detect more accuratly where the position is
int softPotReading;   //this is the rawPosRead deivided by the sectionsize variable to get a final position of where your finger is
int ribbonIndicator;  //Makes the turning on/off of the DLED look neater when the ribbon is activated

//set the default MIDI channel to send
const int MIDIchannel = 1;  //1-15 are valid! This is not a config option since it really doesn't need to be changed

int rawPresRead;   //The raw pressure data from the main ribbon
int globalAT;      //The formatted data for pressure ribbon to send to aftertouch
int prevglobalAT;  //Prevent sending the same pressure values

int velocityWait;  //Value to wait for until MIDI is actually sent if using dynamic velocity
int velocitySent;  //note changing when dynamic velocity is true will use the velocity determined on the first note on

int pressurePotRhtRead;      //formatted right pressure pot read
int prevPressurePotRhtRead;  //previous value of formatted data to prevent repeated sends
int rawRightPad;             //the raw right pad data for visualisation

int pressurePotLftRead;      //formatted left pressure pot read
int prevPressurePotLftRead;  //previous value of formatted data to prevent repeated sends
int rawLeftPad;              //the raw left pad data for visualisation

bool isActive = false;  //is determined by if there is an active MIDI note.
bool isValid = false;   //is determined if the controllers ribbons are in a state where it would send a MIDI note but doesnt care if MIDI messages are actually being sent

//store the total offset to prevent recalculating it for every note
int totalOffSet = 0;

//status of the buttons
int btn1Sts = HIGH;  //state of button 1, LOW is pressed
int btn2Sts = HIGH;  //state of button 2, LOW is pressed
int btn3Sts = HIGH;  //state of button 3, LOW is pressed

//Variables pitch bending
int pitchBendValue;  //amount to pitch bend by
int prevPitchBend;   //previous values to prevent duplicate messages

//Variables used for trautonium mode
int initialPos;
int bendAmount;

//Variables used for in note pitch bend
int inNoteStartPos;
int PBwait = 0;

//noteNums is the note(s) that is currently playing
int noteNums[4];

//prevNoteNums is also the current note(s) playing but is not updated when a new note is played. This helps the controller to determine which note to turn off
int prevNoteNums[4];

//LED
int sleepLed = 255;        //fade the led when OLED is inactive
bool sleepLedDir = false;  //allows the led to fade up and down without going to max brightness

//MISC
int superBreak = 0;  //Helps me get out of deep sub menus


//NOTE LISTS//Lists to reference from to get the note to play
//These lists are what the code references when deciding what note to send + any offsets applied

//Diatonic scale of notes in the key of C from C2 to C6
const int noteNumberList[] = {
  36, 36, 38, 40, 41, 43, 45, 47, 48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71,
  72, 74, 76, 77, 79, 81, 83, 84, 86, 88, 89, 91, 93, 95, 96, 98, 100, 101, 103, 105, 107, 108
};
//C2 Is repeated twice since the start of the ribbon is cut off by the dead zone

//Chromatic scale of notes
const int noteNumListSharp[] = {
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96
};


//OLED VARS//Variables needed for viualisation purposes

//the list below contains every midi note number in letter form. Inteanded for visualisation purposes
const char* noteNumNameNoSharp[] = {
  "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1",
  "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",
  "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
  "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
  "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
  "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
  "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
  "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",
  "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",
  "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8",
  "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9", "MAX", "MAX", "MAX", "MAX",
  "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX"
};
//deffinatly a better way of doing this but it works for now
//no this wasn't typed out manually, I made a python script to do it for me

//For use when you are changing key in changeKey()
const char* keyList[] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };  //inteanded for visualisation purposes

//make it easier for the visuliser to know what note is being played without having to go through a lengthy looking line of code every time
int currentNote = 0;

int displayCount = 0;    //determines what is shown on the OLED
int displaySpecial = 0;  //For showing screens non-normal such as key change
int displayPage = 0;     //Detrmines special OLED pages such as menus or key/oct change

int blinkPixel = false;  //one pixel on the screen blinks to show when the screen is redrawn, used to be more visable on the UNO R3 but now blinks very quickly thanks to the pico
//even if this variable is true pixel blink will only occur if debug is also on

//Show the position of the ribbon as a bar
const int showNoteSecSize = 126 / ((1024 - ribbonDeadZone) / sectionSize);  //gets the size in number of pixels to display to the OLED
//width of the ribbon on the OLED / ((total ribbon size - the ribbon deadzone) / the note section size)
//Essentially how big would each note section would be if you shrunk the ribbon down to the size of the screen

//The position of the ribbon indicator
int ribbonDisplayPos = 0;

//END OF VARIABLE SETUP//



//Create display instance
Adafruit_SSD1306 display(128, 32, &Wire, -1);

//Send MIDI over USB instead of serial
Adafruit_USBD_MIDI usb_midi;  // USB MIDI object
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);


void setup() {
  USBDevice.setManufacturerDescriptor("Quantized Trautonium/Arduino Ribbon to MIDI");
  USBDevice.setProductDescriptor("Quantonium/AR2M");  //This is what shows up under your MIDI device selection, if you want to have a bit of fun :)

  //Set LED pins to outputs
  pinMode(rLed, OUTPUT);
  pinMode(gLed, OUTPUT);
  pinMode(bLed, OUTPUT);

  //display.setRotation(2);//uncomment to flip the display

  //Serial.begin(31250);//Serial can be enabled without interupting MIDI, but currently nothing uses Serial

  //Initiate OLED
  Wire.setSDA(oledSDApin);  //Set the I2C library to use specific pins instead of the default
  Wire.setSCL(oledSCLpin);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  //0x3D for 128x64, 0x3C for 128x32
  display.clearDisplay();                     //clear display
  display.display();

  display.setTextColor(1);  //in this library for the OLED 0 is black and 1 is white

  //Start MIDI
  MIDI.begin();

  //set pins for buttons as inputs
  pinMode(btn3, INPUT_PULLDOWN);
  pinMode(btn2, INPUT_PULLDOWN);
  pinMode(btn1, INPUT_PULLDOWN);
  digitalWrite(btn3, HIGH);
  digitalWrite(btn2, HIGH);
  digitalWrite(btn1, HIGH);

  digitalWrite(softpotPin, HIGH);

  displayCount = 200;  //prevents the liveInfo screen showing instantly after boot. Looks neater in my opinion

  //The Boot screen, not actually nescassary, just looks cool and is currently the only time you can see the current version running, should probably add an about page on the menu
  display.setCursor(0, 0);
  display.print("Quantonium/AR2M");
  display.setCursor(0, 12);
  display.print("Version 2.4.5");  // will update this in every version hopefully
  display.setCursor(0, 24);
  display.print("Starting in 2");
  display.display();
  delay(500);
  analogWrite(bLed, 255);
  display.print("..");
  display.display();
  delay(500);
  analogWrite(gLed, 255);
  display.print("1");
  display.display();
  delay(500);
  analogWrite(rLed, 255);
  display.print("..");
  display.display();
  delay(500);
}

//START OF MUSIC FUNCTIONS//

//get and sends pressure/aftertouch. Also doesn't send data if its the same value as before
void readPressure() {
  rawPresRead = analogRead(pressureRibbon);  //get data for activation of controller

  //I've deliberatly added a 'buffer' so that the pad activates the controller without sending any AT commands unless the pad is held down further
  globalAT = map(constrain(rawPresRead, prBuffer, upperPres), prBuffer, upperPres, 0, 127);  //change data for use as a MIDI CC

  if (globalAT != prevglobalAT) {   //only send an update if a change is detected in the pressure
    if (polyPlayMultiCh == true) {  //send down multiple MIDI channels if that option is enabled
      for (int i = 1; i <= polyCount; i++) {
        MIDI.sendAfterTouch(globalAT, MIDIchannel + i - 1);
        prevglobalAT = globalAT;  //store to compare with later to prevent duplicate messages
      }
    } else {
      MIDI.sendAfterTouch(globalAT, MIDIchannel);  //otherwise only one channel is used
      prevglobalAT = globalAT;
    }
  }

  //Red LED indication, will light up if the pressure has surpassed the buffer, will then increase in intensity when AT is being sent
  if (rawPresRead > lowerPrTr) {  //if left pad is in use turn on red led
    if (globalAT > 0) {           //if pad is being held down more than the 'buffer'
      analogWrite(rLed, map(globalAT, 0, 127, 205, 0));
    } else {  //otherwise show a base value to show that the pad is in use but not being held down very firmly
      analogWrite(rLed, 205);
    }
  } else {  //otherwise turn off the red led
    analogWrite(rLed, 255);
  }
}

//gets the left and right pressure pad data and sends as a MIDI CC channel or as pitch bend
void readModWheel() {
  //get the current values
  rawRightPad = analogRead(pressurePadPinRht);
  rawLeftPad = analogRead(pressurePadPinLft);
  if (padPitchBend == false) {

    pressurePotRhtRead = map(constrain(rawRightPad, 20, upperPresPads), 20, upperPresPads, 0, 127);  //read the right pressure pad and change from 0-upperPressure to 0-127
    //gets the data, add a 'buffer' with constrain, map to fit with a MIDI CC channel
    pressurePotLftRead = map(constrain(rawLeftPad, 20, upperPresPads), 20, upperPresPads, 0, 127);  //read the left pressure pad and change from 0-upperPressure to 0-127

    if (pressurePotRhtRead != prevPressurePotRhtRead) {  //only send if the current value is different to the previous value
      for (int i = 1; i <= polyCount; i++) {
        if (polyPlayMultiCh == true) {
          MIDI.sendControlChange(rhtPotControlChanel, pressurePotRhtRead, MIDIchannel + i - 1);  //send modulation command
        } else {
          MIDI.sendControlChange(rhtPotControlChanel, pressurePotRhtRead, MIDIchannel);  //send modulation command
        }
      }
      prevPressurePotRhtRead = pressurePotRhtRead;  //make the prev reading the same as the current one so to fail the check until the readings change
    }

    if (pressurePotLftRead != prevPressurePotLftRead) {  //only send if the current value is different to the previous value
      for (int i = 1; i <= polyCount; i++) {
        if (polyPlayMultiCh == true) {
          MIDI.sendControlChange(lftPotControlChanel, pressurePotLftRead, MIDIchannel + i - 1);  //send modulation command
        } else {
          MIDI.sendControlChange(lftPotControlChanel, pressurePotLftRead, MIDIchannel);  //send modulation command
        }
      }
      prevPressurePotLftRead = pressurePotLftRead;
    }
  }

  else {                                                                                      //use pads as a pitch bend
    pitchBendValue = map(rawRightPad, 0, 1024, 0, 4096) - map(rawLeftPad, 0, 1024, 0, 4096);  //gets the actual pitch bend value to send
    if (pitchBendValue != prevPitchBend) {                                                    //only send if the current value is different to the previous value
      if (polyPlayMultiCh == true) {                                                          //send though multiple channels if that option is enabled
        for (int i = 1; i <= polyCount; i++) {
          MIDI.sendPitchBend(pitchBendValue, MIDIchannel + i - 1);
        }
      } else {  //otherwise just send though a single channel
        MIDI.sendPitchBend(pitchBendValue, MIDIchannel);
      }
      prevPitchBend = pitchBendValue;  //store current value to compare later
    }
  }
}




//gets the current position of the position ribbon also calculates any offsets, deals with indicator LED as well
void readPosition() {
  rawPosRead = analogRead(softpotPin);            //get raw position value
  totalOffSet = offSetKey + (octaveOffSet * 12);  //this was in noteSelect() but I need this value updated constantly and not just when the controller is active

  //fade up the green DLED
  if (rawPosRead > ribbonDeadZone) {  //turn on green led if postion ribbon is being use
    if (ribbonIndicator < 60) {       //the green led is so bright that I don't want to turn it up that bright compared to the red led
      ribbonIndicator = ribbonIndicator + 10;
      analogWrite(gLed, ribbonIndicator * -1 + 255);
    }
  } else {                      //otherwise fade down the green DLED
    if (ribbonIndicator > 0) {  //but dont make the variable negative, dont think it matters if it does though
      ribbonIndicator = ribbonIndicator - 10;
      analogWrite(gLed, ribbonIndicator * -1 + 255);
    }
  }
}

//gets the ribbon position and the note number. Deals with poly modes as well
void noteSelect() {
  softPotReading = rawPosRead / sectionSize;  //divide the reading by sectionSize to fit bigger 'sections' on the ribbon
  if (noteListSelect == 0) {                  //if playing in diatonic
    for (int i = 1; i <= polyCount; i++) {    //only loop for the number of poly notes that there are

      noteNums[i - 1] = constrain(noteNumberList[softPotReading] + totalOffSet + polyChord[i - 1], 0, 127);  //get the MIDI note number(s) to send and also prevent the number from exceeding the MIDI note number range
    }
  } else {  //if playing in chromatic
    for (int i = 1; i <= polyCount; i++) {
      noteNums[i - 1] = constrain(noteNumListSharp[softPotReading] + offSetSharp, 0, 127);  //get the MIDI note number(s) to send and also prevent the number from exceeding the MIDI note number range
    }
  }
}



//sends the MIDI note on command, expects a velocity value, deals with POLY mode
void noteOn(int velocity) {
  if (polyPlayMultiCh == true) {  //if playing in multiple channel mode
    for (int i = 1; i <= polyCount; i++) {
      MIDI.sendNoteOn(noteNums[i - 1], velocity, MIDIchannel + i - 1);
    }
  } else {
    for (int i = 1; i <= polyCount; i++) {  //otherwise send MIDI down the same channel
      MIDI.sendNoteOn(noteNums[i - 1], velocity, MIDIchannel);
    }
  }
}

//sends the MIDI note off command, deals with POLY mode
void noteOff() {
  if (polyPlayMultiCh == true) {  //if playing in multiple channel mode
    for (int i = 1; i <= polyCount; i++) {
      MIDI.sendNoteOff(prevNoteNums[i - 1], 127, MIDIchannel + i - 1);
    }
  } else {  //otherwise send MIDI down the same channel
    for (int i = 1; i <= polyCount; i++) {
      MIDI.sendNoteOff(prevNoteNums[i - 1], 127, MIDIchannel);
    }
  }
}

//Realistically the function bellow isn't needed but there could be a chance where the note is changed then the controller sends that note off on the wrong note. but to be on the safe side, this function exists
//sends the MIDI note off command for sending the note off of a current note, used to deactivate the proper notes when fully releasing the controller
void noteOffCurrrent() {

  if (polyPlayMultiCh == true) {
    for (int i = 1; i <= polyCount; i++) {  //if playing in multiple channel mode
      MIDI.sendNoteOff(noteNums[i - 1], 127, MIDIchannel + i - 1);
    }
  } else {  //otherwise send MIDI down the same channel
    for (int i = 1; i <= polyCount; i++) {
      MIDI.sendNoteOff(noteNums[i - 1], 127, MIDIchannel);
    }
  }
}


//stores the current note so when a new note is played the controller knows which note to turn off
void storeNote() {
  for (int i = 1; i <= polyCount; i++) {  //only run for the number of poly notes
    prevNoteNums[i - 1] = noteNums[i - 1];
  }
}




void activePB() {
  if (allowInNotePB == true) {
    if (PBwait < 20) {
      PBwait++;
    } else if (PBwait == 20) {
      inNoteStartPos = rawPosRead;
      PBwait++;
    } else {
      pitchBendValue = -constrain(map(inNoteStartPos - rawPosRead, -1024 / sectionSize, 1024 / sectionSize, -4096, 4096), -4096, 4096);  //Takes the difference from the starting point in each note and compares it to the current position and converts it to a useful number
      MIDI.sendPitchBend(pitchBendValue, 1);
    }
  }
}

//Reset pitch bend when about to play a note to stop weird sounds
void resetBend() {
  if (polyPlayMultiCh == true) {
    for (int i = 1; i <= polyCount; i++) {
      MIDI.sendPitchBend(0, MIDIchannel + i - 1);
    }
  } else {
    MIDI.sendPitchBend(0, MIDIchannel);
  }
  inNoteStartPos = 0;
  pitchBendValue = 0;
  PBwait = 0;
}

void trautonium() {
  pitchBendValue = (rawPosRead - initialPos) * 8;  //update the pitch bend value
  if (pitchBendValue != prevPitchBend) {           //only send pitch bend if the new value is different from the old value, prevents duplicate messages
    if (polyPlayMultiCh == true) {
      for (int i = 1; i <= polyCount; i++) {                      //send through multiple MIDI channels if needed
        MIDI.sendPitchBend(pitchBendValue, MIDIchannel + i - 1);  //send a pitch-bend value based on the inital position set earlier
      }
    } else {                                             //otherwise only send it down 1 channel
      MIDI.sendPitchBend(-pitchBendValue, MIDIchannel);  //send a pitch-bend value based on the inital position set earlier
    }
    prevPitchBend = pitchBendValue;  //Store the current pitch bend value to compare later
  }
  delay(5);  //Delay for stability, dont want to flood the DAW with messages even with the code that prevents multiple messages of the same value
}
//END OF MUSIC FUNCTIONS//

//START OF OLED CODE//

//update button status
void updateButtons() {
  btn1Sts = digitalRead(btn1);
  btn2Sts = digitalRead(btn2);
  btn3Sts = digitalRead(btn3);
}

//draw 3 white boxes to represent the buttons
void drawButtons() {
  display.fillRect(118, 0, 10, 10, 1);
  display.fillRect(118, 11, 10, 10, 1);
  display.fillRect(118, 22, 10, 10, 1);
}

//draw up, box and down shapes to show scrolling feature of main menu
void drawSideBar() {
  //Top/up button/arrow
  //the button
  display.fillRect(118, 0, 10, 10, 1);
  //the vertical part of the arrow
  display.drawLine(123, 1, 123, 8, 0);
  display.drawLine(122, 1, 122, 8, 0);
  //the pointers
  display.drawLine(123, 1, 126, 4, 0);
  display.drawLine(122, 1, 119, 4, 0);

  //The middle/select button
  //the button
  display.fillRect(118, 11, 10, 10, 1);
  //the horizontal part of the line
  display.drawLine(119, 15, 126, 15, 0);
  display.drawLine(119, 16, 126, 16, 0);
  //the pointer
  display.drawLine(119, 15, 122, 12, 0);
  display.drawLine(119, 16, 122, 19, 0);

  //the bottom/down button/arrow
  //The button
  display.fillRect(118, 22, 10, 10, 1);
  //the verticle part of the arrow
  display.drawLine(123, 23, 123, 30, 0);
  display.drawLine(122, 23, 122, 30, 0);
  //the pointers
  display.drawLine(122, 30, 119, 27, 0);
  display.drawLine(123, 30, 126, 27, 0);
}

//display one of 3 buttons being pressed, draws black boxes in the white boxes to simulate a press, works with drawButtons and drawSidebar, since there the same outlining shape
void drawPressed(int buttonPressed) {
  if (buttonPressed == 3) {
    display.fillRect(119, 23, 8, 8, 0);
    display.display();
    delay(200);  //delay to prevent repeat button presses and to show the animation on the OLED
  } else if (buttonPressed == 2) {
    display.fillRect(119, 12, 8, 8, 0);
    display.display();
    delay(200);
  } else if (buttonPressed == 1) {
    display.fillRect(119, 1, 8, 8, 0);
    display.display();
    delay(200);
  }
}

//display one of 3 buttons being pressed. Same as drawPressed() but can change the delay time if user is holding down the button
void drawDynamicPressed(int buttonPressed, int delayAmount) {
  if (buttonPressed == 3) {
    display.fillRect(119, 23, 8, 8, 0);
    display.display();
    delay(delayAmount);  //delay to prevent repeat button presses and to show the animation on the OLED
  } else if (buttonPressed == 2) {
    display.fillRect(119, 12, 8, 8, 0);
    display.display();
    delay(delayAmount);
  } else if (buttonPressed == 1) {
    display.fillRect(119, 1, 8, 8, 0);
    display.display();
    delay(delayAmount);
  }
}

//Draw a cross to show if a button press is unable to perform the requested action, is only meant to work with drawButtons
void drawCross(int buttonPressed) {
  if (buttonPressed == 3) {
    display.drawLine(119, 23, 126, 30, 0);
    display.drawLine(119, 30, 126, 23, 0);
    display.display();
    delay(200);  //delay to prevent repeat button presses and to show the animation on the OLED
  } else if (buttonPressed == 2) {
    display.drawLine(119, 12, 126, 19, 0);
    display.drawLine(119, 19, 126, 12, 0);
    display.display();
    delay(200);
  } else if (buttonPressed == 1) {
    display.drawLine(119, 1, 126, 8, 0);
    display.drawLine(119, 8, 126, 1, 0);
    display.display();
    delay(200);
  }
}

//Simplify some of the code, mostly in polySettings
void threeLine(const char* line1, const char* line2, const char* line3) {
  display.setCursor(0, 0);
  display.print(line1);
  display.setCursor(0, 12);
  display.print(line2);
  display.setCursor(0, 24);
  display.print(line3);
}

//inteanded for debugging
//blink one pixel on the display so I can see when it's being updated and if the program is frozen
void screenBlink() {
  if (debug == true) {  //only turn on if debugging is on
    if (blinkPixel == false) {
      display.drawPixel(127, 0, 0);
      blinkPixel = true;
    } else {
      display.drawPixel(127, 0, 1);
      blinkPixel = false;
    }
  }
}

//slowly fades the blue LED to indicate the device is still active but the OLED is off, Prevent burn in to preserve the pannels life
void sleep() {
  if (sleepLed == 255) {
    sleepLedDir = false;
  } else if (sleepLed == 130) {
    sleepLedDir = true;
  }

  if (sleepLedDir == true) {
    sleepLed++;
  } else {
    sleepLed--;
  }
  analogWrite(bLed, sleepLed);
}

void alert(const char* line1, const char* line2, const char* line3) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(line1);
  display.setCursor(0, 12);
  display.print(line2);
  display.setCursor(0, 24);
  display.print(line3);
  display.display();
  delay(3000);
}

//Select from a boolean choice, function needs to know the current value and the 2 options available for the user. Option 1 will return false, option 2 will return true
bool boolSelect(const char* property, bool currentOption, const char* option1, const char* option2) {
  const char* selector = " <<";
  String option1full;
  String option2full;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(property);
  drawButtons();

  if (currentOption == 0) {
    option1full = String(option1) + String(selector);
    option2full = String(option2) + String(" ");
  } else {
    option1full = String(option1) + String(" ");
    option2full = String(option2) + String(selector);
  }

  display.setCursor(0, 12);
  display.print(option1full);
  for (int i = 19 - option1full.length(); i != 0; i--) {
    display.print("-");
  }

  display.setCursor(0, 24);
  display.print(option2full);
  for (int i = 19 - option2full.length(); i != 0; i--) {
    display.print("-");
  }

  while (true) {
    updateButtons();
    drawButtons();
    if (btn1Sts == LOW) {
      drawCross(1);
      updateButtons();
    } else if (btn2Sts == LOW) {
      drawPressed(2);
      updateButtons();
      return 0;
    } else if (btn3Sts == LOW) {
      drawPressed(3);
      updateButtons();
      return 1;
    }
    display.display();
  }
}

//increase or decrease a value with a set max and min
int numberSelect(const char* property, int currentValue, const int minimum, const int maximum) {
  int holdCounter = 0;
  bool btnHold = false;
  int holdDelay = 200;
  while (true) {
    display.clearDisplay();
    drawButtons();
    display.setCursor(0, 0);
    display.print(property);
    //Scale text size/position depending on how long the value is in terms of digit length
    if (String(currentValue).length() == 1) {
      display.setTextSize(3);
      display.setCursor(0, 11);
    } else if (String(currentValue).length() == 2) {
      display.setTextSize(3);
      display.setCursor(0, 11);
    } else {
      display.setTextSize(3);
      display.setCursor(0, 11);
    }

    display.print(currentValue);
    display.setTextSize(1);
    display.setCursor(66, 0);
    display.print("| Exit -");
    display.setCursor(66, 12);
    display.print("| Inc --");
    display.setCursor(66, 24);
    display.print("| Dec --");
    display.display();
    updateButtons();
    if (btn1Sts == LOW) {
      drawCross(1);
      return currentValue;
    } else if (btn2Sts == LOW) {
      if (currentValue != maximum) {
        drawDynamicPressed(2, holdDelay);
        currentValue++;
      } else {
        drawCross(2);
      }
    } else if (btn3Sts == LOW) {
      if (currentValue != minimum) {
        drawDynamicPressed(3, holdDelay);
        currentValue--;
      } else {
        drawCross(3);
      }
    }
    //If the user is holding down the button then speed up the rate of change
    if ((btn2Sts == LOW) || (btn3Sts == LOW)) {
      holdCounter++;
    } else {  //if let go of the button reset count
      holdCounter = 0;
    }

    if (holdCounter >= 30) {
      holdDelay = 15;
    } else if (holdCounter >= 15) {
      holdDelay = 50;
    } else if (holdCounter >= 5) {
      holdDelay = 100;
    } else if (holdCounter < 5) {  //essentially if the button is let go, reset the delay amount
      holdDelay = 200;
    }
    display.display();
  }
}

//Main menu, uses a list/scroll system instead of the old page inteface
void newMenu() {
  displayCount = 200;        //if the display was sleeping make it show the menu once the menu is exited
  int menuSelect = 0;        //Will need to update this if menu items are added or removed
  const int maxMenuNum = 7;  //the total number of menu items -1
  const char* menuList[] = { "Exit Menu", "Poly Settings", "Ribbon Scale", "Pad Behaviour", "Trautonium Mode", "In Note PB", "Velocity Mode", "Pad Control Ch." };
  //maybe add a coresponding list to show current settings on the menu list
  while (true) {
    display.clearDisplay();
    drawSideBar();
    //the top menu option
    display.setCursor(1, 0);  //set cursor 1px out to show the full box
    display.setTextColor(1);  //the white box background for the highlighted menu item


    if (menuSelect == 0) {  //if the first menu item is selected, then show the last item available instead to allow seamless scrolling
      display.print(menuList[maxMenuNum]);
    } else {  //otheriwse show the orevious menu item
      display.print(menuList[menuSelect - 1]);
    }

    //the middle menu option/the currently selected menu item
    display.fillRect(0, 11, 116, 10, 1);  //white box as background
    display.setTextColor(0);              //with black text
    display.setCursor(1, 12);
    display.print(menuList[menuSelect]);  //show the current menu item
    display.setTextColor(1);

    //the bottom menu option
    display.setCursor(1, 24);
    if (menuSelect == maxMenuNum) {  //if the last menu item is currently selected then show the first menu item to allow seamless scrolling
      display.print(menuList[0]);
    } else {  //otherwise show the next menu item
      display.print(menuList[menuSelect + 1]);
    }

    updateButtons();

    if (btn1Sts == LOW) {
      drawPressed(1);
      menuSelect--;
      if (menuSelect < 0) {  //loop back to the last menu option if going backwards
        menuSelect = maxMenuNum;
      }
    }
    if (btn2Sts == LOW) {  //the main code and what happens when the middle button is selected
      drawPressed(2);
      //alota code
      if (menuSelect == 0) {
        break;
      } else if (menuSelect == 1) {
        if (noteListSelect == 1) {
          alert("Not available in ", "chromatic scale!", "");
        } else {
          polySettings();
        }
      } else if (menuSelect == 2) {
        noteListSelect = boolSelect("Ribbon Scale", noteListSelect, "Diatonic", "Chromatic");
      } else if (menuSelect == 3) {
        padPitchBend = boolSelect("Pad Behaviour", padPitchBend, "Send CC", "Send Pitch Bend");
        if ((padPitchBend == true) && (trautoniumMode == true)) {
          alert("Disable TRAUTONIUM", "mode to use", "PAD PITCH BEND!");
          padPitchBend = false;
        } else if ((padPitchBend == true) && (allowInNotePB == true)) {
          alert("Disbale", "IN NOTE PB to", "use PAD PITCH BEND!");
          padPitchBend = false;
        }

      } else if (menuSelect == 4) {
        trautoniumMode = boolSelect("Trautonium Mode", trautoniumMode, "Disabled", "Enabled");
        if ((trautoniumMode == true) && (padPitchBend == true)) {
          alert("Change pad behaviour", "to MIDI CC to use", "TRAUTONIUM mode!");
          trautoniumMode = false;
        } else if ((trautoniumMode == true) && (allowInNotePB == true)) {
          alert("Disable IN NOTE PB", "to use TRAUTONIUM", "mode!");
          trautoniumMode = false;
        }

      } else if (menuSelect == 5) {
        allowInNotePB = boolSelect("In Note PB", allowInNotePB, "Disabled", "Enabled");
        if ((allowInNotePB == true) && (padPitchBend == true)) {
          alert("Change pad behaviour", "to MIDI CC to use", "IN NOTE PB!");
          allowInNotePB = false;
        } else if ((allowInNotePB == true) && (trautoniumMode == true)) {
          alert("Disable TRAUTONIUM", "mode to use", "IN NOTE PB!");
          allowInNotePB = false;
        }

      } else if (menuSelect == 6) {
        dynamicVelocity = boolSelect("Dynamic Velocity", dynamicVelocity, "Disabled", "Enabled");
      } else if (menuSelect == 7) {
        while (true) {  //this option needs an aditional sub-menu
          updateButtons();
          display.clearDisplay();
          drawButtons();
          display.setCursor(0, 0);
          display.print("Exit --------------");
          display.setCursor(0, 12);
          display.print("Left CC (" + String(lftPotControlChanel) + ")");
          for (int i = 19 - String("Left CC (" + String(lftPotControlChanel) + ")").length(); i != 0; i--) {  //print the right number of hyphens depending on the number of digits displated
            display.print("-");
          }

          display.setCursor(0, 24);
          display.print("Right CC (" + String(rhtPotControlChanel) + ")");
          for (int i = 19 - String("Right CC (" + String(rhtPotControlChanel) + ")").length(); i != 0; i--) {  //stupid amount of String() is used but I cant seem to get anything else to work
            display.print("-");
          }

          updateButtons();
          if (btn1Sts == LOW) {
            drawPressed(1);
            break;
          } else if (btn2Sts == LOW) {
            drawPressed(2);
            lftPotControlChanel = numberSelect("Left CC", lftPotControlChanel, 1, 127);
          } else if (btn3Sts == LOW) {
            drawPressed(3);
            rhtPotControlChanel = numberSelect("Right CC", rhtPotControlChanel, 1, 127);
          }
          display.display();
        }
      } else {
        menuSelect = 0;
      }
    }
    if (btn3Sts == LOW) {
      drawPressed(3);
      menuSelect++;
      if (menuSelect > maxMenuNum) {
        menuSelect = 0;
      }
    }
    display.display();
  }
}

//Change the poly mode settings
void polySettings() {
  if (noteListSelect == 1) {  //if trying to access poly play mode in chromatic mode, don't
  } else {
    analogWrite(rLed, 205);
    analogWrite(gLed, 255);
    analogWrite(bLed, 205);  //show purple to indicate that the controller wont output anything
    ribbonIndicator = 0;     //fixes an issue where if the ribbon is being held down then the menu is requested it would then light up once the menu is exited
    while (true) {
      if (superBreak > 0) {
        superBreak = 0;      //since this is the last break set superBreak to 0 incase of any negative or positive number
        displayCount = 200;  //prevents the liveInfo screen showing if button is pressed early on
        break;
      }
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Exit ------------->");
      display.setCursor(0, 12);
      display.print("Poly Count (" + String(polyCount) + ") --->");
      display.setCursor(0, 24);
      display.print("Poly Type -------->");
      drawButtons();

      display.display();

      updateButtons();
      if (btn2Sts == LOW) {
        drawPressed(2);
        while (true) {
          if (superBreak > 0) {
            superBreak--;
            break;
          }
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Exit ------------->");
          display.setCursor(0, 12);
          display.print("Current      Inc ->");
          display.setCursor(0, 24);
          display.print(("Mode: ") + String(polyCount) + ("      Dec ->"));
          drawButtons();
          display.display();
          updateButtons();
          if (btn1Sts == LOW) {
            drawPressed(1);
            updateButtons();
            break;
          } else if (btn3Sts == LOW) {
            if (polyCount <= 1) {
              drawCross(3);
            } else {
              drawPressed(3);
              polyCount--;
            }
          } else if (btn2Sts == LOW) {
            if (polyCount >= maxPolyCount) {
              drawCross(2);
            } else {
              drawPressed(2);
              polyCount++;
            }
          }
        }
      }
      //change what poly modes are played
      else if (btn3Sts == LOW) {
        drawPressed(3);
        updateButtons();
        while (true) {
          if (superBreak > 0) {
            superBreak--;
            break;
          }
          display.clearDisplay();
          drawButtons();
          display.setCursor(0, 0);
          display.print("Exit ------------->");
          display.setCursor(0, 12);
          display.print("Major Chords ----->");
          display.setCursor(0, 24);
          display.print("Minor Chords ----->");
          updateButtons();
          display.display();
          if (btn2Sts == LOW) {
            drawPressed(2);
            updateButtons();
            while (true) {
              if (superBreak > 0) {
                superBreak--;
                break;
              }
              display.clearDisplay();
              drawButtons();
              threeLine("3 Oct      1,8,15 >", "Major 6   1,3,5,6 >", "Next (1/3)-------->");
              updateButtons();
              display.display();
              if (btn1Sts == LOW) {
                drawPressed(1);
                updateButtons();
                polyChord[1] = 12;
                polyChord[2] = 24;
                maxPolyCount = 3;
                if (polyCount == 4) {
                  polyCount = 3;
                }
                superBreak = 3;
              } else if (btn2Sts == LOW) {
                drawPressed(2);
                updateButtons();
                polyChord[1] = 4;
                polyChord[2] = 7;
                polyChord[3] = 9;
                maxPolyCount = 4;
                superBreak = 3;
              } else if (btn3Sts == LOW) {
                drawPressed(3);
                while (true) {
                  if (superBreak > 0) {
                    superBreak--;
                    break;
                  }
                  display.clearDisplay();
                  drawButtons();
                  threeLine("Dom 7    1,3,5,b7 >", "Major 7   1,3,5,7 >", "Next (2/3)-------->");
                  updateButtons();
                  display.display();
                  if (btn1Sts == LOW) {
                    drawPressed(1);
                    updateButtons();
                    polyChord[1] = 4;
                    polyChord[2] = 7;
                    polyChord[3] = 10;
                    maxPolyCount = 4;
                    superBreak = 4;
                  } else if (btn2Sts == LOW) {
                    drawPressed(2);
                    updateButtons();
                    polyChord[1] = 4;
                    polyChord[2] = 7;
                    polyChord[3] = 11;
                    maxPolyCount = 4;
                    superBreak = 4;
                  } else if (btn3Sts == LOW) {
                    drawPressed(3);
                    updateButtons();
                    while (true) {
                      if (superBreak > 0) {
                        superBreak--;
                        break;
                      }
                      display.clearDisplay();
                      drawButtons();
                      threeLine("Aug        1,3,#5 >", "Exit ------------->", "Next (3/3) ------->");
                      updateButtons();
                      display.display();
                      if (btn1Sts == LOW) {
                        drawPressed(1);
                        updateButtons();
                        polyChord[1] = 4;
                        polyChord[2] = 8;
                        polyChord[3] = 0;
                        if (polyCount == 4) {
                          polyCount = 3;
                        }
                        maxPolyCount = 3;
                        superBreak = 5;
                      } else if (btn2Sts == LOW) {
                        drawPressed(2);
                        updateButtons();
                        superBreak = 5;
                      } else if (btn3Sts == LOW) {
                        drawPressed(3);
                        updateButtons();
                        superBreak = 2;
                      }
                    }
                  }
                }
              }
            }
          } else if (btn3Sts == LOW) {
            drawPressed(3);
            updateButtons();
            while (true) {
              if (superBreak > 0) {
                superBreak--;
                break;
              }
              display.clearDisplay();
              drawButtons();
              threeLine("Minor      1,b3,5 >", "Minor 6  1,b3,5,6 >", "Next (1/3)-------->");
              updateButtons();
              display.display();
              if (btn1Sts == LOW) {
                drawPressed(1);
                updateButtons();
                polyChord[1] = 3;
                polyChord[2] = 7;
                polyChord[3] = 0;
                maxPolyCount = 3;
                if (polyCount == 4) {
                  polyCount = 3;
                }
                superBreak = 3;
              } else if (btn2Sts == LOW) {
                drawPressed(2);
                updateButtons();
                polyChord[1] = 3;
                polyChord[2] = 7;
                polyChord[3] = 9;
                maxPolyCount = 4;
                superBreak = 3;
              } else if (btn3Sts == LOW) {
                drawPressed(3);
                while (true) {
                  if (superBreak > 0) {
                    superBreak--;
                    break;
                  }
                  display.clearDisplay();
                  drawButtons();
                  threeLine("Dim 7   1,b3,b5,7 >", "Minor 7  1,b3,5,b7>", "Next (2/3)-------->");
                  updateButtons();
                  display.display();
                  if (btn1Sts == LOW) {
                    drawPressed(1);
                    updateButtons();
                    polyChord[1] = 3;
                    polyChord[2] = 6;
                    polyChord[3] = 9;
                    maxPolyCount = 4;
                    superBreak = 4;
                  } else if (btn2Sts == LOW) {
                    drawPressed(2);
                    updateButtons();
                    polyChord[1] = 3;
                    polyChord[2] = 7;
                    polyChord[3] = 10;
                    maxPolyCount = 4;
                    superBreak = 4;
                  } else if (btn3Sts == LOW) {
                    drawPressed(3);
                    updateButtons();
                    while (true) {
                      if (superBreak > 0) {
                        superBreak--;
                        break;
                      }
                      display.clearDisplay();
                      drawButtons();
                      threeLine("Dim       1,b3,b5 >", "Exit ------------->", "Next (3/3) ------->");
                      updateButtons();
                      display.display();
                      if (btn1Sts == LOW) {
                        drawPressed(1);
                        updateButtons();
                        polyChord[1] = 3;
                        polyChord[2] = 6;
                        polyChord[3] = 0;
                        if (polyCount == 4) {
                          polyCount = 3;
                        }
                        maxPolyCount = 3;
                        superBreak = 5;
                      } else if (btn2Sts == LOW) {
                        drawPressed(2);
                        updateButtons();
                        superBreak = 5;
                      } else if (btn3Sts == LOW) {
                        drawPressed(3);
                        updateButtons();
                        superBreak = 2;
                      }
                    }
                  }
                }
              }
            }
          } else if (btn1Sts == LOW) {
            drawPressed(1);
            updateButtons();
            break;
          }
        }
      } else if (btn1Sts == LOW) {
        drawPressed(1);
        updateButtons();
        break;
      }
    }
  }
}
//want to move this to a seperate file, but for now it works

//This function is now uneeded with the reposition of key and octave select to seperate choices in the idle screen
//show the change key/oct settings
void changeKeyOct() {
  display.clearDisplay();
  drawButtons();
  display.setCursor(0, 0);
  display.print("Exit ------------->");
  display.setCursor(0, 12);
  display.print("Key: ");
  display.print(keyList[offSetKey]);
  display.print(" | Change ->");
  display.setCursor(0, 24);
  display.print("Oct: ");
  if (octaveOffSet >= 0) {
    display.print("+");
  }
  display.print(octaveOffSet);
  display.print(" | Change ->");
  display.display();
  updateButtons();
  if (btn1Sts == LOW) {
    drawPressed(1);
    updateButtons();

    if ((rawPosRead > ribbonDeadZone) || (rawPresRead > 10) || (rawRightPad > 20) || (rawLeftPad > 20)) {  // if any sensor is activated show the live info
      displayCount = 0;
      displayPage = 0;
      displaySpecial = 0;
    } else {
      displayCount = 200;
      displayPage = 0;
      displaySpecial = 0;
    }
  }
  if (btn2Sts == LOW) {  //change key
    displaySpecial = 0;
    drawPressed(2);
    updateButtons();
    displayPage = 2;  //-2 to indicate to oledScreen to show the changeKey screen
  }
  if (btn3Sts == LOW) {  //change octaves
    displaySpecial = 0;
    drawPressed(3);
    updateButtons();
    displayPage = 3;  //-3 to indicate to oledScreen to show the changeOct screen
  }
}

//change the key
void changeKey() {
  display.clearDisplay();
  drawButtons();

  display.setCursor(10, 0);
  display.print("Key:");
  display.setCursor(5, 11);
  display.setTextSize(3);
  display.print(keyList[offSetKey]);
  display.setTextSize(1);
  display.setCursor(48, 0);
  display.print("| Exit --->");
  display.setCursor(48, 12);
  display.print("| Inc ---->");
  display.setCursor(48, 24);
  display.print("| Dec ---->");
  display.display();
  updateButtons();
  if (btn1Sts == LOW) {
    drawPressed(1);
    displayPage = 0;  //back to the idle screen
  }

  else if (btn2Sts == LOW) {
    displaySpecial = 0;
    if (offSetKey == 11) {  //wrap around if youve reached the key of B
      drawPressed(2);
      offSetKey = 0;
    } else {
      drawPressed(2);
      offSetKey++;
    }
  }

  else if (btn3Sts == LOW) {
    displaySpecial = 0;
    if (offSetKey == 0) {
      drawPressed(3);
      offSetKey = 11;
    } else {
      drawPressed(3);
      offSetKey--;
    }
  }
  updateButtons();
}

//change the octave
void changeOct() {
  display.clearDisplay();
  drawButtons();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Octave:");
  display.setCursor(5, 11);
  display.setTextSize(3);
  if (octaveOffSet >= 0) {
    display.print("+");
  }
  display.print(octaveOffSet);
  display.setTextSize(1);
  display.setCursor(48, 0);
  display.print("| Exit --->");
  display.setCursor(48, 12);
  display.print("| Inc ---->");
  display.setCursor(48, 24);
  display.print("| Dec ---->");
  display.display();
  updateButtons();
  display.display();
  if (btn1Sts == LOW) {
    drawPressed(1);
    displayPage = 0;  //change back to the change key/oct menu. Might change this to fully return to the idle screen
  }

  else if (btn2Sts == LOW) {
    displaySpecial = 0;
    if (octaveOffSet == 3) {  //if you allow this value to be higher the controller could wrap around to midi note number 0 assuming you are using default section size of 32
      drawCross(2);
    } else {
      drawPressed(2);
      octaveOffSet++;
    }
  }

  else if (btn3Sts == LOW) {
    displaySpecial = 0;
    if (octaveOffSet == -3) {  //if you allow this value to be lower the controller could wrap around to midi note number 127
      drawCross(3);
    } else {
      drawPressed(3);
      octaveOffSet--;
    }
  }
  updateButtons();
}

//Draws the idle menu when the controller isn't doing anything. Should probably also handle the button presses but this would casue some issues but would also fix some
void idleDisplay() {
  display.clearDisplay();
  display.setTextSize(1);

  //Show ribbon only status
  display.setCursor(0, 0);
  if (noteListSelect == 0) {
    display.print("Change Octave: ");
    if (octaveOffSet >= 0) {
      display.print("+");
    }
    display.print(octaveOffSet);
    display.print(" -");
  } else {
    display.print("N/A in Chromatic");
  }

  display.setCursor(0, 12);
  display.print("Main Menu ---------");

  display.setCursor(0, 24);
  if (noteListSelect == 0) {
    display.print("Change Key: ");
    display.print(keyList[offSetKey]);
    display.print(" ----");
  } else {
    display.print("N/A in Chromatic");
  }
  drawButtons();

  display.display();
}

//This isn't updated with more recent variables ive meade it's really only there if im having an issue with a new feature
//shows variable information on the OLED, intended for debugging purposes.
void showVars() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(rawPosRead);
  display.setCursor(64, 0);
  display.print(rawPresRead);
  display.setCursor(85, 0);
  display.print(pitchBendValue);
  display.setCursor(0, 12);
  //display.print(sectionNum);
  display.setCursor(64, 12);
  if (isActive == true) {
    display.print("Active");
  } else {
    display.print("Off");
  }
  display.setCursor(0, 24);
  display.print(btn3Sts);
  display.setCursor(10, 24);
  display.print(btn2Sts);
  display.setCursor(20, 24);
  display.print(btn1Sts);
  display.setCursor(40, 24);
  display.print(noteNums[0]);
  display.setCursor(60, 24);
  display.print(prevNoteNums[0]);
  display.setCursor(80, 24);
  //display.print(actionPerformed);

  display.display();
}

//shows information useful when controller is active
void liveInfo() {
  display.clearDisplay();
  display.setTextColor(1);

  //This if statement is responcible for changing how the current note is displayed
  if (analogRead(softpotPin) > ribbonDeadZone) {                               //only show a note if there is a valid reading
    display.setTextSize(3);                                                    //make the note size big
    if (noteListSelect == 0) {                                                 //if using diatonic
      currentNote = noteNumberList[rawPosRead / sectionSize] + totalOffSet;    //get note from diatonic list
    } else {                                                                   //otherwise
      currentNote = noteNumListSharp[rawPosRead / sectionSize] + offSetSharp;  //get it from the all note list
    }

    if (String(noteNumNameNoSharp[currentNote]).length() == 3) {  //if the note is 3 characters long centre the note
      if (isActive == true) {                                     //if the controller is sending midi then invert the big note number colour and draw a white box behind the note number
        display.setCursor(38, 1);
        display.setTextColor(0);
        display.fillRect(36, 0, 55, 23, 1);
        display.print(noteNumNameNoSharp[currentNote]);
      } else {  //otherwise show it normally
        display.setCursor(38, 1);
        display.print(noteNumNameNoSharp[currentNote]);
      }
    } else if (String(noteNumNameNoSharp[currentNote]).length() >= 4) {  //if the note is 4 characters long...
      if (isActive == true) {
        display.setTextSize(2);    //make the text smaller
        display.setCursor(42, 5);  //and reposition
        display.setTextColor(0);
        display.fillRect(36, 0, 55, 23, 1);
        display.print(noteNumNameNoSharp[currentNote]);
        display.setTextColor(1);
      } else {  //otherwise show it normally
        display.setCursor(42, 5);
        display.print(noteNumNameNoSharp[currentNote]);
      }
    } else {                   // otherwise it must be a 2 character note
      if (isActive == true) {  //if the controller is sending midi then invert the big note number
        display.setCursor(47, 1);
        display.setTextColor(0);
        display.fillRect(45, 0, 37, 23, 1);
        display.print(noteNumNameNoSharp[currentNote]);
      } else {  //otherwise show it normally
        display.setCursor(47, 1);
        display.print(noteNumNameNoSharp[currentNote]);
      }
    }
    //ribbon position visualiser, the show width is to scale of the note section size
    ribbonDisplayPos = map(rawPosRead, ribbonDeadZone, 1024, 1, 126);                     // get the current position
    display.fillRect(ribbonDisplayPos - showNoteSecSize / 2, 25, showNoteSecSize, 6, 1);  //centre the notch and position it correctly
  }
  //reset incase I forgor
  display.setTextColor(1);
  display.setTextSize(1);


  //Aftertouch/Position indicator
  //lines at either end of the position indicator
  display.drawLine(0, 24, 0, 32, 1);
  display.drawLine(127, 24, 127, 32, 1);
  //Aftertouch Pressure Indicator on position ribbon
  display.drawLine(0, 24, constrain(map(rawPresRead, 0, lowerPrTr, 0, 127), 0, 127), 24, 1);  //show build up to when MIDI send is allowed
  display.drawLine(0, 31, globalAT, 31, 1);                                                   //draw rectangle to show AT pressure


  //left side pad indicator and pitch bend handler

  if (padPitchBend == false) {  //if the controller is using the pads as CCs
    display.setCursor(0, 0);
    display.print("");
    display.print(lftPotControlChanel);  //show which cc the left side is sending out from

    display.fillRect(1, 12, map(rawLeftPad, 0, 1023, 0, 32), 6, 1);  //show actual pressure value

    display.setCursor(96, 0);
    display.print("");
    display.print(rhtPotControlChanel);  //show which cc the right side is sending out from

  } else {  //if the controller is using the pads to send pitch bend
    display.setCursor(0, 0);
    display.print("Pitch");  //show pitch bend either side of the controller instead of the CC channels
    display.setCursor(104, 0);
    display.print("Bend");

    //modify the left side indicator to be inverted compared to normal to show that you are pitching left
    display.fillRect(32 - map(rawLeftPad, 0, 1023, 0, 32), 12, map(rawLeftPad, 0, 1023, 0, 32), 6, 1);  //the code to actually perform this is stupid
    //because fillRect can't handle having a negative width, we have to draw from the left most side. Writting this comment after I wrote the code, I can't tell you how the line above works
  }
  display.drawRect(0, 11, 32, 8, 1);  //draw outline for left side

  //right pressure pad indicator
  display.drawRect(96, 11, 32, 8, 1);                                //draw outline for right side
  display.fillRect(97, 12, map(rawRightPad, 0, 1023, 0, 32), 6, 1);  //fill rectangle to show pressure
  //the right pad does not need to be inverted when performing pitch bend
  display.display();
}

//Handles which function is called to display on the OLED depending on different input/variable states
void oledScreen() {
  if (displayPage > 0) {  //check first that there are no specieal screens to show
    if (displayPage == 1) {
      changeKeyOct();
    } else if (displayPage == 2) {
      changeKey();
    } else if (displayPage == 3) {
      changeOct();
    }
    displaySpecial++;
    if (displaySpecial > 400) {
      displayPage = 0;
      displaySpecial = 0;
    }
  } else if ((rawPosRead > ribbonDeadZone) || (rawPresRead > 10) || (rawRightPad > 20) || (rawLeftPad > 20)) {  // if any sensor is activated show the live info
    displayCount = 0;
  }
  if (debug == false) {  //key change with debug is a bit broken but its not something that needs to be fixed since you shouldn't be playing the instrument in debug mode
    if (displayPage == 0) {
      if (displayCount < 200) {  //show the live info display a bit after the controller has finished playing notes
        liveInfo();
        displayCount++;
        analogWrite(bLed, 255);  //turn off the blue led if its in the sleep state
        sleepLed = 255;          //reset sleep vars to make it neater
        sleepLedDir = false;
      } else if (displayCount < 1500) {  //otherwise show the menu display
        idleDisplay();
        displayCount++;
        analogWrite(bLed, 255);  //turn off the blue led if its in the sleep state
        sleepLed = 255;          //reset sleep vars to make it neater
        sleepLedDir = false;
      } else if (displayCount >= 1500) {
        display.clearDisplay();
        display.display();  //turn the OLED off if not in use, since its an OLED the screen is prone to burn ins
        sleep();            //show blue led to indicate sleep like state
      }
    }
  } else {  //if debugging...
    showVars();
  }
}
//END OF OLED CODE//


//Main logic on how notes are handled
void music() {
  readPosition();  //get position
  readPressure();  //get pressure
  readModWheel();  //update pressure pads
  noteSelect();    //get the note from the position data
  if (isActive == true) {
    activePB();
  }

  //check for a first time note (essentially, playing from nothing)
  if ((rawPresRead >= lowerPrTr) && (rawPosRead >= ribbonDeadZone) && (isActive == false)) {
    resetBend();
    if (dynamicVelocity == true) {  //if dynamic velocity is true then
      if (velocityWait < 3) {       //change the number in this if statement if you want the velociity to be juged sooner or later
        velocityWait++;
      } else {
        velocitySent = globalAT;
        noteOn(velocitySent);     //Send note on
        storeNote();              //store the note just sent
        isActive = true;          //make the controller "active"
        initialPos = rawPosRead;  //store the ionital position to give trautonium mode an offset to work from
        velocityWait = 0;
      }
    } else {
      velocitySent = 127;       //needed for note change later
      noteOn(velocitySent);     //Send note on
      storeNote();              //store the note just sent
      isActive = true;          //make the controller "active"
      initialPos = rawPosRead;  //store the ionital position to give trautonium mode an offset to work from
      velocityWait = 0;
    }
  }

  //Note change
  if ((rawPresRead >= lowerPrTr) && (rawPosRead >= ribbonDeadZone) && (isActive == true)) {  //if there is a change of notes and the controller is still active


    if ((noteNums[0] != prevNoteNums[0]) && (trautoniumMode == false)) {  //If trautonium mode is disabled, run normally
      if (allowInNotePB == true) {                                        //if in note pb is allowed then change the order of note switching, this will break seemless gliding!
        noteOff();                                                        //turn off the previous note
        resetBend();
        noteOn(velocitySent);  //send new note
      } else {                 //otherwise play normally
        noteOn(velocitySent);  //send new note
        noteOff();             //then turn off the previous note
      }
      delay(5);     //wait a bit for stability. Not for the controllers stability, but I've found that DAWs sometimes don't record the transition properly since they see the note off then new note with a tiny gap. Adding a delay gives time for the DAW to register the new note first
      storeNote();  //Store the new note so it can be turned off later

    } else if (trautoniumMode == true) {  //if trautonium is enabled do pitch-bend instead of switching notes
      trautonium();
    }
  }

  //needed to
  if (((rawPresRead < lowerPrTr) || (rawPosRead < ribbonDeadZone)) && (isValid == true)) {
    velocityWait = 0;
  }
  //check if the controller is no longer being used (note off)
  if (((rawPresRead < lowerPrTr) || (rawPosRead < ribbonDeadZone)) && (isActive == true)) {
    isValid = false;
    isActive = false;   //make the controller "inactive"
    noteOff();          //turn off the last recorded note
    noteOffCurrrent();  //and turn off the current note, prevents a tiny hole in the code from making the controller play on its own
  }
}

void loop() {  //The main loop itself doesn't play any music, it did controll the start behaviour before music() was rewritten
  music();
  oledScreen();  //determines what is displayed to the screen when the instrument can be played, does not controll this during menus


  if ((isActive == false) && (displayPage == 0)) {  //only check for button updates when the controller is not active and when there are no other special pages active
    updateButtons();
    if (btn1Sts == LOW) {
      idleDisplay();
      if (noteListSelect == 0) {  //only allow an oct/key change if using diatonic and NOT chromatic. Since Chromatic offsets are programmed differently and dont use the offsets made for Diatonic mode
        drawPressed(1);
        updateButtons();
        displayPage = 3;
      } else {
        drawCross(1);
        updateButtons();
      }
    } else if (btn2Sts == LOW) {
      idleDisplay();
      drawPressed(2);
      analogWrite(rLed, 205);
      analogWrite(bLed, 205);
      newMenu();
      analogWrite(rLed, 255);
      analogWrite(gLed, 255);
      analogWrite(bLed, 255);
    } else if (btn3Sts == LOW) {
      idleDisplay();
      if (noteListSelect == 0) {  //only allow an oct/key change if using diatonic and NOT chromatic. Since Chromatic offsets are programmed differently and dont use the offsets made for Diatonic mode
        drawPressed(3);
        updateButtons();
        displayPage = 2;
      } else {
        drawCross(3);
        updateButtons();
      }
    }
  }
}

//Thank you to Wintergatan for the amazing music you make and for inspiring me to make this project. When you have the time make more music even without the marble machine(s).
