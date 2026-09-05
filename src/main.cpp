#include <TFT_eSPI.h>
#include <SPI.h>
#include <MIDI.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// Create TFT object
TFT_eSPI tft = TFT_eSPI();

//https://rgbcolorpicker.com/565
//Custom Colours here (yes I'm from the UK)




//Pin Configuration that does not include the display
const int btn1 = 4;
const int btn2 = 5;
const int btn3 = 6;

const int rLED = 8;
const int gLED = 9;
const int bLED = 10;

// Define button structure

//the ID will just be the position within the array
//parentID is where each sub-menu came from in 
//childType is whether the selected menu item is another sub-menu or a config 
//0 = config 
//1 = another menu
//childID is the ID of either the sub menu or the config, determined by childType
//if childType is a config, then the program can get it's current state
//iconID is what icon to use, -1 is no icon
//liveDescriptor is what to show if there is a associated value with a config

struct menuItem {
  int parentID, childType, childID, iconID;
  String longName, shortName, liveDescriptor;
  bool disabled;
  String disabledReason;
};


menuItem mainMenu[10] {
  {0,0,0,0,"Key Offset", "Key", "Current Key: ", 0, ""},
  {0,0,1,1,"Octave Offset", "Octave", "Current Oct: ", 0, ""},
  {0,0,2,2,"Scale Select", "Scale", "Current Scl: ", 0, ""},
  {0,1,0,3,"Quick Access Config", "Quick Access", "", 0, ""}
};

menuItem qaConfigMenu {

};


/*Type definition
0 = Fixed state (3-states, if this is used use maxVal to store the total number of states, i.e. either 2 or 3)
1 = Scroll state (>3 states using scroll system, if this is used use maxVal to store the total number of states, max of 10)
2 = Number select ()

The defined state can be thought of as the program default
*/
//each setting and its associated value
struct configLayout {
  int parentID, type, state, minVal, maxVal;
  String name, descriptor, option1, option2, option3, option4, option5, option6, option7, option8, option9, option10;
};

configLayout mainConfig[10] = {
  {0,2,0,-12,12,"Key Offset", "Offset the output to play in different keys"},
  {0,2,0,-3,3,"Octave Offset", "Offset the number of octaves to output"},
  {0,1,1,0,3,"Scale", "Which scale to play in", "Chromatic", "Major","Minor"}
};



/*
  tft.print("Text here")
  tft.setCursor(0,0);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(fontVar);

Acording to AI these are the various different buit-in fonts for setTextFont
Value 	    Font Name	  Description	                    Size/Height
0 or GLCD	  GLCD	      Classic   Adafruit GFX font   	5×7 pixels
2 or FONT2	Font 2    	Standard  bitmap font	          16 pixels high
4 or FONT4	Font 4    	Medium  bitmap font           	26 pixels high
6 or FONT6	Font 6    	Large bitmap font	              48 pixels high
7 or FONT7	Font 7    	7-segment display style       	Numeric display
8 or FONT8	Font 8    	Large numeric font             	For numbers/digits

Therefore a rule for text fonts for this struct:
if the font integer is positive it will refer to the table above and will be set using setTextFont
if the font integer is negative it will refer to a custom FreeFont that has been added to this program and print using setFreeFont
*/

struct sText {
  String text;
  int xPos, yPos, colour, size, datum, font;
};

//The Button element defines the interactable part of the screen
struct Button {
  uint16_t x, y, w, h;
  uint16_t bgColor, textColor, borderColor;
  String label;
  int round;
};

int buttonArrayLen[]= {8,10}; //this will need to be updated, defines the length of each array, my Python head is screaming to use len()

//Probably worth staticing these arrays if possible
//Rules for buttons:
//First 3 elements must ALWAYS be the 3 side button representer
//Parts of the code will reference the size of these first 3 elements if a button is pressed

