//Options//
//Position ribbon
const int sectionSize = 32;//change the 'size' of the note sections on the ribbon
const int upperSP = 920;//set the position where the position ribbon to ignore, this works for my soft pot (https://coolcomponents.co.uk/products/softpot-membrane-potentiometer-500mm)

int offSetSharp = 27;//off set for sharp notes

//Pressure ribbon
const int lowerPrTr = 920; //set the minimum amount of pressure needed for the pressure ribon to activate

//End of Options//

//Code begin//
const int softpotPin = A5;//position ribbon
int preSoftPotRead = analogRead(softpotPin);//preSoftPotRead is the reading used befire it is processed to be sent as a MIDI message, I need both to be able to detect more accuratly where the pressure is
int softPotReading = 0;//this is the presoftpotread deivided by the sectionsize variable to get a final position of where your finger is

const int pressurePotPinLft = A4;//pressure pad left pin
int pressurePotLftRead = analogRead(pressurePotPinLft);

const int pressurePotPinRht = A3;//pressure pad right pin
int pressurePotRhtRead = analogRead(pressurePotPinRht);
int prevPressurePotRhtRead = 0;

//Used for some Control Changes
const int pot0 = A0;
const int pot1 = A1;

int pot0Read = analogRead(pot0);
int pot1Read = analogRead(pot1);

int pot0PrevRead = pot0Read;//prevent midi spam
int pot1PrevRead = pot1Read;//prevent midi spam

const int rhtBtn = 2;//right button
const int midBtn = 3;//Middle button
const int lftBtn = 4;//left button

//status of the buttons
int rhtBtnSts = LOW;
int midBtnSts = LOW;
int lftBtnSts = LOW;

//LEDs
int gLed = 6;//Green LED
int rLed = 7;//Red LED

int lwLed = 12;//left white LED
int rwLed = 8;//right white LED

int noteNum = 0;//The note to play now
int prevNoteNum = 0;//the previous note that was played

int noteVelo = 0;//The reading given by the pressure ribbon, currently programmed to output through the MIDI aftertouch command
int prevNoteVelo = 0;//Prevent sending the same pressure commands


//this is all of the non sharp notes from C2 to around C6
const int noteNumListNoSharp[] = {36, 38, 40, 41, 43, 45, 47, 48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84, 86, 88, 89, 91};//0
//I would recommend using the quantiser offset or Octave offset in VCV to get the key desired to play in rather than altering the controller

//These are all of the notes from A0. The sharp notes on the ribbon didnt sound right to me with the slew limiter on VCV but its probably because im not skilled enough
const int noteNumListSharp[] = {21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96};//1
int noteListSelect = 0;
//int noteNumList[] = {36, 38, 40, 41, 43, 45, 47, 48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84, 86, 88, 89, 91};

bool isOff = true;//Used to determine the status of the controller for the leds and to prevent the main loop from spamming MIDI off messages
bool isRhtPadOff = true;

#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();//I think this configures the serial output to be used for MIDI instead of serial

void setup() {
  //Serial.begin(9600);//DO NOT ENABLE SERIAL UNLESS YOU ARE DUBUGGING!!! THIS WILL BREAK THE MIDI OUTPUT COMPLETELY
  MIDI.begin();//Start MIDI

  //set pins for buttons as inputs
  pinMode(rhtBtn, INPUT);
  pinMode(midBtn, INPUT);
  pinMode(lftBtn, INPUT);

  //for some reason the buttons need a voltage through them to work
  digitalWrite(rhtBtn,HIGH);
  digitalWrite(midBtn,HIGH);
  digitalWrite(lftBtn,HIGH);

  //these position and pressure pots need the pins set to high otherwise the readings are completely useless
  digitalWrite(softpotPin, HIGH);
  digitalWrite(pressurePotPinLft, HIGH);
  digitalWrite(pressurePotPinRht, HIGH);

  //setup LEDs
  pinMode(lwLed, OUTPUT);
  pinMode(rwLed, OUTPUT);
  pinMode(gLed, OUTPUT);//the LEDs onboard the MID shield have there on/off state flipped so High is off and Low is on, for some reason
  pinMode(rLed, OUTPUT);

  digitalWrite(lwLed, HIGH);
  digitalWrite(gLed, HIGH);
  digitalWrite(rLed, LOW);
  //flash LEDs to show power on
  delay (500);
  digitalWrite(rwLed, HIGH);
  digitalWrite(lwLed, LOW);
  digitalWrite(gLed, LOW);
  digitalWrite(rLed, HIGH);
  delay (500);
  digitalWrite(rwLed, LOW);
  digitalWrite(lwLed, HIGH);
  digitalWrite(gLed, HIGH);
  digitalWrite(rLed, HIGH);
}

