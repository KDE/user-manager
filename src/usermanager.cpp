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

K_PLUGIN_FACTORY(UserManagerFactory, registerPlugin<UserManager>();)
K_EXPORT_PLUGIN(UserManagerFactory("user_manager", "user_manager"))

UserManager::UserManager(QWidget* parent, const QVariantList& args) 
 : KCModule(UserManagerFactory::componentData(), parent)
 , m_ui(new Ui::KCMUserManager)
 , m_layout(new QStackedLayout)
{
    Q_UNUSED(args);
    m_ui->setupUi(this);
    m_ui->accountInfo->setLayout(m_layout);

    AccountModel* model = new AccountModel(this);
    QItemSelectionModel* selectionModel = new QItemSelectionModel(model);
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), SLOT(currentChanged(QModelIndex,QModelIndex)));

    m_ui->userList->setModel(model);
    m_ui->userList->setSelectionModel(selectionModel);

    ModelTest* test = new ModelTest(model, 0);

    AccountInfo *widget = new AccountInfo(model);

    m_layout->addWidget(widget);
    m_layout->setCurrentWidget(widget);
}

UserManager::~UserManager()
{
}

void UserManager::currentChanged(const QModelIndex& selected, const QModelIndex& previous)
{

}

#include "usermanager.moc"
