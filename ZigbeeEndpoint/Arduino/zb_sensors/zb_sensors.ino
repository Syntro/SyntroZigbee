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
// Implements a sensor module that can wirelessly report back to readings
// and events to a base station.
//
// This code currently supports 3 sensors attached to an Arduino and
// using a Zigbee radio to report.
//
// The sensors
// 1. A Parallax PIR motion sensor attached to a digital pin input 
// 2. A TMP102 temperature sensor connected via I2C
// 3. A Mini Photocell light sensor connected to an analog pin input
//
//
// The Zigbee attached to the serial lines should be in API mode.
//
// The default configuration assumes Serial and the radio is configured
// for a baud rate of 115200.
//
// Tested with UNO and Duemilanove
//
// The system must be 'armed' before it will send the state of the 
// sensors. The requirement for arming is two-fold. 
//
// 1. The arming packet selects which sensors to enable
// 2. The Arduino needs to know which radio to report back results. 
//    The source radio of the 'arming' packet is used as the dest radio.
//
// To 'arm' the system, write one byte to the Arduino. The one byte is a 
// mask of the sensors of interest:
// SENSOR_TYPE_MOTION | SENSOR_TYPE_TEMP | SENSOR_TYPE_LIGHT
// A value of zero disarms or turns off the sensing.
//
// The Arduino board LED will be lit when the system is 'armed'.
//
// Motion reports will be sent back whenever there is a change.
//
// Temperature reports will be sent every TEMP_SENSOR_INTERVAL seconds.
//
// Light sensing reports will be sent whenever the light changes more
// then LIGHT_SENSOR_THRESHOLD units.
//

#include <Wire.h>

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
#define FRAME_TYPE_TRANSMIT_STATUS  0x8B
#define FRAME_TYPE_RECEIVE_PACKET   0x90
#define FRAME_TYPE_EXPLICIT_RX_IND  0x91

#define SENSOR_TYPE_MOTION          0x01
#define SENSOR_TYPE_TEMPERATURE     0x02
#define SENSOR_TYPE_LIGHT           0x04

#define MAX_RX_FRAMELEN 28
#define BOARD_LED_PIN 13

unsigned int rxState;
unsigned int rxCount;
unsigned int rxFrameLength;
unsigned char rxBuff[MAX_RX_FRAMELEN + 4];
unsigned char clientAddress[10];

unsigned char armed;

// motion sensor parameters
#define MOTION_SENSOR_PIN 2
#define MOTION_SENSOR_HIGH_DELAY 3000
#define MOTION_SENSOR_LOW_DELAY 100
int motion;
unsigned long nextMotionSendTime;

// temp sensor parameters
#define TEMP_SENSOR_INTERVAL 5000
// the I2C bus address of the TMP102
#define TEMP_SENSOR_ADDRESS 0x48
uint16_t temperature;
unsigned long nextTempSendTime;

// light sensor parameters
#define LIGHT_SENSOR_THRESHOLD 20
#define LIGHT_SENSOR_PIN A0
#define LIGHT_SENSOR_DELAY 1000
int light;
unsigned long nextLightSendTime;

void setup() 
{
  Wire.begin();
  
  // initialize the radio receive state machine
  rxState = STATE_GET_START_DELIMITER;
  rxCount = 0;
  
  armed = 0;
  
  resetSensors();
  
  Serial.begin(115200);  
  
  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(MOTION_SENSOR_PIN, INPUT);
}

void loop()
{
  if (readLoop()) {
    processCommand();
  }

  if (armed & SENSOR_TYPE_MOTION) {
    doMotionCheck();
  }
    
  if (armed & SENSOR_TYPE_LIGHT) {
    doLightCheck();
  }
    
  if (armed & SENSOR_TYPE_TEMPERATURE) {
    doTemperatureCheck();
  }
}

void doMotionCheck()
{
  if (millis() > nextMotionSendTime) {
    if (motionChanged()) {
      sendData(SENSOR_TYPE_MOTION, 1, motion);
              
      if (motion) {
        // the sensor stays armed for 3 seconds                        
        nextMotionSendTime = millis() + MOTION_SENSOR_HIGH_DELAY;
      }
      else {
        nextMotionSendTime = millis() + MOTION_SENSOR_LOW_DELAY;
      }
    }
  }
}

