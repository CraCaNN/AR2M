/*
<CraCaN, turns analogue readings from softpots and FSRs into MIDI notes.>
Copyright (C) <2024>  <CraCaN>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TinyUSB.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>

#include <MIDI.h>


//CONFIG OPTIONS////////////////////////////////////////////////////////////
//These options change how the controller behaves and you can modify to set the default behaviour on boot

//Position ribbon
int sectionSize = 32;  //change the 'size' of the note sections on the ribbon. Increase the number to make the sections bigger or decrease the number to make the sections smaller. make sure you have enough notes in the note list if you nmake the section size smaller
//Recommended sizes are:
//32 can fit 4 octaves 2 notes. (Default)
//28 can fit 5 octaves.
//23 can fit 6 octaves and a note
//The above variable does not take into account the variable below so you may have a partial section at the start of the ribbon
const int ribbonDeadZone = 32;  //The ribbons deadzone size. Since when the ribbon isn't in use the value read drops to a low number, usually less than 10. This is how the controller determines if the ribbon isn't in use. this value works for my ribbon: (https://coolcomponents.co.uk/products/softpot-membrane-potentiometer-500mm)

int offSetKey = 0;     //offset for in key notes, allows you to play in any key
int octaveOffSet = 0;  //change to play in higher or lower octaves

int offSetSharp = 27;  //offset for sharp notes, may remove at some point

bool isPadRequired = true;  //Set to false if you want to play music with just the ribbon can be toggled on or off on the menu

bool inNotePitch = false;     //Enables or disables the pitch bend effect within each note section
bool padPitchBend = false;    //Disables MIDI CC on the left and right pressure pads, and changes it into a funky pitch bend wheel
const int countToPitch = 50;  //How long the program will count before allowing pitch bend. NOT IN ms.

//Aftertouch ribbon
const int upperPres = 900;  // set the max aftertouch to the ribbon reading
const int lowerPrTr = 500;  //set the minimum amount of pressure needed for the pressure ribon to activate
const int prBuffer = 550;   //the amount of pressure required to send any aftertouch
//prBuffer - lowerPrTr = How much buffer there is from when MIDI is first sent to when Aftertouch is then sent

//Control change pads
int lftPotControlChanel = 1;  //Chooses which MIDI CC channel to send information out of from the right pad. 1 = Modulation wheel, 74 = Equator height/slide
int rhtPotControlChanel = 2;  //Chooses which MIDI CC channel to send information out of from the left pad.  1 = Modulation wheel, 74 = Equator height/slide

const int upperPresPads = 900;  //set the mac MIDI CC values for the pads at that pad reading

//Poly Play. Works best when in Diatonic mode, plays notes 2 above the current note played on the ribbon. Repeates for the number in the variable
int polyCount = 1;  //MUST BE between 1-4. Number of poly channels supports. Can be changed in the menu

//Debuging
const bool debug = false;  //set to true to show more variables on the OLED and to enable pixel blink

//determines what list is currently in use, 0 is diatonic, 1 is chromatic
int noteListSelect = 0;
//END OF CONFIG OPTIONS//////////////////////////////////////

//BOARD OPTIONS//These options are to match how you might have wired up your own controller///////////////////////////////////////////
const int softpotPin = A0;         //position ribbon pin
const int pressurePotPinLft = A1;  //pressure pad right pin
const int pressurePotPinRht = A2;  //pressure pad right pin
const int pressureRibbon = A3;     //aftertouch ribbon

//Button Pins
const int btn3 = 5;
const int btn2 = 6;
const int btn1 = 7;

//LEDs for Tiny2040
int rLed = 18;
int gLed = 19;
int bLed = 20;

//END OF BOARD OPTIONS//////////////////////////////////////////////////////////


//CODE BEGIN//
//Yes I know I'm not using any local variables, however I did this project initially to get back into Arduino and had no idea how to do that at the time, and I think I would still make a lot of the variables global anyway
int preSoftPotRead = 0;   //preSoftPotRead is the reading used before it is processed to be sent as a MIDI message, I need both to be able to detect more accuratly where the position is
int softPotReading = 0;   //this is the presoftpotread deivided by the sectionsize variable to get a final position of where your finger is
int ribbonIndicator = 0;  //Makes the turning on/off of the DLED look neater when the ribbon is activated

int pressureRibbonRead = 0;

int pressurePotRhtRead = 0;
int prevPressurePotRhtRead = 0;
int rawRightPad = 0;

int pressurePotLftRead = 0;
int prevPressurePotLftRead = 0;
int rawLeftPad = 0;


//status of the buttons
int btn3Sts = LOW;
int btn2Sts = LOW;
int btn1Sts = LOW;

int buttonPressed = 0;  //helps determine which button was pressed to draw on the display

int noteVelo = 0;      //The reading given by the pressure ribbon, currently programmed to output through the MIDI aftertouch command
int prevNoteVelo = 0;  //Prevent sending the same pressure commands

//this is all of the non sharp notes from C2 to around C6 essentially the key of C
const int noteNumberList[] = { 36, 36, 38, 40, 41, 43, 45, 47, 48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84, 86, 88, 89, 91, 93, 95, 96, 98, 100, 101, 103, 105, 107, 108 };  //C2 Is repeated twice since the start of the ribbon is cut off by the "buffer"
//const char * noteNumNameNoSharp[] = {"C2", "C2", "D2", "E2", "F2", "G2", "A2", "B2", "C3", "D3", "E3", "F3", "G3", "A3", "B3", "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", "D5", "E5", "F5", "G5", "A5", "B5", "C6", "D6", "E6", "F6", "G6", "A6", "B6", "C7", "D7", "E7", "F7", "G7", "A7", "B7", "C8"};
//the list below contains every midi note number in letter form for visualisation purposes
const char* noteNumNameNoSharp[] = { "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1", "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0", "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1", "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2", "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5", "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6", "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7", "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8", "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX", "MAX" };
const char* keyList[] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ", "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };  //for visualisation purposes only

//These are all of the notes from A0. The sharp notes on the ribbon didnt sound right to me especially with the portamento effect but its probably because im not skilled enough
const int noteNumListSharp[] = { 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96 };
//Also probably wouldn't sound right with poly mode due to the way I've programmed poly mode

bool isOff = true;  //Used to determine the status of the controller for the leds and to prevent the main loop from spamming MIDI off messages

//Variables for in note pitch shift, settings can be found at top of program
int pitchCount = 0;                //count to incrament until matched with countToPitch
int inNotePos = 0;                 //remember the inital position
bool hasPitchBendStarted = false;  //to help determine the status of pitch bending
int pitchBendValue = 0;            //amount to pitch bend by

//noteNum is the note that is currently playing
int noteNum = 0;
int noteNum2 = 0;
int noteNum3 = 0;
int noteNum4 = 0;

//prevNoteNum is also the current note playing but is not updated when a new note is played. This helps the controller to determine which note to turn off
int prevNoteNum = 0;
int prevNoteNum2 = 0;
int prevNoteNum3 = 0;
int prevNoteNum4 = 0;

int reCheckVars[] = { 0, 0, 0, 0, 0 };  //Like an averager, stores the last 5 readings from reCheck()
bool performReCheck = true;             //probably move this to the options
bool reCheckPassed = false;             //determines if the check has passed so the main loop doesnt contain the lengthy comparisons
int reCheckCount = 0;                   //Helps keep count of what cycle the check is in and where to add the new value

int totalOffSet = 0;

//for the new chord system
// Technically you are playing poly note 1
int polyOffset2 = 4;
int polyOffset3 = 7;
int polyOffset4 = 11;

//Some chords support either 3 or 4 make sure that there is a limiter in place to prevent the code from playing an undesired 4th poly note
int maxPolyCount = 4;

//make it easier for the visuliser to know what note is being played without having to go through a lengthy looking line of code every time
int currentNote = 0;

//LED
int sleepLed = 255;        //fade the led when OLED is inactive
bool sleepLedDir = false;  //allows the led to fade up and down without going to max brightness

//OLED
//OLED variables
int displayUpdateCount = 0;  //determines what is shown on the OLED during idle states
int blinkPixel = false;      //one pixel on the screen blinks to show when the screen is redrawn, used to be more visable on the UNO R3 but now blinks very quickly thanks to the pico

int superBreak = 0;  //Helps me get out of deep sub menus
int menuDepth = 0;   //used to keep track of where how many levels deep im in a submenu, could be used to replace superBreak

//Show the position of the ribbon as a bar
const int showNoteSecSize = 126 / ((1024 - ribbonDeadZone) / sectionSize);  //gets the size in number of pixels to display to the OLED
int ribbonDisplayPos = 0;

//OLED Setup
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Send MIDI over USB instead of serial
Adafruit_USBD_MIDI usb_midi;  // USB MIDI object
// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

void setup() {
  USBDevice.setManufacturerDescriptor("Arduino Ribbon to MIDI");
  USBDevice.setProductDescriptor("The MIDI Stick");

  //Set LED pins to outputs
  pinMode(rLed, OUTPUT);
  pinMode(gLed, OUTPUT);
  pinMode(bLed, OUTPUT);

  //Initiate OLED
  Wire.setSDA(0);
  Wire.setSCL(1);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();  //clear display
  display.display();
  //display.setTextWrap(false);

  //Initialise display settings
  display.setTextColor(1);

  //Start MIDI
  MIDI.begin();

  //set pins for buttons as inputs
  pinMode(btn3, INPUT_PULLDOWN);
  pinMode(btn2, INPUT_PULLDOWN);
  pinMode(btn1, INPUT_PULLDOWN);
  digitalWrite(btn3, HIGH);
  digitalWrite(btn2, HIGH);
  digitalWrite(btn1, HIGH);

  //sometimes I get better readings when the pins are set to high sometimes they don't change, seems to be micro-controller dependant
  digitalWrite(softpotPin, HIGH);
  //digitalWrite(pressureRibbon, HIGH);
  //digitalWrite(pressurePotPinRht, HIGH);

  analogWrite(rLed, 255);
  analogWrite(gLed, 255);
  analogWrite(bLed, 255);

  displayUpdateCount = 100;  //prevents the liveInfo screen showing instantly after boot. Looks neater in my opinion
}

//get and sends pressure/aftertouch. Also doesn't send data if its the same value as before
void readPressure() {
  pressureRibbonRead = analogRead(pressureRibbon);  //get data for activation of controller

  //I've deliberatly added a 'buffer' so that the pad activates the controller without sending any AT commands unless the pad is held down further
  noteVelo = map(constrain(pressureRibbonRead, prBuffer, upperPres), prBuffer, upperPres, 0, 127);  //change data for use as a MIDI CC

  if (noteVelo != prevNoteVelo) {      //only send an update if a change is detected in the pressure
    MIDI.sendAfterTouch(noteVelo, 1);  //Send the pressure reading through aftertouch
    prevNoteVelo = noteVelo;
  }

  //Red LED visualisation
  if (pressureRibbonRead > lowerPrTr) {  //if left pad is in use turn on red led
    if (noteVelo > 0) {                  //if pad is being held down more than the 'buffer'
      analogWrite(rLed, map(noteVelo, 0, 127, 205, 0));
    } else {  //otherwise show a base value to show that the pad is in use but not being held down very firmly
      analogWrite(rLed, 205);
    }
  } else {  //otherwise turn off the red led
    analogWrite(rLed, 255);
  }
}

//gets the left and right pressure pad data and sends as a MIDI CC channel
void readModWheel() {
  rawRightPad = analogRead(pressurePotPinRht);
  rawLeftPad = analogRead(pressurePotPinLft);
  if (padPitchBend == false) {

    //both the best and worst line of code I have ever written
    pressurePotRhtRead = constrain(map(constrain(rawRightPad, 20, upperPresPads), 20, upperPresPads, 0, 127), 0, 127);  //read the right pressure pad and change from 0-1023 to 0-127
    //gets the data, add a 'buffer' with constrain, map to fit with a MIDI CC channel then constrain that so the map doesn't output a negative number
    pressurePotLftRead = constrain(map(constrain(rawLeftPad, 20, upperPresPads), 20, upperPresPads, 0, 127), 0, 127);  //read the left pressure pad and change from 0-1023 to 0-127
    if (pressurePotRhtRead != prevPressurePotRhtRead) {
      MIDI.sendControlChange(rhtPotControlChanel, pressurePotRhtRead, 1);  //send modulation command
      prevPressurePotRhtRead = pressurePotRhtRead;                         //make the prev reading the same as the current one so to fail the check until the readings change
    }

    if (pressurePotLftRead != prevPressurePotLftRead) {
      MIDI.sendControlChange(lftPotControlChanel, pressurePotLftRead, 1);  //send modulation command
      prevPressurePotLftRead = pressurePotLftRead;                         //make the prev reading the same as the current one so to fail the check until the readings change
    }
  } else {
    MIDI.sendPitchBend(rawLeftPad * -8 + rawRightPad * 8, 1);
  }
}

//I plan on rewriting pitch bend when I figure out MIDI 2.0/MPE/UMP as that would solve ALL of the issues that I currently have with the global pitch bend
//Currently pitch bending is kinda scuffed but it sort of works.
void inNotePitchBend() {
  if (inNotePitch == true) {  //only run if inNotePitch is set to true, skip if false
    //Because a delay would prevent me from changing notes i need to count up to a variable to match another this allows me to wait and still use the controller whilst this is happening. Same concept behind the red led flash
    if (noteNum == prevNoteNum) {
      pitchCount++;
    }                                  //no need to reset counter as this will be done as part of the main music loop in switching notes
    if (pitchCount >= countToPitch) {  //if the same note has been played for a certain period of time, enable pitch bending. Prevents it from sounding to scuffed if your gliding on the ribbon
      if (hasPitchBendStarted == false) {
        inNotePos = preSoftPotRead;  //inNotePos determines where your finger is when pitch bending has started
        hasPitchBendStarted = true;
      }
      pitchBendValue = inNotePos - preSoftPotRead;                              //Takes the difference from the starting point in each note and compares it to the current position
      MIDI.sendPitchBend(0 - constrain(pitchBendValue * 100, -6500, 6500), 1);  //send the pitch bend value, tbf these values are kind of arbitrary so maybe thats why it sounds bad
    }
  }
}

//Reset all variables related to the pitch bend function
void resetBend() {              //I had this coppied 4 times, put it in a function to make it neater
  hasPitchBendStarted = false;  //set pitch bending to false so the pitchBend function isnt run
  pitchBendValue = 0;           //reset all the values that determine what pitch bend value to send
  pitchCount = 0;
  inNotePos = 0;
  MIDI.sendPitchBend(0, 1);  //send a pitch bend value of 0 to make the new note sound more accurate
}

//gets the current position of the position ribbon also calculates any offsets, deals with indicator LED as well
void readPosition() {
  preSoftPotRead = analogRead(softpotPin);        //get raw position value
  totalOffSet = offSetKey + (octaveOffSet * 12);  //this was in noteSelect() but I need this value updated constantly and not just when the controller is active

  //fade up the green DLED
  if (preSoftPotRead > ribbonDeadZone) {  //turn on green led if postion ribbon is being use
    if (ribbonIndicator < 60) {           //the green led is so bright that I don't want to turn it up that bright compared to the red led
      ribbonIndicator = ribbonIndicator + 10;
      analogWrite(gLed, ribbonIndicator * -1 + 255);
    }
  } else {                      //otherwise fade down the green DLED
    if (ribbonIndicator > 0) {  //but dont make the variable negative
      ribbonIndicator = ribbonIndicator - 10;
      analogWrite(gLed, ribbonIndicator * -1 + 255);
    }
  }
}

//gets the ribbon position and the note number. Deals with poly modes as well
void noteSelect() {
  readPosition();                                            //read ribbon value value
  softPotReading = preSoftPotRead / sectionSize;             //divide the reading by sectionSize to fit bigger 'sections' on the ribbon
  if (noteListSelect == 0) {                                 //use the right scale
    noteNum = noteNumberList[softPotReading] + totalOffSet;  //apply any offsets and key changes then get the note
    noteNum2 = noteNumberList[softPotReading] + totalOffSet + polyOffset2;
    noteNum3 = noteNumberList[softPotReading] + totalOffSet + polyOffset3;
    noteNum4 = noteNumberList[softPotReading] + totalOffSet + polyOffset4;
  } else {                                                          //doesnt really work on the chromatic scale, a feature i MIGHT work on later or just remove
    noteNum = noteNumListSharp[softPotReading] + offSetSharp;       //get the actual midi note number to send
    noteNum2 = noteNumListSharp[softPotReading + 2] + offSetSharp;  //get the actual midi note number to send
    noteNum3 = noteNumListSharp[softPotReading + 4] + offSetSharp;  //get the actual midi note number to send
    noteNum4 = noteNumListSharp[softPotReading + 6] + offSetSharp;  //get the actual midi note number to send
  }
  if (noteNum > 127) {
    noteNum = 127;
  }
  if (noteNum2 > 127) {
    noteNum2 = 127;
  }
  if (noteNum3 > 127) {
    noteNum3 = 127;
  }
  if (noteNum4 > 127) {
    noteNum4 = 127;
  }
}


void noteOn() {          //sends the MIDI note on command, deals with POLY mode
  if (polyCount == 1) {  //there is probably a better way of doing this with loops, although if I wanted to change the type of chord played i would have to change how the loop behaves
    MIDI.sendNoteOn(noteNum, 127, 1);
  } else if (polyCount == 2) {
    MIDI.sendNoteOn(noteNum, 127, 1);
    MIDI.sendNoteOn(noteNum2, 127, 1);
  } else if (polyCount == 3) {
    MIDI.sendNoteOn(noteNum, 127, 1);
    MIDI.sendNoteOn(noteNum2, 127, 1);
    MIDI.sendNoteOn(noteNum3, 127, 1);
  } else if (polyCount == 4) {
    MIDI.sendNoteOn(noteNum, 127, 1);
    MIDI.sendNoteOn(noteNum2, 127, 1);
    MIDI.sendNoteOn(noteNum3, 127, 1);
    MIDI.sendNoteOn(noteNum4, 127, 1);
  }
}


void noteOff() {  //sends the MIDI note off command, deals with POLY mode
  if (polyCount == 1) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);
  } else if (polyCount == 2) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);
    MIDI.sendNoteOff(prevNoteNum2, 0, 1);
  } else if (polyCount == 3) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);
    MIDI.sendNoteOff(prevNoteNum2, 0, 1);
    MIDI.sendNoteOff(prevNoteNum3, 0, 1);
  } else if (polyCount == 4) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);
    MIDI.sendNoteOff(prevNoteNum2, 0, 1);
    MIDI.sendNoteOff(prevNoteNum3, 0, 1);
    MIDI.sendNoteOff(prevNoteNum4, 0, 1);
  }
}


//stores the current note so when a new note is played the controller knows which note to turn off
void storeNote() {
  prevNoteNum = noteNum;
  prevNoteNum2 = noteNum2;
  prevNoteNum3 = noteNum3;
  prevNoteNum4 = noteNum4;
}


//updadte button status
void updateButtons() {
  btn1Sts = digitalRead(btn1);
  btn2Sts = digitalRead(btn2);
  btn3Sts = digitalRead(btn3);
}


//Functions for SSD1306 128x32 OLED//
/*A brief comment on the performance impact of using the OLED & a rant on the dumb design of the UNO R3

Initially I was worried that the screen would add to much overhead to run alongside everything else and I would have to manually limit when the display is updated with loops
That was an issue on the UNO R3 but now that I have moved to the Pico, it can handle a lot more than the UNO
The contoller is completely functional without the screen, although its harder to see what it's upto and to change settings on the go

ALSO 

why on earth are the dedicated SDA and SCL pins on the UNO R3 AND R4 connected DIRECTLY TO THE A0 AND A1 PINS WITHOUT ANY LABELS???!!! 
THAT TOOK ME FOREVER TO FIGURE OUT WHY I CONNECTED MY OLED SCREEN AND THE RIBBON INSTANTLY BROKE
I only figured out what was going on once I looked a a schematic for the UNO R3 after I noticed that connecting the ribbon to analogue pins 2,3,4,5 made the issue disappere
Thankfully on the Pico the I2C pins on the pico are like every pin other than the analogue pins which is perfect for me.
*/


