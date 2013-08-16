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
#include "createavatarjob.h"
#include "passworddialog.h"
#include "lib/accountmodel.h"

#include <QtGui/QMenu>
#include <QtGui/QToolButton>
#include <QtGui/QDesktopServices>

#include <KDebug>
#include <KImageIO>
#include <KFileDialog>
#include <KImageFilePreview>
#include <KPixmapRegionSelectorDialog>
#include <KIO/Job>
#include <kio/copyjob.h>
#include <KTemporaryFile>
#include <KGlobalSettings>

AccountInfo::AccountInfo(AccountModel* model, QWidget* parent, Qt::WindowFlags f)
 : QWidget(parent, f)
 , m_info(new Ui::AccountInfo())
 , m_model(model)
{
    m_info->setupUi(this);
    connect(m_info->username, SIGNAL(textEdited(QString)), SLOT(hasChanged()));
    connect(m_info->realName, SIGNAL(textEdited(QString)), SLOT(hasChanged()));
    connect(m_info->email, SIGNAL(textEdited(QString)), SLOT(hasChanged()));
    connect(m_info->administrator, SIGNAL(clicked(bool)), SLOT(hasChanged()));
    connect(m_info->automaticLogin, SIGNAL(clicked(bool)), SLOT(hasChanged()));
    connect(m_info->changePassword, SIGNAL(clicked(bool)), SLOT(changePassword()));

    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(dataChanged(QModelIndex)));
    m_info->face->setPopupMode(QToolButton::InstantPopup);
    QMenu* menu = new QMenu(this);

    QAction *openAvatar = new QAction(i18n("Load from file..."), this);
    openAvatar->setIcon(QIcon::fromTheme(QLatin1String("document-open-folder")));
    connect(openAvatar, SIGNAL(triggered(bool)), SLOT(openAvatarSlot()));

    QAction *editClear = new QAction(i18n("Clear Avatar"), this);
    editClear->setIcon(QIcon::fromTheme(QLatin1String("edit-clear")));
    connect(editClear, SIGNAL(triggered(bool)), SLOT(clearAvatar()));

    menu->addAction(openAvatar);
    menu->addAction(editClear);

    m_info->face->setMenu(menu);

    int size = QFontMetrics(KGlobalSettings::fixedFont()).xHeight();
    m_info->username->setMinimumWidth(size * 29);
    m_info->realName->setMinimumWidth(size * 29);
    m_info->email->setMinimumWidth(size * 29);
}

AccountInfo::~AccountInfo()
{
    delete m_info;
}

void AccountInfo::setModelIndex(const QModelIndex& index)
{
    if (!index.isValid() || m_index == index) {
        return;
    }

    m_index = index;
    m_infoToSave.clear();
    loadFromModel();
}

QModelIndex AccountInfo::modelIndex() const
{
    return m_index;
}

void AccountInfo::loadFromModel()
{
    m_info->username->setText(m_model->data(m_index, AccountModel::Username).toString());
    m_info->face->setIconSize(QSize(32,32));
    m_info->face->setIcon(QIcon(m_model->data(m_index, AccountModel::Face).value<QPixmap>()));
    m_info->realName->setText(m_model->data(m_index, AccountModel::RealName).toString());
    m_info->email->setText(m_model->data(m_index, AccountModel::Email).toString());
    m_info->administrator->setChecked(m_model->data(m_index, AccountModel::Administrator).toBool());
    m_info->automaticLogin->setChecked(m_model->data(m_index, AccountModel::AutomaticLogin).toBool());
    m_info->changePassword->setEnabled(m_model->data(m_index, AccountModel::Created).toBool());
}

