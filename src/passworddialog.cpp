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
#include <KGlobalSettings>
#include <KColorScheme>

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

    int size = QFontMetrics(KGlobalSettings::fixedFont()).xHeight();
    setMinimumWidth(size * 50);

    m_negative = strenghtLbl->palette();
    m_neutral = strenghtLbl->palette();
    m_positive = strenghtLbl->palette();
    KColorScheme::adjustForeground(m_negative, KColorScheme::NegativeText, strenghtLbl->foregroundRole());
    KColorScheme::adjustForeground(m_neutral, KColorScheme::NeutralText, strenghtLbl->foregroundRole());
    KColorScheme::adjustForeground(m_positive, KColorScheme::PositiveText, strenghtLbl->foregroundRole());

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
        kDebug() << "Verify password is empty";
        return; //No verification, do nothing
    }

    const QString password = passwordEdit->text();
    if (password != verifyEdit->text()) {
        strenghtLbl->setPalette(m_negative);
        strenghtLbl->setText(i18n("Passwords are not equal"));
        return;
    }

    if (!m_pwSettings) {
        m_pwSettings = pwquality_default_settings ();
        pwquality_set_int_value (m_pwSettings, PWQ_SETTING_MAX_SEQUENCE, 4);
        if (pwquality_read_config (m_pwSettings, NULL, NULL) < 0) {
            kWarning() << "failed to read pwquality configuration\n";
            return;
        }
    }

    int quality = pwquality_check (m_pwSettings, password.toAscii(), NULL, m_username, NULL);

    kDebug() << "Quality: " << quality;

    QString strenght;
    QPalette palette;
    if (quality < 0) {
        palette  = m_negative;
        strenght = i18n("Please, choose another password");
    } else if (quality < 25) {
        palette = m_neutral;
        strenght = i18n("The password is Weak");
    } else if (quality < 50) {
        palette = m_positive;
        strenght = i18n("The password is Good");
    } else if (quality < 75) {
        palette = m_positive;
        strenght = i18n("The password is Very Good");
    } else if (quality < 101) {
        palette = m_positive;
        strenght = i18n("the password is Excellent");
    }

    strenghtLbl->setPalette(palette);
    strenghtLbl->setText(strenght);
    button(KDialog::Ok)->setEnabled(true);
}