//draw white boxes to represent the buttons
void drawButtons() {
  display.fillRect(118, 0, 10, 10, 1);
  display.fillRect(118, 11, 10, 10, 1);
  display.fillRect(118, 22, 10, 10, 1);
}

//display one of 3 buttons being pressed, draws black boxes in the white boxes to simulate a press
void drawPressed() {  //totally could have used a local variable for this but dont know how
  if (buttonPressed == 3) {
    display.fillRect(119, 23, 8, 8, 0);
    display.display();
    buttonPressed = 0;
    delay(200);  //delay to prevent repeat button presses and to show the animation on the OLED
  } else if (buttonPressed == 2) {
    display.fillRect(119, 12, 8, 8, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  } else if (buttonPressed == 1) {
    display.fillRect(119, 1, 8, 8, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  }
}

void drawCross() {  //Draw a cross to show if a button press is unable to perform the requested action
  if (buttonPressed == 3) {
    display.drawLine(119, 23, 126, 30, 0);
    display.drawLine(119, 30, 126, 23, 0);
    display.display();
    buttonPressed = 0;
    delay(200);  //delay to prevent repeat button presses and to show the animation on the OLED
  } else if (buttonPressed == 2) {
    display.drawLine(119, 12, 126, 19, 0);
    display.drawLine(119, 19, 126, 12, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  } else if (buttonPressed == 1) {
    display.drawLine(119, 1, 126, 8, 0);
    display.drawLine(119, 8, 126, 1, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  }
}

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

//The main menu once the middle button is pressed, want to move this to a seperate file because of how big this is but I may need to deal with local vars and I cba
void AR2Menu() {
  analogWrite(rLed, 205);
  analogWrite(gLed, 255);
  analogWrite(bLed, 205);  //show purple to indicate that the controller wont output anything
  ribbonIndicator = 0;     //fixes an issue where if the ribbon is being held down then the menu is requested it would then light up once the menu is exited
  buttonPressed = 2;
  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Exit AR2M Menu --->");
    display.setCursor(0, 12);
    display.print("------------------>");  //used to enable/disable position ribbon only mode, this is no longer needed and will be used at some point
    display.setCursor(0, 24);
    display.print("Next Menu (1/3) -->");
    drawButtons();
    screenBlink();
    display.display();

    updateButtons();
    if (btn1Sts == LOW) {  //exits the menu and returns to the idle screens
      buttonPressed = 1;
      drawPressed();
      updateButtons();
      displayUpdateCount = 100;  //prevents the liveInfo screen showing if button is pressed early on
      analogWrite(rLed, 255);
      analogWrite(bLed, 255);
      break;
    }
    if (btn2Sts == LOW) {  //allows selection of position ribbon mode
      buttonPressed = 2;
      drawCross();
    }
    if (btn3Sts == LOW) {  //progress to next menu layer
      buttonPressed = 3;
      drawPressed();
      while (true) {
        if (superBreak > 0) {
          superBreak--;
          break;
        }
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Ribbon Scale ----->");
        display.setCursor(0, 12);
        display.print("MIDI CCs (" + String(lftPotControlChanel) + "|" + String(rhtPotControlChanel) + ")");
        display.setCursor(108, 12);
        display.print(">");
        display.setCursor(0, 24);
        display.print("Next Menu (2/3) -->");
        drawButtons();
        screenBlink();
        display.display();
        updateButtons();
        if (btn1Sts == LOW) {
          buttonPressed = 1;
          drawPressed();
          while (true) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Ribbon mode select ");
            display.setCursor(0, 12);
            if (noteListSelect == 0) {  //show star to represent the current active mode
              display.print("Diatonic * ------->");
              display.setCursor(0, 24);
              display.print("Chromatic -------->");
            } else {
              display.print("Diatonic --------->");
              display.setCursor(0, 24);
              display.print("Chromatic * ------>");
            }
            drawButtons();
            screenBlink();
            display.display();
            updateButtons();
            if (btn1Sts == LOW) {
              buttonPressed = 1;
              drawCross();
              updateButtons();
            }
            if (btn2Sts == LOW) {
              buttonPressed = 2;
              drawPressed();
              noteListSelect = 0;
              updateButtons();
              break;
            }
            if (btn3Sts == LOW) {
              buttonPressed = 3;
              drawPressed();
              noteListSelect = 1;
              updateButtons();
              break;
            }
          }
        }
        //change the pressure pads CC number. IN the synth I like to use, Surge XT, you can really use any number. But for some special synths like Equator, certain functions only work on specific MIDI CCs
        else if (btn2Sts == LOW) {
          buttonPressed = 2;
          drawPressed();
          while (true) {
            updateButtons();
            display.clearDisplay();
            drawButtons();
            display.setCursor(0, 0);
            display.print("Exit ------------->");
            display.setCursor(0, 12);
            display.print("Left Pad CC (");
            if (lftPotControlChanel < 10) {  //make it look neat :)
              display.print(String(lftPotControlChanel) + ") -->");
            } else if (lftPotControlChanel < 100) {
              display.print(String(lftPotControlChanel) + ") ->");
            } else {
              display.print(String(lftPotControlChanel) + ") >");
            }

            display.setCursor(0, 24);
            display.print("Right Pad CC (");
            if (rhtPotControlChanel < 10) {  //make it look neat :)
              display.print(String(rhtPotControlChanel) + ") ->");
            } else if (rhtPotControlChanel < 100) {
              display.print(String(rhtPotControlChanel) + ") >");
            } else {
              display.print(String(rhtPotControlChanel) + ")>");
            }
            display.display();
            if (btn1Sts == LOW) {
              buttonPressed = 1;
              drawPressed();
              updateButtons();
              break;
            }
            if (btn2Sts == LOW) {
              buttonPressed = 2;
              drawPressed();
              updateButtons();
              while (true) {
                display.clearDisplay();
                drawButtons();

                display.setCursor(0, 0);
                display.print("Left CC");
                display.setCursor(5, 15);
                display.setTextSize(2);
                display.print(lftPotControlChanel);
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
                  buttonPressed = 1;
                  drawPressed();
                  break;
                } else if (btn2Sts == LOW) {
                  buttonPressed = 2;

                  if (lftPotControlChanel == 127) {
                    drawCross();
                  } else {
                    drawPressed();
                    lftPotControlChanel++;
                  }
                } else if (btn3Sts == LOW) {
                  buttonPressed = 3;
                  if (lftPotControlChanel == 1) {  //I initially set this to 0 which meant you could send on MIDI CC 0, which doesn't exist XD
                    drawCross();
                  } else {
                    drawPressed();
                    lftPotControlChanel--;
                  }
                }
              }
            }
            if (btn3Sts == LOW) {
              buttonPressed = 3;
              drawPressed();
              updateButtons();
              while (true) {
                display.clearDisplay();
                drawButtons();

                display.setCursor(0, 0);
                display.print("Right CC");
                display.setCursor(5, 15);
                display.setTextSize(2);
                display.print(rhtPotControlChanel);
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
                  buttonPressed = 1;
                  drawPressed();
                  break;
                } else if (btn2Sts == LOW) {
                  buttonPressed = 2;

                  if (rhtPotControlChanel == 127) {
                    drawCross();
                  } else {
                    drawPressed();
                    rhtPotControlChanel++;
                  }
                } else if (btn3Sts == LOW) {
                  buttonPressed = 3;
                  if (rhtPotControlChanel == 1) {
                    drawCross();
                  } else {
                    drawPressed();
                    rhtPotControlChanel--;
                  }
                }
              }
            }
          }
        } else if (btn3Sts == LOW) {
          buttonPressed = 3;
          drawPressed();
          updateButtons();
          while (true) {
            if (superBreak > 0) {
              superBreak--;
              break;
            }
            updateButtons();
            display.clearDisplay();
            drawButtons();
            display.setCursor(0, 0);
            if (padPitchBend == false) {
              display.print("Pads send MIDI CC >");
            } else {
              display.print("Pads pitch bend -->");
            }
            display.setCursor(0, 12);
            display.print("------------------>");
            display.setCursor(0, 24);
            display.print("Next Menu (3/3)--->");
            display.display();
            if (btn1Sts == LOW) {
              buttonPressed = 1;
              drawPressed();
              if (padPitchBend == false) {
                padPitchBend = true;
              } else {
                padPitchBend = false;
              }
            } else if (btn2Sts == LOW) {
              buttonPressed = 2;
              drawCross();
            } else if (btn3Sts == LOW) {
              buttonPressed = 3;
              drawPressed();
              superBreak = 2;
            }
          }
        }
      }
    }
  }
}

