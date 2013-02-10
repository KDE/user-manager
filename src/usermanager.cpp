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

#include "usermanager.h"
#include "ui_kcm.h"
#include "ui_account.h"
#include "accountinfo.h"

#include "lib/accountmodel.h"
#include "lib/modeltest.h"

#include <QtCore/QDebug>
#include <QtGui/QListView>
#include <QtGui/QStackedLayout>

#include <kpluginfactory.h>

#include <kauth.h>

K_PLUGIN_FACTORY(UserManagerFactory, registerPlugin<UserManager>();)
K_EXPORT_PLUGIN(UserManagerFactory("user_manager", "user_manager"))

UserManager::UserManager(QWidget* parent, const QVariantList& args) 
 : KCModule(UserManagerFactory::componentData(), parent)
 , m_ui(new Ui::KCMUserManager)
 , m_layout(new QStackedLayout)
 , m_model(new AccountModel(this))
 , m_saveNeeded(false)
{
    Q_UNUSED(args);
    m_ui->setupUi(this);
    m_ui->accountInfo->setLayout(m_layout);

    m_selectionModel = new QItemSelectionModel(m_model);
    connect(m_selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), SLOT(currentChanged(QModelIndex,QModelIndex)));
    m_selectionModel->setCurrentIndex(m_model->index(0), QItemSelectionModel::SelectCurrent);

    m_ui->userList->setModel(m_model);
    m_ui->userList->setSelectionModel(m_selectionModel);

    ModelTest* test = new ModelTest(m_model, 0);

    connect(m_ui->addBtn, SIGNAL(clicked(bool)), SLOT(addNewUser()));
}

UserManager::~UserManager()
{
    delete m_model;
}

void UserManager::load()
{
    QList <QModelIndex > modified = m_modifiedAccounts.keys(true);
    Q_FOREACH(const QModelIndex& index, modified) {
        Q_ASSERT(m_accountWidgets.contains(index));
        m_accountWidgets[index]->loadFromModel();
    }
}

void UserManager::save()
{
    if (!m_saveNeeded) {
        return;
    }

    QList <QModelIndex > modified = m_modifiedAccounts.keys(true);
    if (modified.isEmpty()) {
        return;
    }

    KAuth::Action* action = new KAuth::Action(QLatin1String("org.freedesktop.accounts.user-administration"));
    action->setParentWidget(this);
    KAuth::ActionReply reply =  action->execute();

    if (reply.failed()) {
        return;
    }

    Q_FOREACH(const QModelIndex& index, modified) {
        Q_ASSERT(m_accountWidgets.contains(index));
        m_accountWidgets[index]->save();
    }
}

void UserManager::currentChanged(const QModelIndex& selected, const QModelIndex& previous)
{
    if (m_accountWidgets.contains(selected)) {
        m_layout->setCurrentWidget(m_accountWidgets[selected]);
        return;
    }

    AccountInfo* widget = createWidgetForAccount(selected);
    m_accountWidgets.insert(selected, widget);
    m_layout->addWidget(widget);
    m_layout->setCurrentWidget(widget);
}

void UserManager::accountModified(bool modified)
{
    AccountInfo* widget = qobject_cast<AccountInfo*>(sender());
    QModelIndex index = widget->modelIndex();

    if (m_modifiedAccounts.contains(index)) {
        m_modifiedAccounts[index] = modified;
    } else {
        m_modifiedAccounts.insert(index, modified);
    }

    m_saveNeeded = !m_modifiedAccounts.keys(true).isEmpty();
    Q_EMIT changed(m_saveNeeded);
}

void UserManager::addNewUser()
{
    m_selectionModel->setCurrentIndex(m_model->index(m_model->rowCount()-1), QItemSelectionModel::SelectCurrent);
}

AccountInfo* UserManager::createWidgetForAccount(const QModelIndex& selected)
{
    AccountInfo *widget = new AccountInfo(m_model);
    widget->setModelIndex(selected);
    connect(widget, SIGNAL(changed(bool)), SLOT(accountModified(bool)));

    return widget;
}

#include "usermanager.moc"
