#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>

#include <MIDI.h>


//Options//
//Position ribbon
const int sectionSize = 32;//change the 'size' of the note sections on the ribbon. Increase the number to make the sections bigger or decrease the number to make the sections smaller. 
//This does not take into account the variable below so you may have a partial section at the start or end of the ribbon
const int upperSP = 40;//set the position where the position ribbon to ignore any value below, this works for my soft pot (https://coolcomponents.co.uk/products/softpot-membrane-potentiometer-500mm)

int offSetSharp = 27;//off set for sharp notes

bool isPadRequired = true;//Set to false if you want to play music with just the ribbon can be toggled on or off on the menu

bool inNotePitch = false;//Enables or disables the pitch bend effect within each note section
const int countToPitch = 50;//How long the program will count before allowing pitch bend. NOT IN ms.

//Aftertouch Pot (left pot)
const int lowerPrTr = 100; //set the minimum amount of pressure needed for the pressure ribon to activate

//Control change pot (right pot) 
const int rhtPotControlChanel = 1;//Chooses which MIDI CC channel to send information out of. 1 = Modulation wheel, 74 = Equator height/slide

//Poly Play. Works best when in Diatonic mode, plays notes 2 above the current note played on the ribbon. Repeates for the number in the variable
int polyCount = 1;//MUST BE between 1-4. Number of poly channels supports. Can be changed in the menu

//Debuging
const bool debug = false;//set to true to show more variables on the OLED and to enable pixel blink

//End of Options//




//Code begin//
const int softpotPin = A2;//position ribbon
int preSoftPotRead = analogRead(softpotPin);//preSoftPotRead is the reading used before it is processed to be sent as a MIDI message, I need both to be able to detect more accuratly where the position is
int softPotReading = 0;//this is the presoftpotread deivided by the sectionsize variable to get a final position of where your finger is

const int pressurePotPinLft = A1;//pressure pad left pin
int pressurePotLftRead = analogRead(pressurePotPinLft);

const int pressurePotPinRht = A0;//pressure pad right pin
int pressurePotRhtRead = analogRead(pressurePotPinRht);
int prevPressurePotRhtRead = 0;

//Used for some Control Changes
const int pot0 = A3;
int pot0Read = analogRead(pot0);
int pot0PrevRead = pot0Read;//prevent midi spam
/*A3 Currently does not exist in my model. 
Whilst the RP2040 has 4 analogue in pins, the stock Pico from RPi only has 3 to the GPIO pins, which are all in use
There are 3rd party RP2040 which often include all 4 analogue ins. 
I've ordered a few and will find out how good they are
*/

const int btn3 = 6;//button 1
const int btn2 = 7;//button 2
const int btn1 = 8;//button 3

//status of the buttons
int btn3Sts = LOW;
int btn2Sts = LOW;
int btn1Sts = LOW;

//LEDs
const int builtInLedPin = 25;//pin for pico built in led





int noteVelo = 0;//The reading given by the pressure ribbon, currently programmed to output through the MIDI aftertouch command
int prevNoteVelo = 0;//Prevent sending the same pressure commands

//this is all of the non sharp notes from C2 to around C6 essentially the key of C
const int noteNumListNoSharp[] = {36, 38, 40, 41, 43, 45, 47, 48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84, 86, 88, 89, 91, 93, 95, 96, 98, 100, 101, 103, 105, 107, 108};
const char * noteNumNameNoSharp[] = {"C2", "D2", "E2", "F2", "G2", "A2", "B2", "C3", "D3", "E3", "F3", "G3", "A3", "B3", "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", "D5", "E5", "F5", "G5", "A5", "B5", "C6", "D6", "E6", "F6", "G6", "A6", "B6", "C7", "D7", "E7", "F7", "G7", "A7", "B7", "C8"};
//I would recommend using a MIDI transpose function in your choice of DAW instead of altering the abovr line of code

//These are all of the notes from A0. The sharp notes on the ribbon didnt sound right to me especially with the portamento effect but its probably because im not skilled enough
const int noteNumListSharp[] = {21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96};
//Also probably wouldn't sound right with poly mode due to the way I've programmed poly mode

//determines what list is currently in use, 0 is diatonic, 1 is chromatic
int noteListSelect = 0;