void readPosition(){
  preSoftPotRead = analogRead(softpotPin);//get position value
  if (preSoftPotRead < upperSP) {//turn on green led if postion ribbon is being used
    digitalWrite(gLed, LOW);
  }
  else {//otherwise turn it off
    digitalWrite(gLed, HIGH);
  }
}

void readPressure(){
  pressurePotLftRead = analogRead(pressurePotPinLft);
  //Invert the reading
  noteVelo = (1024+(pressurePotLftRead*-1))-900;

  if ((noteVelo>0)&&(noteVelo<128)){//add .15 for every 1 value. Do this because it scales well, probably a better way
    noteVelo = noteVelo*1.25;
  }
  if (noteVelo>127){//make sure that we dont send a value higher than 127 cause this will wrap around to 0
    noteVelo = 127;
  }
  if (noteVelo<0){//make sure we dont send anything lower than 0 cause of ^this^ issue but opposite
    noteVelo = 0;
  }
  
  if (noteVelo != prevNoteVelo) {//only send an update if a change is detected in the pressure
    MIDI.sendAfterTouch(noteVelo, 1);//Send the pressure reading through aftertouch
    prevNoteVelo = noteVelo;
  }

  if (pressurePotLftRead <= lowerPrTr){//turn on the red led if the left pressure pad is being used
    digitalWrite(rLed, LOW);
  }
  else {
    digitalWrite(rLed, HIGH);//otherwise turn it off
  }
}


void readModWheel(){
  pressurePotRhtRead = analogRead(pressurePotPinRht);//read the right pressure pad
  pressurePotRhtRead = ((pressurePotRhtRead/8)*-1)+127;//the original reading starts from 1024 and with pressure decreases to 0, we need it the other way round and inly with a max of 127
  if (pressurePotRhtRead != prevPressurePotRhtRead){//prevent sending infinate MIDI messages and only message when there is a change
    if (pressurePotRhtRead >2){//dont send if there is a small change, this reading can be caused by changes in the other resistor values
      MIDI.sendControlChange(1, pressurePotRhtRead, 1);//send modulation command
      prevPressurePotRhtRead = pressurePotRhtRead;//make the prev reading the same as the current one so to fail the check until the readings change
      isRhtPadOff = false;
    }
    else if (isRhtPadOff == false){//the delay built into the program dont give the arduino enough time to see the right pad coming off in some cases this should solve that
      MIDI.sendControlChange(1, 0, 1);//send modulation command
      isRhtPadOff = true;
    }
  }
}

void readPot0(){//programmed to read and then send MIDI CC 5 (portamento effect)
  pot0Read = analogRead(pot0);
  if (pot0Read != pot0PrevRead){
    MIDI.sendControlChange(5, pot0Read/8, 1);
    pot0PrevRead = pot0Read;
  }
}

void readPot1(){//programmed to read and then send MIDI CC 91 (Effect Depth 1)
  pot1Read = analogRead(pot1);
  if (pot1Read != pot1PrevRead){
    MIDI.sendControlChange(91, pot1Read/8, 1);
    pot1PrevRead = pot1Read;
  }
}


