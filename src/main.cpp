#include <TFT_eSPI.h>
#include <SPI.h>
#include <MIDI.h>

// Create TFT object
TFT_eSPI tft = TFT_eSPI();

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


struct Button {
  uint16_t x, y, w, h;
  uint16_t bgColor, textColor, borderColor;
  String label;
  bool pressed;
};

/*
struct example  {
  int eg1, eg2;
  String eg3;
};

example testVar1[4] = {
  {0,1,"Hello World"},
  {2,3,"Hello World"},
  {4,5,"Hello World"},
  {6,7,"Hello World"},
};

example testVar2[4] = {
  {10,11,"Worlds spinning"},
  {12,13,"Worlds spinning"},
  {14,15,"Worlds spinning"},
  {16,17,"Worlds spinning"},
};

example *pBtnArray[2] = {testVar1, testVar2};
*/

int buttonArrayLen[]= {8,5}; //this will need to be updated, defines the length of each array

//Probably worth staticing these arrays if possible
// 0
Button infoButtons[8] = {
  {0,0,19,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"Button 1",false},
  {0,80,19,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"Button 2",false},
  {0,160,19,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"Button 3",false},
  {20,200,199,59,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Current Key",false},
  {200,200,39,59,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Current Oct",false},
  {0,240,120,79,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Quick Acc 1",false},
  {119,240,120,79,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Quick Acc 2",false},
  {80,285,80,44,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Menu",false}
};

// 1
Button mainMenuButtons[5] = {
  {0,0,39,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"^",false},
  {0,80,39,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,">",false},
  {0,160,39,79,TFT_WHITE,TFT_BLACK,TFT_TRANSPARENT,"v",false},
  {8,200,100,45,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Exit Menu",false},
  {128,200,100,45,TFT_BLACK,TFT_WHITE,TFT_WHITE,"Return",false}
};


//The saviour of several KB of storage and processing power
Button *pBtnArray[2] = {infoButtons, mainMenuButtons};// this honestly took me several hours to come up with, if this didnt work each layout would have to have its own function wo work out what button was pressed



void drawButton(Button &btn) {
  // Draw border
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, btn.borderColor);
  
  // Draw background
  tft.fillRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, btn.bgColor);

  // Draw text centered
  tft.setTextColor(btn.textColor, btn.bgColor);
  tft.setTextSize(1);
  
  int16_t textX = btn.x + (btn.w - tft.textWidth(btn.label)) / 2;
  int16_t textY = btn.y + (btn.h - 8) / 2;
  
  tft.drawString(btn.label, textX, textY, 4);
}

void drawScreenButtons(int FscreenID) {
  for (int i = 0; i < buttonArrayLen[FscreenID]; i++) {// i < x where x needs to be equal to the number of buttons
    drawButton(pBtnArray[FscreenID][i]);
  }
}

void showMessage(const char* message) {
  // Display message at bottom of screen
  tft.fillRect(0, 200, 320, 40, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(message, 10, 210, 4);
  delay(2000);
  tft.fillRect(0, 200, 320, 40, TFT_BLACK);
}

void drawButtons(int ID) {
  switch (ID)
  {
  case 0:
    for (int i = 0; i < 12; i++) {// i < x where x needs to be equal to the number of buttons within an array
      drawButton(infoButtons[i]);
    break;
    }
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
  tft.drawSmoothArc(42,264,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(0,221,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(42,178,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(0,135,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(42,92,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(0,49,30,30,225,315,TFT_WHITE,TFT_TRANSPARENT);
  tft.drawSmoothArc(42,6,30,30,45,135,TFT_WHITE,TFT_TRANSPARENT);
  tft.fillRoundRect(0,200,50,400,10,TFT_BLACK);
  tft.drawRoundRect(0,200,50,400,10,TFT_WHITE);
  
  // Draw all buttons
 
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  uint16_t calData[5] = {399, 3402, 443, 3438, 4};
  
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

  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    handleButtonPress(x, y, screenID);
    delay(200);  // Debounce
  }
}