bool isOff = true;//Used to determine the status of the controller for the leds and to prevent the main loop from spamming MIDI off messages
bool isLftPadOff = true;//Makes sure that when the left pad is off, the controller will set aftertouch to 0


//Variables for in note pitch shift, settings can be found at top of program
int pitchCount = 0;//count to incrament until matched with countToPitch
int inNotePos = 0;//remember the inital position
bool hasPitchBendStarted = false;//to help determine the status of pitch bending
int pitchBendValue = 0;//amount to pitch bend by

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

//OLED Setup
int displayUpdateCount = 0;//determines what is shown on the OLED during idle states
int blinkPixel = false;//one pixel on the screen blinks to show when the screen is redrawn, used to be more visable but now blinks very quickly thanks to the pico

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//MIDI setup
Adafruit_USBD_MIDI usb_midi;// USB MIDI object

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);
//sends MIDI over USB instead of serial

void setup() {
 pinMode(builtInLedPin, OUTPUT);//define on board LED
 digitalWrite(builtInLedPin, HIGH);//Light LED to show when initialisation is done
  
  //Initiate LCD
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();//clear display
  display.display();

  //Initialise display settings
  display.setTextColor(1);
  //display.setRotation(2);//flip display if needed
  display.setCursor(10, 11);//Show boot status on OLED
  display.print("AR2M is booting...");
  display.display();

  //Start MIDI
  MIDI.begin();

  //set pins for buttons as inputs
  pinMode(btn3, INPUT_PULLDOWN);
  pinMode(btn2, INPUT_PULLDOWN);
  pinMode(btn1, INPUT_PULLDOWN);
  digitalWrite(btn3,HIGH);
  digitalWrite(btn2,HIGH);
  digitalWrite(btn1,HIGH);

  //sometimes I get better readings when the pins are set to high sometimes they don't change, seems to be controller dependant
  digitalWrite(softpotPin, HIGH);
  //digitalWrite(pressurePotPinLft, HIGH);
  //digitalWrite(pressurePotPinRht, HIGH);

  displayUpdateCount = 100;//prevents the liveInfo screen showing instantly after boot. Looks neater in my opinion

  delay (500);//technically these delays arn't needed, they just make the OLED look cool ;)
  digitalWrite(builtInLedPin, LOW);//turn off led to show compleated boot status
  display.clearDisplay();
  display.setCursor(25, 11);
  display.print("AR2M is READY");
  display.display();
  delay(500);
}

//gets the current position of the ribbon, used to contain more code which is why its in a function
void readPosition(){
  preSoftPotRead = analogRead(softpotPin);//get position value
}

//get and sends pressure/aftertouch. Also doesn't send data if its the same value as before
void readPressure(){
  pressurePotLftRead = analogRead(pressurePotPinLft);//get data for activation of controller

  //I've deliberatly added a 'buffer' so that the pad activates the controller without sending any AT commands unless the pad is held down further
  noteVelo = map(constrain(pressurePotLftRead, 512, 1023), 512,1023,0,127);//change data for use as a MIDI CC
  
  if (noteVelo != prevNoteVelo) {//only send an update if a change is detected in the pressure
    MIDI.sendAfterTouch(noteVelo, 1);//Send the pressure reading through aftertouch
    prevNoteVelo = noteVelo;
  }
}

//gets the right pressure pad data and sends as a MIDI CC channel
void readModWheel(){
  //both the best and worst line of code I have ever written
  pressurePotRhtRead = constrain(map(constrain(analogRead(pressurePotPinRht),20,1023),20,1023,0,127), 0, 127);//read the right pressure pad and change from 0-1023 to 0-127
  //gets the data, add a 'buffer' with constrain, map to fit with a MIDI CC channel then constrain that so the map doesn't output a negative number

  if (pressurePotRhtRead != prevPressurePotRhtRead){
    MIDI.sendControlChange(rhtPotControlChanel, pressurePotRhtRead, 1);//send modulation command
    prevPressurePotRhtRead = pressurePotRhtRead;//make the prev reading the same as the current one so to fail the check until the readings change
    isLftPadOff = false;
  }
}

