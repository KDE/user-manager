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


#include "accountmodel.h"
#include "accounts_interface.h"
#include "user_interface.h"

#include <QtGui/QIcon>

#include <KLocalizedString>

AccountModel::AccountModel(QObject* parent): QAbstractListModel(parent)
{
    m_dbus = new OrgFreedesktopAccountsInterface("org.freedesktop.Accounts", "/org/freedesktop/Accounts", QDBusConnection::systemBus(), this);
    QDBusPendingReply <QList <QDBusObjectPath > > reply = m_dbus->ListCachedUsers();
    reply.waitForFinished();

    if (reply.isError()) {
        qDebug() << reply.error().message();
        return;
    }

    QList<QDBusObjectPath> users = reply.value();
    Q_FOREACH(const QDBusObjectPath& path, users) {
        m_userPath.append(path.path());
        m_users.insert(path.path(), new OrgFreedesktopAccountsUserInterface("org.freedesktop.Accounts", path.path(), QDBusConnection::systemBus(), this));
    }
}

AccountModel::~AccountModel()
{

}

int AccountModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_users.count();
}

QVariant AccountModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= m_users.count()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return m_users.value(m_userPath.at(index.row()))->realName();
    }

    if (role == Qt::DecorationRole) {
        QFile file(m_users.value(m_userPath.at(index.row()))->iconFile());
        if (!file.exists()) {
            return QIcon::fromTheme("user-identity");
        }

        return QPixmap(file.fileName()).scaled(48, 48);
    }

    return QVariant();
}

QVariant AccountModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    return i18n("Users");
}

