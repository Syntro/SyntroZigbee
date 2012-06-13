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

#include "ZigbeeDevice.h"
	
ZigbeeDevice::ZigbeeDevice()
{
	clear();
}

ZigbeeDevice::ZigbeeDevice(quint64 address, bool readOnly, int pollInterval)
{
	m_address = address;
	m_readOnly = readOnly;
	m_pollInterval = pollInterval;

	if (m_pollInterval < 0)
		m_pollInterval = 0;
}

ZigbeeDevice::ZigbeeDevice(const ZigbeeDevice &rhs)
{
	*this = rhs;
}
	
ZigbeeDevice& ZigbeeDevice::operator=(const ZigbeeDevice &rhs)
{
	if (this != &rhs) {
		m_address = rhs.m_address;
		m_readOnly = rhs.m_readOnly;
		m_pollInterval = rhs.m_pollInterval;
	}

	return *this;
}

void ZigbeeDevice::clear()
{
	m_address = 0;
	m_readOnly = false;
	m_pollInterval = 0;
}
