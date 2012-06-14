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

#include <stdio.h>

#include "ZigbeeController.h"
#include "ZigbeeUtils.h"


#define ZIGBEE_CONTROLLER_BACKGROUND_INTERVAL 50

ZigbeeController::ZigbeeController()
{
	m_stop = true;
	m_debugDump = false;
	m_localAddress = 0;
	m_localTxCount = 0;
	m_localRxCount = 0;
	m_panID = 0;
	m_nodeDiscoverWait = 0;
	m_nodeDiscoverSequence = 0;
	m_port = NULL;
	m_lastFrameID = 0;
	memset(m_pendingFrames, 0, sizeof(m_pendingFrames));
}

ZigbeeController::~ZigbeeController()
{
	closeDevice();

	qDeleteAll(m_zbStats);
}

bool ZigbeeController::isOpen()
{
	if (!m_port)
		return false;

	return m_port->isOpen();
}

bool ZigbeeController::openDevice(QSettings *settings)
{
	BaudRateType baud;

	if (isRunning())
		return false;

	if (m_port) {
		m_port->close();
		delete m_port;
		m_port = NULL;
	}

	if (!settings->contains(ZIGBEE_PORT)) {
		qDebug("No zigbee port setting");
		return false;
	}

	QString name = settings->value(ZIGBEE_PORT).toString();

	int speed = settings->value(ZIGBEE_SPEED, 115200).toInt();

	switch (speed) {
	case 9600:
		baud = BAUD9600;
		break;

	case 19200:
		baud = BAUD19200;
		break;

	case 38400:
		baud = BAUD38400;
		break;

	case 57600:
		baud = BAUD57600;
		break;

	case 115200:
		baud = BAUD115200;
		break;

	default:
		qDebug("Invalid port baud rate %d", speed);
		return false;
	}

	m_port = new QextSerialPort(name, QextSerialPort::EventDriven);

	if (!m_port) {
		qDebug("Error creating serial port object");
		return false;
	}

	m_port->setBaudRate(baud);
	m_port->setDataBits(DATA_8);
	m_port->setStopBits(STOP_1);
	m_port->setParity(PAR_NONE);
	m_port->setFlowControl(FLOW_OFF);

	if (!m_port->open(QIODevice::ReadWrite)) {
		delete m_port;
		m_port = NULL;
		qDebug("Error opening serial port");
		return false;
	}

	memset(m_pendingFrames, 0, sizeof(m_pendingFrames));
	m_lastFrameID = 0;

	return true;
}

void ZigbeeController::closeDevice()
{
	stopRunLoop();

	if (m_port) {
		m_port->close();
		delete m_port;
		m_port = false;
	}
}

void ZigbeeController::startRunLoop()
{
	if (isRunning())
		return;

	if (!m_port)
		return;

	connect(m_port, SIGNAL(readyRead()), this, SLOT(readyRead()));

	m_stop = false;
	
	queryLocalRadio();

	requestNodeDiscover();

	start();
}

void ZigbeeController::stopRunLoop()
{
	if (!isRunning())
		return;

	disconnect(m_port, SIGNAL(readyRead()), this, SLOT(readyRead()));

	m_stop = true;

	for (int i = 0; i < 4; i++) {
		if (wait(500))
			break;

		qDebug("Waiting for ZigbeeController thread to finish...");
	}
}

void ZigbeeController::requestNodeDiscover()
{
	// only one pending at a time
	if (m_nodeDiscoverWait > 0)
		return;

	postATCommand(ZIGBEE_AT_CMD_ND);

	// get smarter about this timing
	m_nodeDiscoverWait = 5 * (1000 / ZIGBEE_CONTROLLER_BACKGROUND_INTERVAL);
	m_nodeDiscoverSequence++;
}

void ZigbeeController::requestNodeIDChange(quint64 address, QString nodeID)
{
	if (nodeID.length() > 20)
		return;

	if (address == m_localAddress) {
		if (m_localNodeID == nodeID)
			return;

		postATCommand(ZIGBEE_AT_CMD_NI, nodeID.toAscii());
		m_newLocalNodeID = nodeID;
	}
	else if (m_zbStats.contains(address)) {
		if (m_zbStats[address]->m_nodeID == nodeID)
			return;

		postRemoteATCommand(address, ZIGBEE_AT_CMD_NI, nodeID.toAscii());
		m_zbStats[address]->m_newNodeID = nodeID;
	}
}

