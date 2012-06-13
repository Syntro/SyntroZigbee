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

#include "ZigbeeUtils.h"

quint16 getU16(QByteArray data, int start)
{
	quint16 val = 0xff & data.at(start);
	val <<= 8;
	val += 0xff & data.at(start + 1);

	return val;
}

void putU16(QByteArray *data, quint16 val, int pos)
{
	if (pos == -1) {
		data->append(0xff & (val >> 8));
		data->append(0xff & val);
	}
	else {
		data->insert(pos, 0xff & (val >> 8));
		data->insert(pos, 0xff & val);
	}
}

quint32 getU32(QByteArray data, int start)
{
	quint32	val = 0xff & data.at(start);

	for (int i = start + 1; i < start + 4; i++) {
		val <<= 8;
		val += 0xff & data.at(i);
	}

	return val;
}

void putU32(QByteArray *data, quint32 val, int pos)
{
	if (pos == -1) {
		data->append(0xff & (val >> 24));
		data->append(0xff & (val >> 16));
		data->append(0xff & (val >> 8));
		data->append(0xff & val);
	}
	else {
		data->insert(pos, 0xff & (val >> 24));
		data->insert(pos, 0xff & (val >> 16));
		data->insert(pos, 0xff & (val >> 8));
		data->insert(pos, 0xff & val);
	}
}

quint64 getU64(QByteArray data, int start)
{
	quint64	val = 0xff & data.at(start);

	for (int i = start + 1; i < start + 8; i++) {
		val <<= 8;
		val += 0xff & data.at(i);
	}

	return val;
}

void putU64(QByteArray *data, quint64 val, int pos)
{
	if (pos == -1) {
		data->append(0xff & (val >> 56));
		data->append(0xff & (val >> 48));
		data->append(0xff & (val >> 40));
		data->append(0xff & (val >> 32));
		data->append(0xff & (val >> 24));
		data->append(0xff & (val >> 16));
		data->append(0xff & (val >> 8));
		data->append(0xff & val);
	}
	else {
		data->insert(pos, 0xff & (val >> 56));
		data->insert(pos, 0xff & (val >> 48));
		data->insert(pos, 0xff & (val >> 40));
		data->insert(pos, 0xff & (val >> 32));
		data->insert(pos, 0xff & (val >> 24));
		data->insert(pos, 0xff & (val >> 16));
		data->insert(pos, 0xff & (val >> 8));
		data->insert(pos, 0xff & val);
	}
}

