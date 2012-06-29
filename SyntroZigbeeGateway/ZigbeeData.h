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

#ifndef ZIGBEEDATA_H
#define ZIGBEEDATA_H

#include <qbytearray.h>

class ZigbeeData
{
public:
	ZigbeeData();
	ZigbeeData(quint64 address, qint64 expireTime, QByteArray data);
	ZigbeeData(const ZigbeeData &rhs);
	
	ZigbeeData& operator=(const ZigbeeData &rhs);

	bool expired(qint64 now);

	quint64 m_address;
	qint64 m_expireTime;
	QByteArray m_data;
};


#endif // ZIGBEEDATA_H
