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
}

AccountInfo::~AccountInfo()
{

}

void AccountInfo::setModelIndex(const QModelIndex& index)
{
    m_index = index;

    qDebug() << m_model->data(m_index, Qt::DisplayRole).toString();
}
