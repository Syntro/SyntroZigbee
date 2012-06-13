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

#include "SyntroZigbeeGateway.h"
#include "SerialPortDlg.h"
#include "SyntroAboutDlg.h"

#define DEFAULT_ROW_HEIGHT 20

SyntroZigbeeGateway::SyntroZigbeeGateway(QSettings *settings, QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), m_settings(settings)
{
	ui.setupUi(this);
	initStatusBar();
	initStatTable();

	loadNodeIDList();

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(ui.actionConnect, SIGNAL(triggered()), this, SLOT(onConnect()));
	connect(ui.actionDisconnect, SIGNAL(triggered()), this, SLOT(onDisconnect()));
	connect(ui.actionConfigure, SIGNAL(triggered()), this, SLOT(onConfigure()));

	ui.actionDisconnect->setEnabled(false);

	m_controller = NULL;
	m_refreshTimer = 0;

	m_client = new ZigbeeClient(this, settings);
	m_client->resumeThread();

	m_syntroStatusTimer = startTimer(3000);

	restoreWindowState();
}

SyntroZigbeeGateway::~SyntroZigbeeGateway()
{
}

void SyntroZigbeeGateway::timerEvent(QTimerEvent *)
{
	if (m_client)
		m_syntroStatus->setText(m_client->getLinkState());

	if (m_controller)
		refreshDisplay();
}

void SyntroZigbeeGateway::refreshDisplay()
{
	QList<ZigbeeStats> list = m_controller->stats();

	if (list.count() == 0)
		return;

	// don't count the header row
	int tableRowCount = ui.m_table->rowCount();

	int row = 0;

	// first all the rows we already have created
	for (row = 0; row < tableRowCount && row < list.count(); row++) {
		ZigbeeStats zb = list.at(row);
		populateRow(row, &zb);
	}

	// if we need to add new rows
	if (tableRowCount < list.count()) {
		for (; row < list.count(); row++) {
			ui.m_table->insertRow(row);
			ui.m_table->setRowHeight(row, DEFAULT_ROW_HEIGHT);

			for (int i = 0; i < ui.m_table->columnCount(); i++) {
				QTableWidgetItem *item = new QTableWidgetItem();
				item->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
				ui.m_table->setItem(row, i, item);
			}

			ZigbeeStats zb = list.at(row);
			populateRow(row, &zb);
		}
	}
}

void SyntroZigbeeGateway::populateRow(int row, ZigbeeStats *stat)
{
	if (stat->m_deviceType & ZIGBEE_DEVICE_TYPE_LOCAL)
		(ui.m_table->item(row, 0))->setText(QString("%1*").arg(stat->m_address, 16, 16, QChar('0')));
	else
		(ui.m_table->item(row, 0))->setText(QString("%1").arg(stat->m_address, 16, 16, QChar('0')));
		
	(ui.m_table->item(row, 1))->setText(stat->m_nodeID);
	(ui.m_table->item(row, 2))->setText(QString("%1").arg(stat->m_txCount));
	(ui.m_table->item(row, 3))->setText(QString("%1").arg(stat->m_rxCount));
}

