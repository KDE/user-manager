/*************************************************************************************
 *  Copyright (C) 2013 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "passworddialog.h"

#include <KPushButton>
PasswordDialog::PasswordDialog(QWidget* parent, Qt::WindowFlags flags): KDialog(parent, flags)
{
    QWidget *widget = new QWidget(this);
    setupUi(widget);
    setMainWidget(widget);
    button(KDialog::Ok)->setEnabled(false);
    password->setFocus();

    connect(password, SIGNAL(textEdited(QString)), SLOT(passwordChanged(QString)));
}

PasswordDialog::~PasswordDialog()
{

}

void PasswordDialog::passwordChanged(const QString& text)
{

}