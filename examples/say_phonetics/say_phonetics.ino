// Please format this file with clang before check-in to GitHub
/*
  Play spoken words from 2MB QuadSPI memory chip FatFs using DAC audio output

  Date:
            2021-04-08 moved some definitions from global to class member to avoid polluting name space
            2021-03-29 created to announce random grid names

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This sketch speaks grid names, e.g. "CN87" as "Charlie November Eight Seven"
            using the NATO Military Phonetic Alphabet.
            Example data (750 KB) includes Barry's recorded voice sampled at 16 khz mono.
            Sample audio is stored in the 2MB Flash chip and played using DAC0 output pin.
            This library is used in Griduino at https://github.com/barry-ha/Griduino

  Preparing audio files:
            Full details in README at https://github.com/barry-ha/Audio_QSPI
            Prepare your WAV file to 16 kHz mono:
            1. Open Audacity
            2. Open a project, e.g. \Documents\Arduino\libraries\Audio_QSPI\audio\_Phonetic Alphabet.aup3
            3. Select "Project rate" of 16000 Hz
            4. Select an audio fragment, such as spoken word "Charlie"
            5. Menu bar > Effect > Normalize
               a. Remove DC offset
               b. Normalize peaks -1.0 dB
            5. Menu bar > File > Export > Export as WAV
               a. Save as type: WAV (Microsoft)
               b. Encoding: Signed 16-bit PCM
               c. Filename = e.g. "c.wav"
            6. Result WAV file contains 2-byte integer numbers in the range -32767 to +32767

  Transferring audio files:
            Format QSPI file system to CircuitPy format (one time)
            Formatting is only done once; file system is then compatible with CIRCUITPY and FEATHERBOOT frameworks

            To save files from Windows onto the QSPI memory chip on Feather M4 Express,
            1. Temporarily load CircuitPy onto the Feather
            2. Drag-and-drop files from within Windows to Feather
            3. Then load your Arduino sketch again

  Mono Audio:
            The DAC on the SAMD51 is a 12-bit output, from 0 - 3.3v
            The largest 12-bit number is 4,096:
            * Writing 0 will set the DAC to minimum (0.0 v) output
            * Writing 4096 sets the DAC to maximum (3.3 v) output
            This example program has no user inputs
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <DS1804.h>             // DS1804 digital potentiometer library
#include <Audio_QSPI.h>         // Play WAV files from Quad-SPI memory chip

// ------- Identity for splash screen and console --------
#define EXAMPLE_TITLE    "Say Phonetics"
#define EXAMPLE_VERSION  "v1.0.0"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    ""
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- configuration options
const int SHORT_PAUSE   = 300;               // msec between spoken sentences
const int LONG_PAUSE    = SHORT_PAUSE * 8;   // msec between test cases
const int howLongToWait = 8;                 // max number of seconds at startup waiting for Serial port to console

// ---------- Hardware Wiring ----------
// Same as Griduino platform

// here's the audio player being demonstrated
AudioQSPI audio_qspi;

// Griduino hardware has an Adafruit TFT Display (Adafruit part 1743)
#define TFT_BL 4    // TFT backlight
#define TFT_CS 5    // TFT chip select pin
#define TFT_DC 12   // TFT display/command pin
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Adafruit Feather M4 Express pin definitions
#define PIN_VCS  A1   // volume chip select
#define PIN_VINC 6    // volume increment
#define PIN_VUD  A2   // volume up/down

// Griduino hardware has a digital volume control (Maxim DS1804-050+)
DS1804 volume = DS1804(PIN_VCS, PIN_VINC, PIN_VUD, DS1804_TEN);
int gVolume   = 20;   // initial digital potentiometer wiper position, 0..99
                      // note that speaker distortion begins at wiper=40 when powered by USB

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A            // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cSCALECOLOR 0xF844           //
#define cTEXTCOLOR  ILI9341_CYAN     // 0, 255, 255
#define cLABEL      ILI9341_GREEN    //
#define cVALUE      ILI9341_YELLOW   // 255, 255, 0
#define cWARN       0xF844           // brighter than ILI9341_RED but not pink

// ========== splash screen ====================================
const int indent = 8;    // indent labels, slight margin on left edge of screen
const int height = 20;   // text row height

const int yRow1 = 8;                // title
const int yRow2 = yRow1 + height;   // compiled date
const int yRow3 = yRow2 + height;   // volume
const int yLine = yRow3 + height;   // horizontal separator
const int yRow4 = yLine + 6;        // loop count
const int yRow5 = yRow4 + height;   // wave info

void startSplashScreen() {
  // screen rows 1-3 are static text
  tft.setTextSize(2);

  tft.setCursor(indent, yRow1);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(EXAMPLE_TITLE);

  tft.setCursor(indent, yRow2);
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(PROGRAM_COMPILED);

  tft.setCursor(indent, yRow3);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Volume ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(gVolume);

  //               x0,y0           x1,y1        color
  tft.drawLine(indent, yLine, 320 - 8, yLine, cTEXTCOLOR);
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT display
  tft.begin();          // initialize TFT display
  tft.setRotation(1);   // 1=landscape (default is 0=portrait)
  clearScreen();        // note that "begin()" does not clear screen

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // start at full brightness

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor
  Serial.begin(115200);           // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(EXAMPLE_TITLE " " EXAMPLE_VERSION);
  Serial.println("Compiled " PROGRAM_COMPILED);
  Serial.println(__FILE__);

  // ----- initialize audio interface and look for memory card
  audio_qspi.begin();

  // ----- init digital potentiometer
  volume.unlock();      // unlock digipot (in case someone else, like an example pgm, has locked it)
  volume.setToZero();   // set digipot hardware to match its ctor (wiper=0) because the chip cannot be read
                        // and all "setWiper" commands are really incr/decr pulses. This gets it sync.
  volume.setWiperPosition(gVolume);
  Serial.print("Volume wiper = ");
  Serial.println(gVolume);

  // ----- echo our initial settings to the console
  char msg[256];
  byte wiperPos = volume.getWiperPosition();
  snprintf(msg, sizeof(msg), "Initial wiper position(%d)", wiperPos);
  Serial.println(msg);
}

// ===== screen helpers
/*
void showLoopCount(int row, int loopCount) {
  tft.setCursor(indent, row);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Starting playback ");

  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(loopCount);
  tft.print(" ");
}
*/
// display a TEXT-only message
void showMessage(int x, int y, const char *msg) {
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.setCursor(x, y);
  tft.print(msg);

  Serial.print(". ");
  Serial.print(msg);
}
// display a message with a VALUE
void showMessage(int x, int y, const char *msg, int value) {
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.setCursor(x, y);
  tft.print(msg);
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(value);
  tft.print("   ");

  Serial.print(". ");
  Serial.print(msg);
  Serial.println(value);
}
void showWaveInfo(WaveInfo meta, char *filename) {
  int y = yRow4;
  int h = height;

  eraseErrorOutline();
  showMessage(indent, y + h * 1, filename);
  showMessage(indent, y + h * 2, "Number samples ", meta.numSamples);
  showMessage(indent, y + h * 3, "Bytes / sample ", meta.bytesPerSample);
  showMessage(indent, y + h * 4, "Overall file size ", meta.filesize);
  showMessage(indent, y + h * 5, "Samples / sec ", meta.samplesPerSec);
  showMessage(indent, y + h * 6, "Hold time ", meta.holdtime);

  tft.setCursor(indent, y + h * 7);
  float playbackTime = (1.0 / meta.samplesPerSec * meta.numSamples);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Playback time ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(playbackTime, 3);
  tft.print("  ");

  Serial.print(". Playback time ");
  Serial.println(playbackTime, 3);
}

