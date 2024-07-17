#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_TinyUSB.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>

#include <MIDI.h>



//CONFIG OPTIONS//These options change how the controller behaves and you can modify to set the default behaviour on boot//

//Position ribbon
int sectionSize = 32;  //change the 'size' of the note sections on the ribbon. Increase the number to make the sections bigger or decrease the number to make the sections smaller. make sure you have enough notes in the note list if you nmake the section size smaller
//Recommended sizes are:
//32 can fit 4 octaves 2 notes. (Default)
//28 can fit 5 octaves.
//23 can fit 6 octaves and a note
//This does not take into account the variable below so you may have a partial section at the start or end of the ribbon
const int ribbonDeadZone = 32;  //The ribbons deadzone size. Since when the ribbon isn't in use the value read drops to a low number, usually less than 10. This is how the controller determines if the ribbon isn't in use. this value works for my ribbon: (https://coolcomponents.co.uk/products/softpot-membrane-potentiometer-500mm)

int offSetKey = 0;     //offset for in key notes, allows you to play in any key
int octaveOffSet = 0;  //change to play in higher or lower octaves

int offSetSharp = 27;  //offset for sharp notes, may remove at some point

bool isPadRequired = true;  //Set to false if you want to play music with just the ribbon can be toggled on or off on the menu

bool inNotePitch = false;     //Enables or disables the pitch bend effect within each note section
const int countToPitch = 50;  //How long the program will count before allowing pitch bend. NOT IN ms.

//Aftertouch Pot (left pot)
const int lowerPrTr = 20;  //set the minimum amount of pressure needed for the pressure ribon to activate

//Control change pot (right pot)
const int rhtPotControlChanel = 1;  //Chooses which MIDI CC channel to send information out of. 1 = Modulation wheel, 74 = Equator height/slide

//Poly Play. Works best when in Diatonic mode, plays notes 2 above the current note played on the ribbon. Repeates for the number in the variable
int polyCount = 1;  //MUST BE between 1-4. Number of poly channels supports. Can be changed in the menu

//Debuging
const bool debug = false;  //set to true to show more variables on the OLED and to enable pixel blink

//determines what list is currently in use, 0 is diatonic, 1 is chromatic
int noteListSelect = 0;
//END OF CONFIG OPTIONS//

//BOARD OPTIONS//These options are to match how you might have wired up the controller//
const int softpotPin = A2;         //position ribbon pin
const int pressurePotPinLft = A1;  //pressure pad left pin
const int pressurePotPinRht = A0;  //pressure pad right pin

//Button Pins
const int btn3 = 6;
const int btn2 = 7;
const int btn1 = 8;

//Analogue potentiometer, used to send MIDI CC but may be removed later
const int pot0 = A3;

/*WARNING ON PIN A3 ON THE STOCK PICO 
Whilst the RP2040 chip has 4 analogue in pins, the stock Pico from RPi only has 3 to the GPIO pins, which are all in use by the position ribbon and the 2 pressure pads
There are 3rd party RP2040 which often include all 4 analogue ins. I found a much more compact pico with USB-C that has all 4 analogue ins to the pins and it works as the stock pico does
Whilst I got this specific Pico on ali express Pimoroni make a 'Tiny Pico' which has the same function, however the built in LED has an input for RG and B whilst the one I'm programming is a DLED
*/

//LEDs//
const int builtInLedPin = 25;  //If you have a board with a standard On/Off LED. 25 is for the stock Pico
#define DATA_PIN 16            //If you have a board with a digital LED. This program has the WS2812b led configured. To change this search FastLED.addLeds and change to your LED type. For more info search 'FastLED arduino' and have a look through their documentation

//END OF BOARD OPTIONS//

//CODE BEGIN//
//Yes I know I'm not using any local variables, however I did this project to get back into Arduino and had no idea how to do that at the time, and I think I would still make a lot of the variables global anyway 
int preSoftPotRead = analogRead(softpotPin);  //preSoftPotRead is the reading used before it is processed to be sent as a MIDI message, I need both to be able to detect more accuratly where the position is 
int softPotReading = 0;  //this is the presoftpotread deivided by the sectionsize variable to get a final position of where your finger is
int ribbonIndicator = 0;                                                                                                                                                                                                                                                                                                                                                                                                                                 //Makes the turning on/off of the DLED look neater when the ribbon is activated