//programmed to read and then send MIDI CC 16 = general purporse 1
void readPot0(){
  /*currently diabled as it doesn't currently exist due to the stock pico only haveing 3 analogue ins
  pot0Read = analogRead(pot0)/8;//divide reading by 8 since 1024/8 = 128
  if (pot0Read != pot0PrevRead){//prevent spam by comparing previous values
    MIDI.sendControlChange(16, pot0Read, 1);//send command
    pot0PrevRead = pot0Read;//store previous value
  }
  */
}


void pressurePadOffWarn(){//flash the green led to indicate that the left pad isnt required to play music
  //to be rewritten if the pico used will contain a digital led
}


void pitchBend() {//Code responsible for pitch bending
  if (inNotePitch == true) {//only run if inNotePitch is set to true, skip if false
    //Because a delay would prevent me from changing notes i need to count up to a variable to match another this allows me to wait and still use the controller whilst this is happening. Same concept behind the red led flash 
    if (noteNum == prevNoteNum) {
      pitchCount++;
    }//no need to reset counter as this will be done as part of the main music loop in switching notes
    if (pitchCount >= countToPitch) {//if the same note has been played for a certain period of time, enable pitch bending
      if (hasPitchBendStarted == false){
        inNotePos = preSoftPotRead;
        hasPitchBendStarted = true;
      }
      pitchBendValue = inNotePos - preSoftPotRead; //Takes the difference from the starting point in each note and compares it to the current position
      MIDI.sendPitchBend(0-constrain(pitchBendValue*100, -6500, 6500), 1);
    }
  }
}

//gets the ribbon position and the note number. Deals with poly modes as well
void noteSelect() {
  preSoftPotRead = analogRead(softpotPin);//read ribbon value value
  softPotReading = preSoftPotRead/sectionSize;//divide the reading by sectionSize to fit bigger 'sections' on the ribbon 
  if (noteListSelect == 0){
    noteNum = noteNumListNoSharp[softPotReading];//get the actual midi note number to send
    noteNum2 = noteNumListNoSharp[softPotReading+2];
    noteNum3 = noteNumListNoSharp[softPotReading+4];
    noteNum4 = noteNumListNoSharp[softPotReading+6];
  }
  else {
    noteNum = noteNumListSharp[softPotReading]+offSetSharp;//get the actual midi note number to send
    noteNum2 = noteNumListSharp[softPotReading+2]+offSetSharp;//get the actual midi note number to send
    noteNum3 = noteNumListSharp[softPotReading+4]+offSetSharp;//get the actual midi note number to send
    noteNum4 = noteNumListSharp[softPotReading+6]+offSetSharp;//get the actual midi note number to send
  }
}

void resetBend() {//I had this coppied 4 times, put it in a function to make it neater
  hasPitchBendStarted = false;
  pitchBendValue = 0;
  pitchCount = 0;
  inNotePos = 0;
  MIDI.sendPitchBend(0,1);
}

void noteOn() {//sends the MIDI note on command, deals with POLY mode
  if (polyCount == 1){
    MIDI.sendNoteOn(noteNum, 127, 1); 
  }
  if (polyCount == 2){
    MIDI.sendNoteOn(noteNum, 127, 1); 
    MIDI.sendNoteOn(noteNum2, 127, 1); 
  }
  if (polyCount == 3){
    MIDI.sendNoteOn(noteNum, 127, 1); 
    MIDI.sendNoteOn(noteNum2, 127, 1); 
    MIDI.sendNoteOn(noteNum3, 127, 1); 
  }
  if (polyCount == 4){
    MIDI.sendNoteOn(noteNum, 127, 1); 
    MIDI.sendNoteOn(noteNum2, 127, 1); 
    MIDI.sendNoteOn(noteNum3, 127, 1); 
    MIDI.sendNoteOn(noteNum4, 127, 1); 
  }
}

