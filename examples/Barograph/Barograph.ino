/*
  Barograph -- demonstrate BMP388 barometric sensor

  From:   
  Date:   2019-20-18 created from example by John KM7O
          2020-03-05 replaced physical button design with touchscreen

  Author: Barry Hansen, barry@k7bwh.com, Seattle, WA

  Purpose: 

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)
            Spec: https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341
            Spec: http://adafru.it/1743

         3. Adafruit BMP388 - Precision Barometric Pressure and Altimeter
            Spec: https://www.adafruit.com/product/3966

*/

#include <Wire.h>
#include "SPI.h"                  // Serial Peripheral Interface
#include "Adafruit_GFX.h"         // Core graphics display library
#include "Adafruit_ILI9341.h"     // TFT color display library
#include "TouchScreen.h"          // Touchscreen built in to 3.2" Adafruit TFT display
#include "Adafruit_BMP3XX.h"      // Precision barometric sensor
#include "bitmaps.h"              // our definition of graphics displayed

// ------- Identity for console
#define PROGRAM_TITLE   "Barograph Demo"
#define PROGRAM_VERSION "v1.0"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"

//--------------CONFIG--------------
//float elevCorr = 4241;  // elevation correction in Pa, 
// use difference between altimeter setting and station pressure: https://www.weather.gov/epz/wxcalc_altimetersetting

float elevCorr = 0;
float maxP = 104000;      // in Pa
float minP = 98000;       // in Pa
byte units = 1;           // units on startup: 0=english=inches mercury, 1=metric=millibars
int graphHeight = 135;    // in pixels
int yShift = 9;           // in pixels

// ---------- Hardware Wiring ----------
/*                                Arduino       Adafruit
  ___Label__Description______________Mega_______Feather M4__________Resource____
TFT Power:
   GND  - Ground                  - ground      - J2 Pin 13
   VIN  - VCC                     - 5v          - Pin 10 J5 Vusb
TFT SPI: 
   SCK  - SPI Serial Clock        - Digital 52  - SCK (J2 Pin 6)  - uses hardw SPI
   MISO - SPI Master In Slave Out - Digital 50  - MI  (J2 Pin 4)  - uses hardw SPI
   MOSI - SPI Master Out Slave In - Digital 51  - MO  (J2 Pin 5)  - uses hardw SPI
   CS   - SPI Chip Select         - Digital 10  - D5  (Pin 3 J5)
   D/C  - SPI Data/Command        - Digital  9  - D12 (Pin 8 J5)
TFT Resistive touch:
   X+   - Touch Horizontal axis   - Digital  4  - A3  (Pin 4 J5)
   X-   - Touch Horizontal        - Analog  A3  - A4  (J2 Pin 8)  - uses analog A/D
   Y+   - Touch Vertical axis     - Analog  A2  - A5  (J2 Pin 7)  - uses analog A/D
   Y-   - Touch Vertical          - Digital  5  - D9  (Pin 5 J5)
*/

#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4    // TFT backlight
  #define TFT_CS   5    // TFT select pin
  #define TFT_DC  12    // TFT display/command pin

  #define BMP_CS  13    // BMP388 sensor, chip select

#else
  // todo: Unknown platform
  #warning You need to define pins for your hardware

#endif

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// This sketch has just one touch area that covers the entire screen
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  #define PIN_XP  A3    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A4    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A5    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   9    // Touchscreen Y- can be a digital pin
#else
  // todo: Unknown platform
  #warning You need to define pins for your hardware

#endif
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 295);

// ---------- Barometric Sensor
Adafruit_BMP3XX baro(BMP_CS); // hardware SPI

// ---------- Onboard LED
#define RED_LED 13    // diagnostics RED LED

const int ledPin = 14;     // for blinking

// ------------ typedef's
typedef struct {
  int x;
  int y;
} Point;

// ------------ definitions
#define SEALEVELPRESSURE_HPA (1013.25)
#define MILLIBARS_PER_INCHES_MERCURY (0.02953)
const int howLongToWait = 8;  // max number of seconds at startup waiting for Serial port to console

