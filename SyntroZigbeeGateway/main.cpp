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

#include "SyntroZigbeeGateway.h"
#include <QtGui/QApplication>

#include "SyntroUtils.h"
#include "ZigbeeCommon.h"
#include "ZigbeeGatewayConsole.h"

int runGuiApp(int argc, char *argv[]);
int runConsoleApp(int argc, char *argv[]);
QSettings *loadSettings(QStringList arglist);


int main(int argc, char *argv[])
{
	if (checkConsoleModeFlag(argc, argv))
		return runConsoleApp(argc, argv);
	else
		return runGuiApp(argc, argv);
}

int runGuiApp(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QSettings *settings = loadSettings(a.arguments());

	SyntroZigbeeGateway *w = new SyntroZigbeeGateway(settings);

	w->show();

	if (settings->value(FULLSCREEN_MODE, false).toBool())
		w->showFullScreen();

	return a.exec();
}

int runConsoleApp(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QSettings *settings = loadSettings(a.arguments());

	ZigbeeGatewayConsole c(settings, &a);

	return a.exec();
}

QSettings *loadSettings(QStringList arglist)
{
	QSettings *settings = loadStandardSettings("SyntroZigbeeGateway", arglist);

	if (!settings->contains(ZIGBEE_PORT)) {
#if defined (Q_WS_WIN)
		settings->setValue(ZIGBEE_PORT, "COM10");
#elif defined (Q_OS_MAC)
		settings->setValue(ZIGBEE_PORT, "/dev/tty.usbserial-A4031BDN");
#else
		settings->setValue(ZIGBEE_PORT, "/dev/ttyUSB0");
#endif
	}

	if (!settings->contains(ZIGBEE_SPEED))
		settings->setValue(ZIGBEE_SPEED, 115200);

	if (!settings->contains(NODE_DISCOVER_INTERVAL))
		settings->setValue(NODE_DISCOVER_INTERVAL, 60);

	if (!settings->contains(MULTICAST_Q_EXPIRE_INTERVAL))
		settings->setValue(MULTICAST_Q_EXPIRE_INTERVAL, 60);

	if (!settings->contains(ZIGBEE_MULTICAST_SERVICE))
		settings->setValue(ZIGBEE_MULTICAST_SERVICE, "zbmc");

	if (!settings->contains(ZIGBEE_E2E_SERVICE))
		settings->setValue(ZIGBEE_E2E_SERVICE, "zbe2e");

	if (!settings->contains(ZIGBEE_PROMISCUOUS_MODE))
		settings->setValue(ZIGBEE_PROMISCUOUS_MODE, true);

	int count = settings->beginReadArray(ZIGBEE_DEVICES);
	settings->endArray();

	if (count == 0) {
		settings->beginWriteArray(ZIGBEE_DEVICES);

		// my test gear
		settings->setArrayIndex(0);
		settings->setValue(ZIGBEE_ADDRESS, "0x0013a2004081abe3");
		settings->setValue(ZIGBEE_READONLY, false);
		settings->setValue(ZIGBEE_POLLINTERVAL, 0);
		settings->endArray();
	}

	if (!settings->contains(FULLSCREEN_MODE))
		settings->setValue(FULLSCREEN_MODE, false);

	return settings;
}
