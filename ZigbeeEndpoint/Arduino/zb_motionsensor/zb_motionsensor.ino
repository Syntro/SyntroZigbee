//
//  Copyright (c) 2012 Pansenti, LLC.
//
//  This file is part of Syntro
//
//  Syntro is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Syntro is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Syntro.  If not, see <http://www.gnu.org/licenses/>.
//
//
// A Parallax PIR motion sensor attached to an Arduino board that also 
// has a Zigbee radio.
//
// The Zigbee should be in API mode.
//
// The default configuration assumes Serial and the radio is configured
// for a baud rate of 115200.
//
// Only tested with an UNO
//
// The system must be 'armed' before it will send the state of the 
// motion sensor. It also needs a destination radio address which it
// gets from the 'arming' packet.
//
// Write one byte to the Arduino, non-zero arms, zero disarms.
//
// The Arduino board LED will be lit when the system is 'armed'.
//
// The Arduino will send back a 1 byte report whenever the sensor changes
// state. One is on, zero is off.
//

#define STATE_GET_START_DELIMITER 0
#define STATE_GET_LENGTH 1
#define STATE_GET_FRAME_TYPE 2
#define STATE_GET_SRC_ADDRESS 3
#define STATE_GET_SRC_SHORT_ADDRESS 4
#define STATE_GET_DST_SHORT_ADDRESS 5
#define STATE_GET_RECEIVE_OPTIONS 6
#define STATE_GET_DATA 20
#define STATE_GET_CHECKSUM 30


#define START_DELIMITER 0x7E

#define FRAME_TYPE_TRANSMIT_REQUEST 0x10
#define FRAME_TYPE_TRANSMIT_STATUS 0x8B
#define FRAME_TYPE_RECEIVE_PACKET 0x90
#define FRAME_TYPE_EXPLICIT_RX_IND 0x91

#define MAX_RX_FRAMELEN 28

#define BOARD_LED_PIN 13

#define MOTION_SENSOR_PIN 2


unsigned int rxState;
unsigned int rxCount;
unsigned int rxFrameLength;
unsigned char rxBuff[MAX_RX_FRAMELEN + 4];
unsigned char clientAddress[10];
bool armed;
int motionDetected;
unsigned long nextSendTime;

void setup() 
{
  rxState = STATE_GET_START_DELIMITER;
  rxCount = 0;
  armed = false;
  // set to an invalid value so we detect 'change' on the first sensor read  
  motionDetected = -1;
  nextSendTime = 0;

  Serial.begin(115200);  
  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(MOTION_SENSOR_PIN, INPUT);
}

void loop()
{
  if (readLoop()) {
    processCommand();
  }

  if (armed) {
    if (millis() > nextSendTime) {
      if (stateChanged()) {
        sendSensorState();
              
        if (motionDetected) {
          // the sensor stays armed for 3 seconds                        
          nextSendTime = millis() + 3000;
        }
        else {
          nextSendTime = millis() + 100;
        }
      }
    }
  } 
}

bool stateChanged()
{
  int val = digitalRead(MOTION_SENSOR_PIN);
  
  if (motionDetected != val) {
    motionDetected = val;
    return true;
  }

  return false;
}

// Assumes that at this point we are looking at a proper 'Receive Packet'
// or an 'Explicit RX Indicator' packet
void processCommand()
{    
  int i, val;
  
  if (rxCount < 17) {
    return;
  }
    
  if (rxBuff[3] == FRAME_TYPE_EXPLICIT_RX_IND) {
    val = rxBuff[21];
  }
  else {
    val = rxBuff[15];
  }
    
  if (val == 0) {
    armed = false;
    digitalWrite(BOARD_LED_PIN, LOW);
  }
  else {
    // copy src address and src short address into txBuff
    for (i = 0; i < 10; i++) {
      clientAddress[i] = rxBuff[i + 4];
    }

    armed = true;
    // so we don't respond immediately
    delay(50);
    digitalWrite(BOARD_LED_PIN, HIGH);
  }
}    

void sendSensorState()
{   
  int i;
  unsigned char tx[24];
      
  // most tx fields never change
  tx[0] = 0x7E;
  tx[1] = 0;      // len MSB
  tx[2] = 15;     // len LSB    
  tx[3] = FRAME_TYPE_TRANSMIT_REQUEST;
  tx[4] = 0x00;   // frame id, we don't want a response

  // bytes 5-12 are the dest address
  // bytes 13-14 are the dest net address
  // we saved this when we got armed
  for (i = 0; i < 10; i++) {
    tx[i+5] = clientAddress[i];
  }
    
  tx[15] = 0x00;  // broadcast radius, default
  tx[16] = 0x00;  // no options
  tx[17] = 0xff & motionDetected;
  tx[18] = calculateChecksum(tx + 3, 15);
      
  Serial.write(tx, 19);  
}

// Return true if we've assembled a valid packet
bool readLoop()
{
  unsigned char c;
  bool readDone = false;
    
  while (!readDone && Serial.available()) {
    c = Serial.read();

    switch (rxState) {
      case STATE_GET_START_DELIMITER:
        if (c == START_DELIMITER) {
          rxBuff[0] = c;
          rxCount = 1;
          rxState = STATE_GET_LENGTH;
        }
            
        break;
          
      case STATE_GET_LENGTH:
        rxBuff[rxCount++] = c;
          
        if (rxCount == 3) {
          rxFrameLength = (rxBuff[1] << 8) + rxBuff[2];
                
          if (rxFrameLength < MAX_RX_FRAMELEN) {
            rxState = STATE_GET_FRAME_TYPE;
          }
          else {
            rxState = STATE_GET_START_DELIMITER;
            rxCount = 0;
          }
        }   
            
        break;

      case STATE_GET_FRAME_TYPE:
        rxBuff[rxCount++] = c;
            
        if (c == FRAME_TYPE_RECEIVE_PACKET || c == FRAME_TYPE_EXPLICIT_RX_IND) {
          rxState = STATE_GET_DATA;
        }
        else {
          // not a real receive packet
          rxState = STATE_GET_START_DELIMITER;
          rxCount = 0;
        }
            
        break;
        
      case STATE_GET_DATA:
        rxBuff[rxCount++] = c;
            
        if (rxCount == rxFrameLength + 3) {
          rxState = STATE_GET_CHECKSUM;
        }
            
        break;
                
      case STATE_GET_CHECKSUM:
        rxBuff[rxCount++] = c;
    
        if (rxCount >= rxFrameLength + 3) {
          c = calculateChecksum(rxBuff + 3, rxFrameLength);
             
          // if the checksum is good, handle it
          if (c == rxBuff[rxCount - 1]) {
            readDone = true;
          }
                                
          rxState = STATE_GET_START_DELIMITER;
        }
        
        break;    
    }     
  }

  return readDone;    
} 

unsigned char calculateChecksum(unsigned char *data, int len)
{
  int i;
  unsigned char sum = data[0];
 
  for (i = 1; i < len; i++) {
    sum += data[i];
  }
  
  return 0xff - sum;
}


