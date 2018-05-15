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

#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "lib/accountmodel.h"

#include <KCModule>

namespace Ui
{
    class KCMUserManager;
};

class QModelIndex;
class AccountInfo;
class QItemSelection;
class QStackedLayout;
class KMessageWidget;
class QItemSelectionModel;
class UserManager : public KCModule
{
    Q_OBJECT
    public:
        explicit UserManager(QWidget *parent, const QVariantList& args);
        ~UserManager() Q_DECL_OVERRIDE;

        void load() Q_DECL_OVERRIDE;
        void save() Q_DECL_OVERRIDE;

    public Q_SLOTS:
        void currentChanged(const QModelIndex &selected, const QModelIndex &previous);
        void dataChanged(const QModelIndex &topLeft ,const QModelIndex &topRight);
        void addNewUser();
        void removeUser();

    private:
        bool m_saveNeeded;
        AccountModel* m_model;
        AccountInfo* m_widget;
        Ui::KCMUserManager* m_ui;
        QItemSelectionModel* m_selectionModel;
        QMap<AccountModel::Role, QVariant> m_cachedInfo;
};

#endif // USER-MANAGER_H_
