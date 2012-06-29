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

#include "ZigbeeData.h"

ZigbeeData::ZigbeeData()
{
	m_address = 0;
	m_expireTime = 0;
}

ZigbeeData::ZigbeeData(quint64 address, qint64 expireTime, QByteArray data)
{
	m_address = address;
	m_expireTime = expireTime;
	m_data = data;
}

ZigbeeData::ZigbeeData(const ZigbeeData &rhs)
{
	*this = rhs;
}
	
ZigbeeData& ZigbeeData::operator=(const ZigbeeData &rhs)
{
	if (this != &rhs) {
		m_address = rhs.m_address;
		m_expireTime = rhs.m_expireTime;
		m_data = rhs.m_data;
	}

	return *this;
}

bool ZigbeeData::expired(qint64 now)
{	
	if (now < m_expireTime) {
		// hmm, wrap, what is max time ?
		// just get it next time
		m_expireTime = now;
		return false;
	}

	return (m_expireTime < now);
}
		