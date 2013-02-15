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

#ifndef USER_SESSION_H
#define USER_SESSION_H

#include <QObject>
#include <QDBusObjectPath>

class QDBusPendingCallWatcher;
class OrgFreedesktopConsoleKitManagerInterface;
class UserSession : public QObject
{
    Q_OBJECT
    public:
        explicit UserSession(QObject* parent = 0);
        virtual ~UserSession();

    public Q_SLOTS:
        void gotSessions(QDBusPendingCallWatcher* call);
        void gotSeats(QDBusPendingCallWatcher* call);
        void gotNewSeat(const QDBusObjectPath &path);

        void addLoggedUser(const QDBusObjectPath &path);

    private:
        void addLoggedUsers(QList<QDBusObjectPath> list);
        void addSeatWatch(QList<QDBusObjectPath> list);

        QList<uint> m_loggedUsers;
        OrgFreedesktopConsoleKitManagerInterface* m_console;
};

#endif //USER_SESSION_H