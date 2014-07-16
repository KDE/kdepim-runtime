/*
 * Copyright (C) 2014  Sandro Knauß <knauss@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "autoconfigkolabmail.h"
#include <QDomDocument>
#include <KDebug>

AutoconfigKolabMail::AutoconfigKolabMail(QObject *parent)
    : Ispdb(parent)
{

}

void AutoconfigKolabMail::startJob(const KUrl &url)
{
    mData.clear();
    QMap< QString, QVariant > map;
    map[QLatin1String("errorPage")] = false;
    map[QLatin1String("no-auth-promt")] = true;
    map[QLatin1String("no-www-auth")] = true;

    KIO::TransferJob *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
    job->setMetaData(map);
    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));
    connect(job, SIGNAL(data(KIO::Job*,QByteArray)),
            this, SLOT(dataArrived(KIO::Job*,QByteArray)));
}

void AutoconfigKolabMail::slotResult(KJob *job)
{
    if (job->error()) {
        if (job->error() == KIO::ERR_INTERNAL_SERVER ||   // error 500
                job->error() == KIO::ERR_DOES_NOT_EXIST) {    // error 404
            if (serverType() == DataBase) {
                setServerType(IspAutoConfig);
                lookupInDb(false, false);
            } else if (serverType() == IspAutoConfig) {
                setServerType(IspWellKnow);
                lookupInDb(false, false);
            } else {
                emit finished(false);
            }
        } else {
            kDebug() << "Fetching failed" << job->error() << job->errorString();
            emit finished(false);
        }
        return;
    }

    KIO::TransferJob *tjob = qobject_cast<KIO::TransferJob *>(job);

    int responsecode = tjob->queryMetaData(QLatin1String("responsecode")).toInt();

    if (responsecode == 401) {
        lookupInDb(true, true);
        return;
    } else if (responsecode != 200  && responsecode != 0 && responsecode != 304) {
        kDebug() << "Fetching failed with" << responsecode;
        emit finished(false);
        return;
    }

    QDomDocument document;
    bool ok = document.setContent(mData);
    if (!ok) {
        kDebug() << "Could not parse xml" << mData;
        emit finished(false);
        return;
    }
    parseResult(document);
}
