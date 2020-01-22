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
#include "avatargallery.h"

#include <pwd.h>
#include <unistd.h>

#include <QMenu>
#include <QToolButton>
#include <QStandardPaths>
#include <QImageReader>
#include <QFontDatabase>
#include <QFileDialog>

#include "user_manager_debug.h"
#include <KJob>
#include <KIO/CopyJob>
#include <KUser>
#include <KI18n/klocalizedstring.h>

AccountInfo::AccountInfo(AccountModel* model, QWidget* parent, Qt::WindowFlags f)
 : QWidget(parent, f)
 , m_info(new Ui::AccountInfo())
 , m_model(model)
{
    m_info->setupUi(this);

    // ensure avatar user icon use correct device pixel ratio
    const qreal dpr = qApp->devicePixelRatio();
    m_model->setDpr(dpr);

    connect(m_info->username, &QLineEdit::textEdited, this, &AccountInfo::hasChanged);
    connect(m_info->realName, &QLineEdit::textEdited, this, &AccountInfo::hasChanged);
    connect(m_info->email, &QLineEdit::textEdited, this, &AccountInfo::hasChanged);
    connect(m_info->administrator, &QAbstractButton::clicked, this, &AccountInfo::hasChanged);
    connect(m_info->automaticLogin, &QAbstractButton::clicked, this, &AccountInfo::hasChanged);
    connect(m_info->changePasswordButton, &QPushButton::clicked, this, &AccountInfo::changePassword);

    connect(m_model, &QAbstractItemModel::dataChanged, this, &AccountInfo::dataChanged);
    m_info->face->setPopupMode(QToolButton::InstantPopup);
    QMenu* menu = new QMenu(this);

    QAction *gallery = new QAction(i18n("Choose from Gallery..."), this);
    gallery->setIcon(QIcon::fromTheme(QStringLiteral("shape-choose"))); // TODO proper icon
    connect(gallery, &QAction::triggered, this, &AccountInfo::openGallery);

    QAction *openAvatar = new QAction(i18n("Load from file..."), this);
    openAvatar->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    connect(openAvatar, &QAction::triggered, this, &AccountInfo::openAvatarSlot);

    QAction *editClear = new QAction(i18n("Clear Avatar"), this);
    editClear->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear")));
    connect(editClear, &QAction::triggered, this, &AccountInfo::clearAvatar);

    menu->addAction(gallery);
    menu->addAction(openAvatar);
    menu->addAction(editClear);

    int iconSizeX = style()->pixelMetric(QStyle::PM_LargeIconSize);
    QSize iconSize(iconSizeX, iconSizeX);
    m_info->face->setIconSize(iconSize);
    m_info->face->setMinimumSize(iconSize);
    m_info->face->setMenu(menu);

    int size = QFontMetrics(QFontDatabase::systemFont(QFontDatabase::FixedFont)).xHeight() * 29;
    m_info->username->setMinimumWidth(size);
    m_info->realName->setMinimumWidth(size);
    m_info->email->setMinimumWidth(size);

    int pixmapSize = m_info->username->sizeHint().height();
    m_negative = QIcon::fromTheme(QStringLiteral("dialog-cancel")).pixmap(pixmapSize, pixmapSize);
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
    QString username = m_model->data(m_index, AccountModel::Username).toString();
    if (!username.isEmpty()) {
        m_info->username->setDisabled(true);//Do not allow to change the username
        m_info->changePasswordButton->setText(i18nc("@label:button", "Change Password"));
    } else {
        m_info->username->setDisabled(false);
        m_info->changePasswordButton->setText(i18nc("@label:button", "Set Password"));
    }
    m_info->username->setText(username);

    m_info->face->setIcon(QIcon(m_model->data(m_index, AccountModel::Face).value<QPixmap>()));
    m_info->realName->setText(m_model->data(m_index, AccountModel::RealName).toString());
    m_info->email->setText(m_model->data(m_index, AccountModel::Email).toString());
    m_info->administrator->setChecked(m_model->data(m_index, AccountModel::Administrator).toBool());
    m_info->automaticLogin->setChecked(m_model->data(m_index, AccountModel::AutomaticLogin).toBool());
}

