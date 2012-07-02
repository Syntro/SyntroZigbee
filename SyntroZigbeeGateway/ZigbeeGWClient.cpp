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

#include "ZigbeeGWClient.h"
#include "ZigbeeUtils.h"

#define	BACKGROUND_INTERVAL (SYNTRO_CLOCKS_PER_SEC / 10)

#define ZIGBEE_DATA_TYPE (SYNTRO_RECORD_TYPE_USER)

#define DEFAULT_EXPIRE_SECS 60
#define MIN_EXPIRE_SECS 30
#define MAX_EXPIRE_SECS 7200

#define MAX_RX_QUEUE_SIZE 100

ZigbeeGWClient::ZigbeeGWClient(QObject *parent, QSettings *settings)
	: Endpoint(parent, settings, BACKGROUND_INTERVAL)
{
	m_multicastPort = -1;
	m_e2ePort = -1;
	m_promiscuousMode = false;
	m_localZigbeeAddress = 0;

	m_rxQExpireSecs = settings->value(MULTICAST_Q_EXPIRE_INTERVAL, DEFAULT_EXPIRE_SECS).toInt(); 	

	if (m_rxQExpireSecs < MIN_EXPIRE_SECS)
		m_rxQExpireSecs = MIN_EXPIRE_SECS;
	else if (m_rxQExpireSecs > MAX_EXPIRE_SECS)
		m_rxQExpireSecs = MAX_EXPIRE_SECS;

	// assumes SYNTRO_CLOCKS_PER_SEC == 1000
	m_rxQExpireTicks = m_rxQExpireSecs * (SYNTRO_CLOCKS_PER_SEC / BACKGROUND_INTERVAL);
}

void ZigbeeGWClient::localRadioAddress(quint64 address)
{
	m_localZigbeeAddress = address;
}

void ZigbeeGWClient::appClientInit()
{
	quint64 address;
	bool ok;
	ZigbeeDevice *zb;

	if (m_settings->contains(ZIGBEE_MULTICAST_SERVICE)) {
		QString multicastStream = m_settings->value(ZIGBEE_MULTICAST_SERVICE).toString();

		m_multicastPort = clientAddService(multicastStream, SERVICETYPE_MULTICAST, true, true);

		if (m_multicastPort < 0)
			logWarn(QString("Error adding multicast service for %1").arg(multicastStream));
	}

	if (m_settings->contains(ZIGBEE_E2E_SERVICE)) {
		QString e2eStream = m_settings->value(ZIGBEE_E2E_SERVICE).toString();

		m_e2ePort = clientAddService(e2eStream, SERVICETYPE_E2E, true, true);

		if (m_e2ePort < 0)
			logWarn(QString("Error adding E2E service for %1").arg(e2eStream));
	}

	m_promiscuousMode = m_settings->value(ZIGBEE_PROMISCUOUS_MODE, false).toBool();

	int count = m_settings->beginReadArray(ZIGBEE_DEVICES);

	for (int i = 0; i < count; i++) {
		m_settings->setArrayIndex(i);

		if (!m_settings->contains(ZIGBEE_ADDRESS)) {
			logWarn(QString("Missing Zigbee long address for entry %1 - not using").arg(i));
			continue;
		}

		address = m_settings->value(ZIGBEE_ADDRESS).toString().toULongLong(&ok, 16);

		if (!ok || address == 0) {
			logWarn(QString("Invalid address : %1").arg(m_settings->value(ZIGBEE_ADDRESS).toString()));
			continue;
		}

		if (m_devices.contains(address)) {
			logWarn(QString("Error duplicate zigbee device entry 0x%1").arg(address, 16, 16, QChar('0')));
			continue;
		}

		bool readOnly = m_settings->value(ZIGBEE_READONLY, false).toBool();
		int pollInterval = m_settings->value(ZIGBEE_POLLINTERVAL, 0).toInt();

		zb = new ZigbeeDevice(address, readOnly, pollInterval);

		if (!zb) {
			logWarn("Error allocating new ZigbeeDevice");
			continue;
		}

		m_devices.insert(address, zb);
	}

	m_settings->endArray();	
}

void ZigbeeGWClient::appClientBackground()
{
	m_rxQExpireTicks--;

	if (m_rxQExpireTicks <= 0) {
		purgeExpiredQueueData();
		m_rxQExpireTicks = m_rxQExpireSecs * (SYNTRO_CLOCKS_PER_SEC / BACKGROUND_INTERVAL);
	}

	if (!clientIsServiceActive(m_multicastPort))
		return;

	if (!clientClearToSend(m_multicastPort))
		return;

	issuePollRequests();

	sendReceivedData();
}

