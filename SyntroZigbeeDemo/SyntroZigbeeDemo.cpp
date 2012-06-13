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

#include "SyntroZigbeeDemo.h"
#include "ZigbeeUtils.h"

#define INVALID_RADIO 0xFFFFFFFFFFFFFFFFULL

SyntroZigbeeDemo::SyntroZigbeeDemo(QSettings *settings, QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), m_settings(settings)
{
	ui.setupUi(this);
	initStatusBar();
	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.scanBtn, SIGNAL(clicked()), this, SLOT(onScan()));
	connect(ui.sendBtn, SIGNAL(clicked()), this, SLOT(onSend()));
	connect(ui.clearBtn, SIGNAL(clicked()), this, SLOT(onClear()));

	m_newRadioList = false;

	m_client = new ZigbeeClient(this, settings);

	connect(m_client, SIGNAL(receiveData(quint64,QByteArray)),
			this, SLOT(receiveData(quint64,QByteArray)), Qt::DirectConnection);

	connect(m_client, SIGNAL(receiveRadioList(QList<ZigbeeStats>)),
			this, SLOT(receiveRadioList(QList<ZigbeeStats>)), Qt::DirectConnection);

	m_client->resumeThread();

	m_refreshTimer = startTimer(100);
	m_statusTimer = startTimer(3000);
	m_scanDelay = 2;
	ui.scanBtn->setEnabled(false);

	restoreWindowState();
}

void SyntroZigbeeDemo::onScan()
{
	quint64 address = 0;
	QByteArray cmd;

	putU16(&cmd, ZIGBEE_AT_CMD_ND);

	m_client->sendData(address, cmd);
	ui.scanBtn->setEnabled(false);
	m_scanDelay = 3;
}

void SyntroZigbeeDemo::onSend()
{
	quint64 address = getCurrentRadio();
	if (address == INVALID_RADIO)
		return;

	QString txt = ui.txBuff->text();
	if (txt.length() < 1)
		return;

	QByteArray data = convertHex(txt);
	if (data.count() == 0)
		return;

	if (!m_client->sendData(address, data))
		QMessageBox::warning(this, "Error", "Send data failed");
}

quint64 SyntroZigbeeDemo::getCurrentRadio()
{
	quint64 address = INVALID_RADIO;

	int n = ui.dstAddress->currentIndex();

	if (n >= 0)
		address = ui.dstAddress->itemData(n).toULongLong();

	return address;
}

QByteArray SyntroZigbeeDemo::convertHex(QString s)
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

void SyntroZigbeeDemo::onClear()
{
	ui.rxBuff->clear();
}

void SyntroZigbeeDemo::receiveRadioList(QList<ZigbeeStats> list)
{
	QMutexLocker lock(&m_radioListMutex);

	m_radioList.clear();

	for (int i = 0; i < list.count(); i++) {
		ZigbeeStats zb = list.at(i);
		m_radioList.insert(zb.m_address, zb);
	}

	m_newRadioList = true;
}

void SyntroZigbeeDemo::receiveData(quint64 address, QByteArray data)
{
	QMutexLocker lock(&m_rxQMutex);

	m_rxAddressQ.enqueue(address);
	m_rxQ.enqueue(data);
}

void SyntroZigbeeDemo::timerEvent(QTimerEvent *timerEvent)
{
	QByteArray data;
	quint64 address = 0;

	if (timerEvent->timerId() == m_statusTimer) {
		m_syntroStatus->setText(m_client->getLinkState());

		if (m_scanDelay > 0) {
			m_scanDelay--;

			if (m_scanDelay == 0)
				ui.scanBtn->setEnabled(true);
		}
	}
	else {
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
	}
}

void SyntroZigbeeDemo::updateRadioListDisplay()
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

void SyntroZigbeeDemo::updateRxFields(quint64 address, QByteArray *data)
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

void SyntroZigbeeDemo::closeEvent(QCloseEvent *)
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

void SyntroZigbeeDemo::initStatusBar()
{
	m_syntroStatus = new QLabel(this);
	ui.statusBar->addWidget(m_syntroStatus, 1);
}

void SyntroZigbeeDemo::saveWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		m_settings->setValue("Geometry", saveGeometry());
		m_settings->setValue("State", saveState());
		m_settings->endGroup();
		m_settings->sync();
	}
}

void SyntroZigbeeDemo::restoreWindowState()
{
	if (m_settings) {
		m_settings->beginGroup("Window");
		restoreGeometry(m_settings->value("Geometry").toByteArray());
		restoreState(m_settings->value("State").toByteArray());
		m_settings->endGroup();
	}
}