bool AccountInfo::save()
{
    if (m_infoToSave.isEmpty()) {
        return false;
    }

    qCDebug(USER_MANAGER_LOG) << "Saving on Index: " << m_index.row();
    QList<AccountModel::Role> failed;
    if (m_infoToSave.contains(AccountModel::Username)  &&
            !m_model->setData(m_index, m_infoToSave[AccountModel::Username], AccountModel::Username)) {
        failed.append(AccountModel::Username);
    }
    if (m_infoToSave.contains(AccountModel::RealName) &&
            !m_model->setData(m_index, m_infoToSave[AccountModel::RealName], AccountModel::RealName)) {
        failed.append(AccountModel::RealName);
    }
    if (m_infoToSave.contains(AccountModel::Email) &&
            !m_model->setData(m_index, m_infoToSave[AccountModel::Email], AccountModel::Email)) {
        failed.append(AccountModel::Email);
    }
    if (m_infoToSave.contains(AccountModel::Administrator) &&
            !m_model->setData(m_index, m_info->administrator->isChecked(), AccountModel::Administrator)) {
        failed.append(AccountModel::Administrator);
    }
    if (m_infoToSave.contains(AccountModel::AutomaticLogin) &&
            !m_model->setData(m_index, m_info->automaticLogin->isChecked(), AccountModel::AutomaticLogin)) {
        failed.append(AccountModel::AutomaticLogin);
    }
    if (m_infoToSave.contains(AccountModel::Password)) {
        if (!m_model->setData(m_index, m_infoToSave[AccountModel::Password], AccountModel::Password)) {
            failed.append(AccountModel::Password);
        }
    }
    if (m_infoToSave.contains(AccountModel::Face)) {
        const QString path = m_infoToSave[AccountModel::Face].toString();

        //we want to save the face using AccountsService, but for backwards compatibility we also
        //save the icon into ~/.face for old apps/DisplayManagers that still expect that
        //This works when setting a face as the current user, but doesn't make sense when setting the icon
        //of another user.
        const QString username = m_model->data(m_index, AccountModel::Username).toString();
        if (username != KUser().loginName()) {
            m_model->setData(m_index, QVariant(path), AccountModel::Face);
        } else {
            QString faceFile = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
            faceFile.append(QLatin1String("/.face"));
            QFile::remove(faceFile);

            KIO::CopyJob* copyJob = KIO::copy(QUrl::fromLocalFile(path), QUrl::fromLocalFile(faceFile), KIO::HideProgressInfo);
            connect(copyJob, &KJob::finished, this, &AccountInfo::avatarModelChanged);
            copyJob->setUiDelegate(nullptr);
            copyJob->setUiDelegateExtension(nullptr);
            copyJob->start();

            QString faceFile2 = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
            faceFile2.append(QLatin1String("/.face.icon"));
            QFile::remove(faceFile2);
            QFile::link(faceFile, faceFile2);
        }
    }

    if (!failed.isEmpty()) {
        qCDebug(USER_MANAGER_LOG) << "Failed Roles: " << failed;
    }

    m_info->username->setEnabled(false);
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
    emit changed(!m_infoToSave.isEmpty());
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

    username.remove(QLatin1Char(' '));
    m_info->username->setText(username);
    return username;
}