int pressurePotLftRead = analogRead(pressurePotPinLft);
int prePressurePotLftRead = 0;

int pressurePotRhtRead = analogRead(pressurePotPinRht);
int prevPressurePotRhtRead = 0;

//Used for some Control Changes
int pot0Read = analogRead(pot0);
int pot0PrevRead = pot0Read;  //prevent midi spam


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
const char* noteNumNameNoSharp[] = { "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1", "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0", "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1", "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2", "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5", "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6", "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7", "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8", "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9", "G#9", "A9", "A#9", "B9" };
const char* keyList[] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ", "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };//for visualisation purposes only

//These are all of the notes from A0. The sharp notes on the ribbon didnt sound right to me especially with the portamento effect but its probably because im not skilled enough
const int noteNumListSharp[] = { 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96 };
//Also probably wouldn't sound right with poly mode due to the way I've programmed poly mode


bool isOff = true;        //Used to determine the status of the controller for the leds and to prevent the main loop from spamming MIDI off messages
bool isLftPadOff = true;  //Makes sure that when the left pad is off, the controller will set aftertouch to 0


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

int totalOffSet = 0;

int currentNote = 0;//make it easier for the visuliser to know what note is being played without having to go through a lengthy looking line of code every time

//Variables for sustain function
int sustainElements = 0;
int sustainElementCount = 0;
int notesToSustain[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
bool sustain = false;  //SET TO FALSE BY DEFAULT
bool isSustaining = false;
int prevSusNote = 0;

//Onboard DLED on some 3rd party Picos
#define NUM_LEDS 1  //number of DLEDs in this case 1 LED on the board
CRGB leds[NUM_LEDS];


//OLED
//OLED variables
int displayUpdateCount = 0;  //determines what is shown on the OLED during idle states
int blinkPixel = false;      //one pixel on the screen blinks to show when the screen is redrawn, used to be more visable on the UNO R3 but now blinks very quickly thanks to the pico

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
  //on board on/off LEDs
  pinMode(builtInLedPin, OUTPUT);     //define on board LED
  digitalWrite(builtInLedPin, HIGH);  //Light LED to show when initialisation has started
  //Onboard DLED (most commonly WS2812B) on some 3rd party boards
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);  //Change LED type here for the digital LED

  //Initiate LCD
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
  //digitalWrite(pressurePotPinLft, HIGH);
  //digitalWrite(pressurePotPinRht, HIGH);
  digitalWrite(builtInLedPin, LOW);  //turn off LED to show when initialisation has finished


  displayUpdateCount = 100;  //prevents the liveInfo screen showing instantly after boot. Looks neater in my opinion
}


//gets the current position of the ribbon
void readPosition() {
  preSoftPotRead = analogRead(softpotPin);  //get raw position value
  totalOffSet = offSetKey + (octaveOffSet * 12);//this was in getNote but I need this value updated for other purposes

  //fade up the green DLED
  if (preSoftPotRead > ribbonDeadZone) {  //turn on green led if postion ribbon is being use
    if (ribbonIndicator < 60) {           //the green led is so bright that I don't want to turn it up that bright compared to the red led
      ribbonIndicator = ribbonIndicator + 10;
      leds[0].g = ribbonIndicator;
      FastLED.show();
    }
  } else {                      //otherwise fade down the green DLED
    if (ribbonIndicator > 0) {  //but dont make the variable negative
      ribbonIndicator = ribbonIndicator - 10;
      leds[0].g = ribbonIndicator;
      FastLED.show();
    }
  }
}


//get and sends pressure/aftertouch. Also doesn't send data if its the same value as before
void readPressure() {
  pressurePotLftRead = analogRead(pressurePotPinLft);  //get data for activation of controller

  //I've deliberatly added a 'buffer' so that the pad activates the controller without sending any AT commands unless the pad is held down further
  noteVelo = map(constrain(pressurePotLftRead, 512, 1023), 512, 1023, 0, 127);  //change data for use as a MIDI CC

  if (noteVelo != prevNoteVelo) {      //only send an update if a change is detected in the pressure
    MIDI.sendAfterTouch(noteVelo, 1);  //Send the pressure reading through aftertouch
    prevNoteVelo = noteVelo;
  }

  lftPadLedControl();
}