void noteOff() {//sends the MIDI note off command, deals with POLY mode
    if (polyCount == 1){
      MIDI.sendNoteOff(prevNoteNum, 0, 1);
    }
    if (polyCount == 2){
      MIDI.sendNoteOff(prevNoteNum, 0, 1);
      MIDI.sendNoteOff(prevNoteNum2, 0, 1);
    }
    if (polyCount == 3){
      MIDI.sendNoteOff(prevNoteNum, 0, 1);
      MIDI.sendNoteOff(prevNoteNum2, 0, 1);
      MIDI.sendNoteOff(prevNoteNum3, 0, 1);
    }
    if (polyCount == 4){
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
void updateButtons(){
  btn1Sts = digitalRead(btn1);
  btn2Sts = digitalRead(btn2);
  btn3Sts = digitalRead(btn3);
}

//Functions for SSD1306 128x32 OLED//
/*
Initially I was worried that the screen would add to much overhead to run alongside everything else
That was an issue on the UNO R3 but now that I have moved to the Pico, it can handle a lot more than the UNO
The contoller is completely functional without the screen, although its harder to see what it's upto and to change settings on the go
*/

int buttonPressed = 0;//helps determine which button was pressed.

//draw white boxes to represent the buttons
void drawButtons(){
  display.fillRect(117, 0, 10, 10, 1);
  display.fillRect(117, 11, 10, 10, 1);
  display.fillRect(117, 22, 10, 10, 1);
}

//display one of 3 buttons being pressed, draws black boxes in the white boxes to simulate a press
void drawButtonPressed(){
  if (buttonPressed == 3){
    display.fillRect(118, 23, 8, 8, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  }
  else if  (buttonPressed == 2){
    display.fillRect(118, 12, 8, 8, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  }
  else if (buttonPressed == 1) {
    display.fillRect(118, 1, 8, 8, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  }
}

void drawCross(){
  if (buttonPressed == 3){
    display.drawLine(118, 23, 125, 30, 0);
    display.drawLine(118, 30, 125, 23, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  }
  else if  (buttonPressed == 2){
    display.drawLine(118, 12, 125, 19, 0);
    display.drawLine(118, 19, 125, 12, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  }
  else if (buttonPressed == 1) {
    display.drawLine(118, 1, 125, 8, 0);
    display.drawLine(118, 8, 125, 1, 0);
    display.display();
    buttonPressed = 0;
    delay(200);
  }
}

//blink one pixel on the display so I can see when it's being updated and if the program is frozen
void screenBlink() {
  if (debug == true){//only turn on if debugging is on
    if (blinkPixel == false){
      display.drawPixel(127, 0, 0);
      blinkPixel = true;
    }
    else{
      display.drawPixel(127, 0, 1);
      blinkPixel = false;
    }
  }
}

//Draws the idle menu when the controller isn't doing anything
void idleDisplay(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  if (isPadRequired == true) {
    display.print("Ribbon only = OFF >");
  }
  else {
    display.print("Ribbon only = ON ->");
  }
  display.setCursor(0,11);
    display.print("AR2M Menu -------->");
  display.setCursor(0,22);
  if (inNotePitch == false){
    display.print("Pitch bend = OFF ->");
  }
  else {
    display.print("Pitch bend = ON -->");

  }
  drawButtons();
  screenBlink();
  
  display.display();
}

void AR2Menu(){//The main menu once the middle button is pressed
  buttonPressed = 2;
  while (true){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Exit AR2M Menu --->");
    display.setCursor(0, 11);
    display.print("Ribbon Scale ----->");
    display.setCursor(0, 22);
    display.print("Next Menu (1/2) -->");
    drawButtons();
    screenBlink();
    display.display();

    updateButtons();
    if (btn1Sts == LOW){//exits the menu and returns to the idle screens
      buttonPressed = 1;
      drawButtonPressed();
      updateButtons();
      displayUpdateCount = 100;//prevents the liveInfo screen showing if button is pressed early on
      break;
    }
    if (btn2Sts == LOW){//allows selection of position ribbon mode
      buttonPressed = 2;
      drawButtonPressed();
      while (true){
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Ribbon mode select ");
        display.setCursor(0, 11);
        if (noteListSelect == 0){//show star to represent the current active mode
          display.print("Diatonic * ------->");
          display.setCursor(0, 22);
          display.print("Chromatic -------->");
        }
        else {
          display.print("Diatonic --------->");
          display.setCursor(0, 22);
          display.print("Chromatic * ------>");
        }
        drawButtons();
        screenBlink();
        display.display();
        updateButtons();
        if (btn2Sts == LOW){
          buttonPressed = 2;
          drawButtonPressed();
          noteListSelect = 0;
          updateButtons();
          break;
        }
        if (btn3Sts == LOW){
          buttonPressed = 3;
          drawButtonPressed();
          noteListSelect = 1;
          updateButtons();
          break;
        }
      }
    } 
    if (btn3Sts == LOW){//progress to next menu layer
      buttonPressed = 3;
      drawButtonPressed();
      while (true){
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Poly Mode -------->");
        display.setCursor(0, 11);
        display.print("------------------>");
        display.setCursor(0, 22);
        display.print("Main Menu (2/2) -->");
        drawButtons();
        screenBlink();
        display.display();
        
        updateButtons();
        if (btn1Sts == LOW){
          buttonPressed = 1;
          drawButtonPressed();
          while (true){
            display.clearDisplay();
            display.setCursor(0, 0);
            display.print("Exit ------------->");
            display.setCursor(0, 11);
            display.print("Current      Dec ->");
            display.setCursor(0, 22);
            display.print(("Mode: ")+String(polyCount)+("      Inc ->"));
            drawButtons();
            display.display();
            updateButtons();
            if (btn1Sts == LOW) {
              buttonPressed = 1;
              drawButtonPressed();
              updateButtons();
              break;
            }
            else if (btn3Sts == LOW) {
              if (polyCount >= 4) {
                buttonPressed = 3;
                drawCross();
              }
              else {
                buttonPressed = 3;
                drawButtonPressed();
                polyCount++;
              }
            }
            else if (btn2Sts == LOW) {
              if (polyCount <= 1) {
                buttonPressed = 2;
                drawCross();
              }
              else {
                buttonPressed = 2;
                drawButtonPressed();
                polyCount--;
              }
            }
          }
        }
        else if (btn2Sts == LOW) {
          //nothing in this slot yet
        }
        else if (btn3Sts == LOW) {
          buttonPressed = 3;
          drawButtonPressed();
          updateButtons();
          break;
        }
      }
    }
  }
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
  display.setCursor(0, 11);
  display.print(pressurePotRhtRead);
  display.setCursor(64, 11);
  if (isOff == false) {
    display.print("Sending");
  }
  else {
    display.print("Off");
  }
  display.setCursor(0, 22);
  display.print(btn3Sts);
  display.setCursor(33, 22);
  display.print(btn2Sts);
  display.setCursor(66, 22);
  display.print(btn1Sts);
  screenBlink();
  display.display();
}

//shows information useful when controller is active
void liveInfo() {
  display.clearDisplay();
  display.setCursor(0, 6);
  display.setTextSize(3);
  if (analogRead(softpotPin) > upperSP){
    display.print(noteNumNameNoSharp[analogRead(softpotPin)/sectionSize]);
  }
  display.setTextSize(1);
  display.setCursor(40, 0);
  display.print("AT:");
  display.print(noteVelo);
  display.setCursor(87, 0);
  display.print("MW:");
  display.print(pressurePotRhtRead);
  display.setCursor(40,11);
  display.print("MIDI is ");
  if (isOff == false) {
    display.print("On");
  }
  else {
    display.print("Off");
  }
  display.setCursor(40, 22);
  display.print("RT:");
  display.print(millis());//show current runtime
  screenBlink();
  display.display();
}


//put the main part of the code in a function seperate from the main loop so I can control the start behaviour a bit easier
void music() {
  if (isOff == true) {//to prevent sending infinate note off commands at the end of the main loop
    isOff = false;
  }
  noteSelect();//get note
  digitalWrite(builtInLedPin, HIGH);

  pressurePotRhtRead = analogRead(pressurePotPinRht);//this next section fixes an issue where if the right pressure pad was released to quickly the value would not always return to 0
  pressurePotRhtRead = ((pressurePotRhtRead/8)*-1)+127;//the original reading starts from 1024 and with pressure decreases to 0, we need it the other way round and inly with a max of 127
  //MIDI.sendControlChange(1, pressurePotRhtRead, 1);//is this needed???

  noteOn();//send the first on
  storeNote();//make prev note the same as current note as if this was different it could reak the loop early and create weird sounds

  while(true){//keep checking the ribon position hasn't changed so we can keep the note ON WITHOUT sending infinate note on/off commands
    noteSelect();//get note

    pitchBend();//only runs if inNotePitch is true, off by default
    pressurePadOffWarn();//Flashes the green led if the check for the left pressure pad is off
    readPressure();//get the pressure
    readModWheel();//get and send the right pressure pad through mod cmd
    readPot0();//get current reading of the pot on A0

    if (debug == false){//if debug is off show the normal screen
      liveInfo();
    }
    else {//otherwise show the more advanced screen
      showVars();
    }

    delay(10);//delay for stability, prevents too many MIDI messages from being sent, probably ok to remove but I dont notice the difference. Any more than this is noticable when playing
    if (isPadRequired == true){
      if ((pressurePotLftRead < lowerPrTr) || (preSoftPotRead < upperSP)){//if the pressure reading OR the soft pot reading is not being used reset pitch bend stuff and break...
        break;
      }
      else if (noteNum != prevNoteNum) {//if the notes have changed and both fingers are on the pressure pad and position ribbon then 
        if (hasPitchBendStarted == true) {//slightly different order to make pitch bend sound somewhat reasonable
          noteOff();
          resetBend();
          noteOn();
        }
        else {
          noteOn();
          noteOff();
          resetBend();
        }
        storeNote();
      }
    }

    else if (isPadRequired == false){//when playing on ribbon only mode, ignore the additional check of the left pad
      if (preSoftPotRead < upperSP){//if the soft pot reading is not being used break...
        resetBend();
        break;//maybe put the break statements in a function
      }
      else if (noteNum != prevNoteNum) {//if the notes have changed then 
        resetBend();//reset pitch bending
        noteOn();//send new note
        noteOff();//turn off previous note
        storeNote();//store the current note
      }
    }
  }
}

void loop() {//The main loop itself doesn't play any music, it does instead control the start behaviour of what plays music
  delay(10);//same as the delay in music function, reduces MIDI messages, they also wont add since the 2 loops happen seperatly
  readPressure();//get the pressure
  readPosition();//get position value
  readModWheel();//get and send the right pressure pad through mod cmd
  readPot0();

  if ((preSoftPotRead > upperSP) || (noteVelo>0) || (pressurePotRhtRead>0)) {
    displayUpdateCount = 0;
  }

  if (debug==false){
    if (displayUpdateCount < 100){//show the live info display a bit after the controller has finished playing notes
      liveInfo();
      displayUpdateCount++;
    }
    else {//otherwise show the menu display
      idleDisplay();
    }
  }
  else {
    showVars();//Comment when not debugging
  }
  
  updateButtons();
  //check for right button presses

  if (btn3Sts == LOW){
    buttonPressed = 3;
    idleDisplay();
    drawButtonPressed();
    displayUpdateCount = 100;

    if (inNotePitch == true) {
      inNotePitch = false;
    }
    else {
      inNotePitch = true;
    }
    delay(200);
  }
  
  if (btn2Sts == LOW){
    buttonPressed = 2;
    idleDisplay();
    drawButtonPressed();
    AR2Menu();
  }
  
  if (btn1Sts == LOW){
    buttonPressed = 1;
    idleDisplay();
    drawButtonPressed();
    displayUpdateCount = 100;
    if (isPadRequired == true){
      isPadRequired = false;
    }
    else {
      isPadRequired = true;
    }
  }

  if (isPadRequired == true){
    while((pressurePotLftRead > lowerPrTr) && (preSoftPotRead > upperSP)) {//If both the pressure pad and postion ribon are in use then play some music!!!
      music();
    }
  }
  else{
    pressurePadOffWarn();//flash red LED to show that the pad isn't required
    while(preSoftPotRead > upperSP){
      music();
    }
  }
  if (isOff==false) {
    noteOff();//...and turn off the note.
    if (inNotePitch == true){
      resetBend();
    }
    displayUpdateCount = 0;//keep the display on the stats info for a bit after midi has turned off
    isOff=true;
    digitalWrite(builtInLedPin, LOW);
  }
}

//Thank you to Wintergatan for the amazing music you make and for inspiring me to make this project. When you have the time make more music even without the marble machine(s).