void ZigbeeController::sendData(quint64 address, QByteArray data)
{
	ZigbeeStats *stats;

	quint8 nextFrameID = getNextFrameID();

	m_statsMutex.lock();
	if (m_zbStats.contains(address)) {
		stats = m_zbStats.value(address);
		stats->m_lastFrameID = nextFrameID;
	}
	else {
		stats = new ZigbeeStats(address, nextFrameID);
		m_zbStats.insert(address, stats);
	}
	m_statsMutex.unlock();

	QByteArray packet;
	int len = 14 + data.length();

	packet.append(ZIGBEE_START_DELIM);
	putU16(&packet, len);
	packet.append(ZIGBEE_FT_TRANSMIT_REQUEST);
	packet.append(nextFrameID);
	putU64(&packet, address);
	putU16(&packet, stats->m_netAddress);
	packet.append((char)0x00);
	packet.append((char)0x00);
	packet.append(data);

	quint8 chksum = checksum(packet, packet.length() - 3);
	packet.append(chksum);

	m_pendingFrames[nextFrameID] = address;
	m_lastFrameID = nextFrameID;

	m_txMutex.lock();

	if (m_txQ.count() > 50)
		m_txQ.dequeue();

	m_txQ.enqueue(packet);

	m_txMutex.unlock();
}

void ZigbeeController::run()
{
	while (!m_stop) {
		doWrites();

		if (m_nodeDiscoverWait > 0) {
			m_nodeDiscoverWait--;

			if (m_nodeDiscoverWait == 0)
				doNodeDiscoverResponse();
		}

		msleep(ZIGBEE_CONTROLLER_BACKGROUND_INTERVAL);
	}
}

void ZigbeeController::doWrites()
{
	QByteArray data;

	m_txMutex.lock();

	if (m_txQ.count() > 0)
		data = m_txQ.dequeue();

	m_txMutex.unlock();

	if (data.length() == 0 || data.length() > 0xffff)
		return;

	if (m_debugDump)
		debugDump("TX Request", data);

	m_port->write(data);

	m_localTxCount++;
}

void ZigbeeController::debugDump(const char *prompt, QByteArray data)
{
	char buff[512];
	char temp[8];

	memset(buff, 0, sizeof(buff));
	memset(temp, 0, sizeof(temp));

	for (int i = 0; i < data.length(); i++) {
		quint8 c = data.at(i);
		sprintf(temp, "%02X ", 0xff & c);
		strcat(buff, temp);
	}

	if (prompt && *prompt)
		qDebug("DEBUG - %s: %s", prompt, buff);
	else
		qDebug("DEBUG - : %s", buff);
}

void ZigbeeController::readyRead()
{
	QMutexLocker lock(&m_rxMutex);
	int i;

	QByteArray rawData = m_port->readAll();

	if (rawData.length() == 0)
		return;

	m_rxBuffer.append(rawData);

	for (i = 0; i < m_rxBuffer.length(); i++) {
		if (m_rxBuffer.at(i) == ZIGBEE_START_DELIM)
			break;
	}

	if (i == m_rxBuffer.length()) {
		// all garbage, we missed the start delim
		m_rxBuffer.clear();
		return;
	}

	// align for simplicity, not too efficient ;-)
	if (i > 0)
		m_rxBuffer.remove(0, i);

	// not even enough to check the frame length field
	if (m_rxBuffer.length() < 3)
		return;

	quint16 frameLen = getU16(m_rxBuffer, 1);

	// not enough yet
	if (m_rxBuffer.length() < frameLen + 4)
		return;

	m_localRxCount++;

	quint8 cksum = checksum(m_rxBuffer, frameLen);

	quint8 packetCksum = 0xff & m_rxBuffer.at(3 + frameLen);

	if (cksum != packetCksum) {
		qDebug("Bad checksum");
		m_rxBuffer.remove(0, frameLen + 4);
		return;
	}

	quint8 frameType = 0xff & m_rxBuffer.at(3);

	switch (frameType) {
	case ZIGBEE_FT_AT_COMMAND_RESPONSE:
		handleATCommandResponse(m_rxBuffer, frameLen + 4);
		break;

	case ZIGBEE_FT_TRANSMIT_STATUS:
		handleTransmitStatus(m_rxBuffer);
		break;

	case ZIGBEE_FT_RECEIVE_PACKET:
		handleReceivePacket(m_rxBuffer, frameLen + 4);
		break;

	case ZIGBEE_FT_EXPLICIT_RX_IND:
		handleExplicitRxPacket(m_rxBuffer, frameLen + 4);
		break;

	case ZIGBEE_FT_REMOTE_COMMAND_RESPONSE:
		handleRemoteATCommandResponse(m_rxBuffer, frameLen + 4);
		break;

	default:
		qDebug("Unhandled frameType 0x%02X", frameType);
		debugDump("RX", m_rxBuffer);
		break;
	}

	m_rxBuffer.remove(0, frameLen + 4);
}