void ZigbeeGWClient::appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *header, int length)
{
	ZigbeeDevice *zb;

	if (servicePort != m_e2ePort) {
		logWarn(QString("Received E2E for wrong port %1").arg(servicePort));
		free(header);
		return;
	}

	// need at least the 64-bit Zigbee address and 1 byte of data
	if (length <= (int)sizeof(quint64)) {
		logWarn(QString("Received E2E of length %1").arg(length));
		free(header);
		return;
	}

	// jump to the data, a 64-bit address, followed by the real data
	quint8 *p = (quint8 *)(header + 1);

	quint64 address = p[0];

	for (int i = 1; i < 8; i++) {
		address <<= 8;
		address += p[i];
	}

	if (address == m_localZigbeeAddress || address == 0) {
		// this is either a command for the gateway or it is a request to
		// execute an AT command on behalf of a Syntro client
		executeLocalRadioCommand(p + 8, length - 8);
		free(header);
		return;
	}

	if (!m_devices.contains(address)) {
		if (!m_promiscuousMode) {
			if (m_badTxDevices.contains(address)) {
				m_badTxDevices[address]++;
			}
			else {
				m_badTxDevices.insert(address, 1);
				logWarn(QString("Rejected write to unauthorized device 0x%1").arg(address, 16, 16, QChar('0')));
			}

			free(header);
			return;
		}

		// add a new entry
		zb = new ZigbeeDevice(address, false, 0);

		if (!zb) {
			free(header);
			return;
		}

		m_devices.insert(address, zb);
	}
	else {
		zb = m_devices.value(address);

		if (zb->m_readOnly) {
			logWarn(QString("Received E2E for read-only device 0x%1").arg(address, 16, 16, QChar('0')));
			free(header);
			return;
		}
	}

	emit sendData(address, QByteArray((const char *)(p + 8), length - 8));

	free(header);
}

void ZigbeeGWClient::issuePollRequests()
{
	// TODO
}

// TODO: process more then one rx packet if we have them
void ZigbeeGWClient::sendReceivedData()
{
	ZigbeeData zbData;

	if (!getRxData(&zbData))
		return;

	int length = sizeof(SYNTRO_RECORD_HEADER) + sizeof(quint64) + zbData.m_data.length();

	SYNTRO_EHEAD *multicast = clientBuildMessage(m_multicastPort, length);
	if (!multicast)
		return;

	SYNTRO_RECORD_HEADER *head = (SYNTRO_RECORD_HEADER *)(multicast + 1);

	convertIntToUC2(ZIGBEE_DATA_TYPE, head->type);
	convertIntToUC2(sizeof(SYNTRO_RECORD_HEADER), head->headerLength);
	convertIntToUC2(0, head->subType);
	convertIntToUC2(0, head->param);
	setSyntroTimestamp(&head->timestamp);

	quint8 *p = (quint8 *)(head + 1);

	for (int i = 56; i >= 0; i -= 8)
		*p++ = 0xff & (zbData.m_address >> i);

	memcpy(p, zbData.m_data.constData(), zbData.m_data.length());

	clientSendMessage(m_multicastPort, multicast, length, SYNTROLINK_MEDPRI);
}

bool ZigbeeGWClient::getRxData(ZigbeeData *zbData)
{
	QMutexLocker lock(&m_rxMutex);
	
	if (m_rxQ.empty())
		return false;

	*zbData = m_rxQ.dequeue();

	return true;
}

void ZigbeeGWClient::receiveData(quint64 address, QByteArray data)
{
	QMutexLocker lock(&m_rxMutex);

	if (m_devices.contains(address) || m_promiscuousMode) {
		while (m_rxQ.size() > MAX_RX_QUEUE_SIZE)
			m_rxQ.removeFirst();

		m_rxQ.enqueue(ZigbeeData(address, (1000 * m_rxQExpireSecs) + SyntroClock(), data));
	}
	else {
		if (m_badRxDevices.contains(address)) {
			m_badRxDevices[address]++;
		}
		else {
			m_badRxDevices.insert(address, 1);
			logWarn(QString("Rejected data received from unauthorized device 0x%1").arg(address, 16, 16, QChar('0')));
		}
	}
}

// we only support one command right now, node discovery 'ND'
void ZigbeeGWClient::executeLocalRadioCommand(quint8 *request, int length)
{
	if (length < 2)
		return;

	if (request[0] == 'N' && request[1] == 'D')
		emit requestNodeDiscover(); 
}

void ZigbeeGWClient::nodeDiscoverResponse(QList<ZigbeeStats> list)
{
	QByteArray data;
	int i, j, len;
	char s[32];
	int recCount = list.count();

	// pack the ZIGBEE_GATEWAY_RESPONSE	header
	putU16(&data, ZIGBEE_AT_CMD_ND);
	putU16(&data, recCount);

	// pack the ZIGBEE_NODE_DATA records
	for (i = 0; i < recCount; i++) {
		ZigbeeStats zb = list.at(i);

		putU64(&data, zb.m_address);
		putU16(&data, zb.m_netAddress);
		data.append((char) zb.m_deviceType);

		memset(s, 0, sizeof(s));
		strncpy(s, qPrintable(zb.m_nodeID), 20);

		len = strlen(s);

		if (len > ZIGBEE_MAX_NODE_ID)
			len = ZIGBEE_MAX_NODE_ID;

		for (j = 0; j < len; j++)
			data.append((char)s[j]);

		// pad out and ensure a terminating zero for client convenience
		for (; j < ZIGBEE_MAX_NODE_ID + 1; j++)
			data.append((char)0x00);
	}

	// add to the multicast rx queue
	m_rxMutex.lock();
	m_rxQ.enqueue(ZigbeeData(0, SyntroClock(), data));
	m_rxMutex.unlock();
}

void ZigbeeGWClient::purgeExpiredQueueData()
{
	QMutexLocker lock(&m_rxMutex);

	qint64 now = SyntroClock();

	while (!m_rxQ.isEmpty()) {
		ZigbeeData zbData = m_rxQ.first();

		if (!zbData.expired(now))
			break;

		m_rxQ.removeFirst();
	}
}