// ----- color scheme
#define cBACKGROUND   0x00A         // a little darker than ILI9341_NAVY, but not black
#define cSCALECOLOR   0xF844        // color picker: http://www.barth-dev.de/online/rgb565-color-picker/
#define cTEXTCOLOR    ILI9341_CYAN
//#define cLABEL         ILI9341_GREEN
//#define cVALUE         ILI9341_YELLOW
//#define cHIGHLIGHT     ILI9341_WHITE
//#define cBUTTONFILL    ILI9341_NAVY
//#define cBUTTONOUTLINE ILI9341_CYAN
//#define cBUTTONLABEL   ILI9341_YELLOW
#define cWARN         0xF844        // a little brighter than ILI9341_RED

// ------------ global scope
float inchesHg;
float Pa;
float hPa;
float feet;
float tempF;
float tempC;
float altMeters;
float altFeet;

// 144 steps at 20 minute refresh time is a 2880 minute (48 hr) graph with 20 minute resolution.
// with 2px per step, we get 10 minutes/px, and 288px per 48 hrs, leaving some space for graph labels
float pressureStack[144] = {};                  // array to hold pressure data, fill with zeros
const int lastIndex = (sizeof(pressureStack) / sizeof(pressureStack[0])) - 1;  // index to the last element in pressure array

float deltaPressure = 5000;     // 5k is arbitrary, it just needs to be out of range to prevent display on cold start
float deltaPressure3h = 5000;
float pascals;

// button debounce vars
//long lastDebounceTime = 0;
//long debounceDelay = 300;

// ============== touchscreen helpers ==========================
bool gTouching = false;             // keep track of previous state
bool newScreenTap(Point* pPoint) {
  // find leading edge of a screen touch
  // returns TRUE only once on initial screen press
  // if true, also return screen coordinates of the touch

  bool result = false;        // assume no touch
  if (gTouching) {
    // the touch was previously processed, so ignore continued pressure until they let go
    if (!ts.isTouching()) {
      // Touching ==> Not Touching transition
      gTouching = false;
    }
  } else {
    // here, we know the screen was not being touched in the last pass,
    // so look for a new touch on this pass
    // The built-in "isTouching" function does most of the debounce and threshhold detection needed
    if (ts.isTouching()) {
      gTouching = true;
      result = true;

      // touchscreen point object has (x,y,z) coordinates, where z = pressure
      TSPoint touch = ts.getPoint();

      // convert resistance measurements into screen pixel coords
      mapTouchToScreen(touch, pPoint);
      Serial.print("Screen touch detected ("); Serial.print(pPoint->x);
      Serial.print(","); Serial.print(pPoint->y); Serial.println(")");
    }
  }
  //delay(100);   // no delay: code above completely handles debouncing without blocking the loop
  return result;
}

void mapTouchToScreen(TSPoint touch, Point* screen) {
  // convert from X+,Y+ resistance measurements to screen coordinates
  // param touch = resistance readings from touchscreen
  // param screen = result of converting touch into screen coordinates
  //
  // Measured readings in Barry's landscape orientation were:
  //   +---------------------+ X=876
  //   |                     |
  //   |                     |
  //   |                     |
  //   +---------------------+ X=160
  //  Y=110                Y=892
  //
  // Typical measured pressures=200..549

  screen->x = 0;
  screen->y = 0;

  // setRotation(1) = landscape orientation = x-,y-axis exchanged
  screen->x = map(touch.y, 100, 900,  0, tft.width());
  screen->y = map(touch.x, 900, 100,  0, tft.height());
  return;
}

// ============== barometer helpers ============================
void getData() {
  if (!baro.performReading()) {
    Serial.println("Error, failed to read barometer");
  }
  // continue anyway, for demo
  tempC = baro.temperature;
  tempF= tempC * 9 / 5 + 32;
  Pa = baro.pressure + elevCorr;
  hPa = Pa / 100;
  inchesHg = 0.0002953 * Pa;
  altMeters = baro.readAltitude(SEALEVELPRESSURE_HPA);
  altFeet = 3.28084 * altMeters;
}

