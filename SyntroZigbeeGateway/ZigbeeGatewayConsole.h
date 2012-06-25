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

#ifndef ZIGBEEGATEWAYCONSOLE_H
#define ZIGBEEGATEWAYCONSOLE_H

#include <QThread>
#include <QSettings>

#include "ZigbeeGWClient.h"
#include "ZigbeeController.h"

class ZigbeeGatewayConsole : public QThread
{
	Q_OBJECT

public:
	ZigbeeGatewayConsole(QSettings *settings, QObject *parent);
	~ZigbeeGatewayConsole() {}

public slots:
	void aboutToQuit();
	void localRadioAddress(quint64 address);
	void nodeDiscoverResponse(QList<ZigbeeStats>);

signals:
	void requestNodeDiscover();
	void requestNodeIDChange(quint64 address, QString nodeID);

protected:
	void run();

private:
	void loadNodeIDList();
	void showStats();
	void showHelp();

	QSettings *m_settings;
	ZigbeeGWClient *m_client;
	ZigbeeController *m_controller;
	quint64 m_localAddress;

	QHash<quint64, QString> m_nodeIDs;
	QString m_portName;
	int m_portSpeed;
};

#endif // ZIGBEEGATEWAYCONSOLE_H