//Change the poly mode settings also want to move this to a seperate file which might be less annoying than AR2Menu
void polySettings() {
  analogWrite(rLed, 205);
  analogWrite(gLed, 255);
  analogWrite(bLed, 205);  //show purple to indicate that the controller wont output anything
  ribbonIndicator = 0;     //fixes an issue where if the ribbon is being held down then the menu is requested it would then light up once the menu is exited
  while (true) {
    if (superBreak > 0) {
      superBreak = 0;            //since this is the last break set superBreak to 0 incase of any negative or positive number
      displayUpdateCount = 100;  //prevents the liveInfo screen showing if button is pressed early on
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
    screenBlink();
    display.display();

    updateButtons();
    if (btn2Sts == LOW) {
      buttonPressed = 2;
      drawPressed();
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
          buttonPressed = 1;
          drawPressed();
          updateButtons();
          break;
        } else if (btn3Sts == LOW) {
          if (polyCount <= 1) {
            buttonPressed = 3;
            drawCross();
          } else {
            buttonPressed = 3;
            drawPressed();
            polyCount--;
          }
        } else if (btn2Sts == LOW) {
          if (polyCount >= maxPolyCount) {
            buttonPressed = 2;
            drawCross();
          } else {
            buttonPressed = 2;
            drawPressed();
            polyCount++;
          }
        }
      }
    }
    //change what poly modes are played
    else if (btn3Sts == LOW) {
      buttonPressed = 3;
      drawPressed();
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
          buttonPressed = 2;
          drawButtons();
          drawPressed();
          updateButtons();
          while (true) {
            if (superBreak > 0) {
              superBreak--;
              break;
            }
            display.clearDisplay();
            drawButtons();
            display.setCursor(0, 0);
            display.print("3 Oct      1,8,15 >");
            display.setCursor(0, 12);
            display.print("Major 6   1,3,5,6 >");
            display.setCursor(0, 24);
            display.print("Next (1/3)-------->");
            updateButtons();
            display.display();
            if (btn1Sts == LOW) {
              buttonPressed = 1;
              drawButtons();
              drawPressed();
              updateButtons();
              polyOffset2 = 12;
              polyOffset3 = 24;
              polyOffset4 = 0;
              maxPolyCount = 3;
              if (polyCount == 4) {
                polyCount = 3;
              }
              superBreak = 3;
            } else if (btn2Sts == LOW) {
              buttonPressed = 2;
              drawButtons();
              drawPressed();
              updateButtons();
              polyOffset2 = 4;
              polyOffset3 = 7;
              polyOffset4 = 9;
              maxPolyCount = 4;
              superBreak = 3;
            } else if (btn3Sts == LOW) {
              buttonPressed = 3;
              drawButtons();
              drawPressed();
              while (true) {
                if (superBreak > 0) {
                  superBreak--;
                  break;
                }
                display.clearDisplay();
                drawButtons();
                display.setCursor(0, 0);
                display.print("Dom 7    1,3,5,b7 >");
                display.setCursor(0, 12);
                display.print("Major 7   1,3,5,7 >");
                display.setCursor(0, 24);
                display.print("Next (2/3)-------->");
                updateButtons();
                display.display();
                if (btn1Sts == LOW) {
                  buttonPressed = 1;
                  drawPressed();
                  updateButtons();
                  polyOffset2 = 4;
                  polyOffset3 = 7;
                  polyOffset4 = 10;
                  maxPolyCount = 4;
                  superBreak = 4;
                } else if (btn2Sts == LOW) {
                  buttonPressed = 2;
                  drawPressed();
                  updateButtons();
                  polyOffset2 = 4;
                  polyOffset3 = 7;
                  polyOffset4 = 11;
                  maxPolyCount = 4;
                  superBreak = 4;
                } else if (btn3Sts == LOW) {
                  buttonPressed = 3;
                  drawPressed();
                  updateButtons();
                  while (true) {
                    if (superBreak > 0) {
                      superBreak--;
                      break;
                    }
                    display.clearDisplay();
                    drawButtons();
                    display.setCursor(0, 0);
                    display.print("Aug        1,3,#5 >");
                    display.setCursor(0, 12);
                    display.print("Exit ------------->");
                    display.setCursor(0, 24);
                    display.print("Next (3/3) ------->");
                    updateButtons();
                    display.display();
                    if (btn1Sts == LOW) {
                      buttonPressed = 1;
                      drawPressed();
                      updateButtons();
                      polyOffset2 = 4;
                      polyOffset3 = 8;
                      polyOffset4 = 0;
                      if (polyCount == 4) {
                        polyCount = 3;
                      }
                      maxPolyCount = 3;
                      superBreak = 5;
                    } else if (btn2Sts == LOW) {
                      buttonPressed = 2;
                      drawPressed();
                      updateButtons();
                      buttonPressed = 2;
                      superBreak = 5;
                    } else if (btn3Sts == LOW) {
                      buttonPressed = 3;
                      drawPressed();
                      updateButtons();
                      buttonPressed = 2;
                      superBreak = 2;
                    }
                  }
                }
              }
            }
          }
        } else if (btn3Sts == LOW) {
          buttonPressed = 3;
          drawButtons();
          drawPressed();
          updateButtons();
          while (true) {
            if (superBreak > 0) {
              superBreak--;
              break;
            }
            display.clearDisplay();
            drawButtons();
            display.setCursor(0, 0);
            display.print("Minor      1,b3,5 >");
            display.setCursor(0, 12);
            display.print("Minor 6  1,b3,5,6 >");
            display.setCursor(0, 24);
            display.print("Next (1/3)-------->");
            updateButtons();
            display.display();
            if (btn1Sts == LOW) {
              buttonPressed = 1;
              drawButtons();
              drawPressed();
              updateButtons();
              polyOffset2 = 3;
              polyOffset3 = 7;
              polyOffset4 = 0;
              maxPolyCount = 3;
              if (polyCount == 4) {
                polyCount = 3;
              }
              superBreak = 3;
            } else if (btn2Sts == LOW) {
              buttonPressed = 2;
              drawButtons();
              drawPressed();
              updateButtons();
              polyOffset2 = 3;
              polyOffset3 = 7;
              polyOffset4 = 9;
              maxPolyCount = 4;
              superBreak = 3;
            } else if (btn3Sts == LOW) {
              buttonPressed = 3;
              drawButtons();
              drawPressed();
              while (true) {
                if (superBreak > 0) {
                  superBreak--;
                  break;
                }
                display.clearDisplay();
                drawButtons();
                display.setCursor(0, 0);
                display.print("Dim 7   1,b3,b5,7 >");
                display.setCursor(0, 12);
                display.print("Minor 7  1,b3,5,b7>");
                display.setCursor(0, 24);
                display.print("Next (2/3)-------->");
                updateButtons();
                display.display();
                if (btn1Sts == LOW) {
                  buttonPressed = 1;
                  drawPressed();
                  updateButtons();
                  polyOffset2 = 3;
                  polyOffset3 = 6;
                  polyOffset4 = 9;
                  maxPolyCount = 4;
                  superBreak = 4;
                } else if (btn2Sts == LOW) {
                  buttonPressed = 2;
                  drawPressed();
                  updateButtons();
                  polyOffset2 = 3;
                  polyOffset3 = 7;
                  polyOffset4 = 10;
                  maxPolyCount = 4;
                  superBreak = 4;
                } else if (btn3Sts == LOW) {
                  buttonPressed = 3;
                  drawPressed();
                  updateButtons();
                  while (true) {
                    if (superBreak > 0) {
                      superBreak--;
                      break;
                    }
                    display.clearDisplay();
                    drawButtons();
                    display.setCursor(0, 0);
                    display.print("Dim       1,b3,b5 >");
                    display.setCursor(0, 12);
                    display.print("Cancel ----------->");
                    display.setCursor(0, 24);
                    display.print("Next (3/3) ------->");
                    updateButtons();
                    display.display();
                    if (btn1Sts == LOW) {
                      buttonPressed = 1;
                      drawPressed();
                      updateButtons();
                      polyOffset2 = 3;
                      polyOffset3 = 6;
                      polyOffset4 = 0;
                      if (polyCount == 4) {
                        polyCount = 3;
                      }
                      maxPolyCount = 3;
                      superBreak = 5;
                    } else if (btn2Sts == LOW) {
                      buttonPressed = 2;
                      drawPressed();
                      updateButtons();
                      buttonPressed = 2;
                      superBreak = 5;
                    } else if (btn3Sts == LOW) {
                      buttonPressed = 3;
                      drawPressed();
                      updateButtons();
                      buttonPressed = 2;
                      superBreak = 2;
                    }
                  }
                }
              }
            }
          }
        } else if (btn1Sts == LOW) {
          buttonPressed = 1;
          drawPressed();
          updateButtons();
          break;
        }
      }
    } else if (btn1Sts == LOW) {
      buttonPressed = 1;
      drawPressed();
      updateButtons();
      break;
    }
  }
}

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
    buttonPressed = 1;
    drawPressed();
    updateButtons();

    if ((preSoftPotRead > ribbonDeadZone) || (pressureRibbonRead > 10) || (rawRightPad > 20) || (rawLeftPad > 20)) {  // if any sensor is activated show the live info
      displayUpdateCount = 0;
    } else {
      displayUpdateCount = 100;
    }
  }
  if (btn2Sts == LOW) {  //change key
    buttonPressed = 2;
    drawPressed();
    updateButtons();
    displayUpdateCount = -2;  //-2 to indicate to oledScreen to show the changeKey screen
  }
  if (btn3Sts == LOW) {  //change octaves
    buttonPressed = 3;
    drawPressed();
    updateButtons();
    displayUpdateCount = -3;  //-3 to indicate to oledScreen to show the changeOct screen
  }
}

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
    buttonPressed = 1;
    drawPressed();
    displayUpdateCount = -1;  //reset display count to show the normal screen
  }

  else if (btn2Sts == LOW) {
    buttonPressed = 2;

    if (offSetKey == 11) {  //wrap around if youve reached the key of B
      drawPressed();
      offSetKey = 0;
    } else {
      drawPressed();
      offSetKey++;
    }
  }

  else if (btn3Sts == LOW) {
    buttonPressed = 3;
    if (offSetKey == 0) {
      drawPressed();
      offSetKey = 11;
    } else {
      drawPressed();
      offSetKey--;
    }
  }
  updateButtons();
}

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
    buttonPressed = 1;
    drawPressed();
    displayUpdateCount = -1;  //reset display count to show the normal screen
  }

  else if (btn2Sts == LOW) {
    buttonPressed = 2;

    if (octaveOffSet == 3) {  //if you allow this value to be higher the controller could wrap around to midi note number 0 assuming you are using default section size of 32
      drawCross();
    } else {
      drawPressed();
      octaveOffSet++;
    }
  }

  else if (btn3Sts == LOW) {
    buttonPressed = 3;
    if (octaveOffSet == -3) {  //if you allow this value to be lower the controller could wrap around to midi note number 127
      drawCross();
    } else {
      drawPressed();
      octaveOffSet--;
    }
  }
  updateButtons();
}