// We want the 16-bit address for future use
void ZigbeeController::handleTransmitStatus(QByteArray packet)
{
	QMutexLocker lock(&m_statsMutex);

	if (m_debugDump)
		debugDump("TX Status", packet);

	quint8 frameId = 0xff & packet.at(4);

	// should never happen
	if (m_pendingFrames[frameId] == 0)
		return;

	// should never happen
	if (!m_zbStats.contains(m_pendingFrames[frameId]))
		return;

	ZigbeeStats *stats = m_zbStats.value(m_pendingFrames[frameId]);

	if (!stats->m_netAddress || stats->m_netAddress == ZIGBEE_BROADCAST_ADDRESS)
		stats->m_netAddress = getU16(packet, 5);

	// successful transmissions
	stats->m_txCount++;

	// free the pendingFrames slot
	m_pendingFrames[frameId] = 0;
}

// Issue a few AT commands to the local radio by putting
// the requests on the txQ.
void ZigbeeController::queryLocalRadio()
{
	m_localAddress = 0;
	m_panID = 0;
	m_localNodeID.clear();

	postATCommand(ZIGBEE_AT_CMD_SH);
	postATCommand(ZIGBEE_AT_CMD_SL);
	postATCommand(ZIGBEE_AT_CMD_ID);
	postATCommand(ZIGBEE_AT_CMD_NI);
}

void ZigbeeController::postATCommand(quint16 atcmd)
{
	QByteArray packet;

	packet.append(ZIGBEE_START_DELIM);
	putU16(&packet, 4);
	packet.append(ZIGBEE_FT_AT_COMMAND);
	packet.append((char)getNextFrameID());
	putU16(&packet, atcmd);
	quint8 chksum = checksum(packet, packet.length() - 3);
	packet.append(chksum);

	m_txMutex.lock();
	m_txQ.enqueue(packet);
	m_txMutex.unlock();
}

void ZigbeeController::postATCommand(quint16 atcmd, QByteArray data)
{
	QByteArray packet;

	int len = 4 + data.length();

	packet.append(ZIGBEE_START_DELIM);
	putU16(&packet, len);
	packet.append(ZIGBEE_FT_AT_COMMAND);
	packet.append((char)getNextFrameID());
	putU16(&packet, atcmd);
	packet.append(data);
	quint8 chksum = checksum(packet, packet.length() - 3);
	packet.append(chksum);

	m_txMutex.lock();
	m_txQ.enqueue(packet);
	m_txMutex.unlock();
}

#define ADDRESS_LOW  0x00000000FFFFFFFFULL
#define ADDRESS_HIGH 0xFFFFFFFF00000000ULL

void ZigbeeController::handleATCommandResponse(QByteArray packet, int packetLen)
{
	quint8 status = 0xff & packet.at(7);

	if (status != 0) {
		debugDump("AT cmd response bad status", packet);
		return;
	}

	quint16 cmd = getU16(packet, 5);

	switch (cmd) {
	case ZIGBEE_AT_CMD_SH:
		if (packetLen == 13) {
			m_localAddress = (m_localAddress & ADDRESS_LOW) + ((quint64)getU32(packet, 8) << 32);

			if ((m_localAddress & ADDRESS_LOW) && (m_localAddress & ADDRESS_HIGH))
				emit localRadioAddress(m_localAddress);
		}

		break;

	case ZIGBEE_AT_CMD_SL:
		if (packetLen == 13) {
			m_localAddress = (m_localAddress & ADDRESS_HIGH) + getU32(packet, 8);

			if ((m_localAddress & ADDRESS_LOW) && (m_localAddress & ADDRESS_HIGH))
				emit localRadioAddress(m_localAddress);
		}

		break;

	case ZIGBEE_AT_CMD_ID:
		if (packetLen == 17)
			m_panID = getU64(packet, 8);

		break;

	case ZIGBEE_AT_CMD_ND:
		handleNDResponsePacket(packet, packetLen);
		break;

	case ZIGBEE_AT_CMD_NI:
		parseLocalNIResponse(packet, packetLen);
		break;
	}
}

