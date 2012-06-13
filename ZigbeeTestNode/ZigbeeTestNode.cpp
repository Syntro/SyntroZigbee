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

#include <qmessagebox.h>
#include <qdebug.h>
#include "ZigbeeTestNode.h"
#include "SerialPortDlg.h"

#define INVALID_RADIO 0xFFFFFFFFFFFFFFFFULL

ZigbeeTestNode::ZigbeeTestNode(QSettings *settings, QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), m_settings(settings)
{
	ui.setupUi(this);
	initStatusBar();
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.scanBtn, SIGNAL(clicked()), this, SLOT(onScan()));
	connect(ui.sendBtn, SIGNAL(clicked()), this, SLOT(onSend()));
	connect(ui.clearBtn, SIGNAL(clicked()), this, SLOT(onClear()));
	connect(ui.actionConnect, SIGNAL(triggered()), this, SLOT(onConnect()));
	connect(ui.actionDisconnect, SIGNAL(triggered()), this, SLOT(onDisconnect()));
	connect(ui.actionConfigure, SIGNAL(triggered()), this, SLOT(onConfigure()));

	m_newRadioList = false;

	ui.actionDisconnect->setEnabled(false);
	ui.scanBtn->setEnabled(false);
	ui.sendBtn->setEnabled(false);
	m_scanDelay = 1;
	m_refreshTimer = 0;
	m_controller = NULL;

	restoreWindowState();
}

ZigbeeTestNode::~ZigbeeTestNode()
{
}

void ZigbeeTestNode::localRadioAddress(quint64 address)
{
	m_localAddress = address;

	if (m_localAddressLabel)
		m_localAddressLabel->setText(QString("%1").arg(m_localAddress, 16, 16, QChar('0')));
}

void ZigbeeTestNode::onConnect()
{
	if (!m_controller) {
		m_controller = new ZigbeeController();

		if (m_controller->openDevice(m_settings)) {
			connect(m_controller, SIGNAL(receiveData(quint64, QByteArray)),
				this, SLOT(receiveData(quint64, QByteArray)), Qt::DirectConnection);

			connect(this, SIGNAL(sendData(quint64, QByteArray)),
				m_controller, SLOT(sendData(quint64, QByteArray)), Qt::DirectConnection);

			connect(m_controller, SIGNAL(localRadioAddress(quint64)), 
				this, SLOT(localRadioAddress(quint64)));

			connect(this, SIGNAL(requestNodeDiscover()), 
				m_controller, SLOT(requestNodeDiscover()));

			connect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
				this, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)), Qt::DirectConnection);

			m_controller->startRunLoop();
			m_refreshTimer = startTimer(500);

			ui.actionConnect->setEnabled(false);
			ui.actionDisconnect->setEnabled(true);
			ui.actionConfigure->setEnabled(false);
			ui.scanBtn->setEnabled(false);
			ui.sendBtn->setEnabled(true);
			m_scanDelay = 10;
			updateStatusBar();
		}
		else {
			qDebug() << "Error opening serial device";
			delete m_controller;
			m_controller = NULL;
		}
	}
}

void ZigbeeTestNode::onDisconnect()
{
	if (m_refreshTimer) {
		killTimer(m_refreshTimer);
		m_refreshTimer = 0;
	}

	if (m_controller) {
		m_controller->closeDevice();

		disconnect(m_controller, SIGNAL(receiveData(quint64, QByteArray)),
			   this, SLOT(receiveData(quint64, QByteArray)));

		disconnect(this, SIGNAL(sendData(quint64, QByteArray)),
			m_controller, SLOT(sendData(quint64, QByteArray)));

		disconnect(m_controller, SIGNAL(localRadioAddress(quint64)), 
			this, SLOT(localRadioAddress(quint64)));

		disconnect(this, SIGNAL(requestNodeDiscover()),
			m_controller, SLOT(requestNodeDiscover()));

		disconnect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
			this, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)));

		delete m_controller;
		m_controller = NULL;

		ui.actionConnect->setEnabled(true);
		ui.actionDisconnect->setEnabled(false);
		ui.actionConfigure->setEnabled(true);
		ui.scanBtn->setEnabled(false);
		ui.sendBtn->setEnabled(false);
	}
}

void ZigbeeTestNode::onConfigure()
{
	SerialPortDlg *dlg = new SerialPortDlg(this, m_settings);

	if (QDialog::Accepted == dlg->exec())
		m_settings->sync();
}

void ZigbeeTestNode::onScan()
{
	if (m_scanDelay == 0) {
		emit requestNodeDiscover();
		m_scanDelay = 20;
		ui.scanBtn->setEnabled(false);
	}
}

void ZigbeeTestNode::nodeDiscoverResponse(QList<ZigbeeStats> list)
{
	QMutexLocker lock(&m_radioListMutex);

	m_radioList.clear();

	for (int i = 0; i < list.count(); i++) {
		ZigbeeStats zb = list.at(i);
		m_radioList.insert(zb.m_address, zb);
	}

	m_newRadioList = true;
}

void ZigbeeTestNode::onSend()
{
	quint64 address = getCurrentRadio();
	if (address == 0)
		return;

	QString txt = ui.txBuff->text();
	if (txt.length() < 1)
		return;

	QByteArray data = convertHex(txt);
	if (data.count() == 0)
		return;

	emit sendData(address, data);
}

