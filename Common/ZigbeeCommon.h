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

#ifndef ZIGBEECOMMON_H
#define ZIGBEECOMMON_H

#define ZIGBEE_DEVICES      "zigbeeDevices"
#define ZIGBEE_ADDRESS      "address"
#define ZIGBEE_NODEID       "nodeID"
#define ZIGBEE_READONLY     "readOnly"
#define ZIGBEE_POLLINTERVAL "pollInterval"

#define ZIGBEE_MULTICAST_SERVICE  "multicastService"
#define ZIGBEE_E2E_SERVICE        "e2eService"
#define ZIGBEE_PROMISCUOUS_MODE   "promiscuousMode"

#define ZIGBEE_PORT             "zigbeePort"
#define ZIGBEE_SPEED            "zigbeeSpeed"
#define NODE_DISCOVER_INTERVAL  "nodeDiscoverInterval"


// Device type from ND response
// LOCAL is appended for the local radio
#define ZIGBEE_DEVICE_TYPE_COORDINATOR    0x00
#define ZIGBEE_DEVICE_TYPE_ROUTER         0x01
#define ZIGBEE_DEVICE_TYPE_ENDPOINT       0x02
#define ZIGBEE_DEVICE_TYPE_LOCAL          0x10

#define ZIGBEE_START_DELIM		          0x7E

// Frame types
#define ZIGBEE_FT_AT_COMMAND              0x08
#define ZIGBEE_FT_TRANSMIT_REQUEST	      0x10
#define ZIGBEE_FT_REMOTE_AT_COMMAND       0x17
#define ZIGBEE_FT_AT_COMMAND_RESPONSE     0x88
#define ZIGBEE_FT_TRANSMIT_STATUS	      0x8B
#define ZIGBEE_FT_RECEIVE_PACKET	      0x90
#define ZIGBEE_FT_EXPLICIT_RX_IND         0x91
#define ZIGBEE_FT_IO_DATA_SAMPLE_RX_IND   0x92
#define ZIGBEE_FT_SENSOR_READ_IND         0x94
#define ZIGBEE_FT_NODE_ID_IND             0x95
#define ZIGBEE_FT_REMOTE_COMMAND_RESPONSE 0x97

#define ZIGBEE_BROADCAST_ADDRESS          0xFFFE

#define ZIGBEE_AT_CMD_SH                  0x5348
#define ZIGBEE_AT_CMD_SL                  0x534C
#define ZIGBEE_AT_CMD_ID                  0x4944
#define ZIGBEE_AT_CMD_MV                  0x4D56
#define ZIGBEE_AT_CMD_ND                  0x4E44
#define ZIGBEE_AT_CMD_NI                  0x4E49
#define ZIGBEE_AT_CMD_OI                  0x4F49

#define ZIGBEE_MAX_NODE_ID                20

typedef struct
{
	quint16 cmd;
	quint16 recCount;  
} ZIGBEE_GATEWAY_RESPONSE;	

typedef struct
{
	quint64 address;
	quint16 netAddress;
	quint8 deviceType;
	char nodeID[ZIGBEE_MAX_NODE_ID + 1];	
} ZIGBEE_NODE_DATA;


#endif // ZIGBEECOMMON_H
