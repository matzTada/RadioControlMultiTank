/***************************************************
based on "touchpaint example" of the Adafruit ILI9341 Shield
  ----> http://www.adafruit.com/products/1651

Tada Matz Westlab
with "MultiAgentBulldozerController"

 ****************************************************/

#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
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
#define BOXSIZE 60
#define PENRADIUS 3
int oldcolor, currentcolor;

//controller settings
int selectedDevice = 0; // 0 for all, 1 for R1, 2 for R2, 3 for R3
#define MYBOXSIZE 80

//communication includes and definitions
#include <XBee.h>
#include <SoftwareSerial.h>
union fourbyte {
  uint32_t dword;
  uint16_t word[2];
  uint8_t byte[4];
};
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse(); // create reusable response objects for responses we expect to handle
SoftwareSerial mySerial(2, 3);
unsigned long commPastMillis = millis();

void sendCommandViaXBee(int device, String command) {
  //string to uint8
  uint8_t sendPayload[command.length()];
  for (int i = 0; i < command.length(); i++) {
    sendPayload[i] = command.charAt(i);
  }

  if (device == 0) {
    XBeeAddress64 addr64 = XBeeAddress64(0x00000000, 0x0000FFFF);
    ZBTxRequest zbTx = ZBTxRequest(addr64, sendPayload, sizeof(sendPayload));
    xbee.send(zbTx);
  }
  else if (device == 1) {
    XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x40B59A78);
    ZBTxRequest zbTx = ZBTxRequest(addr64, sendPayload, sizeof(sendPayload));
    xbee.send(zbTx);
  }
  else if (device == 2) {
    XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x40B59A5E);
    ZBTxRequest zbTx = ZBTxRequest(addr64, sendPayload, sizeof(sendPayload));
    xbee.send(zbTx);
  }
  else if (device == 3) {
    XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x40F93D96);
    ZBTxRequest zbTx = ZBTxRequest(addr64, sendPayload, sizeof(sendPayload));
    xbee.send(zbTx);
  }
}