// 0
Button infoButtons[8] = {
    
  {0,0,19,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"1",0},
  {0,80,19,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"2",0},
  {0,160,19,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"3",0},
  {20,190,199,51,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Key",0},
  {200,190,39,51,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Oct",0},
  {0,240,120,79,TFT_BLACK,TFT_WHITE,TFT_WHITE,"QA 1",0},
  {119,240,120,79,TFT_BLACK,TFT_WHITE,TFT_WHITE,"QA 2",0},
  {80,285,80,35,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Menu",10}
};

// 1
Button mainMenuButtons[10] = {
  //static elements of the menu
  {0,0,39,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"^",0},
  {0,80,39,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,">",0},
  {0,160,39,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"v",0},
  {8,270,100,45,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Exit Menu",10},
  {128,270,100,45,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Return",10},

  //Dynamic elements, 
  {50,0,150,35,TFT_BLACK, TFT_DARKGREY, TFT_DARKGREY, "T Item", 10},
  {50,40,170,35,TFT_BLACK, TFT_LIGHTGREY, TFT_LIGHTGREY, "TM Item", 10},
  {50,80,190,79,TFT_BLACK, TFT_WHITE, TFT_WHITE, "M Item", 0},
  {50,164,170,35,TFT_BLACK, TFT_LIGHTGREY, TFT_LIGHTGREY, "BM Item", 10},
  {50,204,150,35,TFT_BLACK, TFT_DARKGREY, TFT_DARKGREY, "B Item", 10}
};


//The saviour of several KB of flash, processing power, and hopefully more hours of my life than it did to figure this out
Button *pBtnArray[2] = {infoButtons, mainMenuButtons};// this honestly took me several hours to come up with, if this didnt work each layout would have to have its own function to work out what button was pressed

//If true will disable the text renderer and will print the text stored in the button arrays
bool debugTFTbtns = true;

//button renderer if that's a real word
void drawButton(Button &btn) {
  if (btn.round > 0) {
    // Draw border
    tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, btn.round, btn.borderColor);
    
    // Draw background
    tft.fillRoundRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, btn.round, btn.bgColor);
  } else {
    // Draw border
    tft.drawRect(btn.x, btn.y, btn.w, btn.h, btn.borderColor);
    
    // Draw background
    tft.fillRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, btn.bgColor);
  }

  //in the long run I don't want the buttons to be associated with printing out text, since it'll loose positional acuracy
  if (debugTFTbtns == true){
    // Draw text centered
    tft.setTextColor(btn.textColor, btn.bgColor);
    tft.setTextSize(1);
    
    int16_t textX = btn.x + (btn.w - tft.textWidth(btn.label)) / 2;
    int16_t textY = btn.y + (btn.h - 8) / 2;
    
    tft.drawString(btn.label, textX, textY, 2);
  }
}

void drawScreenButtons(int FscreenID) {
  for (int i = 0; i < buttonArrayLen[FscreenID]; i++) {// i < x where x needs to be equal to the number of buttons
    drawButton(pBtnArray[FscreenID][i]);
  }
}


////GLOBAL VARIABLES////

//determines what screen to show along with what buttons to also load
int screenID = -1;
int prevScreenID = -1;




void handleInfoPress(int btnPress) {
  switch (btnPress) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    case 4:
      break;
    case 5:
      break;
    case 6:
      break;
    case 7:
      screenID = 1;
      break;
  }
}


void handleMenuPress(int btnPress) {
  switch (btnPress) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      screenID = 0;
      break;
    case 4:
      break;
    case 5:
      break;
  }
}


void handleButtonPress(uint16_t x, uint16_t y, int ID) {
  for (int i = 0; i<buttonArrayLen[ID]; i++) {
    if (x >= pBtnArray[ID][i].x && x <= (pBtnArray[ID][i].x + pBtnArray[ID][i].w) &&
    y >= pBtnArray[ID][i].y && y <= (pBtnArray[ID][i].y + pBtnArray[ID][i].h)) {
      switch (screenID) {
        case 0:
          handleInfoPress(i);
          break;
        case 1:
          handleMenuPress(i);
          break;
      }
    }
  }
}



