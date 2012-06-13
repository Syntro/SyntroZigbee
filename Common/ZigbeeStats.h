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

#ifndef ZIGBEESTATS_H
#define ZIGBEESTATS_H

#include <qglobal.h>
#include <qstring.h>

#include "ZigbeeCommon.h"


class ZigbeeStats {
public:
	ZigbeeStats();
	ZigbeeStats(quint64 address, quint8 frameId, quint16 netAddress = ZIGBEE_BROADCAST_ADDRESS,
		quint32 discoverSequence = 0);

	ZigbeeStats(const ZigbeeStats &rhs);

	ZigbeeStats& operator=(const ZigbeeStats &rhs);

	void clear();
	void updateFromNodeDiscovery(const ZigbeeStats *rhs);

	quint64 m_address;
	quint16 m_netAddress;
	quint8 m_lastFrameID;
	quint32 m_txCount;
	quint32 m_rxCount;
	quint8 m_lastDeliveryStatus;
	quint8 m_lastDiscoveryStatus;
	quint8 m_lastReceiveOptions;

	quint16 m_panID;
	quint16 m_parentNetAddress;
	quint8 m_deviceType;
    quint16 m_profileID;
	quint16 m_mfgID;

	QString m_nodeID;
	QString m_newNodeID;

	quint32 m_nodeDiscoverSequence;
};

#endif // ZIGBEESTATS_H