//gets the right pressure pad data and sends as a MIDI CC channel
void readModWheel() {
  //both the best and worst line of code I have ever written
  pressurePotRhtRead = constrain(map(constrain(analogRead(pressurePotPinRht), 20, 1023), 20, 1023, 0, 127), 0, 127);  //read the right pressure pad and change from 0-1023 to 0-127
  //gets the data, add a 'buffer' with constrain, map to fit with a MIDI CC channel then constrain that so the map doesn't output a negative number

  if (pressurePotRhtRead != prevPressurePotRhtRead) {
    MIDI.sendControlChange(rhtPotControlChanel, pressurePotRhtRead, 1);  //send modulation command
    prevPressurePotRhtRead = pressurePotRhtRead;                         //make the prev reading the same as the current one so to fail the check until the readings change
    isLftPadOff = false;
  }
}


//programmed to read and then send MIDI CC 16 = general purporse 1
void readPot0() {
  /*currently diabled as it doesn't currently exist due to the stock pico only haveing 3 analogue ins
  pot0Read = analogRead(pot0)/8;//divide reading by 8 since 1024/8 = 128
  if (pot0Read != pot0PrevRead){//prevent spam by comparing previous values
    MIDI.sendControlChange(16, pot0Read, 1);//send command
    pot0PrevRead = pot0Read;//store previous value
  }
  */
}


void lftPadLedControl() {                          //flash the green led to indicate that the left pad isnt required to play music
  if (pressurePotLftRead > lowerPrTr) {            //if left pad is in use turn on red led
    if (noteVelo > 0) {                            //if pad is being held down more than the 'buffer'
      leds[0].r = map(noteVelo, 0, 127, 50, 255);  //show the red LED brightness corrisponding to that level
      FastLED.show();
    } else {  //otherwise show a base value to show that the pad is in use but not being held down very firmly
      leds[0].r = 50;
      FastLED.show();
    }
  } else {  //otherwise turn off the red led
    leds[0].r = 0;
    FastLED.show();
  }
}

//Currently pitch bending is kinda scuffed but it sort of works. It works better on some synths than others. Currently considering adding a smaller ribbon on the side of the stick to allow better pitch bending
void pitchBend() {
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


//gets the ribbon position and the note number. Deals with poly modes as well
void noteSelect() {
  readPosition();                                 //read ribbon value value
  softPotReading = preSoftPotRead / sectionSize;  //divide the reading by sectionSize to fit bigger 'sections' on the ribbon
  if (noteListSelect == 0) {                                 //use the right scale
    noteNum = noteNumberList[softPotReading] + totalOffSet;  //apply any offsets and key changes then get the note
    noteNum2 = noteNumberList[softPotReading + 2] + totalOffSet;
    noteNum3 = noteNumberList[softPotReading + 4] + totalOffSet;
    noteNum4 = noteNumberList[softPotReading + 6] + totalOffSet;
  } else {                                                          //doesnt really work on the chromatic scale, a feature i MIGHT work on later or just remove
    noteNum = noteNumListSharp[softPotReading] + offSetSharp;       //get the actual midi note number to send
    noteNum2 = noteNumListSharp[softPotReading + 2] + offSetSharp;  //get the actual midi note number to send
    noteNum3 = noteNumListSharp[softPotReading + 4] + offSetSharp;  //get the actual midi note number to send
    noteNum4 = noteNumListSharp[softPotReading + 6] + offSetSharp;  //get the actual midi note number to send
  }
}


void noteOn() {          //sends the MIDI note on command, deals with POLY mode
  if (polyCount == 1) {  //there is probably a better way of doing this with loops, although if I wanted to change the type of chord played i would have to change how the loop behaves
    MIDI.sendNoteOn(noteNum, 127, 1);
  }
  if (polyCount == 2) {
    MIDI.sendNoteOn(noteNum, 127, 1);
    MIDI.sendNoteOn(noteNum2, 127, 1);
  }
  if (polyCount == 3) {
    MIDI.sendNoteOn(noteNum, 127, 1);
    MIDI.sendNoteOn(noteNum2, 127, 1);
    MIDI.sendNoteOn(noteNum3, 127, 1);
  }
  if (polyCount == 4) {
    MIDI.sendNoteOn(noteNum, 127, 1);
    MIDI.sendNoteOn(noteNum2, 127, 1);
    MIDI.sendNoteOn(noteNum3, 127, 1);
    MIDI.sendNoteOn(noteNum4, 127, 1);
  }
}


void noteOff() {  //sends the MIDI note off command, deals with POLY mode
  if (polyCount == 1) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);
  }
  if (polyCount == 2) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);
    MIDI.sendNoteOff(prevNoteNum2, 0, 1);
  }
  if (polyCount == 3) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);
    MIDI.sendNoteOff(prevNoteNum2, 0, 1);
    MIDI.sendNoteOff(prevNoteNum3, 0, 1);
  }
  if (polyCount == 4) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);
    MIDI.sendNoteOff(prevNoteNum2, 0, 1);
    MIDI.sendNoteOff(prevNoteNum3, 0, 1);
    MIDI.sendNoteOff(prevNoteNum4, 0, 1);
  }
}


