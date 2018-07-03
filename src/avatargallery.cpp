/**
 *  Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>
 *  Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>
 *  Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *
 */

#include "avatargallery.h"

#include <QDir>
#include <QPushButton>
#include <QStandardPaths>


#include <QDebug>
AvatarGallery::AvatarGallery(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Change your Face"));

    ui.setupUi(this);

    ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(ui.m_FacesWidget, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current, QListWidgetItem *previous) {
        Q_UNUSED(previous);
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!current->icon().isNull());
    });

    connect(ui.m_FacesWidget, &QListWidget::doubleClicked, this, &AvatarGallery::accept);

    const QStringList &locations = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                             QStringLiteral("user-manager/avatars"),
                                                             QStandardPaths::LocateDirectory);

    if (locations.isEmpty()) {
        return;
    }

    const QString &systemFacesPath = locations.last() + QLatin1Char('/');
    QDir avatarsDir(systemFacesPath);

    foreach(const QString &avatarStyle, avatarsDir.entryList(QDir::Dirs | QDir::NoDotDot)) {
        QDir facesDir = (avatarsDir.filePath(avatarStyle));
        const QStringList &avatarList = facesDir.entryList(QDir::Files);
        for (auto it = avatarList.constBegin(), end = avatarList.constEnd(); it != end; ++it) {
            const QString iconPath = (facesDir.absoluteFilePath(*it));
            auto *item = new QListWidgetItem(QIcon(iconPath), it->section(QLatin1Char('.'), 0, 0), ui.m_FacesWidget);
            item->setData(Qt::UserRole, iconPath);
        }
    }

    resize(420, 400); // FIXME
}

QUrl AvatarGallery::url() const
{
    return QUrl::fromLocalFile(ui.m_FacesWidget->currentItem()->data(Qt::UserRole).toString());
}