//Draws the idle menu when the controller isn't doing anything
void idleDisplay() {
  display.clearDisplay();
  display.setTextSize(1);

  //Show ribbon only status
  display.setCursor(0, 0);
  display.print("Poly Settings ---->");

  display.setCursor(0, 12);
  display.print("AR2M Menu -------->");

  display.setCursor(0, 24);
  if (noteListSelect == 0) {
    display.print("Key/Oct is ");
    display.print(keyList[offSetKey]);
    display.print("/");
    if (octaveOffSet >= 0) {
      display.print("+");
    }
    display.print(octaveOffSet);
    display.print(" ->");
  } else {
    display.print("N/A in Chromatic");
  }
  drawButtons();
  screenBlink();
  display.display();
}

//show all the variables on the OLED, intended for debugging purposes
void showVars() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(preSoftPotRead);
  display.setCursor(64, 0);
  display.print(pressureRibbonRead);
  display.setCursor(85, 0);
  display.print(noteVelo);
  display.setCursor(0, 12);
  display.print(pressurePotRhtRead);
  display.setCursor(64, 12);
  if (isOff == false) {
    display.print("Sending");
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
  display.print(reCheckVars[0]);
  display.print(",");
  display.print(reCheckVars[1]);
  display.print(",");
  display.print(reCheckVars[2]);
  display.print(",");
  display.print(reCheckVars[3]);
  display.print(",");
  display.print(reCheckVars[4]);
  screenBlink();
  display.display();
}

