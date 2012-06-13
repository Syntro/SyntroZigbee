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

#ifndef ZIGBEETESTNODE_H
#define ZIGBEETESTNODE_H

#include <QtGui/QMainWindow>
#include <qlabel.h>
#include <qpushbutton.h>

#include "ZigbeeController.h"

#include "ui_zigbeetestnode.h"

#define FULLSCREEN_MODE "fullScreenMode"

class ZigbeeTestNode : public QMainWindow
{
	Q_OBJECT

public:
	ZigbeeTestNode(QSettings *settings, QWidget *parent = 0, Qt::WFlags flags = 0);
	~ZigbeeTestNode();

public slots:
	void onScan();
	void onSend();
	void onClear();
	void onConnect();
	void onDisconnect();
	void onConfigure();
	void receiveData(quint64 address, QByteArray data);
	void localRadioAddress(quint64 address);
	void nodeDiscoverResponse(QList<ZigbeeStats>);

signals:
	void requestNodeDiscover();
	void sendData(quint64 address, QByteArray data);

protected:
	void closeEvent(QCloseEvent *);
	void timerEvent(QTimerEvent *);

private:
	quint64 getCurrentRadio();
	QByteArray convertHex(QString s);
	void updateRadioListDisplay();
	void updateRxFields(quint64 address, QByteArray *data);
	void initStatusBar();
	void updateStatusBar();
	void saveWindowState();
	void restoreWindowState();

	Ui::ZigbeeTestNodeClass ui;

	QSettings *m_settings;
	ZigbeeController *m_controller;

	QMutex m_rxQMutex;
	QQueue<quint64> m_rxAddressQ;
	QQueue<QByteArray> m_rxQ;

	int m_refreshTimer;
	quint64 m_localAddress;

	QMutex m_radioListMutex;
	QMap<quint64, ZigbeeStats> m_radioList;
	bool m_newRadioList;

	QLabel *m_localAddressLabel;
	QLabel *m_portName;
	QLabel *m_portSpeed;

	int m_scanDelay;
};

#endif // ZIGBEETESTNODE_H
