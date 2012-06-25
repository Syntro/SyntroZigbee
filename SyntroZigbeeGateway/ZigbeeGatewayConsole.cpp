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

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#endif

#include "ZigbeeGatewayConsole.h"


ZigbeeGatewayConsole::ZigbeeGatewayConsole(QSettings *settings, QObject *parent)
	: QThread(parent), m_settings(settings)
{
	m_controller = NULL;

	m_client = new ZigbeeGWClient(parent, settings);
	m_client->resumeThread();

	m_portName = m_settings->value(ZIGBEE_PORT).toString();
	m_portSpeed = m_settings->value(ZIGBEE_SPEED).toInt();

	m_localAddress = 0;

	loadNodeIDList();

	m_controller = new ZigbeeController();

	if (m_controller->openDevice(m_settings)) {
		if (m_client) {
			connect(m_controller, SIGNAL(receiveData(quint64, QByteArray)),
				m_client, SLOT(receiveData(quint64, QByteArray)), Qt::DirectConnection);

			connect(m_client, SIGNAL(sendData(quint64,QByteArray)),
				m_controller, SLOT(sendData(quint64,QByteArray)), Qt::DirectConnection);

			connect(m_controller, SIGNAL(localRadioAddress(quint64)), 
				m_client, SLOT(localRadioAddress(quint64)));

			connect(m_client, SIGNAL(requestNodeDiscover()), 
				m_controller, SLOT(requestNodeDiscover()));

			connect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
				m_client, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)), Qt::DirectConnection);
		}

		connect(m_controller, SIGNAL(localRadioAddress(quint64)), 
			this, SLOT(localRadioAddress(quint64)));

		connect(this, SIGNAL(requestNodeDiscover()), 
			m_controller, SLOT(requestNodeDiscover()));

		connect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
				this, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)), Qt::DirectConnection);

		connect(this, SIGNAL(requestNodeIDChange(quint64, QString)),
			m_controller, SLOT(requestNodeIDChange(quint64, QString)));

		m_controller->startRunLoop();

		printf("\nConnected to %s at %d\n", qPrintable(m_portName), m_portSpeed);

		emit requestNodeDiscover();
	}
	else {
		delete m_controller;
		m_controller = NULL;
	}

	start();
}

void ZigbeeGatewayConsole::aboutToQuit()
{
    if (m_client) {
        if (m_controller) {
            connect(m_controller, SIGNAL(receiveData(quint64, QByteArray)),
                m_client, SLOT(receiveData(quint64, QByteArray)), Qt::DirectConnection);

            connect(m_client, SIGNAL(sendData(quint64,QByteArray)),
                m_controller, SLOT(sendData(quint64,QByteArray)), Qt::DirectConnection);

            connect(m_controller, SIGNAL(localRadioAddress(quint64)),
                m_client, SLOT(localRadioAddress(quint64)));

            connect(m_client, SIGNAL(requestNodeDiscover()),
                m_controller, SLOT(requestNodeDiscover()));

            connect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
                m_client, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)), Qt::DirectConnection);
        }

        m_client->exitThread();
    }

	if (m_controller) {
		m_controller->closeDevice();
		delete m_controller;
		m_controller = NULL;
	}

    for (int i = 0; i < 5; i++) {
        if (wait(1000))
            break;

        printf("Waiting for Syntro console thread to exit...\n");
    }
}

void ZigbeeGatewayConsole::loadNodeIDList()
{
	bool ok = false;

	int count = m_settings->beginReadArray(ZIGBEE_DEVICES);

	for (int i = 0; i < count; i++) {
		m_settings->setArrayIndex(i);

		if (!m_settings->contains(ZIGBEE_NODEID))
			continue;

		QString s = m_settings->value(ZIGBEE_ADDRESS).toString();

		quint64 address = s.toULongLong(&ok, 16);

		if (ok) {
			QString nodeID = m_settings->value(ZIGBEE_NODEID).toString();

			if (nodeID.length() > 0) {
				nodeID.truncate(20);
				m_nodeIDs.insert(address, nodeID);
			}
		}
	}

	m_settings->endArray();
}

void ZigbeeGatewayConsole::nodeDiscoverResponse(QList<ZigbeeStats> list)
{
	ZigbeeStats zb;

	for (int i = 0; i < list.count(); i++) {
		zb = list.at(i);

		if (m_nodeIDs.contains(zb.m_address)) {
			if (m_nodeIDs[zb.m_address] != zb.m_nodeID)
				emit requestNodeIDChange(zb.m_address, m_nodeIDs[zb.m_address]);
		}
		else if (zb.m_nodeID.length() > 0) {
			m_nodeIDs.insert(zb.m_address, zb.m_nodeID);
		}
	}
}

void ZigbeeGatewayConsole::localRadioAddress(quint64 address)
{
	m_localAddress = address;
}

void ZigbeeGatewayConsole::showStats()
{
	QList<ZigbeeStats> list = m_controller->stats();

	if (m_localAddress != 0)
		printf("\nLocal radio: %16llx\n\n", m_localAddress);

	printf("         Address               Node ID  TX Count  RX Count\n");
	printf("----------------  --------------------  --------  --------\n");

	for (int i = 0; i < list.count(); i++) {
		ZigbeeStats zb = list.at(i);

		if (zb.m_deviceType & ZIGBEE_DEVICE_TYPE_LOCAL) {
			printf("%16llx* %20s  %8d  %8d\n",
				zb.m_address,
				qPrintable(zb.m_nodeID),
				zb.m_txCount,
				zb.m_rxCount);
		}
		else {
			printf("%16llx  %20s  %8d  %8d\n",
				zb.m_address,
				qPrintable(zb.m_nodeID),
				zb.m_txCount,
				zb.m_rxCount);
		}
	}
}

void ZigbeeGatewayConsole::showHelp()
{
	printf("\nOptions:\n\n");
	printf("  S - Show radio stats\n");
    printf("  D - Run node discover query\n");
	printf("  H - Show this help\n");
	printf("  X - Exit\n");
}

void ZigbeeGatewayConsole::run()
{
	bool timeToQuit = false;

#ifndef WIN32
	struct termios tty;

	tcgetattr(fileno(stdout), &tty);
	tty.c_lflag &= ~ICANON;
	tcsetattr(fileno(stdout), TCSANOW, &tty);
#endif

	if (!m_controller || !m_client)
		timeToQuit = true;

	while (!timeToQuit) {
		printf("\nOption: ");

		switch (toupper(getchar())) {
		case 'S':
			printf("\n");
			showStats();
			printf("\n");
			break;

        case 'D':
            emit requestNodeDiscover();
            break;

		case 'H':
			printf("\n");
			showHelp();
			printf("\n");
			break;

		case 'X':			
			timeToQuit = true;
			break;

		default:
			break;
		}		
	}

	printf("\nExiting\n");
	((QCoreApplication *)parent())->exit();	
}