// ============== Screen Helpers ===============================
void blinky(int qty, int waitTime) {
  for (int i = 0; i <= qty; i++) {
    digitalWrite(ledPin, HIGH);
    delay(waitTime);
    digitalWrite(ledPin, LOW);
    delay(waitTime);
  }
}
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}
void printReadings(int units, float pascals) {
  clearScreen();
  tft.setCursor(0, 0);
  tft.setTextColor(cTEXTCOLOR);
  tft.setTextSize(2);

  switch (units) {
    case 0:
      tft.print(Pa / 3386.4, 2); tft.println(F(" inHg"));
      Serial.print(Pa / 3386.4, 2); Serial.println(F(" inHg"));
      break;
    case 1:
      tft.print(Pa / 100, 2); tft.println(F(" hPa"));
      Serial.print(Pa / 100, 2); Serial.println(F(" hPa"));
      break;
  }
  
  printDeltaP();
  printDeltaP3h();
  drawScale();
  tickMarks(3, 5);      //args: (hours, height)
  tickMarks(6, 8);
  scaleMarks(500, 5);   //args: (Pa, length)
  scaleMarks(1000, 8);
  drawGraph();
  drawIcon();
}

// blob this into printReadings()?
void printDeltaP() {
  tft.setTextColor(cTEXTCOLOR);
  tft.setTextSize(2);
  tft.setCursor(0, 26);
  if (deltaPressure < -116) {
    tft.setTextColor(0xF882);
  }
  if (units == 0) {
    if (abs(deltaPressure) > 2000) {
      tft.println(F("--- inHg/h"));
      return;
    }
    tft.print(deltaPressure / 3386.4, 2); tft.println(F(" inHg/h"));
  }
  else {
    if (abs(deltaPressure) > 400) {
      tft.println(F("--- Pa/h"));
      return;
    }
    tft.print(deltaPressure, 0); tft.println(F(" Pa/h"));
  }
}

void printDeltaP3h() {
  tft.setTextColor(cTEXTCOLOR);  tft.setTextSize(2); tft.setCursor(0, 52);
  if (deltaPressure3h < -348) {
    tft.setTextColor(0xF882);
  }
  if (units == 0) {
    if (abs(deltaPressure3h) > 2000) {
      tft.println(F("--- inHg/3h"));
      return;
    }
    tft.print(deltaPressure3h / 3386.4, 2); tft.println(F(" inHg/3h"));
  }
  else {
    if (abs(deltaPressure3h) > 400) {
      tft.println(F("--- Pa/3h"));
      return;
    }
    tft.print(deltaPressure3h, 0); tft.println(F(" Pa/3h"));
  }
}

void drawIcon() {
  if ( -116 < deltaPressure && deltaPressure <= -50) {
    tft.drawBitmap(230, 5, falling, 80, 80, cTEXTCOLOR);
    return;
  }
  else if ( (-200) < deltaPressure && deltaPressure <= (-116)) {
    tft.drawBitmap(230, 5, rapidlyFalling, 80, 80, 0xF882);
    return;
  }
  else if ( (-2000) < deltaPressure && deltaPressure <= (-200)) {
    tft.drawBitmap(230, 5, vRapidlyFalling, 80, 80, 0xF882);
    return;
  }
  else if ( 50 <= deltaPressure && deltaPressure < 116) {
    tft.drawBitmap(230, 5, rising, 80, 80, cTEXTCOLOR);
    return;
  }
  else if ( 116 <= deltaPressure && deltaPressure < 200) {
    tft.drawBitmap(230, 5, rapidlyRising, 80, 80, cTEXTCOLOR);
    return;
  }
  else if ( 200 <= deltaPressure && deltaPressure < 2000) {
    tft.drawBitmap(230, 5, vRapidlyRising, 80, 80, cTEXTCOLOR);
    return;
  }
  tft.drawBitmap(230, 5, steady, 80, 80, cTEXTCOLOR);
}

void tickMarks(int t, int h) { //t in hours
  int x = 319;
  int deltax = t * 6;
  for (x = 319; x > 31; x = x - deltax) {
    tft.drawLine(x, 239 - yShift, x, 239 - yShift - h, cSCALECOLOR);
  }
}

