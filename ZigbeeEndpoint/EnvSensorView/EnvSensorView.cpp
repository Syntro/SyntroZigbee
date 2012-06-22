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

#include "qformlayout.h"

#include "EnvSensorView.h"
#include "ZigbeeUtils.h"

#define SENSOR_TYPE_MOTION          0x01
#define SENSOR_TYPE_TEMPERATURE     0x02
#define SENSOR_TYPE_LIGHT           0x04


EnvSensorView::EnvSensorView(QSettings *settings, QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), m_settings(settings)
{
	ui.setupUi(this);
	initStatusBar();
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));

	loadSensorList();
	layoutWindow();

	m_client = new ZigbeeClient(this, settings);

	connect(m_client, SIGNAL(receiveData(quint64,QByteArray)),
			this, SLOT(receiveData(quint64,QByteArray)), Qt::DirectConnection);

	m_client->resumeThread();

	m_refreshTimer = startTimer(100);
	m_statusTimer = startTimer(3000);

	restoreWindowState();
}

void EnvSensorView::loadSensorList()
{
	bool ok;
	quint64 address;

	int count = m_settings->beginReadArray(SENSOR_RADIOS);
	
	for (int i = 0; i < count; i++) {
		m_settings->setArrayIndex(i);
		QString s = m_settings->value(SENSOR_ZB_ADDRESS).toString();

		address = s.toULongLong(&ok, 16);

		if (ok && !m_addresses.contains(address)) {
			s = m_settings->value(SENSOR_LOCATION).toString();

			if (s.length() < 1)
				s = QString("Sensor %1").arg(m_addresses.count());

			m_addresses.append(address);
			m_locations.append(s);
		}
	}		

	m_settings->endArray();
}

// Pack the data into a uint32, the high byte is the sensor index
// the lower bytes are the data, depends on the type
void EnvSensorView::receiveData(quint64 address, QByteArray data)
{
	quint32 value;
	quint32 sensor = 256;
	
	for (int i = 0; i < m_addresses.count(); i++) {
		if (m_addresses.at(i) == address) {
			sensor = i;
			break;
		}
	}

	// if we didn't find or too many sensors ignore it
	if (sensor >= 256)
		return;


	// for compatibility, one byte of data is a motion sensor
	if (data.length() == 1) {
		if (data.at(0) != 0)
			sensor |= 1;

		m_rxQMutex.lock();
		m_rxMotionStateQ.append(value);
		m_rxQMutex.unlock();

		return;
	}

	int pos = 0;

	while (pos < data.length()) {
		switch (data.at(pos)) {
		case SENSOR_TYPE_MOTION:
			if ((data.length() < (pos + 3)) || data.at(pos + 1) != 1)
				return;

			value = sensor << 24;

			if (data.at(pos + 2) != 0)
				value |= 1;
				
			m_rxQMutex.lock();
			m_rxMotionStateQ.enqueue(value);
			m_rxQMutex.unlock();
			pos += 3;
			break;

		case SENSOR_TYPE_TEMPERATURE:
			if ((data.length() < (pos + 4)) || data.at(pos + 1) != 2)
				return;

			value = getU16(data, pos + 2);
			value |= (sensor << 24);

			m_rxQMutex.lock();
			m_rxCurrentTempQ.enqueue(value);
			m_rxQMutex.unlock();
			pos += 4;
			break;

		case SENSOR_TYPE_LIGHT:
			if ((data.length() < (pos + 4)) || data.at(pos + 1) != 2)
				return;

			value = getU16(data, pos + 2);
			value |= (sensor << 24);

			m_rxQMutex.lock();
			m_rxCurrentLightQ.enqueue(value);
			m_rxQMutex.unlock();
			pos += 4;
			break;

		default:
			return;
		}
	}	
}

void EnvSensorView::timerEvent(QTimerEvent *timerEvent)
{
	bool done;
	quint32 packedVal;
	
	if (timerEvent->timerId() == m_statusTimer) {
		m_syntroStatus->setText(m_client->getLinkState());
	}
	else {
		done = false;
		while (!done) {
			m_rxQMutex.lock();
			if (!m_rxMotionStateQ.empty()) {
				packedVal = m_rxMotionStateQ.dequeue();
			}
			else {
				done = true;
			}
			m_rxQMutex.unlock();

			if (!done)
				updateMotionState(0xff & (packedVal >> 24), 0x01 & packedVal);
		}

		done = false;
		while (!done) {
			m_rxQMutex.lock();
			if (!m_rxCurrentTempQ.empty()) {
				packedVal = m_rxCurrentTempQ.dequeue();
			}
			else {
				done = true;
			}
			m_rxQMutex.unlock();

			if (!done)
				updateCurrentTemp(0xff & (packedVal >> 24), 0xffff & packedVal);
		}	

		done = false;
		while (!done) {
			m_rxQMutex.lock();
			if (!m_rxCurrentLightQ.empty()) {
				packedVal = m_rxCurrentLightQ.dequeue();
			}
			else {
				done = true;
			}
			m_rxQMutex.unlock();

			if (!done)
				updateCurrentLight(0xff & (packedVal >> 24), 0xffff & packedVal);
		}	
	}
}

