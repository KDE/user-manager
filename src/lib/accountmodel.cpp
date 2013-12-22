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
#include "usersessions.h"

#include "accounts_interface.h"
#include "user_interface.h"

#include <QtGui/QIcon>

#include <KDebug>
#include <KLocalizedString>
#include <kiconloader.h>

#include <sys/types.h>
#include <unistd.h>

typedef OrgFreedesktopAccountsInterface AccountsManager;
typedef OrgFreedesktopAccountsUserInterface Account;
AccountModel::AccountModel(QObject* parent)
 : QAbstractListModel(parent)
 , m_sessions(new UserSession(this))
{
    m_dbus = new AccountsManager("org.freedesktop.Accounts", "/org/freedesktop/Accounts", QDBusConnection::systemBus(), this);
    QDBusPendingReply <QList <QDBusObjectPath > > reply = m_dbus->ListCachedUsers();
    reply.waitForFinished();

    if (reply.isError()) {
        kDebug() << reply.error().message();
        return;
    }

    QList<QDBusObjectPath> users = reply.value();
    Q_FOREACH(const QDBusObjectPath& path, users) {
        addAccount(path.path());
    }

    //Adding fake "new user" directly into cache
    addAccountToCache("new-user", 0);

    connect(m_dbus, SIGNAL(UserAdded(QDBusObjectPath)), SLOT(UserAdded(QDBusObjectPath)));
    connect(m_dbus, SIGNAL(UserDeleted(QDBusObjectPath)), SLOT(UserDeleted(QDBusObjectPath)));

    connect(m_sessions, SIGNAL(userLogged(uint, bool)), SLOT(userLogged(uint, bool)));
}

AccountModel::~AccountModel()
{
    delete m_dbus;
    qDeleteAll(m_users);
}

int AccountModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_userPath.count();
}

QVariant AccountModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= m_users.count()) {
        return QVariant();
    }

    QString path = m_userPath.at(index.row());
    Account* acc = m_users.value(path);
    if (!acc) {
        //new user
        return newUserData(role);
    }

    switch(role) {
        case Qt::DisplayRole || AccountModel::FriendlyName:
            if (!acc->realName().isEmpty()) {
                return acc->realName();
            }
            return acc->userName();
        case Qt::DecorationRole || AccountModel::FriendlyName:
        {
            QFile file(acc->iconFile());
            int size = IconSize(KIconLoader::Dialog);
            if (!file.exists()) {
                return QIcon::fromTheme("user-identity").pixmap(size, size);
            }
            return QPixmap(file.fileName()).scaled(size, size);
        }
        case AccountModel::RealName:
            return acc->realName();
        case AccountModel::Username:
            return acc->userName();
        case AccountModel::Email:
            return acc->email();
        case AccountModel::Administrator:
            return acc->accountType() == 1;
        case AccountModel::AutomaticLogin:
            return acc->automaticLogin();
        case AccountModel::Logged:
            if (m_loggedAccounts.contains(path)) {
                return m_loggedAccounts[path];
            }
            return QVariant();
        case AccountModel::Created:
            return true;
    }

    return QVariant();
}

