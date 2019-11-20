/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
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


#ifndef ACCOUNTMODEL_H
#define ACCOUNTMODEL_H

#include "user_manager_debug.h"
#include <QStringList>
#include <QAbstractListModel>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <KEMailSettings>

class UserSession;
class OrgFreedesktopAccountsInterface;
class OrgFreedesktopAccountsUserInterface;

class AutomaticLoginSettings {
public:
    AutomaticLoginSettings();
    QString autoLoginUser() const;
    bool setAutoLoginUser(const QString &username);
private:
    QString m_autoLoginUser;
};

class AccountModel : public QAbstractListModel
{
    Q_OBJECT
    public:
        enum Role {
            FriendlyName = Qt::DisplayRole,
            Face = Qt::DecorationRole,
            RealName = Qt::UserRole,
            Username,
            Password,
            Email,
            Administrator,
            AutomaticLogin,
            Logged,
            Created
        };

        explicit AccountModel(QObject* parent);
        ~AccountModel() override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
        bool removeAccountKeepingFiles(int row, bool keepFile = false);
        void setDpr(qreal dpr);

        QVariant newUserData(int role) const;
        bool newUserSetData(const QModelIndex& index, const QVariant& value, int roleInt);

    public Q_SLOTS:
        void UserAdded(const QDBusObjectPath &dbusPah);
        void UserDeleted(const QDBusObjectPath &path);
        void Changed();
        void userLogged(uint uid, bool logged);

    private:
        const QString accountPathForUid(uint uid) const;
        void addAccount(const QString &path);
        void addAccountToCache(const QString &path, OrgFreedesktopAccountsUserInterface *acc, int pos = -1);
        void replaceAccount(const QString &path, OrgFreedesktopAccountsUserInterface *acc, int pos);
        void removeAccount(const QString &path);
        bool checkForErrors(QDBusPendingReply <void> reply) const;
        QString cryptPassword(const QString &password) const;
        UserSession* m_sessions;
        QStringList m_userPath;
        OrgFreedesktopAccountsInterface* m_dbus;
        QHash<AccountModel::Role, QVariant> m_newUserData;
        QHash<QString, OrgFreedesktopAccountsUserInterface*> m_users;
        QHash<QString, bool> m_loggedAccounts;
        KEMailSettings m_kEmailSettings;
        AutomaticLoginSettings m_autoLoginSettings;
        qreal m_dpr = 1;
};

QDebug operator<<(QDebug debug, AccountModel::Role role);
#endif // ACCOUNTMODEL_H
