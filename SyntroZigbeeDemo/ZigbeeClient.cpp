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

#include "ZigbeeClient.h"
#include "ZigbeeCommon.h"
#include "ZigbeeUtils.h"

#define	ZIGBEECLIENT_BACKGROUND_INTERVAL (SYNTRO_CLOCKS_PER_SEC / 10)


ZigbeeClient::ZigbeeClient(QObject *parent, QSettings *settings)
	: Endpoint(parent, settings, ZIGBEECLIENT_BACKGROUND_INTERVAL)
{
	m_receivePort = -1;
	m_controlPort = -1;
}

void ZigbeeClient::appClientInit()
{
	if (m_settings->contains(ZIGBEE_MULTICAST_SERVICE)) {
		QString multicastStream = m_settings->value(ZIGBEE_MULTICAST_SERVICE).toString();
					
		m_receivePort = clientAddService(multicastStream, SERVICETYPE_MULTICAST, false, true);

		if (m_receivePort < 0)
			logWarn(QString("Error adding multicast service for %1").arg(multicastStream));
	}

	if (m_settings->contains(ZIGBEE_E2E_SERVICE)) {
		QString e2eStream = m_settings->value(ZIGBEE_E2E_SERVICE).toString();

		m_controlPort = clientAddService(e2eStream, SERVICETYPE_E2E, false, true);

		if (m_controlPort < 0)
			logWarn(QString("Error adding E2E service for %1").arg(e2eStream));
	}
}

void ZigbeeClient::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multicast, int len)
{
	if (servicePort != m_receivePort) {
		logWarn(QString("Multicast received to invalid port %1").arg(servicePort));
		free(multicast);
		return;
	}

	if (len <= (int)(sizeof(SYNTRO_RECORD_HEADER) + sizeof(quint64))) {
		logWarn(QString("Multicast length is unexpected : %1").arg(len - sizeof(SYNTRO_RECORD_HEADER)));
		free(multicast);
		return;
	}

	SYNTRO_RECORD_HEADER *head = (SYNTRO_RECORD_HEADER *)(multicast + 1);

	quint8 *p = (quint8 *)(head + 1);

	quint64 address = p[0];

	for (int i = 1; i < 8; i++) {
		address <<= 8;
		address += p[i];
	}

	len -= sizeof(SYNTRO_RECORD_HEADER) + sizeof(quint64);

	if (address == 0)
		processRadioList(QByteArray((const char *)(p + 8), len));
	else
		emit receiveData(address, QByteArray((const char *)(p + 8), len));

	clientSendMulticastAck(servicePort);

	free(multicast);
}

bool ZigbeeClient::sendData(quint64 address, QByteArray data)
{
	if (!clientIsConnected())
		return false;

	if (!clientIsServiceActive(m_controlPort))
		return false;

	int length = sizeof(quint64) + data.length();

	SYNTRO_EHEAD *head = clientBuildMessage(m_controlPort, length);

	quint8 *p = (quint8 *)(head + 1);

	for (int i = 56; i >= 0; i -= 8)
		*p++ = 0xff & (address >> i);		

	memcpy(p, data.constData(), data.length());

	clientSendMessage(m_controlPort, head, length, SYNTROLINK_MEDPRI);

	return true;
}

void ZigbeeClient::processRadioList(QByteArray data)
{
	QList<ZigbeeStats> list;
	ZigbeeStats zb;
	ZIGBEE_GATEWAY_RESPONSE zgr;
	ZIGBEE_NODE_DATA node;
	int pos = 0;

	if (data.length() < (int)sizeof(ZIGBEE_GATEWAY_RESPONSE))
		return;

	zgr.cmd = getU16(data, pos);
	pos += 2;

	if (zgr.cmd != ZIGBEE_AT_CMD_ND)
		return;

	zgr.recCount = getU16(data, pos);
	pos += 2;

	if (zgr.recCount < 1)
		emit receiveRadioList(list);

	if (data.length() < (int)(sizeof(zgr) + (zgr.recCount * sizeof(node))))
		return;

	for (int i = 0; i < zgr.recCount; i++) {
		zb.clear();

		zb.m_address = getU64(data, pos);
		pos += 8;
		zb.m_netAddress = getU16(data, pos);
		pos += 2;
		zb.m_deviceType = data.at(pos);
		pos++;

		for (int i = 0; i < ZIGBEE_MAX_NODE_ID; i++, pos++)
			node.nodeID[i] = data.at(pos);

		node.nodeID[ZIGBEE_MAX_NODE_ID] = 0;
		pos++;

		zb.m_nodeID = node.nodeID;

		list.append(zb);
	}

	emit receiveRadioList(list);
}
