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
// This is a simple Arduino demo to show a Syntro app controlling
// the Arduino board LED remotely using a Zigbee radio.
//
// The Zigbee should be in API mode and attached to the Serial1 port of
// the Maple at 115200 baud. 
//
// Any Arduino should work.
//
// The protocol is very simple, write two bytes and the Arduino will
// interpret that as a write command. If the second byte is non-zero
// the led will be turned on. Otherwise, it will be turned off.
//
// Write one byte to the Arduino and that will be interpreted as a read.
// The Arduino will respond back with a two byte write. The first byte
// will be the number of data bytes, always 0x01 for now, and the 
// second byte will be the state of the led pin.
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

#define MAX_RX_FRAMELEN 64

#define BOARD_LED_PIN 13

unsigned int rxState;
unsigned int rxCount;
unsigned int rxFrameLength;
unsigned char rxBuff[MAX_RX_FRAMELEN + 4];

void setup() 
{
    rxState = STATE_GET_START_DELIMITER;
    rxCount = 0;
    Serial.begin(115200);  
    pinMode(BOARD_LED_PIN, OUTPUT);
}

void loop() 
{
    unsigned char c;
    
    while (Serial.available()) {
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
                    }
                }   
            
                break;

            case STATE_GET_FRAME_TYPE:
                rxBuff[rxCount++] = c;
            
                if (c == FRAME_TYPE_RECEIVE_PACKET) {
                    rxState = STATE_GET_DATA;
                }
                else {
                    // not a real receive packet
                    rxState = STATE_GET_START_DELIMITER;
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
                        handleRxData();
                    }
                                
                    rxState = STATE_GET_START_DELIMITER;
                }
        
                break;    
        } 
    }    
} 

// Assumes we are looking at a proper Zigbee Receive Packet
// Data starts at byte 15 and goes to rxCount - 2
void handleRxData()
{
#define RECEIVE_DATA_START 15    
   
    int len = (rxCount - 1) - RECEIVE_DATA_START;
    
    if (len == 1) {
        handleRead();
    }   
    else if (len > 1) {
        // this is a write, the second byte is the value    
        if (rxBuff[RECEIVE_DATA_START + 1] == 0) {
            digitalWrite(BOARD_LED_PIN, LOW);
        }
        else {
            digitalWrite(BOARD_LED_PIN, HIGH);
        }
    }   
}    

void handleRead()
{
    int i, len;
    unsigned char txBuff[24];
    
    len = 16;           // 14 + the actual data, two bytes for this demo
    
    txBuff[0] = 0x7E;
    txBuff[1] = 0;      // len MSB
    txBuff[2] = len;    // len LSB    
    txBuff[3] = FRAME_TYPE_TRANSMIT_REQUEST;
    txBuff[4] = 0x00;   // frame id, we don't want a response      
        
    // copy src address and src short address into dst fields
    for (i = 5; i < 15; i++) {
        txBuff[i] = rxBuff[i-1];
    }
     
    txBuff[15] = 0x00;  // broadcast radius, default
    txBuff[16] = 0x00;  // no options
        
    // our data
    txBuff[17] = 0x01;  // number of bytes
    txBuff[18] = digitalRead(BOARD_LED_PIN);
            
    txBuff[19] = calculateChecksum(txBuff + 3, len);
    len += 4;

    delay(100);
    
    Serial.write(txBuff, len);
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