//shows information useful when controller is active
void liveInfo() {
  display.clearDisplay();
  display.setTextColor(1);
  //display.drawRect(0, 24, 128, 8, 1);  //big rectangle for ribbon position
  display.drawLine(0, 24, 0, 32, 1);
  display.drawLine(127, 24, 127, 32, 1);

  //draw the note number BIG
  if (analogRead(softpotPin) > ribbonDeadZone) {
    display.setTextSize(3);
    if (noteListSelect == 0) {
      currentNote = noteNumberList[preSoftPotRead / sectionSize] + totalOffSet;
    } else {
      currentNote = noteNumListSharp[preSoftPotRead / sectionSize] + offSetSharp;
    }

    if (String(noteNumNameNoSharp[currentNote]).length() == 3) {  //if the note contains a sharp XOR a minus then change the position of the big note
      if (isOff == false) {                                       //if the controller is sending midi then invert the big note number
        display.setTextSize(3);
        display.setCursor(38, 1);
        display.setTextColor(0);
        display.fillRect(36, 0, 55, 23, 1);
        display.print(noteNumNameNoSharp[currentNote]);
        display.setTextColor(1);
      } else {  //otherwise show it normally
        display.setTextSize(3);
        display.setCursor(38, 1);
        display.print(noteNumNameNoSharp[currentNote]);
      }
    } else if (String(noteNumNameNoSharp[currentNote]).length() == 4) {  //if the note contains a shard AND a minus then change the text size and reposition the big note
      if (isOff == false) {                                              //if the controller is sending midi then invert the big note number
        display.setTextSize(2);
        display.setCursor(42, 5);
        display.setTextColor(0);
        display.fillRect(36, 0, 55, 23, 1);
        display.print(noteNumNameNoSharp[currentNote]);
        display.setTextColor(1);
      } else {  //otherwise show it normally
        display.setTextSize(2);
        display.setCursor(42, 5);
        display.print(noteNumNameNoSharp[currentNote]);
      }
    } else {
      if (isOff == false) {  //if the controller is sending midi then invert the big note number
        display.setTextSize(3);
        display.setCursor(47, 1);
        display.setTextColor(0);
        display.fillRect(45, 0, 37, 23, 1);
        display.print(noteNumNameNoSharp[currentNote]);
        display.setTextColor(1);
      } else {  //otherwise show it normally
        display.setTextSize(3);
        display.setCursor(47, 1);
        display.print(noteNumNameNoSharp[currentNote]);
      }
    }
    //finger position visualiser
    ribbonDisplayPos = map(preSoftPotRead, ribbonDeadZone, 1024, 1, 126);
    display.fillRect(ribbonDisplayPos - showNoteSecSize / 2, 25, showNoteSecSize, 6, 1);
  }

  display.setTextSize(1);
  //Aftertouch Pressure Indicator on position ribbon
  display.drawRect(0, 11, 32, 8, 1);
  display.drawLine(0, 31, noteVelo, 31, 1);

  display.drawLine(0, 24, constrain(map(pressureRibbonRead, 0, lowerPrTr, 0, 127), 0, 127), 24, 1);  //show pressure build up to when MIDI is sent

  if (padPitchBend == false) {
    //MIDI CC right pad indicator
    display.setCursor(92, 0);
    display.print("CC ");
    display.print(rhtPotControlChanel);

    //MIDI CC left pad indicator
    display.setCursor(0, 0);
    display.print("CC ");
    display.print(lftPotControlChanel);
    display.drawRect(95, 11, 33, 8, 1);
    display.fillRect(1, 12, map(rawLeftPad, 0, 1023, 0, 32), 6, 1);
  } else {
    display.setCursor(0, 0);
    display.print("Pitch");

    display.setCursor(104, 0);
    display.print("Bend");
    //modify the left side indicator to be inverted compared to normal to show some form of pitch bend
    display.drawRect(0, 11, 33, 8, 1);
    display.fillRect(32 - map(rawLeftPad, 0, 1023, 0, 32), 12, map(rawLeftPad, 0, 1023, 0, 32), 6, 1);
  }

  display.drawRect(95, 11, 33, 8, 1);
  display.fillRect(96, 12, map(rawRightPad, 0, 1023, 0, 32), 6, 1);



  display.display();
}