void setup(void) {
  // while (!Serial);     // used for leonardo debugging

  Serial.begin(9600);
  Serial.println(F("Touch Paint!"));

  tft.begin();

  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  Serial.println("Touchscreen started");

  tft.fillScreen(ILI9341_BLACK);

  // make the color selection boxes
  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("AL");
  tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
  tft.setCursor(BOXSIZE, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("#1");
  tft.fillRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
  tft.setCursor(BOXSIZE * 2, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("#2");
  tft.fillRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
  tft.setCursor(BOXSIZE * 3, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("#3");

  // select the current color 'red'
  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
  currentcolor = ILI9341_RED;

  //draw command boxes
  tft.fillRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, currentcolor);
  tft.drawRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
  tft.setCursor(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("<-");
  tft.fillRect(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, currentcolor);
  tft.drawRect(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
  tft.setCursor(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("^^");
  tft.fillRect(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, currentcolor);
  tft.drawRect(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
  tft.setCursor(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("->");

  tft.fillRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, currentcolor);
  tft.drawRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
  tft.setCursor(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 1);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("<~");
  tft.fillRect(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, currentcolor);
  tft.drawRect(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
  tft.setCursor(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 1);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("__");
  tft.fillRect(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, currentcolor);
  tft.drawRect(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
  tft.setCursor(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 1);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("~>");

  tft.fillRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 2, MYBOXSIZE * 3, MYBOXSIZE, currentcolor);
  tft.drawRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 2, MYBOXSIZE * 3, MYBOXSIZE, ILI9341_WHITE);
  tft.setCursor(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 2);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
  tft.println("STOP");

  selectedDevice = 0;

  //communication settings
  mySerial.begin(9600);
  xbee.begin(mySerial);
}


void loop()
{
  // See if there's any  touch data for us
  if (ts.bufferEmpty()) {
    return;
  }
  /*
  // You can also wait for a touch
  if (! ts.touched()) {
    return;
  }
  */

  // Retrieve a point
  TS_Point p = ts.getPoint();

  /*
   Serial.print("X = "); Serial.print(p.x);
   Serial.print("\tY = "); Serial.print(p.y);
   Serial.print("\tPressure = "); Serial.println(p.z);
  */

  // Scale from ~0->4000 to tft.width using the calibration #'s
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  Serial.print("("); Serial.print(p.x);
  Serial.print(", "); Serial.print(p.y);
  Serial.println(")");

  if (p.y < BOXSIZE) {
    oldcolor = currentcolor;

    if (p.x < BOXSIZE) {
      currentcolor = ILI9341_RED;
      selectedDevice = 0;
      tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    } else if (p.x < BOXSIZE * 2) {
      currentcolor = ILI9341_YELLOW;
      selectedDevice = 1;
      tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    } else if (p.x < BOXSIZE * 3) {
      currentcolor = ILI9341_GREEN;
      selectedDevice = 2;
      tft.drawRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    } else if (p.x < BOXSIZE * 4) {
      currentcolor = ILI9341_CYAN;
      selectedDevice = 3;
      tft.drawRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
    }

    if (oldcolor != currentcolor) {
      if (oldcolor == ILI9341_RED) {
        tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
        tft.setCursor(0, 0);
        tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
        tft.println("AL");
      }
      if (oldcolor == ILI9341_YELLOW) {
        tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
        tft.setCursor(BOXSIZE, 0);
        tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
        tft.println("#1");
      }
      if (oldcolor == ILI9341_GREEN) {
        tft.fillRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
        tft.setCursor(BOXSIZE * 2, 0);
        tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
        tft.println("#2");
      }
      if (oldcolor == ILI9341_CYAN) {
        tft.fillRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
        tft.setCursor(BOXSIZE * 3, 0);
        tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
        tft.println("#3");
      }
    }

    //draw command boxes
    tft.fillRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, currentcolor);
    tft.drawRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
    tft.setCursor(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 0);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
    tft.println("<-");
    tft.fillRect(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, currentcolor);
    tft.drawRect(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
    tft.setCursor(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 0);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
    tft.println("^^");
    tft.fillRect(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, currentcolor);
    tft.drawRect(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 0, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
    tft.setCursor(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 0);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
    tft.println("->");

    tft.fillRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, currentcolor);
    tft.drawRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
    tft.setCursor(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 1);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
    tft.println("<~");
    tft.fillRect(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, currentcolor);
    tft.drawRect(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
    tft.setCursor(MYBOXSIZE * 1, BOXSIZE + MYBOXSIZE * 1);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
    tft.println("__");
    tft.fillRect(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, currentcolor);
    tft.drawRect(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 1, MYBOXSIZE, MYBOXSIZE, ILI9341_WHITE);
    tft.setCursor(MYBOXSIZE * 2, BOXSIZE + MYBOXSIZE * 1);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
    tft.println("~>");

    tft.fillRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 2, MYBOXSIZE * 3, MYBOXSIZE, currentcolor);
    tft.drawRect(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 2, MYBOXSIZE * 3, MYBOXSIZE, ILI9341_WHITE);
    tft.setCursor(MYBOXSIZE * 0, BOXSIZE + MYBOXSIZE * 2);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(4);
    tft.println("STOP");

  }

  if (((p.y - PENRADIUS) > BOXSIZE) && ((p.y + PENRADIUS) < tft.height())) {
    tft.fillCircle(p.x, p.y, PENRADIUS, currentcolor);
  }

  //judge touch command
  if (p.y > BOXSIZE + MYBOXSIZE * 0 && p.y < BOXSIZE + MYBOXSIZE * 1) {
    if (p.x < MYBOXSIZE) {
      if (millis() - commPastMillis > 500) {
        commPastMillis = millis();
        Serial.println("send: l");
        sendCommandViaXBee(selectedDevice, "l");
      }
    } else if (p.x < MYBOXSIZE * 2) {
      if (millis() - commPastMillis > 500) {
        commPastMillis = millis();
        Serial.println("send: s");
        sendCommandViaXBee(selectedDevice, "s");
      }
    } else if (p.x < MYBOXSIZE * 3) {
      if (millis() - commPastMillis > 500) {
        commPastMillis = millis();
        Serial.println("send: r");
        sendCommandViaXBee(selectedDevice, "r");
      }
    }
  }
  if (p.y > BOXSIZE + MYBOXSIZE * 1 && p.y < BOXSIZE + MYBOXSIZE * 2) {
    if (p.x < MYBOXSIZE) {
      if (millis() - commPastMillis > 500) {
        commPastMillis = millis();
        Serial.println("send: L");
        sendCommandViaXBee(selectedDevice, "L");
      }
    } else if (p.x < MYBOXSIZE * 2) {
      if (millis() - commPastMillis > 500) {
        commPastMillis = millis();
        Serial.println("send: b");
        sendCommandViaXBee(selectedDevice, "b");
      }
    } else if (p.x < MYBOXSIZE * 3) {
      if (millis() - commPastMillis > 500) {
        commPastMillis = millis();
        Serial.println("send: R");
        sendCommandViaXBee(selectedDevice, "R");
      }
    }
  }
  if (p.y > BOXSIZE + MYBOXSIZE * 2 && p.y < BOXSIZE + MYBOXSIZE * 3) {
    if (millis() - commPastMillis > 500) {
      commPastMillis = millis();
      Serial.println("send: t");
      sendCommandViaXBee(selectedDevice, "t");
    }
  }

}
