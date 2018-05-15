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

#include "createavatarjob.h"
#include "user_manager_debug.h"

#include <QImage>
#include <QPixmap>
#include <QTemporaryFile>
#include <QtGlobal>

#include <KIO/CopyJob>
#include <KPixmapRegionSelectorDialog>

CreateAvatarJob::CreateAvatarJob(QObject* parent) : KJob(parent)
{
}

void CreateAvatarJob::setUrl(const QUrl& url)
{
    m_url = url;
}

QString CreateAvatarJob::avatarPath() const
{
    return m_tmpFile;
}

void CreateAvatarJob::start()
{
    QMetaObject::invokeMethod(this, "doStart", Qt::QueuedConnection);
}

void CreateAvatarJob::doStart()
{
    qCDebug(USER_MANAGER_LOG) << "Starting: " << m_url;

    QTemporaryFile file;
    file.open();
    m_tmpFile = file.fileName();
    file.remove();

    qCDebug(USER_MANAGER_LOG) << "From: " << m_url << "to: " << m_tmpFile;
    KIO::CopyJob* job = KIO::copy(m_url, QUrl::fromLocalFile(m_tmpFile), KIO::HideProgressInfo);
    connect(job, SIGNAL(finished(KJob*)), SLOT(copyDone(KJob*)));
    job->setUiDelegate(nullptr);
    job->start();
}

void CreateAvatarJob::copyDone(KJob* job)
{
    if (job->error()) {
        qCDebug(USER_MANAGER_LOG) << "Error:" << job->errorString();
        setError(job->error());
        emitResult();
        return;
    }

    QImage face = KPixmapRegionSelectorDialog::getSelectedImage(QPixmap(m_tmpFile), 192, 192);
    if (face.isNull()) {
        qCDebug(USER_MANAGER_LOG) << "Icon region selection aborted";
        setError(UserDefinedError);
        emitResult();
        return;
    }

    /* Accountmanager doesn't allow icon which are bigger than 1MB */
    face = face.scaledToWidth(qMin(600, face.width()));

    /* Delete file, otherwise face.save() will fail */
    QFile::remove(m_tmpFile);
    if (! face.save(m_tmpFile, "PNG", 10)) {
        qCDebug(USER_MANAGER_LOG) << "Saving icon failed";
        setError(UserDefinedError);
        emitResult();
        return;
    }

    emitResult();
}