//Currently not implamented due to some issue, may look at this later
//Checks the readings to reduce "noise" in the ribbon, works as a sort of rolling average
void reCheck() {
  if (performReCheck == true) {  //only run the check if enabled
    if ((noteNum = reCheckVars[0]) && (noteNum = reCheckVars[1]) && (noteNum = reCheckVars[2]) && (noteNum = reCheckVars[3]) && (noteNum = reCheckVars[4])) {
      reCheckPassed = true;
    } else {
      reCheckPassed = false;
      reCheckVars[reCheckCount] = noteNum;
      reCheckCount++;
      if (reCheckCount == 5) {
        reCheckCount = 0;
      }
    }
  }
}

//Handles which function is called depending on different input/variable states
void oledScreen() {
  if (displayUpdateCount < 0) {  //check first that there isnt any negatives which indicate a special screen function
    if (displayUpdateCount == -1) {
      changeKeyOct();
    } else if (displayUpdateCount == -2) {
      changeKey();
    } else if (displayUpdateCount == -3) {
      changeOct();
    }
  } else if ((preSoftPotRead > ribbonDeadZone) || (pressureRibbonRead > 10) || (rawRightPad > 20) || (rawLeftPad > 20)) {  // if any sensor is activated show the live info
    displayUpdateCount = 0;
  }
  if (debug == false) {
    if ((displayUpdateCount >= 0) && (displayUpdateCount < 100)) {  //show the live info display a bit after the controller has finished playing notes
      liveInfo();
      displayUpdateCount++;
      analogWrite(bLed, 255);  //turn off the blue led if its in the sleep state
      sleepLed = 255;          //reset sleep vars to make it neater
      sleepLedDir = false;
    } else if ((displayUpdateCount >= 0) && (displayUpdateCount < 1000)) {  //otherwise show the menu display
      idleDisplay();
      displayUpdateCount++;
      analogWrite(bLed, 255);  //turn off the blue led if its in the sleep state
      sleepLed = 255;          //reset sleep vars to make it neater
      sleepLedDir = false;
    } else if ((displayUpdateCount >= 0) && (displayUpdateCount >= 1000)) {
      display.clearDisplay();
      display.display();  //turn the OLED off if not in use, since its an OLED the screen is prone to burn ins
      sleep();            //show blue led to indicate sleep like state
    }
  } else {
    showVars();
  }
}



