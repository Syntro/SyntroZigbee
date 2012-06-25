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

#ifndef SYNTROZIGBEEGATEWAY_H
#define SYNTROZIGBEEGATEWAY_H

#include <QtGui/QMainWindow>
#include <qsignalmapper.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include "ZigbeeGWClient.h"
#include "ZigbeeController.h"

#include "ui_syntrozigbeegateway.h"

#define FULLSCREEN_MODE "fullScreenMode"

class SyntroZigbeeGateway : public QMainWindow
{
	Q_OBJECT

public:
	SyntroZigbeeGateway(QSettings *settings, QWidget *parent = 0, Qt::WFlags flags = 0);
	~SyntroZigbeeGateway();

public slots:
	void onAbout();
	void onConnect();
	void onDisconnect();
	void onConfigure();
	void localRadioAddress(quint64 address);
	void nodeDiscoverResponse(QList<ZigbeeStats>);

signals:
	void requestNodeDiscover();
	void requestNodeIDChange(quint64 address, QString nodeID);

protected:
	void closeEvent(QCloseEvent *);
	void timerEvent(QTimerEvent *);

private:
	void refreshDisplay();
	void populateRow(int row, ZigbeeStats *stat);
	void loadLocalAddress();
	void loadNodeIDList();
	void initStatusBar();
	void initStatTable();
	void updateStatusBar();
	void saveWindowState();
	void restoreWindowState();

	Ui::SyntroZigbeeGatewayClass ui;

	QSettings *m_settings;
	ZigbeeGWClient *m_client;
	ZigbeeController *m_controller;

	int m_syntroStatusTimer;
	int m_refreshTimer;

	quint64 m_localAddress;

	QLabel *m_syntroStatus;
	QLabel *m_localAddressLabel;
	QLabel *m_portName;
	QLabel *m_portSpeed;
	bool m_portOpen;

	QHash<quint64, QString> m_nodeIDs;
};

#endif // SYNTROZIGBEEGATEWAY_H
