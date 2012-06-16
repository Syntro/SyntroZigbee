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
// A Parallax PIR motion sensor attached to a Maple board that also 
// has a Zigbee radio.
//
// The Zigbee should be in API mode.
//
// The default configuration assumes Serial1 and the radio is configured
// for a baud rate of 115200.
//
// Any of the Maple boards will work: Maple, MapleMini or MapleNative.
//
// I'm powering the sensor with 3.3v so any of the GPIO pins are suitable
// for input. The default is pin 5 for no particular reason.
//
// The system must be 'armed' before it will send the state of the 
// motion sensor. It also needs a destination radio address which it
// gets from the 'arming' packet.
//
// Write one byte to the Maple, non-zero arms, zero disarms.
//
// The Maple board LED will be lit when the system is 'armed'.
//
// The Maple will send back a 1 byte report whenever the sensor changes
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

// you can assign BOARD_LED_PIN for this if you want to test without a sensor
#define MOTION_SENSOR_PIN 5

unsigned int rxState;
unsigned int rxCount;
unsigned int rxFrameLength;
unsigned char rxBuff[MAX_RX_FRAMELEN + 4];
unsigned char clientAddress[10];
bool haveClient;
int sensorState;
unsigned long nextSendTime;

void setup() 
{
    rxState = STATE_GET_START_DELIMITER;
    rxCount = 0;
    haveClient = false;    
    sensorState = -1;
    nextSendTime = 0;

    Serial1.begin(115200);  
    pinMode(BOARD_LED_PIN, OUTPUT);
    pinMode(MOTION_SENSOR_PIN, INPUT_PULLDOWN);
}

void loop()
{
    if (readLoop()) {
        processCommand();
    }

    if (haveClient) {
        if (millis() > nextSendTime) {
       	    if (stateChanged()) {
               	sendState();
               
                if (sensorState) {
                    // the sensor stays armed for 5 seconds                        
	       	    nextSendTime = millis() + 5000;
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
  
    if (sensorState != val) {
        sensorState = val;
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
        haveClient = false;
        digitalWrite(BOARD_LED_PIN, LOW);
    }
    else {
    	// copy src address and src short address into txBuff
    	for (i = 0; i < 10; i++) {
            clientAddress[i] = rxBuff[i + 4];
    	}

    	haveClient = true;
        // so we don't respond immediately
        delay(50);
	digitalWrite(BOARD_LED_PIN, HIGH);
    }
}    

void sendState()
{   
    int i;
    unsigned char tx[24];
      
    // most tx fields that never change
    tx[0] = 0x7E;
    tx[1] = 0;      // len MSB
    tx[2] = 15;     // len LSB    
    tx[3] = FRAME_TYPE_TRANSMIT_REQUEST;
    tx[4] = 0x00;   // frame id, we don't want a response

    // bytes 5-12 are the dest address
    // bytes 13-14 are the dest net address
    // they need to be filled in when we know our dest
    for (i = 0; i < 10; i++) {
        tx[i+5] = clientAddress[i];
    }
    
    tx[15] = 0x00;  // broadcast radius, default
    tx[16] = 0x00;  // no options
    tx[17] = 0xff & sensorState;
    tx[18] = calculateChecksum(tx + 3, 15);
       
    Serial1.write((void *)tx, 19);  
}

// Return true if we've assembled a valid packet
bool readLoop()
{
    unsigned char c;
    bool readDone = false;
    
    while (!readDone && Serial1.available()) {
        c = Serial1.read();

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