void EnvSensorView::updateMotionState(quint32 sensor, bool state)
{
	if ((int)sensor < m_motionWidgets.count()) {
		QLabel *motionLabel = m_motionWidgets.at(sensor);

		QPalette pal(motionLabel->palette());
		
		if (state)
			pal.setColor(QPalette::Window, Qt::green);
		else
			pal.setColor(QPalette::Window, Qt::transparent);

		motionLabel->setPalette(pal);		
	}
}

// The temp data comes from a TMP102 in Celsius
// The units are in 0.0625 degree increments
void EnvSensorView::updateCurrentTemp(quint32 sensor, quint32 rawTemp)
{
	double fahrenheit = rawTemp;
	fahrenheit *= (1.8 * 0.0625);
	fahrenheit += 32;

	if ((int)sensor < m_temperatureWidgets.count()) {
		QLabel *temperatureLabel = m_temperatureWidgets.at(sensor);
		temperatureLabel->setText(QString("%1 F").arg(fahrenheit, 0, 'f', 2)); 
	}
}

void EnvSensorView::updateCurrentLight(quint32 sensor, quint32 light)
{
	if ((int) sensor < m_lightWidgets.count()) {
		QLabel *lightLabel = m_lightWidgets.at(sensor);
		lightLabel->setText(QString::number(light));
	}
}

void EnvSensorView::closeEvent(QCloseEvent *)
{
	if (m_statusTimer) {
		killTimer(m_statusTimer);
		m_statusTimer = 0;
	}

	if (m_refreshTimer) {
		killTimer(m_refreshTimer);
		m_refreshTimer = 0;
	}

	if (m_client) {
		disconnect(m_client, SIGNAL(receiveData(quint64,QByteArray)),
			this, SLOT(receiveData(quint64,QByteArray)));

		m_client->exitThread();
		delete m_client;
	}

	saveWindowState();
}

#define WIDGET_MIN_HEIGHT 28
#define WIDGET_MIN_WIDTH 28

void EnvSensorView::layoutWindow()
{
	QWidget *centralWidget = new QWidget(this);
	QGridLayout *layout = new QGridLayout(centralWidget);
	layout->setSpacing(6);
	layout->setContentsMargins(11, 11, 11, 11);

	QLabel *location = new QLabel("Location", centralWidget);
	layout->addWidget(location, 0, 0);

	QLabel *temperature = new QLabel("Temperature", centralWidget);
	layout->addWidget(temperature, 0, 1);

	QLabel *light = new QLabel("Light", centralWidget);
	layout->addWidget(light, 0, 2);

	QLabel *motion = new QLabel("Motion", centralWidget);
	layout->addWidget(motion, 0, 3);

	for (int i = 0; i < m_addresses.count(); i++) {
		location = new QLabel(m_locations.at(i), centralWidget);
		location->setMinimumSize(QSize(0, WIDGET_MIN_HEIGHT));
		location->setMaximumSize(QSize(100, WIDGET_MIN_HEIGHT));
		layout->addWidget(location, i + 1, 0);

		temperature = new QLabel("??? F", centralWidget);
		temperature->setMinimumSize(QSize(WIDGET_MIN_WIDTH, WIDGET_MIN_HEIGHT));
		temperature->setMaximumSize(QSize(100, WIDGET_MIN_HEIGHT));
		layout->addWidget(temperature, i + 1, 1);
		m_temperatureWidgets.append(temperature);			

		light = new QLabel("???", centralWidget);
		light->setMinimumSize(QSize(WIDGET_MIN_WIDTH, WIDGET_MIN_HEIGHT));
		light->setMaximumSize(QSize(100, WIDGET_MIN_HEIGHT));
		layout->addWidget(light, i + 1, 2);
		m_lightWidgets.append(light);			

		motion = new QLabel(centralWidget);
		motion->setMinimumSize(QSize(WIDGET_MIN_WIDTH, WIDGET_MIN_HEIGHT));
		motion->setMaximumSize(QSize(WIDGET_MIN_WIDTH, WIDGET_MIN_HEIGHT));
		motion->setAutoFillBackground(true);
		motion->setFrameShape(QFrame::Panel);
		motion->setFrameShadow(QFrame::Raised);
		layout->addWidget(motion, i + 1, 3);
		m_motionWidgets.append(motion);
	}

	setCentralWidget(centralWidget);
}

void EnvSensorView::initStatusBar()
{
	m_syntroStatus = new QLabel(this);
	ui.statusBar->addWidget(m_syntroStatus, 1);
}

void EnvSensorView::saveWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		m_settings->setValue("Geometry", saveGeometry());
		m_settings->setValue("State", saveState());
		m_settings->endGroup();
		m_settings->sync();
	}
}

void EnvSensorView::restoreWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		restoreGeometry(m_settings->value("Geometry").toByteArray());
		restoreState(m_settings->value("State").toByteArray());
		m_settings->endGroup();
	}
}