void ZigbeeController::parseLocalNIResponse(QByteArray packet, int packetLen)
{
	if (m_debugDump)
		debugDump("AT NI", packet);

	// if this is a write response packet there is no data
	if (packetLen == 9) {
		m_localNodeID = m_newLocalNodeID;
	}
	// else this was a read
	else {
		char nodeID[24];

		memset(nodeID, 0, sizeof(nodeID));

		for (int i = 8, j = 0; i < packetLen - 1 && j < 20; i++, j++)
			nodeID[j] = packet.at(i);

		if (strlen(nodeID) > 0 && strcmp(nodeID, " "))
			m_localNodeID = nodeID;
	}
}

void ZigbeeController::postRemoteATCommand(quint64 address, quint16 atcmd, QByteArray data)
{
	QByteArray packet;

	quint16 netAddress = m_zbStats[address]->m_netAddress;

	int len = 15 + data.length();

	packet.append(ZIGBEE_START_DELIM);
	putU16(&packet, len);

	packet.append(ZIGBEE_FT_REMOTE_AT_COMMAND);
	packet.append((char)getNextFrameID());
	putU64(&packet, address);
	putU16(&packet, netAddress);
	packet.append(0x02); // remote cmd options, apply and ACK
	putU16(&packet, atcmd);

	if (data.length() > 0)
		packet.append(data);

	quint8 chksum = checksum(packet, packet.length() - 3);
	packet.append(chksum);

	m_txMutex.lock();
	m_txQ.enqueue(packet);
	m_txMutex.unlock();
}

void ZigbeeController::handleRemoteATCommandResponse(QByteArray packet, int packetLen)
{
	if (packetLen < 19) {
		debugDump("Remote AT cmd response too short", packet);
		return;
	}

	quint8 status = 0xff & packet.at(17);

	if (status != 0) {
		debugDump("Remote AT cmd response bad status", packet);
		return;
	}

	quint64 address = getU64(packet, 5);
	quint16 netAddress = getU16(packet, 13);
	quint16 cmd = getU16(packet, 15);

	switch (cmd) {
	case ZIGBEE_AT_CMD_NI:
		if (m_debugDump)
			debugDump("Remote AT NI", packet);

		// this should always be the case
		if (m_zbStats.contains(address)) {
			m_zbStats[address]->m_netAddress = netAddress;
			m_zbStats[address]->m_nodeID = m_zbStats[address]->m_newNodeID;
		}

		break;

	default:
		debugDump("Unhandled remote AT cmd response", packet);
		break;
	}
}

void ZigbeeController::handleNDResponsePacket(QByteArray packet, int packetLen)
{
	if (m_debugDump)
		debugDump("ND Response", packet);

	ZigbeeStats *newZB = parseNDResponse(packet, packetLen);

	if (!newZB) {
		debugDump("Bad AT ND response", packet);
		return;
	}

	if (m_zbStats.contains(newZB->m_address)) {
		ZigbeeStats *zb = m_zbStats.value(newZB->m_address);

        if (m_debugDump) {
            if (newZB->m_netAddress != zb->m_netAddress) {
                debugDump("New netAddress", packet);
            }

            if (newZB->m_nodeID != zb->m_nodeID) {
                debugDump("New nodeID", packet);
            }
        }

		zb->updateFromNodeDiscovery(newZB);
		delete newZB;
	}
	else {
		m_zbStats.insert(newZB->m_address, newZB);
	}
}

ZigbeeStats* ZigbeeController::parseNDResponse(QByteArray packet, int packetLen)
{
	int pos;

	if (packetLen < 29)
		return NULL;

	ZigbeeStats *zb = new ZigbeeStats();

	if (!zb)
		return NULL;

	zb->m_netAddress = getU16(packet, 8); // MY
	zb->m_address = getU64(packet, 10); // SH-SL
	
	for (pos = 18; pos < packetLen; pos++) {
		unsigned char c = 0xff & packet.at(pos);

		if (c == 0)
			break;

		zb->m_nodeID.append(c);
	}

	if (zb->m_nodeID == " ")
		zb->m_nodeID.clear();

	// need at least 9 more bytes, 8 + chksum
	if (packetLen - pos < 9) {
		delete zb;
		return NULL;
	}
	
	pos++;
	
	zb->m_parentNetAddress = getU16(packet, pos);
	pos += 2;

	zb->m_deviceType = 0xff & packet.at(pos);
	
	// skip the status field
	pos += 2;

	zb->m_profileID = getU16(packet, pos);
	pos += 2;

	zb->m_mfgID = getU16(packet, pos);

	zb->m_nodeDiscoverSequence = m_nodeDiscoverSequence;

	return zb;
}

