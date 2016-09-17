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

#ifndef PASSWORD_DIALOG_H
#define PASSWORD_DIALOG_H

#include "ui_password.h"

#include <pwquality.h>

#include <QDialog>
class QDialogButtonBox;


class QTimer;
class PasswordDialog : public QDialog, private Ui::PasswordDlg
{
    Q_OBJECT

    public:
        explicit PasswordDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
        virtual ~PasswordDialog();

        void setUsername(const QByteArray &username);
        QString password() const;
    private Q_SLOTS:
        void passwordChanged();
        void checkPassword();

    private:
        QString errorString(int error);
        QPalette m_negative;
        QPalette m_neutral;
        QPalette m_positive;
        QDialogButtonBox *buttons;

        pwquality_settings_t *m_pwSettings;
        QByteArray m_username;
        QTimer *m_timer;
};

#endif //PASSWORD_DIALOG_H
