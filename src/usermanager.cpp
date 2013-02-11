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
#include <QtGui/QVBoxLayout>

#include <kpluginfactory.h>

#include <kauth.h>

K_PLUGIN_FACTORY(UserManagerFactory, registerPlugin<UserManager>();)
K_EXPORT_PLUGIN(UserManagerFactory("user_manager", "user_manager"))

UserManager::UserManager(QWidget* parent, const QVariantList& args) 
 : KCModule(UserManagerFactory::componentData(), parent)
 , m_saveNeeded(false)
 , m_model(new AccountModel(this))
 , m_widget(new AccountInfo(m_model, this))
 , m_ui(new Ui::KCMUserManager)
{
    Q_UNUSED(args);
    QVBoxLayout *layout = new QVBoxLayout();
    m_ui->setupUi(this);
    m_ui->accountInfo->setLayout(layout);
    layout->addWidget(m_widget);

    m_selectionModel = new QItemSelectionModel(m_model);
    connect(m_selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), SLOT(currentChanged(QModelIndex,QModelIndex)));
    m_selectionModel->setCurrentIndex(m_model->index(0), QItemSelectionModel::SelectCurrent);

    m_ui->userList->setModel(m_model);
    m_ui->userList->setSelectionModel(m_selectionModel);

    ModelTest* test = new ModelTest(m_model, 0);
    Q_UNUSED(test)

    connect(m_ui->addBtn, SIGNAL(clicked(bool)), SLOT(addNewUser()));
    connect(m_ui->removeBtn, SIGNAL(clicked(bool)), SLOT(removeUser()));
}

UserManager::~UserManager()
{
    delete m_model;
}

void UserManager::load()
{
//     QList <QModelIndex > modified = m_modifiedAccounts.keys(true);
//     Q_FOREACH(const QModelIndex& index, modified) {
//         m_accountWidgets[index.row()]->loadFromModel();
//     }
}

void UserManager::save()
{
//     if (!m_saveNeeded) {
//         return;
//     }
//
//     QList <QModelIndex > modified = m_modifiedAccounts.keys(true);
//     if (modified.isEmpty()) {
//         return;
//     }
//
//     KAuth::Action* action = new KAuth::Action(QLatin1String("org.freedesktop.accounts.user-administration"));
//     action->setParentWidget(this);
//     KAuth::ActionReply reply =  action->execute();
//
//     if (reply.failed()) {
//         return;
//     }
//
//     Q_FOREACH(const QModelIndex& index, modified) {
//         qDebug() << "Saving: " << index.row();
//         m_accountWidgets[index.row()]->save();
//         m_selectionModel->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
//         m_modifiedAccounts.remove(index);
//     }
}

void UserManager::currentChanged(const QModelIndex& selected, const QModelIndex& previous)
{
    m_cachedInfo.clear();
    if (m_widget->hasChanges()) {
        m_cachedInfo = m_widget->changes();
    }
    m_widget->setModelIndex(selected);
}

void UserManager::accountModified(bool modified)
{
//     AccountInfo* widget = qobject_cast<AccountInfo*>(sender());
//     QModelIndex index = widget->modelIndex();
//
//     m_modifiedAccounts[index] = modified;
//
//     m_saveNeeded = !m_modifiedAccounts.keys(true).isEmpty();
//     Q_EMIT changed(m_saveNeeded);
}

void UserManager::addNewUser()
{
    m_selectionModel->setCurrentIndex(m_model->index(m_model->rowCount()-1), QItemSelectionModel::SelectCurrent);
}

void UserManager::removeUser()
{
//     QModelIndex selected = m_selectionModel->currentIndex();
//     qDebug() << "Removing user: " << selected.row();
//
//     m_model->removeRow(selected.row());
//     delete m_accountWidgets[selected.row()];
//     m_accountWidgets.removeAt(selected.row());
//     m_modifiedAccounts.remove(selected);
//
//     m_saveNeeded = !m_modifiedAccounts.keys(true).isEmpty();
//     Q_EMIT changed(m_saveNeeded);
}

#include "usermanager.moc"
