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

#ifndef SYNTROZIGBEEDEMO_H
#define SYNTROZIGBEEDEMO_H

#include <QtGui/QMainWindow>
#include <qsignalmapper.h>
#include <qlabel.h>
#include <qpushbutton.h>

#include "ZigbeeClient.h"
#include "ZigbeeStats.h"

#include "ui_syntrozigbeedemo.h"

#define FULLSCREEN_MODE "fullScreenMode"

class SyntroZigbeeDemo : public QMainWindow
{
	Q_OBJECT

public:
	SyntroZigbeeDemo(QSettings *settings, QWidget *parent = 0, Qt::WFlags flags = 0);

public slots:
	void onScan();
	void onSend();
	void onClear();
	void receiveData(quint64 address, QByteArray data);
	void receiveRadioList(QList<ZigbeeStats>);

protected:
	void closeEvent(QCloseEvent *);
	void timerEvent(QTimerEvent *);

private:
	quint64 getCurrentRadio();
	QByteArray convertHex(QString s);
	void updateRadioListDisplay();
	void updateRxFields(quint64 address, QByteArray *data);
	void initStatusBar();
	void saveWindowState();
	void restoreWindowState();

	Ui::SyntroZigbeeDemoClass ui;

	QSettings *m_settings;
	ZigbeeClient *m_client;

	QMutex m_radioListMutex;
	QMap<quint64, ZigbeeStats> m_radioList;
	bool m_newRadioList;

	QMutex m_rxQMutex;
	QQueue<QByteArray> m_rxQ;
	QQueue<quint64> m_rxAddressQ;

	int m_refreshTimer;
	int m_statusTimer;
	int m_scanDelay;
	QLabel *m_syntroStatus;
};

#endif // SYNTROZIGBEEDEMO_H
