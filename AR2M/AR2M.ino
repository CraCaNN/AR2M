//Options
//Debugging
const bool debug = false;//Set to true to enable serial and DISABLE MIDI. Serial will print out pot values, atm not much will happen with this variable

//Position ribbon
const int sectionSize = 32;//change the 'size' of the note sections on the ribbon 
const int offSet = 0; //27 for midi c3 at base of strip 
const int upperSP = 920;//set the position where the position ribbon to ignore

//Pressure ribbon
const int lowerPrTr = 920; //set the minimum amount of pressure needed for the pressure ribon to activate

//End of Options//

//Code begin//
const int softpotPin = A5;//position ribbon
int preSoftPotRead = analogRead(softpotPin);

const int pressurePotPinLft = A4;//pressure pad left
int pressurePotLftRead = analogRead(pressurePotPinLft);

const int pressurePotPinRht = A3;//pressure pad right
int pressurePotRhtRead = analogRead(pressurePotPinRht);

//For use as pitch and modulation 'wheel'
const int pot0 = A0;
const int pot1 = A1;


int pot0Read = analogRead(pot0);
int pot1Read = analogRead(pot1);


const int rhtBtn = 2;//right button
const int midBtn = 3;//Middle button
const int lftBtn = 4;//left button

//status on the buttons
int rhtBtnSts = LOW;
int midBtnSts = LOW;
int lftBtnSts = LOW;

#include <MIDI.h>

int gLed = 6;
int rLed = 7;

bool isPosition = false;
bool isPressure = false;

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {

  //set pins for buttons as inputs
  pinMode(rhtBtn, INPUT);
  pinMode(midBtn, INPUT);
  pinMode(lftBtn, INPUT);

  digitalWrite(rhtBtn,HIGH);
  digitalWrite(midBtn,HIGH);
  digitalWrite(lftBtn,HIGH);

  MIDI.begin();//start MIDI

  //these position and pressure pots need the pins set to high otherwise the readings are completely useless
  digitalWrite(softpotPin, HIGH);
  digitalWrite(pressurePotPinLft, HIGH);
  digitalWrite(pressurePotPinRht, HIGH);

  //Serial.begin(9600);//DO NOT ENABLE SERIAL UNLESS YOU ARE DUBUGGING!!! THIS WILL BREAK THE MIDI OUTPUT COMPLETELY
  
  //These LEDs on the MIDI sheild have HIGH as off and LOW as on, for some reason
  pinMode(gLed, OUTPUT);
  digitalWrite(gLed, HIGH);

  pinMode(rLed, OUTPUT);
  digitalWrite(rLed, LOW);
  //flash LEDs to show power on

  delay (500);

  digitalWrite(gLed, LOW);
  digitalWrite(rLed, HIGH);

  delay (500);

  digitalWrite(gLed, HIGH);
  digitalWrite(rLed, HIGH);


}


int noteNum = 0;//The note to play now
int prevNoteNum = 0;//the previous note that was played

int preNoteVelo = 0;//LED support
int noteVelo = 0;//The reading given by the pressure ribbon, currently programmed to output through the MIDI aftertouch command
int prevVeloRead = 127;//Previous note velocity, shouldnt be needed anymore. Could be included later to prevent to many MIDI messages being sent but pressure readings do change a lot so may not bother

int convRhtPad = 0;

int softPotReading = 64;

//this is all of the non sharp notes from C2 to around C6
const int noteNumList[] = {36, 38, 40, 41, 43, 45, 47, 48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84, 86, 88, 89, 91};
//These are all of the notes from A0
//const int noteNumList[] = {21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96};

bool isOff = true; 

int antiSpamCount = 0;

void readPosition(){
  preSoftPotRead = analogRead(softpotPin);//get position value  
  if (preSoftPotRead < upperSP) {
    digitalWrite(gLed, LOW);
  }
  else {
    digitalWrite(gLed, HIGH);
  }
}

void readPressure(){
  pressurePotLftRead = analogRead(pressurePotPinLft);
  //Invert the reading
  noteVelo = (1024+(pressurePotLftRead*-1))-900;

  if ((noteVelo>0)&&(noteVelo<128)){//add .15 for every 1 value
    noteVelo = noteVelo*1.25;
  }
  if (noteVelo>127){//make sure that we dont send a value higher than 127 cause this will wrap around to 0
    noteVelo = 127;
  }
  if (noteVelo<0){//make sure we dont send anything lower than 0 cause of ^this^ issue but opposite
    noteVelo = 0;
  }
  
  if (pressurePotLftRead <= lowerPrTr){
    digitalWrite(rLed, LOW);
  }
  else {
    digitalWrite(rLed, HIGH);
  }
}

