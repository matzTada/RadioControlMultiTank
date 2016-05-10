/***************************************************
  This is our touchscreen painting example for the Adafruit ILI9341 Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/


#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <SD.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Size of the color selection boxes and the paintbrush size
#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor;

#define SD_CS 4

long changePastMillis = millis();
long changeInterval = 60L * 1000L;
int changeCount = 0;

#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3); //RX, TX

long pastMillis = millis();
long interval = 15L * 1000L;

int color = ILI9341_WHITE;

void setup(void) {
  // while (!Serial);     // used for leonardo debugging

  Serial.begin(9600);
  mySerial.begin(9600);

  Serial.println(F("Touch Paint!"));

  tft.begin();

  mySerial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    mySerial.println("failed!");
  }
  mySerial.println("OK!");

  if (!ts.begin()) {
    tft.println("Couldn't start touchscreen controller");
    while (1);
  }
  tft.println("Touchscreen started");

  tft.fillScreen(ILI9341_BLACK);

  Serial.println("TFT Touch x XBee Wi-Fi");
  mySerial.println("TFT Touch x XBee Wi-Fi");
  Serial.print("GET /eng/index.html HTTP/1.1\nhost:www.sd.keio.ac.jp\n\n");

  tft.setTextWrap(false);

  tft.setRotation(3);
}


void loop()
{
  //  // See if there's any  touch data for us
  if (!ts.bufferEmpty()) {
    //    Serial.println("touched");
    changeCount++;
    if (changeCount > 3) changeCount = 0;
    switch (changeCount) {
      case 0 :
        tft.setRotation(2);
        bmpDraw("purple.bmp", 0, 0);
        break;
      case 1:
        tft.setRotation(3);
        bmpDraw("westlab.bmp", 0, 0);
        break;
      default:
        tft.setRotation(3);
        tft.setCursor(0, 0);
        tft.fillScreen(ILI9341_BLACK);
        break;
    }
  }

  if (millis() - changePastMillis > changeInterval) {
    changePastMillis = millis();
    changeCount++;
    if (changeCount > 3) changeCount = 0;
    switch (changeCount) {
      case 0 :
        tft.setRotation(2);
        bmpDraw("purple.bmp", 0, 0);
        break;
      case 1:
        tft.setRotation(3);
        bmpDraw("westlab.bmp", 0, 0);
        break;
      default:
        tft.setRotation(3);
        tft.setCursor(0, 0);
        tft.fillScreen(ILI9341_BLACK);
        break;
    }
  }

  // Retrieve a point
  TS_Point p = ts.getPoint();

  if (millis() - pastMillis > interval) {
    //    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0,  0);
    pastMillis = millis();
    // put your main code here, to run repeatedly:
    Serial.print("GET /eng/index.html HTTP/1.1\nhost:www.sd.keio.ac.jp\n\n");
  }

  if (Serial.available()) {
    char temp = (char)Serial.read();
    mySerial.print(temp);
    switch (random(0, 8)) {
      case 0: color = ILI9341_BLACK; break;
      case 1: color = ILI9341_BLUE; break;
      case 2: color = ILI9341_RED; break;
      case 3: color = ILI9341_GREEN; break;
      case 4: color = ILI9341_CYAN; break;
      case 5: color = ILI9341_MAGENTA; break;
      case 6: color = ILI9341_YELLOW; break;
      case 7: color = ILI9341_WHITE; break;
    }
    tft.setTextColor(color);
    tft.print(temp);
    //    Serial.print(temp);
  }
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20

void bmpDraw(char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if ((x >= tft.width()) || (y >= tft.height())) return;

  mySerial.println();
  mySerial.print(F("Loading image '"));
  mySerial.print(filename);
  mySerial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    mySerial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    mySerial.print(F("File size: ")); mySerial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    mySerial.print(F("Image Offset: ")); mySerial.println(bmpImageoffset, DEC);
    // Read DIB header
    mySerial.print(F("Header size: ")); mySerial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      mySerial.print(F("Bit Depth: ")); mySerial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        mySerial.print(F("Image size: "));
        mySerial.print(bmpWidth);
        mySerial.print('x');
        mySerial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width()  - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);

        for (row = 0; row < h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col = 0; col < w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r, g, b));
          } // end pixel
        } // end scanline
        mySerial.print(F("Loaded in "));
        mySerial.print(millis() - startTime);
        mySerial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp) mySerial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File & f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File & f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

