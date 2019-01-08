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

#include <QIcon>

#include "user_manager_debug.h"
#include <KLocalizedString>
#include <kiconloader.h>

#include <KAuth/KAuthActionReply>
#include <KAuth/KAuthExecuteJob>

#include <sys/types.h>
#include <unistd.h>

#include <KConfig>
#include <KConfigGroup>

#define SDDM_CONFIG_FILE    "/etc/sddm.conf"

AutomaticLoginSettings::AutomaticLoginSettings()
{
    KConfig config(QStringLiteral(SDDM_CONFIG_FILE));
    m_autoLoginUser = config.group("Autologin").readEntry("User", QString());
}

QString AutomaticLoginSettings::autoLoginUser() const
{
    return m_autoLoginUser;
}

bool AutomaticLoginSettings::setAutoLoginUser(const QString& username)
{
    KAuth::Action saveAction(QStringLiteral("org.kde.kcontrol.kcmsddm.save"));
    saveAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmsddm"));
    QVariantMap args;

    args[QStringLiteral("sddm.conf")] = QStringLiteral(SDDM_CONFIG_FILE);
    args[QStringLiteral("sddm.conf/Autologin/User")] = username;

    saveAction.setHelperId(QStringLiteral("org.kde.kcontrol.kcmsddm"));
    saveAction.setArguments(args);

    auto job = saveAction.execute();
    if (!job->exec()) {
        qCWarning(USER_MANAGER_LOG) << "fail" << job->errorText();
        return false;
    }

    m_autoLoginUser = username;
    return true;
}

typedef OrgFreedesktopAccountsInterface AccountsManager;
typedef OrgFreedesktopAccountsUserInterface Account;
AccountModel::AccountModel(QObject* parent)
 : QAbstractListModel(parent)
 , m_sessions(new UserSession(this))
{
    m_dbus = new AccountsManager(QStringLiteral("org.freedesktop.Accounts"), QStringLiteral("/org/freedesktop/Accounts"), QDBusConnection::systemBus(), this);
    QDBusPendingReply <QList <QDBusObjectPath > > reply = m_dbus->ListCachedUsers();
    reply.waitForFinished();

    if (reply.isError()) {
        qCDebug(USER_MANAGER_LOG) << reply.error().message();
        return;
    }

    QList<QDBusObjectPath> users = reply.value();
    Q_FOREACH(const QDBusObjectPath& path, users) {
        addAccount(path.path());
    }

    // Adding fake "new user" directly into cache
    addAccountToCache(QStringLiteral("new-user"), nullptr);

    m_kEmailSettings.setProfile(m_kEmailSettings.defaultProfileName());

    connect(m_dbus, &OrgFreedesktopAccountsInterface::UserAdded, this, &AccountModel::UserAdded);
    connect(m_dbus, &OrgFreedesktopAccountsInterface::UserDeleted, this, &AccountModel::UserDeleted);

    connect(m_sessions, &UserSession::userLogged, this, &AccountModel::userLogged);
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
        case Qt::DecorationRole || AccountModel::Face:
        {
            QFile file(acc->iconFile());
            int size = IconSize(KIconLoader::Dialog);
            if (!file.exists()) {
                return QIcon::fromTheme(QStringLiteral("user-identity")).pixmap(size, size);
            }
            return QPixmap(file.fileName()).scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        case AccountModel::RealName:
            return acc->realName();
        case AccountModel::Username:
            return acc->userName();
        case AccountModel::Email:
            return acc->email();
        case AccountModel::Administrator:
            return acc->accountType() == 1;
        case AccountModel::AutomaticLogin: {
            const QString username = index.data(AccountModel::Username).toString();
            return m_autoLoginSettings.autoLoginUser() == username;
        }
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
        return newUserSetData(index, value, role);
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
            m_kEmailSettings.setSetting(KEMailSettings::RealName, value.toString());

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
            m_kEmailSettings.setSetting(KEMailSettings::EmailAddress, value.toString());

            emit dataChanged(index, index);
            return true;
        case AccountModel::Administrator:
            if (checkForErrors(acc->SetAccountType(value.toBool() ? 1 : 0))) {
                return false;
            }

            emit dataChanged(index, index);
            return true;
        case AccountModel::AutomaticLogin:
        {
            const bool autoLoginSet = value.toBool();
            const QString username = index.data(AccountModel::Username).toString();

            //if the checkbox is set and the SDDM config is not already us, set it to us
            //all rows need updating as we may have unset it from someone else.
            if (autoLoginSet && m_autoLoginSettings.autoLoginUser() != username) {
                if (m_autoLoginSettings.setAutoLoginUser(username)) {
                    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), 0));
                    return true;
                }
                return false;
            }
            //if the checkbox is not set and the SDDM config is set to us, then clear it
            else if (!autoLoginSet && m_autoLoginSettings.autoLoginUser() == username) {
                if (m_autoLoginSettings.setAutoLoginUser(QString())) {
                    emit dataChanged(index, index);
                    return true;
                }
                return false;
            }
            return true;
        }
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
            return QIcon::fromTheme(QStringLiteral("list-add-user")).pixmap(IconSize(KIconLoader::Dialog), IconSize(KIconLoader::Dialog));
        case AccountModel::Created:
            return false;
    }
    return QVariant();
}

