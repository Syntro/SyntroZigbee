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

#include "MotionSensorViewer.h"


MotionSensorViewer::MotionSensorViewer(QSettings *settings, QWidget *parent, Qt::WFlags flags)
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

void MotionSensorViewer::loadSensorList()
{
	bool ok;
	quint64 address;

	int count = m_settings->beginReadArray(MOTION_SENSORS);
	
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

	// queue some OFF states to initialize
	for (int i = 0; i < m_addresses.count(); i++)
		m_rxSensorStateQ.append(i);
}

// Pack the data, byte[0] is the sensor list position, max 256 sensors
// Bit:0 of byte[1] (0x0100) is the sensor state, on or off
void MotionSensorViewer::receiveData(quint64 address, QByteArray data)
{
	quint32 sensor = 0xff;
	
	for (int i = 0; i < m_addresses.count(); i++) {
		if (m_addresses.at(i) == address) {
			sensor = i;
			break;
		}
	}

	if (sensor != 0xff) {
		if (data.at(0) != 0)
			sensor |= 0x1000;
		
		m_rxQMutex.lock();
		m_rxSensorStateQ.append(sensor);
		m_rxQMutex.unlock();
	}
}

void MotionSensorViewer::timerEvent(QTimerEvent *timerEvent)
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
			if (!m_rxSensorStateQ.empty()) {
				packedVal = m_rxSensorStateQ.dequeue();
			}
			else {
				done = true;
			}
			m_rxQMutex.unlock();

			if (!done)
				updateSensorState(packedVal & 0xff, packedVal & 0x1000);
		}
	}
}

void MotionSensorViewer::updateSensorState(quint32 sensor, bool state)
{
	if (sensor < m_sensorDisplay.count()) {
		QLabel *sensorLabel = m_sensorDisplay.at(sensor);

		QPalette pal(sensorLabel->palette());
		
		if (state)
			pal.setColor(QPalette::Window, Qt::red);
		else
			pal.setColor(QPalette::Window, Qt::green);

		sensorLabel->setPalette(pal);		
	}
}

void MotionSensorViewer::closeEvent(QCloseEvent *)
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

void MotionSensorViewer::layoutWindow()
{
	QWidget *centralWidget = new QWidget(this);
	QFormLayout *formLayout = new QFormLayout(centralWidget);
	formLayout->setSpacing(6);
	formLayout->setContentsMargins(11, 11, 11, 11);
	formLayout->setObjectName(QString::fromUtf8("formLayout"));

	for (int i = 0; i < m_addresses.count(); i++) {
		QLabel *label = new QLabel(m_locations.at(i), centralWidget);
		formLayout->setWidget(i, QFormLayout::LabelRole, label);

		QLabel *value = new QLabel(centralWidget);
		value->setMinimumSize(QSize(32, 32));
		value->setMaximumSize(QSize(32, 32));
		value->setAutoFillBackground(true);
		value->setFrameShape(QFrame::Panel);
		value->setFrameShadow(QFrame::Raised);
		formLayout->setWidget(i, QFormLayout::FieldRole, value);
		m_sensorDisplay.append(value);
	}

	setCentralWidget(centralWidget);
}

void MotionSensorViewer::initStatusBar()
{
	m_syntroStatus = new QLabel(this);
	ui.statusBar->addWidget(m_syntroStatus, 1);
}

void MotionSensorViewer::saveWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		m_settings->setValue("Geometry", saveGeometry());
		m_settings->setValue("State", saveState());
		m_settings->endGroup();
		m_settings->sync();
	}
}

void MotionSensorViewer::restoreWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		restoreGeometry(m_settings->value("Geometry").toByteArray());
		restoreState(m_settings->value("State").toByteArray());
		m_settings->endGroup();
	}
}