//now depreceated since it was to buggy, plan on adding an alternative at some point
void sustainFunc() {
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
void drawButtonPressed() {  //totally could have used a local variable for this but dont know how
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


void AR2Menu() {  //The main menu once the middle button is pressed
  leds[0].r = 50;
  leds[0].g = 0;
  leds[0].b = 50;
  FastLED.show();       //show purple to indicate that the controller wont output anything
  ribbonIndicator = 0;  //fixes an issue where if the ribbon is being held down then the menu is requested it would then light up once the menu is exited
  buttonPressed = 2;
  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Exit AR2M Menu --->");
    display.setCursor(0, 12);
    display.print("Ribbon Scale ----->");
    display.setCursor(0, 24);
    display.print("Next Menu (1/2) -->");
    drawButtons();
    screenBlink();
    display.display();

    updateButtons();
    if (btn1Sts == LOW) {  //exits the menu and returns to the idle screens
      buttonPressed = 1;
      drawButtonPressed();
      updateButtons();
      displayUpdateCount = 100;  //prevents the liveInfo screen showing if button is pressed early on
      leds[0].r = 0;
      leds[0].b = 0;   //turn off purple led
      FastLED.show();  //show purple to indicate that the instrument wont play anything
      break;
    }
    if (btn2Sts == LOW) {  //allows selection of position ribbon mode
      buttonPressed = 2;
      drawButtonPressed();
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
          drawButtonPressed();
          noteListSelect = 0;
          updateButtons();
          break;
        }
        if (btn3Sts == LOW) {
          buttonPressed = 3;
          drawButtonPressed();
          noteListSelect = 1;
          updateButtons();
          break;
        }
      }
    }
    if (btn3Sts == LOW) {  //progress to next menu layer
      buttonPressed = 3;
      drawButtonPressed();
      while (true) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Poly Mode -------->");
        display.setCursor(0, 12);
        display.print("------------------>");
        display.setCursor(0, 24);
        display.print("Main Menu (2/2) -->");
        drawButtons();
        screenBlink();
        display.display();

        updateButtons();
        if (btn1Sts == LOW) {
          buttonPressed = 1;
          drawButtonPressed();
          while (true) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Exit ------------->");
            display.setCursor(0, 12);
            display.print("Current      Dec ->");
            display.setCursor(0, 24);
            display.print(("Mode: ") + String(polyCount) + ("      Inc ->"));
            drawButtons();
            display.display();
            updateButtons();
            if (btn1Sts == LOW) {
              buttonPressed = 1;
              drawButtonPressed();
              updateButtons();
              break;
            } else if (btn3Sts == LOW) {
              if (polyCount >= 4) {
                buttonPressed = 3;
                drawCross();
              } else {
                buttonPressed = 3;
                drawButtonPressed();
                polyCount++;
              }
            } else if (btn2Sts == LOW) {
              if (polyCount <= 1) {
                buttonPressed = 2;
                drawCross();
              } else {
                buttonPressed = 2;
                drawButtonPressed();
                polyCount--;
              }
            }
          }
        } else if (btn2Sts == LOW) {
          //nothing in this slot yet
        } else if (btn3Sts == LOW) {
          buttonPressed = 3;
          drawButtonPressed();
          updateButtons();
          break;
        }
      }
    }
  }
}


