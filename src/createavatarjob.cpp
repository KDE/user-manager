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

#include <KDebug>
#include <kio/copyjob.h>
#include <KTemporaryFile>
#include <KPixmapRegionSelectorDialog>

CreateAvatarJob::CreateAvatarJob(QObject* parent) : KJob(parent)
{
}

void CreateAvatarJob::setUrl(const KUrl& url)
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
    kDebug() << "Starting: " << m_url;

    KTemporaryFile file;
    file.open();
    m_tmpFile = file.fileName();
    file.remove();

    kDebug() << "From: " << m_url << "to: " << m_tmpFile;
    KIO::CopyJob* job = KIO::copy(m_url, KUrl(m_tmpFile), KIO::HideProgressInfo);
    connect(job, SIGNAL(finished(KJob*)), SLOT(copyDone(KJob*)));
    job->setUiDelegate(0);
    job->start();
}

void CreateAvatarJob::copyDone(KJob* job)
{
    if (job->error()) {
        kDebug() << "Error:" << job->errorString();
        setError(job->error());
        emitResult();
        return;
    }

    QImage face = KPixmapRegionSelectorDialog::getSelectedImage(QPixmap(m_tmpFile), 192, 192);
    face.save(m_tmpFile, "PNG", 10);
    emitResult();
}