// ----- error message helpers
int isErrorShowing = false;
void drawErrorOutline() {
  isErrorShowing = true;
  tft.fillRect(indent / 2, yRow4, tft.width() - indent, height * 3 + 2, cBACKGROUND);
  tft.drawRect(indent / 2, yRow4, tft.width() - indent, height * 3 + 2, cWARN);
}
void eraseErrorOutline() {
  if (isErrorShowing) {
    tft.fillRect(indent / 2, yRow4, tft.width() - indent, height * 3 + 2, cBACKGROUND);
  }
  isErrorShowing = false;
}
void showError(const char *line1, const char *line2, const char *filename) {
  int x = indent;
  int y = yRow4;

  drawErrorOutline();
  tft.setTextColor(cWARN, cBACKGROUND);
  tft.setCursor(indent, y + 3 + height * 0);
  tft.print(line1);
  tft.setCursor(indent, y + 3 + height * 1);
  tft.print(line2);
  tft.setCursor(indent, y + 3 + height * 2);
  tft.print(filename);
  delay(LONG_PAUSE);
}

// ------ here's the meat of this potato -------
void sayGrid(const char *name) {
  Serial.print("Say ");
  Serial.println(name);

  for (int ii = 0; ii < strlen(name); ii++) {

    // example: choose the filename to play
    char myfile[32];
    char letter = name[ii];
    snprintf(myfile, sizeof(myfile), "/audio/%c.wav", letter);

    // example: read WAV attributes and display it on screen while playing it
    WaveInfo info;
    bool rc = audio_qspi.getInfo(&info, myfile);
    if (rc) {
      showWaveInfo(info, myfile);
    } else {
      showError("Unable to read WAV file", "See console log", myfile);
    }

    // example: play audio through DAC
    rc = audio_qspi.play(myfile);
    if (!rc) {
      Serial.print("sayGrid(");
      Serial.print(letter);
      Serial.println(") failed");
      delay(LONG_PAUSE);
    }
  }
}

//=========== main work loop ===================================
int gLoopCount = 1;

void loop() {

  Serial.print("Loop ");
  Serial.println(gLoopCount);
  showMessage(indent, yRow4, "Starting playback ", gLoopCount);

  // ----- play audio clip at 16 khz/float
  sayGrid("aaaa");
  delay(LONG_PAUSE);   // extra pause between tests

  sayGrid("abcdefghijklmnopqrstuvwxyz0123456789");
  delay(LONG_PAUSE);   // extra pause between tests

  sayGrid("cn87");
  delay(SHORT_PAUSE);
  sayGrid("cn88");
  delay(SHORT_PAUSE);
  sayGrid("dn07");
  delay(SHORT_PAUSE);
  sayGrid("dn08");
  delay(SHORT_PAUSE);
  sayGrid("k7bwh");
  delay(SHORT_PAUSE);

  // debug error message display
  // char myfile[] = "bogus.wav";
  // showError("Unable to read WAV file", "See console log", myfile);

  delay(LONG_PAUSE);   // extra pause between loops
  gLoopCount++;
}