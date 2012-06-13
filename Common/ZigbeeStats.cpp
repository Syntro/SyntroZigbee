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

#include "ZigbeeStats.h"


ZigbeeStats::ZigbeeStats()
{
	m_address = 0;
	m_netAddress = ZIGBEE_BROADCAST_ADDRESS;
	m_lastFrameID = 0;
	m_txCount = 0;
	m_rxCount = 0;
	m_lastDeliveryStatus = 0;
	m_lastDiscoveryStatus = 0;
	m_lastReceiveOptions = 0;
	m_panID = 0;
	m_parentNetAddress = 0;
	m_deviceType = 0;
    m_profileID = 0;
	m_mfgID = 0;
	m_nodeDiscoverSequence = 0;
}

ZigbeeStats::ZigbeeStats(quint64 address, quint8 frameID, quint16 netAddress, quint32 discoverSequence)
{
	m_address = address;
	m_netAddress = netAddress;
	m_lastFrameID = frameID;
	m_txCount = 0;
	m_rxCount = 0;
	m_lastDeliveryStatus = 0;
	m_lastDiscoveryStatus = 0;
	m_lastReceiveOptions = 0;
	m_panID = 0;
	m_parentNetAddress = 0;
	m_deviceType = 0;
    m_profileID = 0;
	m_mfgID = 0;
	m_nodeDiscoverSequence = discoverSequence;
}

ZigbeeStats::ZigbeeStats(const ZigbeeStats &rhs)
{
	*this = rhs;
}

ZigbeeStats& ZigbeeStats::operator=(const ZigbeeStats &rhs)
{
	if (this != &rhs) {
		m_address = rhs.m_address;
		m_netAddress = rhs.m_netAddress;
		m_lastFrameID = rhs.m_lastFrameID;
		m_txCount = rhs.m_txCount;
		m_rxCount = rhs.m_rxCount;
		m_lastDeliveryStatus = rhs.m_lastDeliveryStatus;
		m_lastDiscoveryStatus = rhs.m_lastDiscoveryStatus;
		m_lastReceiveOptions = rhs.m_lastReceiveOptions;
		m_panID = rhs.m_panID;
		m_parentNetAddress = rhs.m_parentNetAddress;
		m_deviceType = rhs.m_deviceType;
		m_profileID = rhs.m_profileID;
		m_mfgID = rhs.m_mfgID;
		m_nodeID = rhs.m_nodeID;
		m_newNodeID = rhs.m_newNodeID;
		m_nodeDiscoverSequence = rhs.m_nodeDiscoverSequence;
	}

	return *this;
}

void ZigbeeStats::clear()
{
	m_address = 0;
	m_netAddress = ZIGBEE_BROADCAST_ADDRESS;
	m_lastFrameID = 0;
	m_txCount = 0;
	m_rxCount = 0;
	m_lastDeliveryStatus = 0;
	m_lastDiscoveryStatus = 0;
	m_lastReceiveOptions = 0;
	m_panID = 0;
	m_parentNetAddress = 0;
	m_deviceType = 0;
    m_profileID = 0;
	m_mfgID = 0;
	m_nodeDiscoverSequence = 0;
	m_nodeID.clear();
	m_newNodeID.clear();
}

void ZigbeeStats::updateFromNodeDiscovery(const ZigbeeStats *rhs)
{
	m_netAddress = rhs->m_netAddress;
	m_parentNetAddress = rhs->m_parentNetAddress;
	m_deviceType = rhs->m_deviceType;
	m_profileID = rhs->m_profileID;
	m_mfgID = rhs->m_mfgID;
	m_nodeID = rhs->m_nodeID;
	m_nodeDiscoverSequence = rhs->m_nodeDiscoverSequence;
}