bool AccountModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid()) {
        return false;
    }

    if (index.row() >= m_users.count()) {
        return false;
    }

    QString path = m_userPath.at(index.row());
    Account* acc = m_users.value(path);
    if (!acc) {
        return newUserSetData(value, role);
    }

    switch(role) {
        //The modification of the face file should be done outside
        case AccountModel::Face:
            if (checkForErrors(acc->SetIconFile(value.toString()))) {
                return false;
            }
            emit dataChanged(index, index);
            return true;
        case AccountModel::RealName:
            if (checkForErrors(acc->SetRealName(value.toString()))) {
                return false;
            }

            m_dbus->UncacheUser(acc->userName()).waitForFinished();
            m_dbus->CacheUser(acc->userName());
            emit dataChanged(index, index);
            return true;
        case AccountModel::Username:
            if (checkForErrors(acc->SetUserName(value.toString()))) {
                return false;
            }

            emit dataChanged(index, index);
            return true;
        case AccountModel::Password:
            if (checkForErrors(acc->SetPassword(cryptPassword(value.toString()), QString()))) {
                return false;
            }

            emit dataChanged(index, index);
            return true;
        case AccountModel::Email:
            if (checkForErrors(acc->SetEmail(value.toString()))) {
                return false;
            }

            emit dataChanged(index, index);
            return true;
        case AccountModel::Administrator:
            if (checkForErrors(acc->SetAccountType(value.toBool() ? 1 : 0))) {
                return false;
            }

            emit dataChanged(index, index);
            return true;
        case AccountModel::AutomaticLogin:
            if (checkForErrors(acc->SetAutomaticLogin(value.toBool()))) {
                return false;
            }

            emit dataChanged(index, index);
            return true;
        case AccountModel::Logged:
            m_loggedAccounts[path] = value.toBool();
            emit dataChanged(index, index);
            return true;
        case AccountModel::Created:
            qFatal("AccountModel NewAccount should never be set");
            return false;

    }

    return QAbstractItemModel::setData(index, value, role);
}

bool AccountModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_UNUSED(count);
    Q_UNUSED(parent);
    return removeAccountKeepingFiles(row, true);
}

bool AccountModel::removeAccountKeepingFiles(int row, bool keepFile)
{
    Account* acc = m_users.value(m_userPath.at(row));
    QDBusPendingReply <void > rep = m_dbus->DeleteUser(acc->uid(), keepFile);
    rep.waitForFinished();

    return !rep.isError();
}

QVariant AccountModel::newUserData(int role) const
{
    switch(role) {
        case Qt::DisplayRole || AccountModel::FriendlyName:
            return i18n("New User");
        case Qt::DecorationRole || AccountModel::Face:
            return QIcon::fromTheme("list-add-user").pixmap(IconSize(KIconLoader::Dialog), IconSize(KIconLoader::Dialog));
        case AccountModel::Created:
            return false;
    }
    return QVariant();
}

bool AccountModel::newUserSetData(const QVariant& value, int roleInt)
{
    AccountModel::Role role = (AccountModel::Role) roleInt;
    m_newUserData[role] = value;
    if (m_newUserData.count() < 5) {
        return true;
    }

    int userType = value.toBool() ? 1 : 0;
    QDBusPendingReply <QDBusObjectPath > reply = m_dbus->CreateUser(m_newUserData[Username].toString(), m_newUserData[RealName].toString(), userType);
    reply.waitForFinished();

    if (reply.isError()) {
        kDebug() << reply.error().name();
        kDebug() << reply.error().message();
        return false;
    }

    Account *acc = new Account("org.freedesktop.Accounts", reply.value().path(), QDBusConnection::systemBus(), this);
    acc->SetAutomaticLogin(m_newUserData[AutomaticLogin].toBool());
    acc->SetEmail(m_newUserData[Email].toString());
    acc->deleteLater();

    m_newUserData.clear();

    return true;
}

void AccountModel::addAccount(const QString& path)
{
    Account *acc = new Account("org.freedesktop.Accounts", path, QDBusConnection::systemBus(), this);
    qulonglong uid = acc->uid();
    if (!acc->isValid() || acc->lastError().isValid() || acc->systemAccount()) {
        return;
    }

    connect(acc, SIGNAL(Changed()), SLOT(Changed()));
    if (uid == getuid()) {
        addAccountToCache(path, acc, 0);
        return;
    }

    addAccountToCache(path, acc);
}

void AccountModel::addAccountToCache(const QString& path, Account* acc, int pos)
{
    if (pos > -1) {
        m_userPath.insert(pos, path);
    } else {
        m_userPath.append(path);
    }

    m_users.insert(path, acc);
    m_loggedAccounts[path] = false;
}