void ZigbeeController::handleReceivePacket(QByteArray packet, int packetLen)
{
	if (m_debugDump)
		debugDump("RX", packet);

	quint64 address = getU64(packet, 4);

	m_statsMutex.lock();

	if (m_zbStats.contains(address)) {
		ZigbeeStats *stats = m_zbStats.value(address);
		stats->m_lastReceiveOptions = 0xff & packet.at(14);
		stats->m_rxCount++;
	}
	else {
		quint16 netAddress = getU16(packet, 12);
		ZigbeeStats *stats = new ZigbeeStats(address, 0, netAddress, m_nodeDiscoverSequence);
		stats->m_lastReceiveOptions = 0xff & packet.at(14);
		stats->m_rxCount = 1;
		m_zbStats.insert(address, stats);
	}

	m_statsMutex.unlock();

	QByteArray data = packet.mid(15, packetLen - 16);

	emit receiveData(address, data);
}

// Not doing anything with the extra RX Explicit fields for now
void ZigbeeController::handleExplicitRxPacket(QByteArray packet, int packetLen)
{
	if (m_debugDump)
		debugDump("RX", packet);

	quint64 address = getU64(packet, 4);

	m_statsMutex.lock();

	if (m_zbStats.contains(address)) {
		ZigbeeStats *stats = m_zbStats.value(address);
		stats->m_lastReceiveOptions = 0xff & packet.at(20);
		stats->m_rxCount++;
	}
	else {
		quint16 netAddress = getU16(packet, 12);
		ZigbeeStats *stats = new ZigbeeStats(address, 0, netAddress, m_nodeDiscoverSequence);
		stats->m_lastReceiveOptions = 0xff & packet.at(20);
		stats->m_rxCount = 1;
		m_zbStats.insert(address, stats);
	}

	m_statsMutex.unlock();

	QByteArray data = packet.mid(21, packetLen - 22);

	emit receiveData(address, data);
}

quint8 ZigbeeController::getNextFrameID()
{
	m_lastFrameID++;

	if (m_lastFrameID == 0)
		m_lastFrameID = 1;

	return m_lastFrameID;
}

// sum of frameLen bytes mod 0xff subtracted from 0xff
// frame data starts at byte 3, after start delim and length fields
quint8 ZigbeeController::checksum(QByteArray data, int frameLen)
{
	quint8 checksum = 0;

	for (int i = 0; i < frameLen; i++)
		checksum += 0xff & data.at(i + 3);

	return 0xff - checksum;
}

QList<ZigbeeStats> ZigbeeController::stats()
{
	QMutexLocker lock(&m_statsMutex);
	QList<ZigbeeStats> list;
	ZigbeeStats zb;

	list.append(localRadio());

	QMapIterator<quint64, ZigbeeStats *> i(m_zbStats);

	while (i.hasNext()) {
		i.next();
		zb = *(i.value());
		list.append(zb);
	}

	return list;
}

void ZigbeeController::doNodeDiscoverResponse()
{
	QMutexLocker lock(&m_statsMutex);
	QList<ZigbeeStats> list;
	ZigbeeStats zb;

	list.append(localRadio());

	QMapIterator<quint64, ZigbeeStats *> i(m_zbStats);

	while (i.hasNext()) {
		i.next();

		zb = *(i.value());

		if (zb.m_nodeDiscoverSequence == m_nodeDiscoverSequence)
			list.append(zb);
	}

	emit nodeDiscoverResponse(list);
}

ZigbeeStats ZigbeeController::localRadio()
{
	ZigbeeStats zb;

	zb.m_address = m_localAddress;

	zb.m_deviceType = ZIGBEE_DEVICE_TYPE_LOCAL;
	zb.m_netAddress = 0x00;

	zb.m_nodeID = m_localNodeID;
	zb.m_panID = m_panID;

	zb.m_txCount = m_localTxCount;
	zb.m_rxCount = m_localRxCount;

	return zb;
}
