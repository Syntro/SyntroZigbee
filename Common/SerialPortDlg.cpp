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

#include <qlayout.h>
#include <qformlayout.h>

#include "qextserialenumerator.h"
#include "SerialPortDlg.h"
#include "ZigbeeCommon.h"

#define NUM_PORT_SPEEDS 5
static int port_speed[NUM_PORT_SPEEDS] = { 9600, 19200, 38400, 57600, 115200 }; 


SerialPortDlg::SerialPortDlg(QWidget *parent, QSettings *settings)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint), m_settings(settings)
{
	layoutWindow();
	connect(m_okBtn, SIGNAL(clicked()), this, SLOT(onOK()));
	connect(m_cancelBtn, SIGNAL(clicked()), this, SLOT(onCancel()));

	setWindowTitle("Configure Gateway");
}

SerialPortDlg::~SerialPortDlg()
{
}

void SerialPortDlg::onOK()
{
	int n = m_portCombo->currentIndex();
	QString port = m_portCombo->itemData(n).toString();	
	
	n = m_speedCombo->currentIndex();
	int speed = m_speedCombo->itemData(n).toInt();

	m_settings->setValue(ZIGBEE_PORT, port);
	m_settings->setValue(ZIGBEE_SPEED, speed);

	accept();
}

void SerialPortDlg::onCancel()
{
	reject();
}

void SerialPortDlg::layoutWindow()
{
	QWidget *centralWidget = new QWidget(this);
	centralWidget->setMinimumSize(QSize(360, 120));

	QFormLayout *formLayout = new QFormLayout();

	m_portCombo = new QComboBox(this);
	m_portCombo->setMinimumSize(QSize(160, 24));
	m_portCombo->setMaximumSize(QSize(400, 24));

	QString currentPort = m_settings->value(ZIGBEE_PORT, "").toString();

	QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

	for (int i = 0; i < ports.count(); i++) {
		QextPortInfo info = ports.at(i);

#if defined Q_OS_WIN
		if (info.friendName.length() > 0)
			m_portCombo->addItem(info.friendName, info.portName);
		else
			m_portCombo->addItem(info.portName, info.portName);

		if (currentPort == info.portName)
			m_portCombo->setCurrentIndex(i);
#elif defined Q_OS_MAC
		m_portCombo->addItem(info.portName, info.portName);

		if (currentPort == info.portName)
			m_portCombo->setCurrentIndex(i);
 
#else
		m_portCombo->addItem(info.physName, info.physName);

		if (currentPort == info.physName)
			m_portCombo->setCurrentIndex(i);
#endif		
	}

	if (ports.count() == 0)
		m_okBtn->setEnabled(false);

	formLayout->addRow("Serial Port", m_portCombo);

	m_speedCombo = new QComboBox(this);
	m_speedCombo->setMinimumSize(QSize(80, 24));
	m_speedCombo->setMaximumSize(QSize(80, 24));

	int currentSpeed = m_settings->value(ZIGBEE_SPEED, 115200).toInt();
		
	for (int i = 0; i < NUM_PORT_SPEEDS; i++) {
		m_speedCombo->addItem(QString::number(port_speed[i]), port_speed[i]);

		if (currentSpeed == port_speed[i])
			m_speedCombo->setCurrentIndex(i);
	}

	formLayout->addRow("Port Speed", m_speedCombo);

	QHBoxLayout *btnLayout = new QHBoxLayout();
	m_okBtn = new QPushButton("Ok", this);
	m_okBtn->setMaximumSize(QSize(80, 24));
	btnLayout->addWidget(m_okBtn);

	m_cancelBtn = new QPushButton("Cancel", this);
	m_cancelBtn->setMaximumSize(QSize(80, 24));
	btnLayout->addWidget(m_cancelBtn);

	QVBoxLayout *vLayout = new QVBoxLayout(centralWidget);

	vLayout->addLayout(formLayout);
	vLayout->addSpacing(20);
	vLayout->addLayout(btnLayout);
}