void AccountModel::removeAccount(const QString& path)
{
    m_userPath.removeAll(path);
    delete m_users.take(path);
    m_loggedAccounts.remove(path);
}

bool AccountModel::checkForErrors(QDBusPendingReply<void> reply) const
{
    reply.waitForFinished();
    if (reply.isError()) {
        kDebug() << reply.error().name();
        kDebug() << reply.error().message();
        return true;
    }

    return false;
}

QVariant AccountModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    return i18n("Users");
}

void AccountModel::UserAdded(const QDBusObjectPath& path)
{
    Account* acc = new Account("org.freedesktop.Accounts", path.path(), QDBusConnection::systemBus(), this);
    if (acc->systemAccount()) {
        return;
    }
    connect(acc, SIGNAL(Changed()), SLOT(Changed()));

    //We are adding one row in the bottom but we are updating the information
    //of the just added user, which will be the penultimate row since
    //"New User" is always the row in the bottom.
    //TODO: Move "New User" to a Proxy Model
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    addAccountToCache(path.path(), acc, row - 1);
    endInsertRows();

    //Notify that the penultimate row has been modified, so new data (from the
    //freshly added account is updated
    QModelIndex changedIndex = index(row - 1, 0);
    emit dataChanged(changedIndex, changedIndex);
}

void AccountModel::UserDeleted(const QDBusObjectPath& path)
{
    if (!m_userPath.contains(path.path())) {
        kDebug() << "User Deleted but not found: " << path.path();
        return;
    }

    beginRemoveRows(QModelIndex(), m_userPath.indexOf(path.path()), m_userPath.indexOf(path.path()));
    removeAccount(path.path());
    endRemoveRows();
}

void AccountModel::Changed()
{
    Account* acc = qobject_cast<Account*>(sender());
    acc->path();

    QModelIndex accountIndex = index(m_userPath.indexOf(acc->path()), 0);
    Q_EMIT dataChanged(accountIndex, accountIndex);
}

void AccountModel::userLogged(uint uid, bool logged)
{
    QString path = accountPathForUid(uid);
    int row = m_userPath.indexOf(path);

    setData(index(row), logged, Logged);
}

const QString AccountModel::accountPathForUid(uint uid) const
{
    QHash<QString, Account*>::ConstIterator i;
    for (i = m_users.constBegin(); i != m_users.constEnd(); ++i) {
        if (i.value() && i.value()->uid() == uid) {
            return i.key();
        }
    }

    return QString();
}

QString AccountModel::cryptPassword(const QString& password) const
{
    QString cryptedPassword;
    QByteArray alpha = "0123456789ABCDEFGHIJKLMNOPQRSTUVXYZ"
                       "abcdefghijklmnopqrstuvxyz./";
    QByteArray salt("$6$");//sha512
    int len = alpha.count();
    for(int i = 0; i < 16; i++){
        salt.append(alpha.at((qrand() % len)));
    }

    return crypt(password.toUtf8(), salt);
}

QDebug operator<<(QDebug debug, AccountModel::Role role)
{
    switch(role) {
        case AccountModel::FriendlyName:
            debug << "AccountModel::FriendlyName";
            break;
        case AccountModel::Face:
            debug << "AccountModel::Face";
            break;
        case AccountModel::RealName:
            debug << "AccountModel::RealName";
            break;
        case AccountModel::Username:
            debug << "AccountModel::Username";
            break;
        case AccountModel::Password:
            debug << "AccountModel::Password";
            break;
        case AccountModel::Email:
            debug << "AccountModel::Email";
            break;
        case AccountModel::Administrator:
            debug << "AccountModel::Administrator";
            break;
        case AccountModel::AutomaticLogin:
            debug << "AccountModel::AutomaticLogin";
            break;
        case AccountModel::Logged:
            debug << "AccountModel::Logged";
            break;
        case AccountModel::Created:
            debug << "AccountModel::Created";
            break;
    }
    return debug;
}