void SyntroZigbeeGateway::onConnect()
{
	if (!m_controller) {
		m_controller = new ZigbeeController();

		if (m_controller->openDevice(m_settings)) {
			if (m_client) {
				connect(m_controller, SIGNAL(receiveData(quint64, QByteArray)),
					m_client, SLOT(receiveData(quint64, QByteArray)), Qt::DirectConnection);

				connect(m_client, SIGNAL(sendData(quint64,QByteArray)),
					m_controller, SLOT(sendData(quint64,QByteArray)), Qt::DirectConnection);

				connect(m_controller, SIGNAL(localRadioAddress(quint64)), 
					m_client, SLOT(localRadioAddress(quint64)));

				connect(m_client, SIGNAL(requestNodeDiscover()), 
					m_controller, SLOT(requestNodeDiscover()));

				connect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
					m_client, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)), Qt::DirectConnection);
			}

			connect(m_controller, SIGNAL(localRadioAddress(quint64)), 
				this, SLOT(localRadioAddress(quint64)));

			connect(this, SIGNAL(requestNodeDiscover()), 
				m_controller, SLOT(requestNodeDiscover()));

			connect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
					this, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)), Qt::DirectConnection);

			connect(this, SIGNAL(requestNodeIDChange(quint64, QString)),
				m_controller, SLOT(requestNodeIDChange(quint64, QString)));

			m_controller->startRunLoop();
			m_refreshTimer = startTimer(500);

			ui.actionConnect->setEnabled(false);
			ui.actionDisconnect->setEnabled(true);
			ui.actionConfigure->setEnabled(false);

			updateStatusBar();

			emit requestNodeDiscover();
		}
		else {
			qDebug() << "Error opening serial device";
			delete m_controller;
			m_controller = NULL;
		}
	}
}

void SyntroZigbeeGateway::onDisconnect()
{
	if (m_refreshTimer) {
		killTimer(m_refreshTimer);
		m_refreshTimer = 0;
	}

	if (m_client) {
		if (m_controller) {
			disconnect(m_controller, SIGNAL(receiveData(quint64, QByteArray)),
				m_client, SLOT(receiveData(quint64, QByteArray)));

			disconnect(m_client, SIGNAL(sendData(quint64,QByteArray)),
				m_controller, SLOT(sendData(quint64,QByteArray)));

			disconnect(m_controller, SIGNAL(localRadioAddress(quint64)), 
				m_client, SLOT(localRadioAddress(quint64)));

			disconnect(m_client, SIGNAL(requestNodeDiscover()), 
				m_controller, SLOT(requestNodeDiscover()));

			disconnect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
				m_client, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)));
		}
	}

	if (m_controller) {
		disconnect(m_controller, SIGNAL(localRadioAddress(quint64)), 
			this, SLOT(localRadioAddress(quint64)));

		disconnect(this, SIGNAL(requestNodeDiscover()), 
			m_controller, SLOT(requestNodeDiscover()));

		disconnect(m_controller, SIGNAL(nodeDiscoverResponse(QList<ZigbeeStats>)),
				this, SLOT(nodeDiscoverResponse(QList<ZigbeeStats>)));

		disconnect(this, SIGNAL(requestNodeIDChange(quint64, QString)),
			m_controller, SLOT(requestNodeIDChange(quint64, QString)));

		m_controller->closeDevice();
		delete m_controller;
		m_controller = NULL;

		ui.actionConnect->setEnabled(true);
		ui.actionDisconnect->setEnabled(false);
		ui.actionConfigure->setEnabled(true);
	}
}

void SyntroZigbeeGateway::localRadioAddress(quint64 address)
{
	if (m_localAddressLabel)
		m_localAddressLabel->setText(QString("%1").arg(address, 16, 16, QChar('0')));
}

void SyntroZigbeeGateway::nodeDiscoverResponse(QList<ZigbeeStats> list)
{
	ZigbeeStats zb;

	for (int i = 0; i < list.count(); i++) {
		zb = list.at(i);

		if (m_nodeIDs.contains(zb.m_address)) {
			if (m_nodeIDs[zb.m_address] != zb.m_nodeID)
				emit requestNodeIDChange(zb.m_address, m_nodeIDs[zb.m_address]);
		}
		else if (zb.m_nodeID.length() > 0) {
			m_nodeIDs.insert(zb.m_address, zb.m_nodeID);
		}
	}
}

