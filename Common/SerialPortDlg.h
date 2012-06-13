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

#ifndef SERIALPORTDLG_H
#define SERIALPORTDLG_H

#include <qdialog.h>
#include <qsettings.h>
#include <qpushbutton.h>
#include <qcombobox.h>


class SerialPortDlg : public QDialog
{
	Q_OBJECT

public:
	SerialPortDlg(QWidget *parent, QSettings *settings);
	~SerialPortDlg();

public slots:
	void onOK();
	void onCancel();

private:
	void layoutWindow();

	QSettings *m_settings;

	QComboBox *m_portCombo;
	QComboBox *m_speedCombo;

	QPushButton *m_okBtn;
	QPushButton *m_cancelBtn;
};

#endif // SERIALPORTDLG
