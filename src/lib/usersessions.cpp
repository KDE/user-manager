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
#include "console_interface.h"
#include "session_interface.h"
#include <QDBusPendingReply>
#include <QDBusObjectPath>

typedef OrgFreedesktopConsoleKitManagerInterface Consolekit;
typedef OrgFreedesktopConsoleKitSessionInterface Session;
UserSession::UserSession(QObject* parent): QObject(parent)
{
    m_console = new Consolekit("org.freedesktop.Accounts", "/org/freedesktop/Accounts", QDBusConnection::systemBus(), this);
    QDBusPendingReply<QList<QDBusObjectPath> >  reply = m_console->GetSessions();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(gotSessions(QDBusPendingCallWatcher*)));
}

UserSession::~UserSession()
{
    delete m_console;
}

void UserSession::gotSessions(QDBusPendingCallWatcher* call)
{
    QDBusPendingReply<QList<QDBusObjectPath> >  reply = *call;
    if (reply.isError()) {
        qDebug() << reply.error().name();
        qDebug() << reply.error().message();
    } else {
        addLoggedUsers(reply.value());
    }

    call->deleteLater();
}

void UserSession::addLoggedUsers(QList< QDBusObjectPath > list)
{
    Session *session = 0;
    Q_FOREACH(const QDBusObjectPath &path, list) {
        session = new Session("org.freedesktop.Accounts", path.path(), QDBusConnection::systemBus(), this);
        if (session->IsActive().value()) {
            m_loggedUsers.append(session->GetUnixUser().value());
        }
    }
}