void SyntroZigbeeGateway::loadNodeIDList()
{
	bool ok = false;

	int count = m_settings->beginReadArray(ZIGBEE_DEVICES);

	for (int i = 0; i < count; i++) {
		m_settings->setArrayIndex(i);

		if (!m_settings->contains(ZIGBEE_NODEID))
			continue;

		QString s = m_settings->value(ZIGBEE_ADDRESS).toString();

		quint64 address = s.toULongLong(&ok, 16);

		if (ok) {
			QString nodeID = m_settings->value(ZIGBEE_NODEID).toString();

			if (nodeID.length() > 0) {
				nodeID.truncate(20);
				m_nodeIDs.insert(address, nodeID);
			}
		}
	}

	m_settings->endArray();
}

void SyntroZigbeeGateway::onConfigure()
{
	SerialPortDlg *dlg = new SerialPortDlg(this, m_settings);

	if (QDialog::Accepted == dlg->exec())
		m_settings->sync();
}

void SyntroZigbeeGateway::closeEvent(QCloseEvent *)
{
	onDisconnect();

	if (m_syntroStatusTimer) {
		killTimer(m_syntroStatusTimer);
		m_syntroStatusTimer = 0;
	}

	if (m_client) {
		m_client->exitThread();
		delete m_client;
		m_client = NULL;
	}

	saveWindowState();
}

void SyntroZigbeeGateway::updateStatusBar()
{
	m_portName->setText(m_settings->value(ZIGBEE_PORT).toString());
	m_portSpeed->setText(m_settings->value(ZIGBEE_SPEED).toString());
}

void SyntroZigbeeGateway::initStatusBar()
{
	m_syntroStatus = new QLabel(this);
	m_syntroStatus->setFrameShape(QFrame::Panel);
	m_syntroStatus->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_syntroStatus, 1);

	m_localAddressLabel = new QLabel(this);
	m_localAddressLabel->setFrameShape(QFrame::Panel);
	m_localAddressLabel->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_localAddressLabel);

	m_portName = new QLabel(this);
	m_portName->setFrameShape(QFrame::Panel);
	m_portName->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_portName);

	m_portSpeed = new QLabel(this);
	m_portSpeed->setFrameShape(QFrame::Panel);
	m_portSpeed->setFrameShadow(QFrame::Sunken);
	ui.statusBar->addWidget(m_portSpeed);
}

void SyntroZigbeeGateway::initStatTable()
{
	ui.m_table->setColumnCount(4);

	ui.m_table->setHorizontalHeaderLabels(QStringList() << "Address" << "Node ID" << "TX Count" << "RX Count");

	ui.m_table->setSelectionMode(QAbstractItemView::NoSelection);
	ui.m_table->horizontalHeader()->setStretchLastSection(true);

	setCentralWidget(ui.m_table);
}

void SyntroZigbeeGateway::saveWindowState()
{
	if (!m_settings)
		return;

	m_settings->beginGroup("Window");
	m_settings->setValue("Geometry", saveGeometry());
	m_settings->setValue("State", saveState());
	m_settings->endGroup();

	m_settings->beginWriteArray("ZBStatTable");

	for (int i = 0; i < ui.m_table->columnCount(); i++) {
		m_settings->setArrayIndex(i);
		m_settings->setValue("colWidth", ui.m_table->columnWidth(i));
	}

	m_settings->endArray();
}

void SyntroZigbeeGateway::restoreWindowState()
{
	if (!m_settings)
		return;

	m_settings->beginGroup("Window");
	restoreGeometry(m_settings->value("Geometry").toByteArray());
	restoreState(m_settings->value("State").toByteArray());
	m_settings->endGroup();

	int count = m_settings->beginReadArray("ZBStatTable");

	// -1 since our last col stretches to fit
	int colCount = ui.m_table->columnCount() - 1;

	for (int i = 0; i < colCount && i < count; i++) {
		m_settings->setArrayIndex(i);
		int width = m_settings->value("colWidth").toInt();

		if (width > 200)
			width = 200;

		ui.m_table->setColumnWidth(i, width);
	}

	m_settings->endArray();
}

void SyntroZigbeeGateway::onAbout()
{
	SyntroAbout *dlg = new SyntroAbout(this, m_settings);
	dlg->show();
}