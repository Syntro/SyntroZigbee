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

#ifndef ZIGBEE_UTILS
#define ZIGBEE_UTILS

#include <qbytearray.h>

quint16 getU16(QByteArray data, int start);
void putU16(QByteArray *data, quint16 val, int pos = -1);

quint32 getU32(QByteArray data, int start);
void putU32(QByteArray *data, quint32 val, int pos = -1);

quint64 getU64(QByteArray data, int start);
void putU64(QByteArray *data, quint64 val, int pos = -1);

#endif // ZIGBEE_UTILS
