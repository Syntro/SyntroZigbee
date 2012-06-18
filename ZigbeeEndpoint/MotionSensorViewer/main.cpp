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
#include <QtGui/QApplication>

#include "SyntroUtils.h"
#include "ZigbeeCommon.h"


QSettings *loadSettings(QStringList arglist)
{
	QSettings *settings = loadStandardSettings("MotionSensorViewer", arglist);

	if (!settings->contains(ZIGBEE_MULTICAST_SERVICE))
		settings->setValue(ZIGBEE_MULTICAST_SERVICE, "zbmc");

	if (!settings->contains(ZIGBEE_E2E_SERVICE))
		settings->setValue(ZIGBEE_E2E_SERVICE, "zbe2e");

	int count = settings->beginReadArray(MOTION_SENSORS);
	settings->endArray();

	if (count == 0) {
		// setup some examples
		settings->beginWriteArray(MOTION_SENSORS);

		settings->setArrayIndex(0);
		settings->setValue(SENSOR_ZB_ADDRESS, "0x0013a200409879e4");
		settings->setValue(SENSOR_LOCATION, "Garage");
		settings->setArrayIndex(1);
		settings->setValue(SENSOR_ZB_ADDRESS, "0x0013a200408d4b96");
		settings->setValue(SENSOR_LOCATION, "Office");
		settings->setArrayIndex(2);
		settings->setValue(SENSOR_ZB_ADDRESS, "0x0013a2004081abe3");
		settings->setValue(SENSOR_LOCATION, "Back Deck");

		settings->endArray();
	}

	return settings;
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QSettings *settings = loadSettings(a.arguments());

	MotionSensorViewer *w = new MotionSensorViewer(settings);

	w->show();

	return a.exec();
}
