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
#include <KLocalizedString>
#include <KMessageBox>

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
    connect(m_widget, SIGNAL(changed(bool)), SIGNAL(changed(bool)));
}

UserManager::~UserManager()
{
    delete m_model;
}

void UserManager::load()
{
    m_widget->loadFromModel();
}

void UserManager::save()
{
    m_widget->save();
}

void UserManager::currentChanged(const QModelIndex& selected, const QModelIndex& previous)
{
    m_widget->setModelIndex(selected);
    m_ui->removeBtn->setEnabled(selected.row() < m_model->rowCount() - 1);
}

void UserManager::addNewUser()
{
    m_selectionModel->setCurrentIndex(m_model->index(m_model->rowCount()-1), QItemSelectionModel::SelectCurrent);
}

void UserManager::removeUser()
{
    KGuiItem keep;
    keep.setText(i18n("Delete files"));
    KGuiItem deletefiles;
    deletefiles.setText(i18n("Keep files"));

    int result = KMessageBox::questionYesNoCancel(this, i18n("Are you should you want to "), i18n("AAA"), keep, deletefiles);
    if (result == KMessageBox::Cancel) {
        return;
    }

    bool deleteFiles  = result == KMessageBox::Yes ? true : false;
    m_model->removeAccountKeepingFiles(m_selectionModel->currentIndex().row(), deleteFiles);

    Q_EMIT changed(false);
}

#include "usermanager.moc"
