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

#include <KDebug>
#include <QTimer>
#include <KPushButton>
#include <klocalizedstring.h>

PasswordDialog::PasswordDialog(QWidget* parent, Qt::WindowFlags flags)
    : KDialog(parent, flags)
    , m_timer(new QTimer(this))
    , m_pwSettings(0)
{
    QWidget *widget = new QWidget(this);
    setupUi(widget);
    setMainWidget(widget);
    button(KDialog::Ok)->setEnabled(false);
    passwordEdit->setFocus();
    m_timer->setInterval(400);
    m_timer->setSingleShot(true);

    connect(m_timer, SIGNAL(timeout()), SLOT(checkPassword()));
    connect(passwordEdit, SIGNAL(textEdited(QString)), SLOT(passwordChanged(QString)));
    connect(verifyEdit, SIGNAL(textEdited(QString)), SLOT(passwordChanged(QString)));
}

PasswordDialog::~PasswordDialog()
{
    pwquality_free_settings(m_pwSettings);
}

void PasswordDialog::passwordChanged(const QString& text)
{
    m_timer->start();
    strenghtLbl->clear();
}

void PasswordDialog::setUsername(const QByteArray& username)
{
    m_username = username;
}

void PasswordDialog::checkPassword()
{
    kDebug() << "Checking password";

    if (verifyEdit->text().isEmpty()) {
        return; //No verification, do nothing
    }

    const QString password = passwordEdit->text();
    if (password != verifyEdit->text()) {
        strenghtLbl->setText(i18n("Passwords are not equal"));
        return;
    }

    if (!m_pwSettings) {
        m_pwSettings = pwquality_default_settings ();
        pwquality_set_int_value (m_pwSettings, PWQ_SETTING_MAX_SEQUENCE, 4);
        if (pwquality_read_config (m_pwSettings, NULL, NULL) < 0) {
            kWarning() << "failed to read pwquality configuration\n";
        }
    }

    int quality = pwquality_check (m_pwSettings, password.toAscii(), NULL, m_username, NULL);

    if (quality < 0) return;

    kDebug() << quality;
    QString strenght;
    if (quality < 25) {
        strenght = i18n("Poor");
    } else if (quality < 50) {
        strenght = i18n("Fair");
    } else if (quality < 75) {
        strenght = i18n("Good");
    } else if (quality < 101) {
        strenght = i18n("Excellent");
    }

    strenghtLbl->setText(strenght);
    button(KDialog::Ok)->setEnabled(true);
}