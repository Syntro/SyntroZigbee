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

#ifndef MOTIONSENSORVIEWER_H
#define MOTIONSENSORVIEWER_H

#include <QtGui/QMainWindow>
#include <qlabel.h>

#include "ui_motionsensorviewer.h"

#include "ZigbeeClient.h"
#include "ZigbeeStats.h"

#define MOTION_SENSORS	   "motionSensors"
#define SENSOR_ZB_ADDRESS  "address"
#define SENSOR_LOCATION    "location"

class MotionSensorViewer : public QMainWindow
{
	Q_OBJECT

public:
	MotionSensorViewer(QSettings *settings, QWidget *parent = 0, Qt::WFlags flags = 0);

public slots:
	void receiveData(quint64 address, QByteArray data);

protected:
	void closeEvent(QCloseEvent *);
	void timerEvent(QTimerEvent *);

private:
	void updateSensorState(quint32 sensor, bool state);
	void loadSensorList();
	void layoutWindow();
	void initStatusBar();
	void saveWindowState();
	void restoreWindowState();

	Ui::MotionSensorViewerClass ui;

	QSettings *m_settings;
	ZigbeeClient *m_client;

	QMutex m_rxQMutex;
	QQueue<quint32> m_rxSensorStateQ;
	
	QList<quint64> m_addresses;
	QStringList m_locations;

	QList<QLabel *> m_sensorDisplay;

	int m_statusTimer;
	int m_refreshTimer;
	QLabel *m_syntroStatus;
};

#endif // MOTIONSENSORVIEWER_H