QByteArray ZigbeeTestNode::convertHex(QString s)
{
	QByteArray data;
	bool ok;

	QStringList list = s.split(' ');

	for (int i = 0; i < list.count(); i++) {
		int val = list.at(i).toInt(&ok, 16);

		if (!ok || val < 0 || val > 255) {
			QMessageBox::warning(this, "Bad Input", QString("Not a hex value - %1").arg(list.at(i)));
			data.clear();
			break;
		}

		data.append((char)val);
	}

	return data;
}

void ZigbeeTestNode::onClear()
{
	ui.rxBuff->clear();
}

quint64 ZigbeeTestNode::getCurrentRadio()
{
	quint64 address = INVALID_RADIO;

	int n = ui.dstAddress->currentIndex();

	if (n >= 0)
		address = ui.dstAddress->itemData(n).toULongLong();

	return address;
}

void ZigbeeTestNode::updateRadioListDisplay()
{
	QMutexLocker lock(&m_radioListMutex);
	int pos = 0;
	quint64 current = getCurrentRadio();

	ui.dstAddress->clear();

	// first entry is always this
	ui.dstAddress->addItem(QString("Choose radio"), (quint64)INVALID_RADIO);

	QMapIterator<quint64, ZigbeeStats> i(m_radioList);

	while (i.hasNext()) {
		i.next();

		ZigbeeStats zb = i.value();

		if (zb.m_nodeID.length() > 0) {
			if (zb.m_deviceType & ZIGBEE_DEVICE_TYPE_LOCAL)
				ui.dstAddress->addItem(QString("%1 (%2*)").arg(zb.m_nodeID).arg(zb.m_address, 16, 16, QChar('0')), zb.m_address);
			else
				ui.dstAddress->addItem(QString("%1 (%2)").arg(zb.m_nodeID).arg(zb.m_address, 16, 16, QChar('0')), zb.m_address);
		}
		else {
			if (zb.m_deviceType & ZIGBEE_DEVICE_TYPE_LOCAL)
				ui.dstAddress->addItem(QString("%1*").arg(zb.m_address, 16, 16, QChar('0')), zb.m_address);
			else
				ui.dstAddress->addItem(QString("%1").arg(zb.m_address, 16, 16, QChar('0')), zb.m_address);
		}

		if (zb.m_address == current)
			pos = ui.dstAddress->count() - 1;
	}	

	ui.dstAddress->setCurrentIndex(pos);

	m_newRadioList = false;
}

void ZigbeeTestNode::receiveData(quint64 address, QByteArray data)
{
	QMutexLocker lock(&m_rxQMutex);

	m_rxAddressQ.enqueue(address);
	m_rxQ.enqueue(data);
}

void ZigbeeTestNode::timerEvent(QTimerEvent *)
{
	QByteArray data;
	quint64 address = 0;

	if (m_newRadioList)
		updateRadioListDisplay();

	m_rxQMutex.lock();

	if (!m_rxQ.empty()) {
		data = m_rxQ.dequeue();
		address = m_rxAddressQ.dequeue();
	}

	m_rxQMutex.unlock();

	if (data.count() > 0)
		updateRxFields(address, &data);

	if (m_scanDelay > 0) {
		m_scanDelay--;

		if (m_scanDelay == 0)
			ui.scanBtn->setEnabled(true);
	}
}

void ZigbeeTestNode::updateRxFields(quint64 address, QByteArray *data)
{
	char temp[8];
	QString s;

	m_radioListMutex.lock();

	if (m_radioList.contains(address)) {
		ZigbeeStats zb = m_radioList.value(address);

		if (zb.m_nodeID.length() > 0)
			ui.rxDevice->setText(QString("%1 (0x%2)").arg(zb.m_nodeID).arg(address, 16, 16, QChar('0')));
		else
			ui.rxDevice->setText(QString("0x%1").arg(address, 16, 16, QChar('0')));
	}
	else {
		ui.rxDevice->setText(QString("0x%1").arg(address, 16, 16, QChar('0')));
	}

	m_radioListMutex.unlock();

	for (int i = 0; i < data->count(); i++) {
		quint32 c = 0xff & data->at(i);
		sprintf(temp, "%02X ", c);
		s.append(temp);
	}

	ui.rxBuff->setText(s);
}

void ZigbeeTestNode::closeEvent(QCloseEvent *)
{
	onDisconnect();
	saveWindowState();
}

void ZigbeeTestNode::updateStatusBar()
{
	m_portName->setText(m_settings->value(ZIGBEE_PORT).toString());
	m_portSpeed->setText(m_settings->value(ZIGBEE_SPEED).toString());
}

void ZigbeeTestNode::initStatusBar()
{
	m_localAddressLabel = new QLabel(this);
	m_localAddressLabel->setFrameShape(QFrame::Panel);
	m_localAddressLabel->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_localAddressLabel, 1);

	m_portName = new QLabel(this);
	m_portName->setFrameShape(QFrame::Panel);
	m_portName->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_portName, 0);

	m_portSpeed = new QLabel(this);
	m_portSpeed->setFrameShape(QFrame::Panel);
	m_portSpeed->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_portSpeed);
}

void ZigbeeTestNode::saveWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		m_settings->setValue("Geometry", saveGeometry());
		m_settings->setValue("State", saveState());
		m_settings->endGroup();
	}
}

void ZigbeeTestNode::restoreWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		restoreGeometry(m_settings->value("Geometry").toByteArray());
		restoreState(m_settings->value("State").toByteArray());
		m_settings->endGroup();
	}
}