void bootScreen() {
  
  //43 is the magic number, soo close
  /*//fancy boot animation, too many problems to work out this early on
  for (int i = 0; i < 2; i++){
    for (int j = 0; j < 85; j++) {
      tft.drawSmoothArc(42,264-j,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
      tft.drawSmoothArc(0,221-j,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
      tft.fillRoundRect(0,200,50,400,10,TFT_BLACK);
      tft.drawRoundRect(0,200,50,400,10,TFT_WHITE);
      tft.drawSmoothArc(42,178-j,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
      tft.drawSmoothArc(0,135-j,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
      tft.drawSmoothArc(42,92-j,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
      tft.drawSmoothArc(0,49-j,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
      tft.drawSmoothArc(42,6-j,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
      
      delay(20);
    }
  }
  */
  tft.drawSmoothArc(137,264,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(95,221,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(137,178,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(95,135,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(137,92,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(95,49,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(137,6,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
  tft.fillRoundRect(95,200,50,400,10,TFT_BLACK);
  tft.drawRoundRect(95,200,50,400,10,TFT_WHITE);


  digitalWrite(rLED,HIGH);
  tft.drawLine(100,200,0,0,TFT_WHITE);
  delay(40);
  tft.drawLine(102,200,12,0,TFT_WHITE);
  delay(40);
  tft.drawLine(104,200,24,0,TFT_WHITE);
  delay(40);
  tft.drawLine(106,200,36,0,TFT_WHITE);
  delay(40);
  tft.drawLine(108,200,48,0,TFT_WHITE);
  delay(40);
  tft.drawLine(110,200,60,0,TFT_WHITE);
  delay(40);
  tft.drawLine(112,200,72,0,TFT_WHITE);
  delay(40);
  tft.drawLine(114,200,84,0,TFT_WHITE);
  delay(40);
  tft.drawLine(116,200,96,0,TFT_WHITE);
  delay(40);
  tft.drawLine(118,200,108,0,TFT_WHITE);
  delay(40);
  digitalWrite(rLED,LOW);
  digitalWrite(gLED,HIGH);
  tft.drawFastVLine(120,0,200,TFT_WHITE);
  delay(40);
  tft.drawLine(122,200,134,0,TFT_WHITE);
  delay(40);
  tft.drawLine(124,200,146,0,TFT_WHITE);
  delay(40);
  tft.drawLine(126,200,158,0,TFT_WHITE);
  delay(40);
  tft.drawLine(128,200,170,0,TFT_WHITE);
  delay(40);
  tft.drawLine(130,200,182,0,TFT_WHITE);
  delay(40);
  tft.drawLine(132,200,194,0,TFT_WHITE);
  delay(40);
  tft.drawLine(134,200,206,0,TFT_WHITE);
  delay(40);
  tft.drawLine(136,200,218,0,TFT_WHITE);
  delay(40);
  tft.drawLine(138,200,230,0,TFT_WHITE);
  digitalWrite(gLED,LOW);
  digitalWrite(bLED,HIGH);
  delay(400);
  digitalWrite(bLED,LOW);
  delay(1000);

}

//specify which button to get the status of
int updateBtn(int btn) {
  switch (btn)
  {
  case 1:
    return (digitalRead(btn1));
  case 2:
    return (digitalRead(btn2));
  case 3:
    return (digitalRead(btn3));
  default:
    return(LOW);
  }
}

void setup() {
  USBDevice.setManufacturerDescriptor("Quantized Trautonium/Arduino Ribbon to MIDI");
  USBDevice.setProductDescriptor("Quantonium/AR2M");  //This is what shows up under your MIDI device selection, if you want to have a bit of fun :)

  Serial1.begin(115200);//might be serial1 or serial I can't remember
  uint16_t calData[5] = {399, 3402, 443, 3438, 4};

  pinMode(btn1,INPUT_PULLDOWN);
  pinMode(btn2,INPUT_PULLDOWN);
  pinMode(btn3,INPUT_PULLDOWN);
  
  pinMode(rLED, OUTPUT);
  pinMode(gLED, OUTPUT);
  pinMode(bLED, OUTPUT);

  // Initialize TFT
  tft.init();
  //tft.setRotation(1);  // Landscape mode
  tft.fillScreen(TFT_BLACK);
  
  // Set up touch calibration
  tft.setTouch(calData);
  
  // Draw title
  bootScreen();
}




void loop() {
  //this if statement will probably become a func and will manage if the screen layout has requested to be changed through the screenID variable
  if (screenID == -1) {
    tft.fillScreen(TFT_BLACK);
    drawScreenButtons(0);
    screenID = 0;
    prevScreenID = 0;
  } else if (screenID != prevScreenID) {
    tft.fillScreen(TFT_BLACK);
    drawScreenButtons(screenID);
    prevScreenID = screenID;
  }

  if (updateBtn(1) == HIGH) {

  }

  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    handleButtonPress(x, y, screenID);
    delay(200);  // Debounce
  }
}
