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

#include "accountinfo.h"
#include "ui_account.h"
#include "lib/accountmodel.h"

#include <QtCore/QDebug>

AccountInfo::AccountInfo(AccountModel* model, QWidget* parent, Qt::WindowFlags f)
 : QWidget(parent, f)
 , m_info(new Ui::AccountInfo())
 , m_model(model)
{
    m_info->setupUi(this);
    connect(m_info->username, SIGNAL(textChanged(QString)), SLOT(hasChanged()));
    connect(m_info->realName, SIGNAL(textChanged(QString)), SLOT(hasChanged()));
    connect(m_info->email, SIGNAL(textChanged(QString)), SLOT(hasChanged()));
    connect(m_info->administrator, SIGNAL(toggled(bool)), SLOT(hasChanged()));
    connect(m_info->automaticLogin, SIGNAL(toggled(bool)), SLOT(hasChanged()));
}

AccountInfo::~AccountInfo()
{
    delete m_info;
}

void AccountInfo::setModelIndex(const QModelIndex& index)
{
    m_index = index;

    m_info->username->setText(m_model->data(m_index, AccountModel::Username).toString());
    m_info->face->setIconSize(QSize(32,32));
    m_info->face->setIcon(QIcon(m_model->data(m_index, AccountModel::Face).value<QPixmap>()));
    m_info->realName->setText(m_model->data(m_index, AccountModel::RealName).toString());
    m_info->email->setText(m_model->data(m_index, AccountModel::Email).toString());
    m_info->administrator->setChecked(m_model->data(m_index, AccountModel::Administrator).toBool());
    m_info->automaticLogin->setChecked(m_model->data(m_index, AccountModel::AutomaticLogin).toBool());
}

void AccountInfo::hasChanged()
{
    if (m_info->realName->text() != m_model->data(m_index, AccountModel::RealName).toString()) {
        Q_EMIT changed(true);
        return;
    }

    if (m_info->username->text() != m_model->data(m_index, AccountModel::Username).toString()) {
        Q_EMIT changed(true);
        return;
    }

    if (m_info->email->text() != m_model->data(m_index, AccountModel::Email).toString()) {
        Q_EMIT changed(true);
        return;
    }

    if (m_info->administrator->isChecked() != m_model->data(m_index, AccountModel::Administrator).toBool()) {
        Q_EMIT changed(true);
        return;
    }

    if (m_info->automaticLogin->isChecked() != m_model->data(m_index, AccountModel::AutomaticLogin).toBool()) {
        Q_EMIT changed(true);
        return;
    }

    Q_EMIT changed(false);
}