void scaleMarks(int p, int len) { //t in hours
  int y = 239 - yShift;
  int deltay = map(p, 0, maxP - minP, 0, graphHeight);
  for (y = 239 - yShift; y > 239 - graphHeight - yShift + 5; y = y - deltay) {
    tft.drawLine(31, y, 31 + len, y, cSCALECOLOR);
    tft.drawLine(319, y, 319 - len, y, cSCALECOLOR);
  }
}

void drawScale() {
  //write limits of pressure scale in consistent units
  tft.setTextColor(cSCALECOLOR);  tft.setTextSize(1);
  if (units == 0) {
    tft.setCursor(1, 233 - yShift);
    tft.print(minP / 3386.4, 2);
    tft.setCursor(1, 239 - graphHeight - yShift);
    tft.print(maxP / 3386.4, 2);
    tft.setCursor(1, 233 - (graphHeight / 2) - yShift);
    tft.print((minP + (maxP - minP) / 2) / 3386.4, 2);
  }
  else {
    tft.setCursor(10, 233 - yShift);
    tft.print(minP / 100, 0);
    tft.setCursor(4, 239 - graphHeight - yShift);
    tft.print(maxP / 100, 0);
    tft.setCursor(4, 236 - (graphHeight / 2) - yShift);
    tft.print((minP + (maxP - minP) / 2) / 100, 0);
  }
  //draw horizontal lines
  tft.drawLine(31, 239 - graphHeight - yShift, 319, 239 - graphHeight - yShift, cSCALECOLOR);
  tft.drawLine(31, 239 - yShift, 319, 239 - yShift, cSCALECOLOR);
  //draw vertical lines, and label 24, 48 hrs
  tft.drawLine(319, 239 - graphHeight - yShift, 319, 239 - yShift, cSCALECOLOR);
  tft.drawLine(175, 239 - graphHeight - yShift, 175, 239 - yShift, cSCALECOLOR);
  tft.drawLine(31, 239 - graphHeight - yShift, 31, 239 - yShift, cSCALECOLOR);
  tft.setCursor(175, 232);
  tft.print(24);
  tft.setCursor(31, 232);;
  tft.print(48);
}