//put the main music logic part of the code in a function seperate from the main loop so I can control the start behaviour a bit easier
void music() {
  if (isOff == true) {  //to prevent sending infinate note off commands at the end of the main loop
    isOff = false;
  }
  noteSelect();  //get note

  digitalWrite(bLed, HIGH);  //turn off the blue led if its in the sleep state
  sleepLed = 255;            //reset sleep vars to make it neater
  sleepLedDir = false;

  noteOn();     //send the first on
  storeNote();  //make prev note the same as current note as if this was different it could reak the loop early and create weird sounds

  while (true) {  //keep checking the ribon position hasn't changed so we can keep the note ON WITHOUT sending infinate note on/off commands
    //update all readings
    noteSelect();       //get note
    inNotePitchBend();  //only runs if inNotePitch is true, off by default
    readPressure();     //get the pressure
    readModWheel();     //get and send the right pressure pad through mod cmd

    oledScreen();  //run OLED code

    if ((pressureRibbonRead < lowerPrTr) || (preSoftPotRead < ribbonDeadZone)) {  //if the pressure reading OR the soft pot reading is not being used break...
      break;
    } else if (noteNum != prevNoteNum) {  //if the notes have changed and both fingers are on the pressure pad and position ribbon then
      noteOn();
      noteOff();
      storeNote();
    }
    delay(10);  //delay for stability, prevents too many MIDI messages from being sent, probably ok to remove but I dont notice the difference. Any more than this is noticable when playing
  }
}