void readPressureRht(){
  pressurePotRhtRead = analogRead(pressurePotPinRht);
  //Invert the reading
  //convRhtPad = (1024+(pressurePotLftRead*-1))-900;
  convRhtPad = (1024+(pressurePotRhtRead*-1))*16;

  
  if (convRhtPad > 250){
    if ((convRhtPad>0)&&(convRhtPad<16384)){
      convRhtPad = convRhtPad-250;
    }
    if (convRhtPad>16384){//make sure that we dont send a value higher than 16384 cause this will wrap around to 0
      convRhtPad = 16384;
    }
    if (convRhtPad<0){//make sure we dont send anything lower than 0 cause of ^this^ issue but opposite
      convRhtPad = 0;
    }
    /*
    if (pressurePotLftRead <= lowerPrTr){
      digitalWrite(rLed, LOW);
    }
    else {
      digitalWrite(rLed, HIGH);
    }
    */
    MIDI.sendPitchBend(convRhtPad-8192,1);
  }
}

void readModWheel(){
  pressurePotRhtRead = analogRead(pressurePotPinRht);
  MIDI.sendControlChange(1, ((pressurePotRhtRead/8)*-1)+127, 1);
}

void loop() {//the juice of the program
  readPressure();//get the pressure
  readPosition();//get position value
  //readPressureRht();


  readModWheel();
  //MIDI.sendPitchBend(convRhtPad-8192,1);

  rhtBtnSts = digitalRead(rhtBtn);
  if (rhtBtnSts == LOW){
    MIDI.sendStart();
    delay(200);
    
  }
  midBtnSts = digitalRead(midBtn);
  if (midBtnSts == LOW){
    MIDI.sendStop();
    delay(200);
    
  }
  lftBtnSts = digitalRead(lftBtn);
  if (lftBtnSts == LOW){
    MIDI.sendContinue();
    delay(200);
  }
  while((pressurePotLftRead <lowerPrTr) && (preSoftPotRead <upperSP)) {//If both the pressure pad and postion ribon are in use then play some music!!!
    if (isOff == true) {
      isOff = false;
    }
    preSoftPotRead = analogRead(softpotPin);//read ribbon value value
    softPotReading = preSoftPotRead/sectionSize;//divide the reading by sectionSize to fit bigger 'sections' on the ribbon 
    noteNum = noteNumList[softPotReading];//get the actual midi note number to send

    readPressure();//get pressure

    MIDI.sendNoteOn(noteNum, 127, 1);//send the currently playing note

    prevNoteNum = noteNum;//make the notes the same to prevent the break condition later 

    while(true){//keep checking the ribon position hasn't changed so we can keep the note ON
      preSoftPotRead = analogRead(softpotPin);//read ribbon value value
      softPotReading = preSoftPotRead/sectionSize;//divide the reading by sectionSize to fit bigger 'sections' on the ribbon 
      noteNum = noteNumList[softPotReading];//get the actual midi note number to send
      
      readPressure();//get pressure
      readModWheel();//read pot1
      //readPressureRht();

      delay(10);//delay for stability, could probably remove it but if it aint broke dont fix

      MIDI.sendAfterTouch(noteVelo, 1);//Send the pressure reading through aftertouch
      //MIDI.sendPitchBend(pot0Read,1);
      //MIDI.sendControlChange(1, pot1Read, 1);

      if ((pressurePotLftRead > lowerPrTr) || (preSoftPotRead >upperSP)){//if the pressure reading OR the SP reading is not being used break...
        break;
      }
      else if (noteNum != prevNoteNum) {//if the notes have changed send the new note on FIRST then turn off the previous one, this solves an unwanted retrigger issue in VCV Rack
        
        MIDI.sendNoteOn(noteNum, 127, 1);
        MIDI.sendNoteOff(prevNoteNum, 0, 1);
        prevNoteNum = noteNum;//store the previous note
      }
    }
  }
  if (isOff==false) {
    MIDI.sendNoteOff(prevNoteNum, 0, 1);//...and turn off the note.
    isOff=true;
  }
}

/** My thought space, I need to put somewhere
Ill be honest I had a big issue where the program wasn't turning off the MIDI notes in the correct order
so they continued to play in VCV, but through many iterations I programmed something that worked, 
and I'm not to sure what I did, I later broke it again and couldn't fix it, thankfully I had a backup
I don't want to change anything major yet cause I have other ideas I want to work on.
Once I'm happy with how the controller behaves/feels I will try and clean up the code :)


Things I wanted to do but won't now:
I wanted a little OLED (128x32) screen to configure basic stuff like the note offset but the screen changes
the behaviour of the position ribbon in a bad way so no.

The MIDI shield has some buttons and pots on it, I can figure out the pots, but I always somehow struggle 
to program multiple buttons in arduino, so for now they are not used. 

Also the example code for the buttons and pots for this sheild seems overcomplicated and I can't understand it
**/