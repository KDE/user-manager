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

#include "usersessions.h"
#include "login1_interface.h"

#include <QDBusPendingReply>

#include "user_manager_debug.h"

QDBusArgument &operator<<(QDBusArgument &argument, const UserInfo &userInfo)
{
    argument.beginStructure();
    argument << userInfo.id << userInfo.name << userInfo.path;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, UserInfo &userInfo)
{
    argument.beginStructure();
    argument >> userInfo.id >> userInfo.name >> userInfo.path;
    argument.endStructure();
    return argument;
}

typedef OrgFreedesktopLogin1ManagerInterface Manager;
UserSession::UserSession(QObject* parent): QObject(parent)
{
    qDBusRegisterMetaType<UserInfo>();
    qDBusRegisterMetaType<UserInfoList>();

    m_manager = new Manager(QStringLiteral("org.freedesktop.login1"), QStringLiteral("/org/freedesktop/login1"), QDBusConnection::systemBus());
    connect(m_manager, &OrgFreedesktopLogin1ManagerInterface::UserNew, this, &UserSession::UserNew);
    connect(m_manager, &OrgFreedesktopLogin1ManagerInterface::UserRemoved, this, &UserSession::UserRemoved);

    QDBusPendingReply <UserInfoList> reply = m_manager->ListUsers();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &UserSession::listUsersSlot);
}

UserSession::~UserSession()
{
    delete m_manager;
}

void UserSession::listUsersSlot(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<UserInfoList> reply = *watcher;
    if (reply.isError()) {
        qCWarning(USER_MANAGER_LOG) << reply.error().name() << reply.error().message();
    } else {
        const UserInfoList userList = reply.value();
        for (const UserInfo &userInfo : userList) {
            UserNew(userInfo.id);
        }
    }

    watcher->deleteLater();
}

void UserSession::UserNew(uint id)
{
    qCDebug(USER_MANAGER_LOG) << id;
    Q_EMIT userLogged(id, true);
}

void UserSession::UserRemoved(uint id)
{
    qCDebug(USER_MANAGER_LOG) << id;
    Q_EMIT userLogged(id, false);
}
