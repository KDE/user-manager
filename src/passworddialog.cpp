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

#include "user_manager_debug.h"

#include <QTimer>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFontDatabase>

#include <klocalizedstring.h>
#include <KColorScheme>

PasswordDialog::PasswordDialog(QWidget* parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
    , m_pwSettings(nullptr)
    , m_timer(new QTimer(this))
{
    setWindowTitle(i18nc("Title for change password dialog", "New Password"));

    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(this);
    setupUi(widget);
    layout->addWidget(widget);
    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    buttons->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
    layout->addWidget(buttons);
    setLayout(layout);

    passwordEdit->setFocus();
    m_timer->setInterval(400);
    m_timer->setSingleShot(true);

    int size = QFontMetrics(QFontDatabase::systemFont(QFontDatabase::FixedFont)).xHeight();
    setMinimumWidth(size * 50);

    m_negative = strenghtLbl->palette();
    m_neutral = strenghtLbl->palette();
    m_positive = strenghtLbl->palette();
    KColorScheme::adjustForeground(m_negative, KColorScheme::NegativeText, strenghtLbl->foregroundRole());
    KColorScheme::adjustForeground(m_neutral, KColorScheme::NeutralText, strenghtLbl->foregroundRole());
    KColorScheme::adjustForeground(m_positive, KColorScheme::PositiveText, strenghtLbl->foregroundRole());

    connect(m_timer, SIGNAL(timeout()), SLOT(checkPassword()));
    connect(passwordEdit, SIGNAL(textEdited(QString)), SLOT(passwordChanged()));
    connect(verifyEdit, SIGNAL(textEdited(QString)), SLOT(passwordChanged()));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

PasswordDialog::~PasswordDialog()
{
    pwquality_free_settings(m_pwSettings);
}

void PasswordDialog::passwordChanged()
{
    m_timer->start();
    strenghtLbl->clear();
}

void PasswordDialog::setUsername(const QByteArray& username)
{
    m_username = username;
}

QString PasswordDialog::password() const
{
    return passwordEdit->text();
}

void PasswordDialog::checkPassword()
{
    qCDebug(USER_MANAGER_LOG) << "Checking password";
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);

    if (verifyEdit->text().isEmpty()) {
        qCDebug(USER_MANAGER_LOG) << "Verify password is empty";
        return; //No verification, do nothing
    }

    const QString password = passwordEdit->text();
    if (password != verifyEdit->text()) {
        strenghtLbl->setPalette(m_negative);
        strenghtLbl->setText(i18n("Passwords do not match"));
        return;
    }

    if (!m_pwSettings) {
        m_pwSettings = pwquality_default_settings ();
        pwquality_set_int_value (m_pwSettings, PWQ_SETTING_MAX_SEQUENCE, 4);
        if (pwquality_read_config (m_pwSettings, nullptr, nullptr) < 0) {
            qCWarning(USER_MANAGER_LOG) << "failed to read pwquality configuration\n";
            return;
        }
    }

    int quality = pwquality_check (m_pwSettings, password.toUtf8(), nullptr, m_username, nullptr);

    qCDebug(USER_MANAGER_LOG) << "Quality: " << quality;

    QString strenght;
    QPalette palette;
    if (quality < 0) {
        palette  = m_negative;
        strenght = errorString(quality);
    } else if (quality < 25) {
        palette = m_neutral;
        strenght = i18n("This password is weak");
    } else if (quality < 50) {
        palette = m_positive;
        strenght = i18n("This password is good");
    } else if (quality < 75) {
        palette = m_positive;
        strenght = i18n("This password is very good");
    } else if (quality < 101) {
        palette = m_positive;
        strenght = i18n("This password is excellent");
    }

    strenghtLbl->setPalette(palette);
    strenghtLbl->setText(strenght);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
}

QString PasswordDialog::errorString(int error)
{
    switch(error) {
        case PWQ_ERROR_MIN_LENGTH:
        {
            int length;
            pwquality_get_int_value(m_pwSettings, PWQ_SETTING_MIN_LENGTH, &length);
            return i18ncp("Error returned when the password is invalid"
                ,"The password should be at least %1 character"
                ,"The password should be at least %1 characters", length);
        }
        case PWQ_ERROR_MIN_UPPERS:
        {
            int amount;
            pwquality_get_int_value(m_pwSettings, PWQ_SETTING_UP_CREDIT, &amount);
            return i18ncp("Error returned when the password is invalid"
                ,"The password should contain at least %1 uppercase letter"
                ,"The password should contain at least %1 uppercase letters", amount);
        }
        case PWQ_ERROR_MIN_LOWERS:
        {
            int amount;
            pwquality_get_int_value(m_pwSettings, PWQ_SETTING_LOW_CREDIT, &amount);
            return i18ncp("Error returned when the password is invalid"
                ,"The password should contain at least %1 lowercase letter"
                ,"The password should contain at least %1 lowercase letters", amount);
        }
        case PWQ_ERROR_USER_CHECK:
            return i18nc("Error returned when the password is invalid", "Your username should not be part of your password");
        case PWQ_ERROR_GECOS_CHECK:
            return i18nc("Error returned when the password is invalid", "Your name should not be part of your password");
        case PWQ_ERROR_MIN_DIGITS:
        {
            int amount;
            pwquality_get_int_value(m_pwSettings, PWQ_SETTING_DIG_CREDIT, &amount);
            return i18ncp("Error returned when the password is invalid"
                ,"The password should contain at least %1 number"
                ,"The password should contain at least %1 numbers", amount);
        }
        case PWQ_ERROR_MIN_OTHERS:
        {
            int amount;
            pwquality_get_int_value(m_pwSettings, PWQ_SETTING_OTH_CREDIT, &amount);
            return i18ncp("Error returned when the password is invalid"
                ,"The password should contain at least %1 special character (like punctuation)"
                ,"The password should contain at least %1 special characters (like punctuation)", amount);
        }
        case PWQ_ERROR_MIN_CLASSES:
            return i18nc("Error returned when the password is invalid", "The password should contain a mixture of letters, numbers, spaces and punctuation");
        case PWQ_ERROR_MAX_CONSECUTIVE:
            return i18nc("Error returned when the password is invalid", "The password should not contain too many repeated characters");
        case PWQ_ERROR_MAX_CLASS_REPEAT:
            return i18nc("Error returned when the password is invalid", "The password should be more varied in letters, numbers and punctuation");
        case PWQ_ERROR_MAX_SEQUENCE:
            return i18nc("Error returned when the password is invalid", "The password should not contain sequences like 1234 or abcd");
        case PWQ_ERROR_CRACKLIB_CHECK:
            return i18nc("Error returned when the password is invalid", "This password can't be used, it is too simple");
    };

    return i18nc("Returned when a more specific error message has not been found"
            , "Please choose another password");
}
