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

#ifndef ZIGBEE_DEVICE_H
#define ZIGBEE_DEVICE_H

#include <qglobal.h>

class ZigbeeDevice {
public:
	ZigbeeDevice();
	ZigbeeDevice(quint64 address, bool readOnly, int pollInterval);
	ZigbeeDevice(const ZigbeeDevice &rhs);
	
	ZigbeeDevice& operator=(const ZigbeeDevice &rhs);

	void clear();

	quint64 m_address;
	bool m_readOnly;
	int m_pollInterval;
};

#endif // ZIGBEE_DEVICE_H