void loop() {//the juice of the program
  delay(10);//dame as the delay further down, reduces MIDI messages, they also wont add since the 2 loops happen seperatly
  readPressure();//get the pressure
  readPosition();//get position value
  readModWheel();//get and send the right pressure pad through mod cmd

  //check for button presses
  rhtBtnSts = digitalRead(rhtBtn);
  if (rhtBtnSts == LOW){
    MIDI.sendStart();//send command
    delay(200);//delay to prevent repeat presses
  }
  midBtnSts = digitalRead(midBtn);
  if (midBtnSts == LOW){
    MIDI.sendStop();//send command
    delay(200);//delay to prevent repeat presses
  }
  lftBtnSts = digitalRead(lftBtn);
  if (lftBtnSts == LOW){
    if (noteListSelect == 0){
      noteListSelect = 1;
      digitalWrite(rwLed, HIGH);
      digitalWrite(lwLed, LOW);
    }
    else {
      noteListSelect = 0;
      digitalWrite(rwLed, LOW);
      digitalWrite(lwLed, HIGH);
    }
    delay(200);//delay to prevent repeat presses
  }
  while((pressurePotLftRead <lowerPrTr) && (preSoftPotRead <upperSP)) {//If both the pressure pad and postion ribon are in use then play some music!!!
    if (isOff == true) {
      isOff = false;
    }
    preSoftPotRead = analogRead(softpotPin);//read ribbon value value
    softPotReading = preSoftPotRead/sectionSize;//divide the reading by sectionSize to fit bigger 'sections' on the ribbon 
    if (noteListSelect == 0){
      noteNum = noteNumListNoSharp[softPotReading];//get the actual midi note number to send
    }
    else {
      noteNum = noteNumListSharp[softPotReading]+offSetSharp;//get the actual midi note number to send
    }
    

    pressurePotRhtRead = analogRead(pressurePotPinRht);//this next section fixes an issue where if the right pressure pad was released to quickly the value would not always return to 0
    pressurePotRhtRead = ((pressurePotRhtRead/8)*-1)+127;//the original reading starts from 1024 and with pressure decreases to 0, we need it the other way round and inly with a max of 127
    MIDI.sendControlChange(1, pressurePotRhtRead, 1);

    MIDI.sendNoteOn(noteNum, 127, 1);//send the currently playing note

    prevNoteNum = noteNum;//make the notes the same to prevent the break condition later 

    while(true){//keep checking the ribon position hasn't changed so we can keep the note ON WITHOUT sending infinate note on/off commands
      preSoftPotRead = analogRead(softpotPin);//read ribbon value value
      softPotReading = preSoftPotRead/sectionSize;//divide the reading by sectionSize to fit bigger 'sections' on the ribbon 
      if (noteListSelect == 0){
        noteNum = noteNumListNoSharp[softPotReading];//get the actual midi note number to send
      }
      else {
        noteNum = noteNumListSharp[softPotReading]+offSetSharp;//get the actual midi note number to send
      }
      
      readPressure();//get the pressure
      readModWheel();//get and send the right pressure pad through mod cmd
      readPot0();
      readPot1();

      //MIDI.sendAfterTouch(noteVelo, 1);//Send the pressure reading through aftertouch

      delay(10);//delay for stability, prevents too many MIDI messages from being sent, probably ok to remove but I dont notice the difference

      if ((pressurePotLftRead > lowerPrTr) || (preSoftPotRead >upperSP)){//if the pressure reading OR the soft pot reading is not being used break...
        break;
      }
      else if (noteNum != prevNoteNum) {//if the notes have changed and both fingers are on the pressure pad and position ribbon then 
        
        MIDI.sendNoteOn(noteNum, 127, 1);//send the new note on FIRST then 
        MIDI.sendNoteOff(prevNoteNum, 0, 1);//turn off the previous one, this solves an unwanted retrigger issue in VCV Rack and alows proper behaviour of slew
        prevNoteNum = noteNum;//store the previous note
      }
    }
  }
  if (isOff==false) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);//...and turn off the note.
    isOff=true;
  }
}

//Thank you to Wintergatan for the amazing music you make. When you have the time make more music even without the marble machine(s).