void drawGraph() {
  int i = lastIndex;
  int deltax = 2;
  int x = 319;
  for (i = lastIndex; i > 0 ; i--) {
    int p1 =  map(pressureStack[i], minP, maxP, 0, graphHeight);
    int p2 =  map(pressureStack[i - 1], minP, maxP, 0, graphHeight);
    if (pressureStack[i - 1] != 0) {
      tft.drawLine(x, 239 - p1 - yShift, x - deltax, 239 - p2 - yShift, 0x07E0);
    }
    x = x - deltax;
  }

}
void adjustUnits() {
  units++;
  if (units > 1) {
    units = 0;
  }
  Serial.print("Units changed to: "); Serial.println(units);
  getData();
  printReadings(units, Pa);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  while (millis() < targetTime) {
    if (Serial) break;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " __DATE__ " " __TIME__);  // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  Serial.print("Last element index = "); Serial.println(lastIndex);

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(1);                 // landscape (default is portrait)
  clearScreen();

  // ----- announce ourselves
  tft.setCursor(0, 0);
  tft.setTextColor(cTEXTCOLOR);
  tft.setTextSize(3);
  tft.println(PROGRAM_TITLE);
  tft.println(PROGRAM_VERSION);
  tft.println(PROGRAM_LINE1);
  tft.println(PROGRAM_LINE2);

  delay(2000);
  clearScreen();
/*
// --> bmp3_defs.h

// API success codes
#define BMP3_OK        INT8_C(0)

// API error codes
#define BMP3_E_NULL_PTR                   INT8_C(-1)
#define BMP3_E_DEV_NOT_FOUND              INT8_C(-2)
#define BMP3_E_INVALID_ODR_OSR_SETTINGS   INT8_C(-3)
#define BMP3_E_CMD_EXEC_FAILED            INT8_C(-4)
#define BMP3_E_CONFIGURATION_ERR          INT8_C(-5)
#define BMP3_E_INVALID_LEN                INT8_C(-6)
#define BMP3_E_COMM_FAIL                  INT8_C(-7)
#define BMP3_E_FIFO_WATERMARK_NOT_REACHED INT8_C(-8)

// API warning codes
#define BMP3_W_SENSOR_NOT_ENABLED         UINT8_C(1)
#define BMP3_W_INVALID_FIFO_REQ_FRAME_CNT UINT8_C(2)
*/

  // ----- init barometer
  if (!baro.begin()) {
    Serial.println("Error, unable to initialize BMP388, check your wiring");
    tft.setCursor(0, 80);
    tft.setTextColor(cWARN);
    tft.setTextSize(3);
    tft.println("Error!\n Unable to init\n  BMP388 sensor\n   check wiring");
    delay(4000);
  }

  // Set up BMP388 oversampling and filter initialization
  baro.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
  baro.setPressureOversampling(BMP3_OVERSAMPLING_32X);
  baro.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_127);
  // baro.setOutputDataRate(BMP3_ODR_50_HZ);

  pinMode(ledPin, OUTPUT);    // for blinking

  // Get first data point (done twice because first reading is always bad)
  getData();
  pressureStack[lastIndex] = Pa + elevCorr;
  printReadings(units, pressureStack[lastIndex]);

  getData();
  pressureStack[lastIndex] = Pa + elevCorr;
  printReadings(units, pressureStack[lastIndex]);
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimer1 = millis();    // timer for value update (5 min)
uint32_t prevTimer2 = millis();    // timer for graph/array update (20 min)
uint32_t prevTimer3 = millis();    // timer for emergency alert (10 sec)

const int READ_BAROMETER_INTERVAL = 5*60*1000;  // Timer 1
const int LOG_PRESSURE_INTERVAL = 20*60*1000;   // Timer 2
const int CHECK_CRASH_INTERVAL = 10*1000;       // Timer 3

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTimer1 > millis()) { prevTimer1 = millis(); }
  if (prevTimer2 > millis()) { prevTimer2 = millis(); }
  if (prevTimer3 > millis()) { prevTimer3 = millis(); }

  // check for crashing pressure and blink a LED if it's happening
  if (millis() - prevTimer3 > CHECK_CRASH_INTERVAL) {
    if ( (-200) < deltaPressure && deltaPressure <= (-116)) {
      blinky(2, 500);
    }
    else if ( (-2000) < deltaPressure && deltaPressure <= (-200)) {
      blinky(5, 200);
    }
    prevTimer3 = millis();
    return;
  }

  // every 5 minutes acquire/print temp and pressure
  if (millis() - prevTimer1 > READ_BAROMETER_INTERVAL) {
    getData();
    printReadings(units, Pa);

    // every 20 minutes log, display, and graph pressure/delta pressure
    if (millis() - prevTimer2 > LOG_PRESSURE_INTERVAL) {  // 1200000 for 20 minutes
      //shift array left
      int i = 0;
      for (i = 0; i < lastIndex; i++) {
        pressureStack[i] = pressureStack[i + 1];
      }
      //and tack on the latest pressure
      pressureStack[lastIndex] = Pa;
      
      //calculate pressure change and reprint all to screen
      deltaPressure = pressureStack[lastIndex] - pressureStack[140];
      deltaPressure3h = pressureStack[lastIndex] - pressureStack[134];
      printReadings(units, Pa);
      prevTimer2 = millis();
    }
    prevTimer1 = millis();
  }

  // check if user wants to change the display units
  //if ( (millis() - lastDebounceTime) > debounceDelay) {
  //  if (digitalRead(unitPin) == LOW) {
  //    units++;
  //    if (units > 1) {
  //      units = 0;
  //    }
  //    //Serial.println(units);
  //    getData();
  //    printReadings(units, Pa);
  //    lastDebounceTime = millis();
  //  }
  //}

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {
    adjustUnits();             // change between "inches mercury" and "millibars" units
  }
}