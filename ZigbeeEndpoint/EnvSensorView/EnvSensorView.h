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

#ifndef ENVSENSORVIEW_H
#define ENVSENSORVIEW_H

#include <QtGui/QMainWindow>
#include <qlabel.h>

#include "ui_envsensorview.h"

#include "ZigbeeClient.h"

#define SENSOR_RADIOS	   "SensorRadios"
#define SENSOR_ZB_ADDRESS  "address"
#define SENSOR_LOCATION    "location"

class EnvSensorView : public QMainWindow
{
	Q_OBJECT

public:
	EnvSensorView(QSettings *settings, QWidget *parent = 0, Qt::WFlags flags = 0);

public slots:
	void receiveData(quint64 address, QByteArray data);

protected:
	void closeEvent(QCloseEvent *);
	void timerEvent(QTimerEvent *);

private:
	void updateMotionState(quint32 sensor, bool state);
	void updateCurrentTemp(quint32 sensor, quint32 rawTemp);
	void updateCurrentLight(quint32 sensor, quint32 light);
	void loadSensorList();
	void layoutWindow();
	void initStatusBar();
	void saveWindowState();
	void restoreWindowState();

	Ui::EnvSensorViewClass ui;

	QSettings *m_settings;
	ZigbeeClient *m_client;

	QMutex m_rxQMutex;
	QQueue<quint32> m_rxMotionStateQ;
	QQueue<quint32> m_rxCurrentTempQ;
	QQueue<quint32> m_rxCurrentLightQ;	

	QList<quint64> m_addresses;
	QStringList m_locations;

	QList<QLabel *> m_temperatureWidgets;
	QList<QLabel *> m_lightWidgets;
	QList<QLabel *> m_motionWidgets;
	

	int m_statusTimer;
	int m_refreshTimer;
	QLabel *m_syntroStatus;
};

#endif // ENVSENSORVIEW_H
