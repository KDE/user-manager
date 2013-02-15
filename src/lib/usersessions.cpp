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
#include "seat_interface.h"

#include <QDBusPendingReply>
#include <QDBusObjectPath>

typedef OrgFreedesktopConsoleKitManagerInterface Consolekit;
typedef OrgFreedesktopConsoleKitSessionInterface Session;
typedef OrgFreedesktopConsoleKitSeatInterface Seat;

UserSession::UserSession(QObject* parent): QObject(parent)
{
    m_console = new Consolekit("org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", QDBusConnection::systemBus(), this);

    QDBusPendingReply<QList<QDBusObjectPath> >  reply = m_console->GetSessions();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(gotSessions(QDBusPendingCallWatcher*)));

    reply = m_console->GetSeats();
    watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(gotSeats(QDBusPendingCallWatcher*)));

    connect(m_console, SIGNAL(SeatAdded(QDBusObjectPath)), SLOT(gotNewSeat(QDBusObjectPath)));
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

void UserSession::gotSeats(QDBusPendingCallWatcher* call)
{
    QDBusPendingReply<QList<QDBusObjectPath> >  reply = *call;
    if (reply.isError()) {
        qDebug() << reply.error().name();
        qDebug() << reply.error().message();
    } else {
        addSeatWatch(reply.value());
    }

    call->deleteLater();
}

void UserSession::gotNewSeat(const QDBusObjectPath& path)
{
    Seat *seat = new Seat("org.freedesktop.ConsoleKit", path.path(), QDBusConnection::systemBus(), this);

    QDBusPendingReply<QList<QDBusObjectPath> >  reply = seat->GetSessions();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(gotSessions(QDBusPendingCallWatcher*)));
}

void UserSession::addLoggedUsers(QList< QDBusObjectPath > list)
{
    Q_FOREACH(const QDBusObjectPath &path, list) {
        addLoggedUser(path);
    }
}

void UserSession::addLoggedUser(const QDBusObjectPath& path)
{
    Session *session = 0;
    session = new Session("org.freedesktop.ConsoleKit", path.path(), QDBusConnection::systemBus(), this);
    qDebug() << "Logged user: " << session->GetUnixUser().value();
    m_loggedUsers.insert(path, session->GetUnixUser().value());
    delete session;
}

void UserSession::removeLoggedUser(const QDBusObjectPath& path)
{
    m_loggedUsers.remove(path);
}

void UserSession::addSeatWatch(QList< QDBusObjectPath > list)
{
    Seat *seat = 0;
    Q_FOREACH(const QDBusObjectPath &path, list) {
        seat = new Seat("org.freedesktop.ConsoleKit", path.path(), QDBusConnection::systemBus(), this);
        connect(seat, SIGNAL(SessionAdded(QDBusObjectPath)), SLOT(addLoggedUser(QDBusObjectPath)));
        connect(seat, SIGNAL(SessionRemoved(QDBusObjectPath)), SLOT(removeLoggedUser(QDBusObjectPath)));
    }
}