void changeKey() {
  buttonPressed = 3;
  idleDisplay();
  drawButtonPressed();
  displayUpdateCount = 100;
  leds[0].r = 50;
  leds[0].g = 0;
  leds[0].b = 50;
  FastLED.show();       //show purple to indicate that the controller wont output anything
  ribbonIndicator = 0;  //fixes an issue where if the ribbon is being held down then the menu is requested it would then light up once the menu is exited
  while (true) {
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
      drawButtonPressed();
      updateButtons();
      leds[0].r = 0;
      leds[0].g = 0;
      leds[0].b = 0;
      break;
    }
    if (btn2Sts == LOW) {//change key
      buttonPressed = 2;
      drawButtonPressed();
      updateButtons();
      while (true) {
        display.clearDisplay();
        drawButtons();

        display.setCursor(10, 0);
        display.print("Key:");
        display.setCursor(5, 11);
        display.setTextSize(3);
        display.print(keyList[offSetKey]);
        display.setTextSize(1);
        display.setCursor(45, 0);
        display.print("| Exit --->");
        display.setCursor(45, 12);
        display.print("| Inc ---->");
        display.setCursor(45, 24);
        display.print("| Dec ---->");
        display.display();
        updateButtons();
        if (btn1Sts == LOW) {
          buttonPressed = 1;
          drawButtonPressed();
          break;
        }

        else if (btn2Sts == LOW) {
          buttonPressed = 2;

          if (offSetKey == 11) {
            drawCross();
          } else {
            drawButtonPressed();
            offSetKey++;
          }
        }

        else if (btn3Sts == LOW) {
          buttonPressed = 3;
          if (offSetKey == 0) {
            drawCross();
          } else {
            drawButtonPressed();
            offSetKey--;
          }
        }
      }
      //break;
    }
    if (btn3Sts == LOW) {//change octaves
      buttonPressed = 3;
      drawButtonPressed();
      updateButtons();
      while (true) {
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
        display.setCursor(45, 0);
        display.print("| Exit --->");
        display.setCursor(45, 12);
        display.print("| Inc ---->");
        display.setCursor(45, 24);
        display.print("| Dec ---->");
        display.display();
        updateButtons();
        display.display();
        if (btn1Sts == LOW) {
          buttonPressed = 1;
          drawButtonPressed();
          break;
        }

        else if (btn2Sts == LOW) {
          buttonPressed = 2;

          if (octaveOffSet == 3) {  //if you allow this value to be higher the controller could wrap around to midi note number 0 assuming you are using default section size of 32
            drawCross();
          } else {
            drawButtonPressed();
            octaveOffSet++;
          }
        }

        else if (btn3Sts == LOW) {
          buttonPressed = 3;
          if (octaveOffSet == -3) {  //if you allow this value to be lower the controller could wrap around to midi note number 127
            drawCross();
          } else {
            drawButtonPressed();
            octaveOffSet--;
          }
        }
      }
    }
  }
}

//Draws the idle menu when the controller isn't doing anything
void idleDisplay() {
  display.clearDisplay();
  display.setTextSize(1);

  //Show ribbon only status
  display.setCursor(0, 0);
  if (isPadRequired == true) {
    display.print("Ribbon only is OFF>");
  } else {
    display.print("Ribbon only is  ON>");
  }

  display.setCursor(0, 12);
  display.print("AR2M Menu -------->");

  display.setCursor(0, 24);
  display.print("Key/Oct is ");
  display.print(keyList[offSetKey]);
  display.print("/");
  if (octaveOffSet >= 0) {
    display.print("+");
  }
  display.print(octaveOffSet);
  display.print(" ->");
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
  display.print(pressurePotLftRead);
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
  display.setCursor(33, 24);
  display.print(btn2Sts);
  display.setCursor(66, 24);
  display.print(btn1Sts);
  screenBlink();
  display.display();
}