void loop() {      //The main loop itself doesn't play any music, it does instead control the start behaviour of when plays music
  delay(10);       //same as the delay in music function, reduces MIDI messages, they also wont add since the 2 loops happen seperatly
  readPressure();  //get the pressure
  readPosition();  //get position value
  readModWheel();  //get and send the right pressure pad through mod cmd

  oledScreen();

  while ((pressureRibbonRead > lowerPrTr) && (preSoftPotRead > ribbonDeadZone)) {
    music();
  }

  if (isOff == false) {
    noteOff();             //...and turn off the note.
    if (polyCount == 1) {  //send note off on the current note as well, I think it this solves an issue where rapidally sending note on/off causes issues. I say I think becuase I didn't think it solved the bug but I haven't experienced the bug in a while
      MIDI.sendNoteOff(noteNum, 0, 1);
    }
    if (polyCount == 2) {
      MIDI.sendNoteOff(noteNum, 0, 1);
      MIDI.sendNoteOff(noteNum2, 0, 1);
    }
    if (polyCount == 3) {
      MIDI.sendNoteOff(noteNum, 0, 1);
      MIDI.sendNoteOff(noteNum2, 0, 1);
      MIDI.sendNoteOff(noteNum3, 0, 1);
    }
    if (polyCount == 4) {
      MIDI.sendNoteOff(noteNum, 0, 1);
      MIDI.sendNoteOff(noteNum2, 0, 1);
      MIDI.sendNoteOff(noteNum3, 0, 1);
      MIDI.sendNoteOff(noteNum4, 0, 1);
    }
    if (inNotePitch == true) {
      resetBend();
    }
    isOff = true;
  }
  if ((isOff == true) && (displayUpdateCount >= 0)) {
    updateButtons();
    //check for right button presses
    if (btn3Sts == LOW) {

      buttonPressed = 3;
      idleDisplay();

      if (noteListSelect == 0) {
        drawPressed();
        updateButtons();
        displayUpdateCount = -1;  //set to -1 to prevent counter from changing so that the key change can be displayed and the controller can still be used
      } else {
        drawCross();
        updateButtons();
      }
    } else if (btn2Sts == LOW) {
      buttonPressed = 2;
      idleDisplay();
      drawPressed();
      AR2Menu();
    } else if (btn1Sts == LOW) {
      buttonPressed = 1;
      idleDisplay();
      drawPressed();
      polySettings();
      analogWrite(rLed, 255);
      analogWrite(gLed, 255);
      analogWrite(bLed, 255);  //turn off purple leds to show active controller
    }
  }
}

//Thank you to Wintergatan for the amazing music you make and for inspiring me to make this project. When you have the time make more music even without the marble machine(s).
