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

#ifndef ZIGBEE_CONTROLLER
#define ZIGBEE_CONTROLLER

#include <qthread.h>
#include <qbytearray.h>
#include <qmutex.h>
#include <qqueue.h>
#include <qsettings.h>
#include <qhash.h>
#include <qstringlist.h>

#include "qextserialport.h"
#include "ZigbeeStats.h"
#include "ZigbeeCommon.h"

// limited by the one-byte frame id field
#define MAX_PENDING_FRAMES 256


class ZigbeeController : public QThread {
	Q_OBJECT

public:
	ZigbeeController();
	~ZigbeeController();

	bool openDevice(QSettings *settings);
	void closeDevice();
	void startRunLoop();
	void stopRunLoop();
	bool isOpen();
	QList<ZigbeeStats> stats();
	ZigbeeStats localRadio();

public slots:
	void readyRead();
	void sendData(quint64 address, QByteArray data);
	void requestNodeDiscover();
	void requestNodeIDChange(quint64 address, QString nodeID);

signals:
	void receiveData(quint64 address, QByteArray data);
	void localRadioAddress(quint64 address);
	void nodeDiscoverResponse(QList<ZigbeeStats>);

protected:
	void run();

private:
	void doWrites();
	quint8 checksum(QByteArray data, int frameLen);
	quint8 getNextFrameID();
	void handleATCommandResponse(QByteArray packet, int packetLen);
	void parseLocalNIResponse(QByteArray packet, int packetLen);
	void handleTransmitStatus(QByteArray packet);
	void handleReceivePacket(QByteArray packet, int packetLen);
	void handleExplicitRxPacket(QByteArray packet, int packetLen);
	void queryLocalRadio();
	void postATCommand(quint16 atcmd);
	void postATCommand(quint16 atcmd, QByteArray data);
	void postRemoteATCommand(quint64 address, quint16 atcmd, QByteArray data);
	void handleRemoteATCommandResponse(QByteArray packet, int packetLen);
	void handleNDResponsePacket(QByteArray packet, int packetLen);
	ZigbeeStats *parseNDResponse(QByteArray packet, int packetLen);
	void doNodeDiscoverResponse();
	void debugDump(const char *prompt, QByteArray data);

	volatile bool m_stop;
	bool m_debugDump;
	quint64 m_localAddress;
	QString m_localNodeID;
	QString m_newLocalNodeID;
	quint32 m_localTxCount;
	quint32 m_localRxCount;
	quint64 m_panID;
	quint32 m_nodeDiscoverWait;
	quint32 m_nodeDiscoverSequence;

	QMutex m_txMutex;
	QQueue<QByteArray> m_txQ;

	QextSerialPort *m_port;

	QMutex m_statsMutex;
	QMap<quint64, ZigbeeStats *> m_zbStats;

	quint64 m_pendingFrames[MAX_PENDING_FRAMES];
	quint8 m_lastFrameID;

	QMutex m_rxMutex;
	QByteArray m_rxBuffer;
};

#endif // ZIGBEE_CONTROLLER
