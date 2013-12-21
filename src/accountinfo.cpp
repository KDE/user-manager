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
#include "passwordedit.h"

#include <pwd.h>
#include <utmp.h>

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

#define MAX_USER_LEN  (UT_NAMESIZE - 1)

AccountInfo::AccountInfo(AccountModel* model, QWidget* parent, Qt::WindowFlags f)
 : QWidget(parent, f)
 , m_info(new Ui::AccountInfo())
 , m_model(model)
 , m_passwordEdit(new PasswordEdit(this))
{
    m_info->setupUi(this);
    //If I remove this from the .ui file the layouting gets screwed...
    m_info->formLayout->removeWidget(m_info->passwordEdit);
    delete m_info->passwordEdit;

    connect(m_info->username, SIGNAL(textEdited(QString)), SLOT(hasChanged()));
    connect(m_info->realName, SIGNAL(textEdited(QString)), SLOT(hasChanged()));
    connect(m_info->email, SIGNAL(textEdited(QString)), SLOT(hasChanged()));
    connect(m_info->administrator, SIGNAL(clicked(bool)), SLOT(hasChanged()));
    connect(m_info->automaticLogin, SIGNAL(clicked(bool)), SLOT(hasChanged()));
    connect(m_passwordEdit, SIGNAL(focused()), SLOT(changePassword()));
    connect(m_passwordEdit, SIGNAL(textEdited(QString)), SLOT(changePassword()));

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

    int iconSizeX = IconSize(KIconLoader::Dialog);
    QSize iconSize(iconSizeX, iconSizeX);
    m_info->face->setIconSize(iconSize);
    m_info->face->setMinimumSize(iconSize);
    m_info->face->setMenu(menu);

    int size = QFontMetrics(KGlobalSettings::fixedFont()).xHeight() * 29;
    m_info->username->setMinimumWidth(size);
    m_info->realName->setMinimumWidth(size);
    m_info->email->setMinimumWidth(size);

    QWidget::setTabOrder(m_info->email, m_passwordEdit);
    QWidget::setTabOrder(m_passwordEdit, m_info->administrator);

    m_passwordEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_passwordEdit->setMinimumWidth(size);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    int row;
    QFormLayout::ItemRole role;
    m_info->formLayout->getWidgetPosition(m_info->label_2, &row, &role);
    m_info->formLayout->insertRow(row, m_info->label_3, m_passwordEdit);

    int pixmapSize = m_info->username->sizeHint().height();
    m_negative = QIcon::fromTheme("dialog-cancel").pixmap(pixmapSize, pixmapSize);
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
    m_info->face->setIcon(QIcon(m_model->data(m_index, AccountModel::Face).value<QPixmap>()));
    m_info->realName->setText(m_model->data(m_index, AccountModel::RealName).toString());
    m_info->email->setText(m_model->data(m_index, AccountModel::Email).toString());
    m_info->administrator->setChecked(m_model->data(m_index, AccountModel::Administrator).toBool());
    m_info->automaticLogin->setChecked(m_model->data(m_index, AccountModel::AutomaticLogin).toBool());
    m_passwordEdit->clear();
}

bool AccountInfo::save()
{
    if (m_infoToSave.isEmpty()) {
        return false;
    }

    kDebug() << "Saving on Index: " << m_index.row();
    QList<AccountModel::Role> failed;
    if (!m_model->setData(m_index, m_info->username->text(), AccountModel::Username)) {
        failed.append(AccountModel::Username);
    }
    if (!m_model->setData(m_index, m_info->realName->text(), AccountModel::RealName)) {
        failed.append(AccountModel::RealName);
    }
    if (!m_model->setData(m_index, m_info->email->text(), AccountModel::Email)) {
        failed.append(AccountModel::Email);
    }
    if (!m_model->setData(m_index, m_info->administrator->isChecked(), AccountModel::Administrator)) {
        failed.append(AccountModel::Administrator);
    }
    if (!m_model->setData(m_index, m_info->automaticLogin->isChecked(), AccountModel::AutomaticLogin)) {
        failed.append(AccountModel::AutomaticLogin);
    }
    if (m_infoToSave.contains(AccountModel::Password)) {
        if (!m_model->setData(m_index, m_infoToSave[AccountModel::Password], AccountModel::Password)) {
            failed.append(AccountModel::Password);
        }
    }
    if (m_infoToSave.contains(AccountModel::Face)) {
        const QString path = m_infoToSave[AccountModel::Face].toString();
        QString faceFile = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
        faceFile.append(QLatin1String("/.face"));

        QFile::remove(faceFile);
        KIO::CopyJob* moveJob = KIO::move(KUrl(path), KUrl(faceFile), KIO::HideProgressInfo);
        connect(moveJob, SIGNAL(finished(KJob*)), SLOT(avatarModelChanged(KJob*)));
        moveJob->setUiDelegate(0);
        moveJob->start();
    }

    if (!failed.isEmpty()) {
        kDebug() << "Failed Roles: " << failed;
    }
    m_infoToSave.clear();
    return true;
}

