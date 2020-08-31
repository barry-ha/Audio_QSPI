/*
  File: view_settings3.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for one-time Griduino setup.
            Since it's NOT intended for a driver in motion, we can use
            a smaller font and put a little more stuff onto the screen.

            +-----------------------------------+
            |              Settings 3           |
            |                                   |
            | Distance      (o)[ Miles        ] |
            |               ( )[ Kilometers   ] |
            |                                   |
            |                                   |
            |                                   |
            |                                   |
            | Version 0.22                      |
            |     Compiled Aug 21 2020  08:58   |
            +-----------------------------------+
*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "model.cpp"                // "Model" portion of model-view-controller
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;        // Griduino.ino
extern Model* model;                // "model" portion of model-view-controller

void setFontSize(int font);         // Griduino.ino
int getOffsetToCenterTextOnButton(String text, int leftEdge, int width ); // Griduino.ino
void drawAllIcons();                // draw gear (settings) and arrow (next screen) // Griduino.ino
void showScreenBorder();            // optionally outline visible area

// ========== forward reference ================================

// ============== constants ====================================
// vertical placement of text rows   ---label---         ---button---
const int yRow1 = 70;             // "Distance",         "Miles"
const int yRow2 = yRow1 + 50;     //                     "Kilometers"
const int yRow9 = 234;            // "v0.22, Aug 21 2020 45:67:89"

// color scheme: see constants.h
#define col1 10                   // left-adjusted column of text
#define xButton 160               // indented column of buttons

// ========== globals ==========================================
enum txtSettings3 {
  SETTINGS=0, 
  COMPILED,
};
TextField txtSettings3[] = {
  //        text                  x, y     color
  TextField("Settings 3",      col1, 20,   cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
  TextField("Distance",        col1,yRow1, cVALUE),                 // 
  TextField(PROGRAM_VERSION ", " PROGRAM_COMPILED, 
                               col1,yRow9, cLABEL, ALIGNCENTER),    // [COMPILED]
};
const int numSettingsFields = sizeof(txtSettings3)/sizeof(txtSettings3[0]);

// ========== settings helpers =================================
void fMiles() {
  Serial.println("->->-> Clicked DISTANCE MILES button.");
  model->setMiles();
}
void fKilometers() {
  Serial.println("->->-> Clicked DISTANCE KILOMETERS button.");
  model->setKilometers();
}

enum buttonID {
  eMILES = 0,
  eKILOMETERS,
};
TimeButton settings3Buttons[] = {
  // label             origin         size      touch-target     
  // text                x,y           w,h      x,y      w,h  radius  color   function
  {"Miles",        xButton,yRow1-26, 130,40, {130, 30, 180,60},  4,  cVALUE,  fMiles      },   // [eMILES] set units English
  {"Kilometers",   xButton,yRow2-26, 130,40, {130, 90, 180,60},  4,  cVALUE,  fKilometers },   // [eKILOMETERS] set units Metric
};
const int nSettingsButtons = sizeof(settings3Buttons)/sizeof(settings3Buttons[0]);

// ========== settings screen view =============================
void updateSettings3Screen() {
  // called on every pass through main()

  // ----- show selected radio buttons by filling in the circle
  for (int ii=eMILES; ii<=eKILOMETERS; ii++) {
    TimeButton item = settings3Buttons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);
    int buttonFillColor = cBACKGROUND;

    if (ii==eMILES && !model->gMetric) {
      buttonFillColor = cLABEL;
    } 
    if (ii==eKILOMETERS && model->gMetric) {
      buttonFillColor = cLABEL;
    }
    tft.fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }

}
void startSettings3Screen() {
  // called once each time this view becomes active
  tft.fillScreen(cBACKGROUND);      // clear screen
  txtSettings3[0].setBackground(cBACKGROUND);                  // set background for all TextFields in this view
  TextField::setTextDirty( txtSettings3, numSettingsFields );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  drawAllIcons();                   // draw gear (settings) and arrow (next screen)
  showScreenBorder();               // optionally outline visible area

  // ----- draw text fields
  for (int ii=0; ii<numSettingsFields; ii++) {
      txtSettings3[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<nSettingsButtons; ii++) {
    TimeButton item = settings3Buttons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);
    tft.setCursor(xx, item.y+item.h/2+5);
    tft.setTextColor(item.color);
    tft.print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft.drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,  // debug: draw outline around hit target
                 item.hitTarget.size.x, item.hitTarget.size.y, 
                 cWARN);
    #endif
  }

  // ----- show outlines of radio buttons
  for (int ii=eMILES; ii<=eKILOMETERS; ii++) {
    TimeButton item = settings3Buttons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);

    // outline the radio button
    // the active button will be indicated in updateSettings3Screen()
    tft.drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  updateSettings3Screen();             // fill in values immediately, don't wait for the main loop to eventually get around to it

  // debug: show centerline on display
  //                        x1,y1            x2,y2            color
  //tft.drawLine(tft.width()/2,0, tft.width()/2,tft.height(), cWARN); // debug
}
bool onTouchSettings3(Point touch) {
  Serial.println("->->-> Touched settings screen.");
  bool handled = false;             // assume a touch target was not hit
  for (int ii=0; ii<nSettingsButtons; ii++) {
    TimeButton item = settings3Buttons[ii];
    if (touch.x >= item.x && touch.x <= item.x+item.w
     && touch.y >= item.y && touch.y <= item.y+item.h) {
        handled = true;             // hit!
        item.function();            // do the thing
        updateSettings3Screen();    // update UI immediately, don't wait for laggy mainline loop
        model->save();              // after UI is refreshed, run this slow "save to nvr" operation

        #ifdef SHOW_TOUCH_TARGETS
          const int radius = 3;     // debug: show where touched
          tft.fillCircle(touch.x, touch.y, radius, cWARN);  // debug - show dot
        #endif
     }
  }
  return handled;                   // true=handled, false=controller uses default action
}