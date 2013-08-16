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

#include <pwquality.h>

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
    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(dataChanged(QModelIndex,QModelIndex)));
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
    Q_UNUSED(previous);
    m_widget->setModelIndex(selected);
    bool enabled = false;

    //If it is not last and not first
    if (selected.row() < m_model->rowCount() - 1 && selected.row() > 0) {
        enabled = true;
    }

    m_ui->removeBtn->setEnabled(enabled);
    m_selectionModel->setCurrentIndex(selected, QItemSelectionModel::SelectCurrent);
}

void UserManager::dataChanged(const QModelIndex& topLeft, const QModelIndex& topRight)
{
    Q_UNUSED(topRight);
    if (m_selectionModel->currentIndex() != topLeft) {
        return;
    }

    currentChanged(topLeft, topLeft);
}

void UserManager::addNewUser()
{
    m_selectionModel->setCurrentIndex(m_model->index(m_model->rowCount()-1), QItemSelectionModel::SelectCurrent);
}

void UserManager::removeUser()
{
    QModelIndex index = m_selectionModel->currentIndex();

    KGuiItem keep;
    keep.setText(i18n("Keep files"));
    KGuiItem deletefiles;
    deletefiles.setText(i18n("Delete files"));

    QString warning = i18n("What do you want to do after deleting %1 ?", m_model->data(index, AccountModel::FriendlyName).toString());
    if (!m_model->data(index, AccountModel::Logged).toBool()) {
        warning.append("\n\n");
        warning.append(i18n("This user is using the system right now, removing it will cause problems"));
    }

    int result = KMessageBox::questionYesNoCancel(this, warning, i18n("Delete User"), keep, deletefiles);
    if (result == KMessageBox::Cancel) {
        return;
    }

    bool deleteFiles  = result == KMessageBox::Yes ? false : true;
    m_model->removeAccountKeepingFiles(index.row(), deleteFiles);

    Q_EMIT changed(false);
}

#include "usermanager.moc"