bool AccountInfo::save()
{
    if (m_infoToSave.isEmpty()) {
        return false;
    }

    kDebug() << "Saving on Index: " << m_index.row();
    if (!m_model->setData(m_index, m_info->username->text(), AccountModel::Username)) {
        return false;
    }
    if (!m_model->setData(m_index, m_info->realName->text(), AccountModel::RealName)) {
        return false;
    }
    if (!m_model->setData(m_index, m_info->email->text(), AccountModel::Email)) {
        return false;
    }
    if (!m_model->setData(m_index, m_info->administrator->isChecked(), AccountModel::Administrator)) {
        return false;
    }
    if (!m_model->setData(m_index, m_info->automaticLogin->isChecked(), AccountModel::AutomaticLogin)) {
        return false;
    }
    if (m_infoToSave.contains(AccountModel::Password)) {
        if (!m_model->setData(m_index, m_infoToSave[AccountModel::Password], AccountModel::Password)) {
            return false;
        }
    }
    if (m_infoToSave.contains(AccountModel::Face)) {
        const QString path = m_infoToSave[AccountModel::Face].toString();
        QString faceFile = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
        faceFile.append(QLatin1String("/.face"));

        QFile::remove(faceFile);
        KIO::CopyJob* moveJob = KIO::move(KUrl(path), KUrl(faceFile), KIO::HideProgressInfo);
        connect(moveJob, SIGNAL(finished(KJob*)), SLOT(avatarModelChanged()));
        moveJob->setUiDelegate(0);
        moveJob->start();
    }

    m_infoToSave.clear();
    return true;
}

void AccountInfo::hasChanged()
{

    QMap<AccountModel::Role, QVariant> infoToSave;
    if (m_info->realName->text() != m_model->data(m_index, AccountModel::RealName).toString()) {
        infoToSave.insert(AccountModel::RealName, m_info->realName->text());
    }

    if (m_info->username->text() != m_model->data(m_index, AccountModel::Username).toString()) {
        infoToSave.insert(AccountModel::Username, m_info->username->text());
    }

    if (m_info->email->text() != m_model->data(m_index, AccountModel::Email).toString()) {
        infoToSave.insert(AccountModel::Email, m_info->email->text());
    }

    if (m_info->administrator->isChecked() != m_model->data(m_index, AccountModel::Administrator).toBool()) {
        infoToSave.insert(AccountModel::Administrator, m_info->administrator->isChecked());
    }

    if (m_info->automaticLogin->isChecked() != m_model->data(m_index, AccountModel::AutomaticLogin).toBool()) {
        infoToSave.insert(AccountModel::AutomaticLogin, m_info->automaticLogin->isChecked());
    }

    if (m_infoToSave.contains(AccountModel::Face)) {
        infoToSave[AccountModel::Face] = m_infoToSave[AccountModel::Face];
    }

    m_infoToSave = infoToSave;
    Q_EMIT changed(!m_infoToSave.isEmpty());
}

void AccountInfo::dataChanged(const QModelIndex& index)
{
    if (m_index != index) {
        return;
    }

    hasChanged();
}

void AccountInfo::openAvatarSlot()
{
    KFileDialog dlg(QDir::homePath(), KImageIO::pattern(KImageIO::Reading), this);

    dlg.setOperationMode(KFileDialog::Opening);
    dlg.setCaption(i18nc("@title:window", "Choose Image"));
    dlg.setMode(KFile::File);

    KImageFilePreview *imagePreviewer = new KImageFilePreview(&dlg);
    dlg.setPreviewWidget(imagePreviewer);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    KUrl url(dlg.selectedFile());
    CreateAvatarJob *job = new CreateAvatarJob(this);
    connect(job, SIGNAL(finished(KJob*)), SLOT(avatarCreated(KJob*)));
    job->setUrl(url);
    job->start();
}

void AccountInfo::avatarCreated(KJob* job)
{
    kDebug() << "Avatar created";
    CreateAvatarJob *aJob = qobject_cast<CreateAvatarJob*>(job);
    m_info->face->setIcon(QIcon(aJob->avatarPath()));
    m_infoToSave.insert(AccountModel::Face, aJob->avatarPath());
    Q_EMIT changed(true);
}

void AccountInfo::avatarModelChanged()
{
    m_model->setData(m_index, QVariant(), AccountModel::Face);
    m_info->face->setIcon(QIcon(m_model->data(m_index, AccountModel::Face).value<QPixmap>()));
}

void AccountInfo::clearAvatar()
{
    m_info->face->setIcon(QIcon::fromTheme("user-identity").pixmap(48, 48));
    m_infoToSave.insert(AccountModel::Face, QString());
    Q_EMIT changed(true);
}

void AccountInfo::changePassword()
{
    QScopedPointer<PasswordDialog> dialog(new PasswordDialog(this));
    dialog->setUsername(m_model->data(m_index, AccountModel::Username).toByteArray());
    dialog->setModal(true);
    if (!dialog->exec()) {
        return;
    }

    m_infoToSave[AccountModel::Password] = dialog->password();
    Q_EMIT changed(true);
}