bool motionChanged()
{
  int val = digitalRead(MOTION_SENSOR_PIN);
  
  if (motion != val) {
    motion = val;
    return true;
  }

  return false;
}

void doLightCheck()
{
  if (millis() > nextLightSendTime) {
    if (lightChanged()) {
      sendData(SENSOR_TYPE_LIGHT, 2, light);
      nextLightSendTime = millis() + LIGHT_SENSOR_DELAY;
    }
  }
}

bool lightChanged()
{
  int val = analogRead(LIGHT_SENSOR_PIN);
  
  if (abs(val - light) > LIGHT_SENSOR_THRESHOLD) {
    light = val;
    return true;
  }
 
  return false; 
}

void doTemperatureCheck()
{
  if (millis() > nextTempSendTime) {
    if (readTemperature()) {
      sendData(SENSOR_TYPE_TEMPERATURE, 2, temperature);
    }

    nextTempSendTime = millis() + TEMP_SENSOR_INTERVAL;
  }     
}

bool readTemperature()
{
  uint8_t lsb;
  
  Wire.requestFrom(TEMP_SENSOR_ADDRESS, 2);
  
  if (Wire.available() >= 2) {
    temperature = Wire.read();
    lsb = Wire.read();
    temperature = ( temperature << 4) | (lsb >> 4);
    // This is Celsius, in 0.0625 increments
    // let the clients convert to Fahrenheit and/or floating point
    return true;
  }

  return false;  
}

void resetSensors()
{
  // set to an invalid value so we detect 'change' on the first sensor read  
  motion = -1;
  nextMotionSendTime = 0;

  temperature = 0;
  nextTempSendTime = 0;
  
  // an invalid value at startup
  light = 5000;
  nextLightSendTime = 0;
}

// Only handles dataLen = 1, 2 or 4 for now
void sendData(uint8_t msgType, uint8_t dataLen, uint32_t data)
{
  int i;
  unsigned char tx[24];
  
  // 16 = 14 for Zigbee overhead, 1 for sensor type, 1 for dataLen
  int len = 16 + dataLen;
  
  // most tx fields never change
  tx[0] = 0x7E;
  tx[1] = 0;            // len MSB
  tx[2] = 0xff & len;   // len LSB    
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
  
  tx[17] = msgType;
  tx[18] = dataLen;
  
  if (dataLen == 1) {
    tx[19] = 0xff & data;
  }
  else if (dataLen == 2) {
    tx[19] = 0xff & (data >> 8);
    tx[20] = 0xff & data;
  }
  else if (dataLen == 4) {
    tx[19] = 0xff & (data >> 24);
    tx[20] = 0xff & (data >> 16);
    tx[21] = 0xff & (data >> 8);
    tx[22] = 0xff & data;
  }
  else {
    return;
  }
    
  tx[len + 3] = calculateChecksum(tx + 3, len);
      
  Serial.write(tx, len + 4);

  delay(50);  
}

// Assumes that at this point we are looking at a proper Zigbee
// Receive Packet frame (0x90) or Explicit Receive Indicator frame (0x91)  
// We only understand one command, a 1-byte arming packet.
// The data byte is treated as a bitmask of which sensors to enable
// or zero to disable all. We keep the src address as the dest radio
// address to report too.
// Any data beyond the first byte is ignored.
void processCommand()
{    
  int i, val;
  
  if (rxCount < 17) {
    return;
  }
    
  if (rxBuff[3] == FRAME_TYPE_EXPLICIT_RX_IND) {
    armed = rxBuff[21];
  }
  else {
    armed = rxBuff[15];
  }
    
  if (armed == 0) {
    digitalWrite(BOARD_LED_PIN, LOW);
  }
  else {
    // copy src address and src short address into txBuff
    for (i = 0; i < 10; i++) {
      clientAddress[i] = rxBuff[i + 4];
    }

    // this might be a new arming host so reset to send initial status
    resetSensors();
    
    // so we don't respond immediately
    delay(50);
    digitalWrite(BOARD_LED_PIN, HIGH);
  }
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


