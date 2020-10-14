/*
  File: view.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Abstract base class for all Griduino "view" modules.
            Contains default implementation for common functions
            and for templates to be used in derived classes.
*/
#pragma once

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors

class View {
  public:
    // public member variables go here
    Adafruit_ILI9341* tft;            // an instance of the TFT Display
    const int screenID;               // unique identifier of which screen this is
    int screenRotation;               // 1=landscape, 3=landscape 180-degrees

  public:
    /**
     * Constructor
     */
    //View() { }                      // no default ctor (must be told 'tft')
    View(Adafruit_ILI9341* vtft, int vid) 
      : tft( vtft ), screenID( vid )
    {
    }

    /**
     * Called on every pass through main()
     */
    virtual void updateScreen() {
    }

    /**
     * Called once each time this view becomes active
     */
    virtual void startScreen() {
      // todo - make this pure virtual
    }

    /**
     * Called whenever the touchscreen has an event for this view
     */
    virtual bool onTouch(Point touch) {
      Serial.println("->->-> Touched screen.");
      return false;                     // true=handled, false=controller uses default action
    }

    /**
     * Call to load/save configuration from non-volatile RAM
     */
    virtual void loadConfig() {
      // default: loads nothing
    }
    virtual void saveConfig() {
      // default: saves nothing
    }

  protected:
    void clearScreen(uint16_t color=cBACKGROUND) { // clear the screen
      tft->fillScreen(color);
    }

    void showScreenBorder() {           // optionally outline visible area
      #ifdef SHOW_SCREEN_BORDER
        tft->drawRect(0, 0, gScreenWidth, gScreenHeight, ILI9341_BLUE);  // debug: border around screen
      #endif
    };
    /**
     * Rotate screen right-side-up / upside-down
     * 1=landscape, 3=landscape 180-degrees 
     * This is "protected" to ensure *only* the Settings page will set rotation.
     */
    void setScreenRotation(int rot) {
      Serial.print("Rotating screen to: "); Serial.println(rot);
      this->screenRotation = rot;
      tft->setRotation(rot);             // 0=portrait (default), 1=landscape, 3=180 degrees 
      clearScreen();  // todo - necessary?
      startScreen();  // todo - necessary?
      updateScreen(); // todo - necessary?
    }

};  // end class View

// ----------------------------------------------------------
// In alphabetical order:
//      Grid, Help, Settings2, Settings3, Settings4, Splash, Status, Time, Volume
class ViewGrid : public View {
  public:
    ViewGrid(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

/* moved to "view_help.h"
class ViewHelp : public View {
  public:
    ViewHelp(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};
*/

/* moved to "view_help.h"
class ViewSettings2 : public View {
  public:
    ViewSettings2(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};
*/

class ViewSettings3 : public View {
  public:
    ViewSettings3(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};
/* moved to "view_settings4.h"
class ViewSettings4 : public View {
  public:
    ViewSettings4(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};
*/

class ViewSplash : public View {
  public:
    ViewSplash(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewStatus : public View {
  public:
    ViewStatus(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewTime : public View {
  public:
    ViewTime(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

/* moved to "view_volume.h"
class ViewDate : public View {
  public:
    ViewDate(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};
*/

/* moved to "view_volume.h"
class ViewVolume : public View {
  public:
    ViewVolume(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
    void loadConfig();
    void saveConfig();
};
*/
