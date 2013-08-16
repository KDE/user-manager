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
#include <QDBusObjectPath>

#include <KDebug>

QDBusArgument &operator<<(QDBusArgument &argument, const UserInfo &userInfo)
{
    argument.beginStructure();
    argument << userInfo.userId << userInfo.userName << userInfo.path;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, UserInfo &userInfo)
{
    argument.beginStructure();
    argument >> userInfo.userId >> userInfo.userName >> userInfo.path;
    argument.endStructure();
    return argument;
}

typedef OrgFreedesktopLogin1ManagerInterface Manager;
UserSession::UserSession(QObject* parent): QObject(parent)
{
    qDBusRegisterMetaType<UserInfo>();
    qDBusRegisterMetaType<UserInfoList>();

    m_manager = new Manager("org.freedesktop.login1", "/org/freedesktop/login1", QDBusConnection::systemBus());
    connect(m_manager, SIGNAL(UserNew(uint,QDBusObjectPath)), SLOT(UserNew(uint)));
    connect(m_manager, SIGNAL(UserRemoved(uint,QDBusObjectPath)), SLOT(UserRemoved(uint)));

    QDBusPendingReply <UserInfoList> reply = m_manager->ListUsers();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(listUsersSlot(QDBusPendingCallWatcher*)));
}

UserSession::~UserSession()
{
    delete m_manager;
}

void UserSession::listUsersSlot(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<UserInfoList> reply = *watcher;
    if (reply.isError()) {
        kWarning() << reply.error().name() << reply.error().message();
    } else {
        UserInfoList userList = reply.value();
        Q_FOREACH(const UserInfo &userInfo, userList) {
            UserNew(userInfo.userId);
        }
    }

    watcher->deleteLater();
}

void UserSession::UserNew(uint id)
{
    kDebug() << id;
    Q_EMIT userLogged(id, true);
}

void UserSession::UserRemoved(uint id)
{
    kDebug() << id;
    Q_EMIT userLogged(id, false);
}