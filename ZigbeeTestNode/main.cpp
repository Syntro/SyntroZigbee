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

#include "ZigbeeTestNode.h"
#include <QtGui/QApplication>
#include <qdir.h>


QSettings *loadSettings()
{
	QSettings *settings = new QSettings(QDir::currentPath() + "/ZigbeeTestNode.ini", QSettings::IniFormat);

	if (!settings->contains(ZIGBEE_PORT))
		settings->setValue(ZIGBEE_PORT, "/dev/ttyUSB0");

	if (!settings->contains(ZIGBEE_SPEED))
		settings->setValue(ZIGBEE_SPEED, 115200);

	return settings;
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QSettings *settings = loadSettings();

	ZigbeeTestNode *w = new ZigbeeTestNode(settings);

	w->show();

	return a.exec();
}