//shows teh next and previous 3 notes along the bottom of the screen
void showCloseNotes() {
  /*
  display.setTextSize(1);
  display.setCursor(0, 22);
  display.print(noteNumNameNoSharp[analogRead(softpotPin)/sectionSize-3]);
  display.setCursor(15, 22);
  display.print(noteNumNameNoSharp[analogRead(softpotPin)/sectionSize-2]);
  display.setCursor(30, 22);
  display.print(noteNumNameNoSharp[analogRead(softpotPin)/sectionSize-1]);

  display.setCursor(86, 22);
  display.print(noteNumNameNoSharp[analogRead(softpotPin)/sectionSize+1]);
  display.setCursor(101, 22);
  display.print(noteNumNameNoSharp[analogRead(softpotPin)/sectionSize+2]);
  display.setCursor(116, 22);
  display.print(noteNumNameNoSharp[analogRead(softpotPin)/sectionSize+3]);
  */

  //show the position of the finger on the ribbon on the OLED
  
  
}



//shows information useful when controller is active
void liveInfo() {
  display.clearDisplay();
  display.setTextColor(1);
  display.drawRect(0, 24, 128, 8, 1);//big rectangle for ribbon position
  //draw the note number BIG
  if (analogRead(softpotPin) > ribbonDeadZone) {
    display.setTextSize(3);
    currentNote = noteNumberList[preSoftPotRead / sectionSize] + totalOffSet;
    if (String(noteNumNameNoSharp[currentNote]).indexOf("#")>0) {//if the note contains a sharp then change the position of the big note
      if (isOff == false) {  //if the controller is sending midi then invert the big note number
        display.setCursor(38, 1);
        display.setTextColor(0);
        display.fillRect(36, 0, 55, 23, 1);
        display.print(noteNumNameNoSharp[currentNote]);
        display.setTextColor(1);
      }
      else {  //otherwise show it normally
        display.setCursor(38, 1);
        display.print(noteNumNameNoSharp[currentNote]);
      }
    }
    else {
      if (isOff == false) {  //if the controller is sending midi then invert the big note number
        display.setCursor(47, 1);
        display.setTextColor(0);
        display.fillRect(45, 0, 37, 23, 1);
        display.print(noteNumNameNoSharp[currentNote]);
        display.setTextColor(1);
      }
      else {  //otherwise show it normally
        display.setCursor(47, 1);
        display.print(noteNumNameNoSharp[currentNote]);
      }
    }
    //finger position visualiser
    ribbonDisplayPos = map(preSoftPotRead, ribbonDeadZone, 1024, 1, 126);
    display.fillRect(ribbonDisplayPos - showNoteSecSize / 2, 25, showNoteSecSize, 6, 1);
  }

  display.setTextSize(1);
  //Aftertouch Pressure Indicator
  display.setCursor(0, 0);
  display.print("AT");
  display.drawRect(0, 11, 32, 8, 1);
  display.fillRect(1, 12, map(noteVelo, 0, 127, 0, 32), 6, 1);

  display.fillRect(1, 14, constrain(map(pressurePotLftRead, 0, 512, 0, 30), 0, 30), 2, 1);  //shows the section of the 'buffer' as a thin line

  //MIDI CC indicator
  display.setCursor(110, 0);
  display.print(" CC");
  display.setCursor(95, 0);
  display.print(rhtPotControlChanel);

  display.drawRect(95, 11, 33, 8, 1);
  display.fillRect(96, 12, map(pressurePotRhtRead, 0, 127, 0, 32), 6, 1);
  display.display();
}