void AccountInfo::hasChanged()
{
    m_info->nameValidation->setPixmap(m_positive);
    m_info->usernameValidation->setPixmap(m_positive);
    m_info->emailValidation->setPixmap(m_positive);

    QMap<AccountModel::Role, QVariant> infoToSave;
    const QString name = cleanName(m_info->realName->text());
    if (name != m_model->data(m_index, AccountModel::RealName).toString()) {
        if (validateName(name)) {
            infoToSave.insert(AccountModel::RealName, name);
        }
    }

    const QString username = cleanUsername(m_info->username->text());
    if (username != m_model->data(m_index, AccountModel::Username).toString()) {
        if (validateUsername(username)) {
            infoToSave.insert(AccountModel::Username, username);
        }
    }

    const QString email = cleanEmail(m_info->email->text());
    if (email != m_model->data(m_index, AccountModel::Email).toString()) {
        if (validateEmail(email)) {
            infoToSave.insert(AccountModel::Email, email);
        }
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

    if (m_infoToSave.contains(AccountModel::Password)) {
        infoToSave[AccountModel::Password] = m_infoToSave[AccountModel::Password];
    }


    m_infoToSave = infoToSave;
    Q_EMIT changed(!m_infoToSave.isEmpty());
}

QString AccountInfo::cleanName(QString name) const
{
    return name;
}

bool AccountInfo::validateName(const QString& name) const
{
    if (!name.isEmpty() && name.trimmed().isEmpty()) {
        m_info->realName->clear();
        return false;
    }

    return true;
}

QString AccountInfo::cleanUsername(QString username)
{
    if (username.isEmpty()) {
        return username;
    }

    if (username[0].isUpper()) {
        username[0] = username[0].toLower();
    }

    username.remove(' ');
    m_info->username->setText(username);
    return username;
}

bool AccountInfo::validateUsername(QString username) const
{
    if (username.isEmpty()) {
        return false;
    }

    QByteArray userchar = username.toUtf8();
    if (getpwnam(userchar) != NULL) {
        m_info->usernameValidation->setPixmap(m_negative);
        m_info->usernameValidation->setToolTip(i18n("This username is already used"));
        return false;
    }

    QString errorTooltip;

    char first = userchar.at(0);
    bool valid = (first >= 'a' && first <= 'z');

    if (!valid) {
        errorTooltip.append(i18n("The username must start with a letter"));
        errorTooltip.append("\n");
    }

    Q_FOREACH(const char c, userchar) {
        valid = (
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            (c == '_') || (c == '.') ||
            (c == '-')
        );
        if (!valid) {
            break;
        }
    }

    if (!valid) {
        errorTooltip.append(i18n("The username can contain only letters, numbers, score, underscore and dot"));
        errorTooltip.append("\n");
    }

    if (username.count() > MAX_USER_LEN) {
        errorTooltip.append(i18n("The username is too long"));
        valid = false;
    }

    if (!errorTooltip.isEmpty()) {
        m_info->usernameValidation->setPixmap(m_negative);
        m_info->usernameValidation->setToolTip(errorTooltip);
        return false;
    }
    return true;
}

QString AccountInfo::cleanEmail(QString email)
{
    if (email.isEmpty()) {
        return email;
    }

    email = email.toLower().remove(' ');
    int pos = m_info->email->cursorPosition();
    m_info->email->setText(email);
    m_info->email->setCursorPosition(pos);

    return email;
}

bool AccountInfo::validateEmail(const QString& email) const
{
    if (email.isEmpty()) {
        return false;
    }

    QString strPatt = "\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,63}\\b";
    QRegExp rx(strPatt);
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    rx.setPatternSyntax(QRegExp::RegExp);
    if (!rx.exactMatch(email)) {
        m_info->emailValidation->setPixmap(m_negative);
        m_info->emailValidation->setToolTip(i18n("This e-mail address is incorrect"));
    }

    return true;
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

void AccountInfo::avatarModelChanged(KJob* job)
{
    KIO::CopyJob* cJob = qobject_cast<KIO::CopyJob*>(job);
    m_model->setData(m_index, QVariant(cJob->destUrl().path()), AccountModel::Face);
    m_info->face->setIcon(QIcon(m_model->data(m_index, AccountModel::Face).value<QPixmap>()));
}

void AccountInfo::clearAvatar()
{
    QSize icon(IconSize(KIconLoader::Dialog), IconSize(KIconLoader::Dialog));
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
    m_passwordEdit->setText(dialog->password());
    Q_EMIT changed(true);
}