bool AccountInfo::validateUsername(const QString &username) const
{
    if (username.isEmpty()) {
        return false;
    }

    const QByteArray userchar = username.toUtf8();
    if (getpwnam(userchar.constData()) != nullptr) {
        m_info->usernameValidation->setPixmap(m_negative);
        m_info->usernameValidation->setToolTip(i18n("This username is already used"));
        return false;
    }

    QString errorTooltip;

    char first = userchar.at(0);
    bool valid = (first >= 'a' && first <= 'z');

    if (!valid) {
        errorTooltip.append(i18n("The username must start with a letter"));
        errorTooltip.append(QLatin1Char('\n'));
    }

    for (const char c : userchar) {
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
        errorTooltip.append(QLatin1Char('\n'));
    }

    static const long MAX_USER_NAME_LENGTH = []() {
        long result = sysconf(_SC_LOGIN_NAME_MAX);
        if (result < 0) {
            qWarning("Could not query LOGIN_NAME_MAX, defaulting to 32");
            result = 32;
        }
        return result;
    }();

    if (username.size() > MAX_USER_NAME_LENGTH) {
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

    email = email.toLower().remove(QLatin1Char(' '));
    int pos = m_info->email->cursorPosition();
    m_info->email->setText(email);
    m_info->email->setCursorPosition(pos);

    return email;
}

bool AccountInfo::validateEmail(const QString& email) const
{
    if (email.isEmpty()) {
        return true;
    }

    QString strPatt = QStringLiteral("\\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,63}\\b");
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

    //If we have no username, we assume this was user-new
    if (m_info->username->text().isEmpty()) {
        loadFromModel();
        return;
    }
    hasChanged();
}

void AccountInfo::openGallery()
{
    QScopedPointer<AvatarGallery> gallery(new AvatarGallery());
    if (gallery->exec() != QDialog::Accepted) {
        return;
    }

    QString path = gallery->url().toLocalFile();
    m_info->face->setIcon(QIcon(path));
    m_infoToSave.insert(AccountModel::Face, path);
    emit changed(true);
}

QStringList AccountInfo::imageFormats() const
{
    QStringList result;
    const QList<QByteArray> supportedMimes = QImageReader::supportedMimeTypes();
    for (const QByteArray &b: supportedMimes) {
        if (! b.isEmpty())
            result.append(QString::fromLatin1(b));
    }
    return result;
}

void AccountInfo::openAvatarSlot()
{
    QFileDialog dlg(this, i18nc("@title:window", "Choose Image"), QDir::homePath());

    dlg.setMimeTypeFilters(imageFormats());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFile);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    QUrl url = QUrl::fromLocalFile(dlg.selectedFiles().first());
    CreateAvatarJob *job = new CreateAvatarJob(this);
    connect(job, &KJob::finished, this, &AccountInfo::avatarCreated);
    job->setUrl(url);
    job->start();
}

void AccountInfo::avatarCreated(KJob* job)
{
    if (! job->error()) {
        qCDebug(USER_MANAGER_LOG) << "Avatar created";
        CreateAvatarJob *aJob = qobject_cast<CreateAvatarJob*>(job);
        m_info->face->setIcon(QIcon(aJob->avatarPath()));
        m_infoToSave.insert(AccountModel::Face, aJob->avatarPath());
        emit changed(true);
    }
}

void AccountInfo::avatarModelChanged(KJob* job)
{
    KIO::CopyJob* cJob = qobject_cast<KIO::CopyJob*>(job);
    m_model->setData(m_index, QVariant(cJob->destUrl().path()), AccountModel::Face);
    m_info->face->setIcon(QIcon(m_model->data(m_index, AccountModel::Face).value<QPixmap>()));
    // If there is a leftover temp file, remove it
    if (cJob->srcUrls().constFirst().path().startsWith(QLatin1String("/tmp/"))) {
        QFile::remove(cJob->srcUrls().constFirst().path());
    }
}

void AccountInfo::clearAvatar()
{
    m_info->face->setIcon(QIcon::fromTheme(QStringLiteral("user-identity")).pixmap(48, 48));
    m_infoToSave.insert(AccountModel::Face, QString());
    emit changed(true);
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
    emit changed(true);
}