//put the main part of the code in a function seperate from the main loop so I can control the start behaviour a bit easier
void music() {
  if (isOff == true) {  //to prevent sending infinate note off commands at the end of the main loop
    isOff = false;
  }
  noteSelect();  //get note
  digitalWrite(builtInLedPin, HIGH);

  /*
  pressurePotRhtRead = analogRead(pressurePotPinRht);//this next section fixes an issue where if the right pressure pad was released to quickly the value would not always return to 0
  pressurePotRhtRead = ((pressurePotRhtRead/8)*-1)+127;//the original reading starts from 1024 and with pressure decreases to 0, we need it the other way round and inly with a max of 127
  //MIDI.sendControlChange(1, pressurePotRhtRead, 1);//is this needed???
  */

  noteOn();     //send the first on
  storeNote();  //make prev note the same as current note as if this was different it could reak the loop early and create weird sounds

  while (true) {   //keep checking the ribon position hasn't changed so we can keep the note ON WITHOUT sending infinate note on/off commands
    noteSelect();  //get note

    pitchBend();     //only runs if inNotePitch is true, off by default
    readPressure();  //get the pressure
    readModWheel();  //get and send the right pressure pad through mod cmd
    readPot0();      //get current reading of the pot on A0

    if (debug == false) {  //if debug is off show the normal screen
      liveInfo();
    } else {  //otherwise show the more advanced screen
      showVars();
    }

    if (isPadRequired == true) {
      if ((pressurePotLftRead < lowerPrTr) || (preSoftPotRead < ribbonDeadZone)) {  //if the pressure reading OR the soft pot reading is not being used reset pitch bend stuff and break...
        break;
      } else if (noteNum != prevNoteNum) {  //if the notes have changed and both fingers are on the pressure pad and position ribbon then
        noteOn();
        noteOff();
        storeNote();
      }
    }

    else if (isPadRequired == false) {        //when playing on ribbon only mode, ignore the additional check of the left pad
      if (preSoftPotRead < ribbonDeadZone) {  //if the soft pot reading is not being used break...
        break;
      } else if (noteNum != prevNoteNum) {  //if the notes have changed then
        noteOn();                           //send new note
        noteOff();                          //turn off previous note
        storeNote();                        //store the current note
      }
    }
    delay(10);
  }
  delay(10);  //delay for stability, prevents too many MIDI messages from being sent, probably ok to remove but I dont notice the difference. Any more than this is noticable when playing
}

void loop() {      //The main loop itself doesn't play any music, it does instead control the start behaviour of when plays music
  delay(10);       //same as the delay in music function, reduces MIDI messages, they also wont add since the 2 loops happen seperatly
  readPressure();  //get the pressure
  readPosition();  //get position value
  readModWheel();  //get and send the right pressure pad through mod cmd
  readPot0();

  if (isPadRequired == true) {
    while ((pressurePotLftRead > lowerPrTr) && (preSoftPotRead > ribbonDeadZone)) {  //If both the pressure pad and postion ribon are in use then play some music!!!
      music();
    }
  } else {
    while (preSoftPotRead > ribbonDeadZone) {
      music();
    }
  }

  if (isOff == false) {
    noteOff();             //...and turn off the note.
    if (polyCount == 1) {  //send note off on the current note as well, trying to resolve a bug but it doesnt seem to be working atm
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
    displayUpdateCount = 0;  //keep the display on the stats screen for a bit after midi has turned off
    isOff = true;
    digitalWrite(builtInLedPin, LOW);
  }

  //if any of the pads or the ribbon are in use then switch the display to show the status of said ribbons and pads
  if ((preSoftPotRead > ribbonDeadZone) || (pressurePotLftRead > lowerPrTr) || (pressurePotRhtRead > 0)) {
    displayUpdateCount = 0;
  }

  if (debug == false) {
    if (displayUpdateCount < 100) {  //show the live info display a bit after the controller has finished playing notes
      liveInfo();
      displayUpdateCount++;
    } else {  //otherwise show the menu display
      idleDisplay();
    }
  } else {
    showVars();  //Comment when not debugging
  }

  updateButtons();
  //check for right button presses

  if (btn3Sts == LOW) {
    changeKey();
  }

  if (btn2Sts == LOW) {
    buttonPressed = 2;
    idleDisplay();
    drawButtonPressed();
    AR2Menu();
  }

  if (btn1Sts == LOW) {
    buttonPressed = 1;
    idleDisplay();
    drawButtonPressed();
    displayUpdateCount = 100;
    if (isPadRequired == true) {
      isPadRequired = false;
    } else {
      isPadRequired = true;
    }
  }
}

//Thank you to Wintergatan for the amazing music you make and for inspiring me to make this project. When you have the time make more music even without the marble machine(s).
