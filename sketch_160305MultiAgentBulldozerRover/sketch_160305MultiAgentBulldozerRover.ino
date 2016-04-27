/**
2016/3/5
Multi Agent-Like System with XBee and TAMIYA Bulldozer Kit

Catch Packet
Drive Motors with Motor Driver
Hop Packet
*/

#include <XBee.h>
#include <SoftwareSerial.h>

#define MOT_1_H 4
#define MOT_1_L 5
#define MOT_1_PWM 6
#define MOT_2_H 7
#define MOT_2_L 8
#define MOT_2_PWM 9

union fourbyte {
  uint32_t dword;
  uint16_t word[2];
  uint8_t byte[4];
};

XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse(); // create reusable response objects for responses we expect to handle

SoftwareSerial mySerial(2, 3);

void motorControl(int number, int H, int L, int pwm) {
  if (number == 1) {
    digitalWrite(MOT_1_H, H);
    digitalWrite(MOT_1_L, L);
    analogWrite(MOT_1_PWM, pwm);
  }
  else if (number == 2) {
    digitalWrite(MOT_2_H, H);
    digitalWrite(MOT_2_L, L);
    analogWrite(MOT_2_PWM, pwm);
  }
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  xbee.begin(mySerial);

  Serial.println(F("Multi Agent Bulldozer Rover"));

  //motor setup
  pinMode(MOT_1_H, OUTPUT);
  pinMode(MOT_1_L, OUTPUT);
  pinMode(MOT_1_PWM, OUTPUT);
  pinMode(MOT_2_H, OUTPUT);
  pinMode(MOT_2_L, OUTPUT);
  pinMode(MOT_2_PWM, OUTPUT);
}

// continuously reads packets, looking for ZB Receive or Modem Status
void loop() {
  //  Serial.println("--- loop start --- ");

  //xbee receiving
  xbee.readPacket();

  if (xbee.getResponse().isAvailable()) {
    // got something

    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      // got a zb rx packet
      Serial.print("got zb rx packet >>> ");

      // now fill our zb rx class
      xbee.getResponse().getZBRxResponse(rx);

      //remote source address
      Serial.print(rx.getRemoteAddress64().getMsb(), HEX);//data type of return value is uint32_t
      Serial.print(",");
      Serial.print(rx.getRemoteAddress64().getLsb(), HEX);//data type of return value is uint32_t
      Serial.print(",");

      //data(payload) length
      Serial.print(rx.getDataLength());
      Serial.print(",");
      //      Serial.println("");

      //data(payload)
      uint8_t receivePayload [rx.getDataLength()];
      for (int i = 0; i < rx.getDataLength(); i++) {
        receivePayload[i] = rx.getData(i);
      }
      for (int i = 0; i < rx.getDataLength(); i++) {
        Serial.print((char)receivePayload[i]);
      }
      Serial.print(",");

      uint8_t payloadType = receivePayload[0];

      //judge whether the packet is hopping packet or not
      if (payloadType == 't') { //stop
        Serial.print(F("===STOP==="));
        motorControl(1, LOW, LOW, 0);
        motorControl(2, LOW, LOW, 0);
      }
      else if (payloadType == 's') {
        Serial.print(F("===STRAIGHT==="));
        motorControl(1, LOW, HIGH, 127);
        motorControl(2, LOW, HIGH, 127);
      }
      else if (payloadType == 'r') {
        Serial.print(F("===RIGHT ROTATE==="));
        motorControl(1, HIGH, LOW, 200);
        motorControl(2, LOW, HIGH, 200);
      }
      else if (payloadType == 'l') {
        Serial.print(F("===LEFT ROTATE==="));
        motorControl(1, LOW, HIGH, 200);
        motorControl(2, HIGH, LOW, 200);
      }
      else if (payloadType == 'b') {
        Serial.print(F("===BACK==="));
        motorControl(1, HIGH, LOW, 127);
        motorControl(2, HIGH, LOW, 127);
      }
      else if (payloadType == 'R') {
        Serial.print(F("===RIGHT ROTATE==="));
        motorControl(1, LOW, HIGH, 127);
        motorControl(2, LOW, HIGH, 200);
      }
      else if (payloadType == 'L') {
        Serial.print(F("===LEFT ROTATE==="));
        motorControl(1, LOW, HIGH, 200);
        motorControl(2, LOW ,HIGH, 127);
      }
      else if (payloadType == 0xF0) { //hopping packet!!
        uint8_t remainHop = receivePayload[1];
        Serial.print(" remainHop: ");
        Serial.print(remainHop);
        if (0 <= remainHop && remainHop <= 48) { //no more hop needed
          Serial.print(" >>> no more hop needed");
        }
        else { //packet should be hopped 1 or more times
          Serial.print(" >>> hop packet !!!");

          uint8_t allHop = receivePayload[2];
          union fourbyte sendPacketLSB;

          //calc next destination address by receivePayload
          sendPacketLSB.byte[0] = receivePayload[6 + (allHop - remainHop) * 4];
          sendPacketLSB.byte[1] = receivePayload[5 + (allHop - remainHop) * 4];
          sendPacketLSB.byte[2] = receivePayload[4 + (allHop - remainHop) * 4];
          sendPacketLSB.byte[3] = receivePayload[3 + (allHop - remainHop) * 4];

          Serial.print(sendPacketLSB.dword, HEX);
          //make send payload
          receivePayload[1] = remainHop - 1; //decrease counter
          Serial.print(" [1]:");
          Serial.print((int)receivePayload[1]);
          Serial.print(" ");

          XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, sendPacketLSB.dword);
          ZBTxRequest zbTx = ZBTxRequest(addr64, receivePayload, sizeof(receivePayload));
          xbee.send(zbTx);
        }
      }
      else { //the packet is not hopping packet
        Serial.print(" >>> got normal packet");
      }
    }

    Serial.println("");
  }
}
