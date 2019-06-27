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

    connect(m_timer, &QTimer::timeout, this, &PasswordDialog::checkPassword);
    connect(passwordEdit, &QLineEdit::textEdited, this, &PasswordDialog::passwordChanged);
    connect(verifyEdit, &QLineEdit::textEdited, this, &PasswordDialog::passwordChanged);
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

    // Doesn't need freeing currently. Only set to internal members of pwquality.
    void *auxerror;
    int quality = pwquality_check(m_pwSettings, password.toUtf8().constData(),
                                  nullptr, m_username.constData(), &auxerror);

    qCDebug(USER_MANAGER_LOG) << "Quality: " << quality;

    QString strenght;
    QPalette palette;
    if (quality < 0) {
        palette  = m_negative;
        strenght = errorString(quality, auxerror);
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

QString PasswordDialog::errorString(int error, void *auxerror)
{
    // Translations may be problematic on Debian-derived distros for packaging
    // reasons.
    // https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=931171
    // Ubuntu-derived should be fine since they have central language packages,
    // even though assignment to the gnome package is not ideal.
    // https://bugs.launchpad.net/ubuntu/+source/libpwquality/+bug/1834480
    char buf[PWQ_MAX_ERROR_MESSAGE_LEN]; // arbitrary size
    const QString errorString =
            QString::fromUtf8(pwquality_strerror(buf, PWQ_MAX_ERROR_MESSAGE_LEN,
                                                 error, auxerror));
    if (!errorString.isEmpty()) {
        return errorString;
    }

    return i18nc("Returned when a more specific error message has not been found"
            , "Please choose another password");
}