bool AccountModel::newUserSetData(const QModelIndex &index, const QVariant& value, int roleInt)
{
    AccountModel::Role role = static_cast<AccountModel::Role>(roleInt);
    m_newUserData[role] = value;
    QList<AccountModel::Role> roles = m_newUserData.keys();
    if (!roles.contains(Username) || !roles.contains(RealName) || !roles.contains(Administrator)) {
        return true;
    }


    int userType = m_newUserData[Administrator].toBool() ? 1 : 0;
    QDBusPendingReply <QDBusObjectPath > reply = m_dbus->CreateUser(m_newUserData[Username].toString(), m_newUserData[RealName].toString(), userType);
    reply.waitForFinished();

    if (reply.isError()) {
        qCDebug(USER_MANAGER_LOG) << reply.error().name();
        qCDebug(USER_MANAGER_LOG) << reply.error().message();
        m_newUserData.clear();
        return false;
    }

    m_newUserData.remove(Username);
    m_newUserData.remove(RealName);
    m_newUserData.remove(Administrator);

    //If we don't have anything else to set just return
    if (m_newUserData.isEmpty()) {
        return true;
    }

    UserAdded(reply.value());

    QHash<AccountModel::Role, QVariant>::const_iterator i = m_newUserData.constBegin();
    while (i != m_newUserData.constEnd()) {
        qCDebug(USER_MANAGER_LOG) << "Setting extra:" << i.key() << "with value:" << i.value();
        setData(index, i.value(), i.key());
        ++i;
    }

    m_newUserData.clear();

    return true;
}

void AccountModel::addAccount(const QString& path)
{
    Account *acc = new Account(QStringLiteral("org.freedesktop.Accounts"), path, QDBusConnection::systemBus(), this);
    qulonglong uid = acc->uid();
    if (!acc->isValid() || acc->lastError().isValid() || acc->systemAccount()) {
        return;
    }

    connect(acc, &OrgFreedesktopAccountsUserInterface::Changed, this, &AccountModel::Changed);
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

void AccountModel::replaceAccount(const QString &path, OrgFreedesktopAccountsUserInterface *acc, int pos)
{
    if (pos >= m_userPath.size() || pos < 0) {
        return;
    }
    m_userPath.replace(pos, path);

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
        qCDebug(USER_MANAGER_LOG) << reply.error().name();
        qCDebug(USER_MANAGER_LOG) << reply.error().message();
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

void AccountModel::UserAdded(const QDBusObjectPath& dbusPath)
{
    QString path = dbusPath.path();
    if (m_userPath.contains(path)) {
        qCDebug(USER_MANAGER_LOG) << "We already have:" << path;
        return;
    }

    Account* acc = new Account(QStringLiteral("org.freedesktop.Accounts"), path, QDBusConnection::systemBus(), this);
    if (acc->systemAccount()) {
        return;
    }
    connect(acc, &OrgFreedesktopAccountsUserInterface::Changed, this, &AccountModel::Changed);

    // First, we modify "new-user" to become the new created user
    int row = rowCount();
    replaceAccount(path, acc, row - 1);
    QModelIndex changedIndex = index(row - 1, 0);
    emit dataChanged(changedIndex, changedIndex);

    // Then we add new-user again.
    beginInsertRows(QModelIndex(), row, row);
    addAccountToCache(QStringLiteral("new-user"), nullptr);
    endInsertRows();
}

void AccountModel::UserDeleted(const QDBusObjectPath& path)
{
    if (!m_userPath.contains(path.path())) {
        qCDebug(USER_MANAGER_LOG) << "User Deleted but not found: " << path.path();
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

    return QString::fromUtf8(crypt(password.toUtf8().constData(), salt.constData()));
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


