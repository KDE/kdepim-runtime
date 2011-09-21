/*
    Copyright (C) 2009    Frank Osterfeld <osterfeld@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef IMPORTITEMSJOB_H
#define IMPORTITEMSJOB_H

#include "itemimportreader.h"

#include <akonadi/item.h>
#include <akonadi/collection.h>

#include <KJob>

#include <QFile>
#include <QProcess>
#include <QString>

class ImportItemsJob : public KJob {
    Q_OBJECT
public:

    enum Error {
        ExporterError = KJob::UserDefinedError,
        CouldNotOpenFile,
        ItemSyncFailed
    };

    explicit ImportItemsJob( const Akonadi::Collection&, QObject *parent = 0 );
    ~ImportItemsJob();

    Akonadi::Collection collection() const;

    void setFlagsSynchronizable( bool flagsSynchronizable );

    /* reimp */ void start();

private:
    void syncItems( const Akonadi::Item::List& items );
    void cleanupAndEmitResult();
    void startImport();

private Q_SLOTS:
    void doStart();
    void exporterError( QProcess::ProcessError error );
    void exporterFinished( int exitCode, QProcess::ExitStatus status );
    void slotItemsFetched( KJob* job );
    void readBatch();
    void syncDone( KJob* );

private:
    QProcess* m_exporter;
    QString m_exportFileName;
    Akonadi::Collection m_collection;
    QFile m_file;
    QList<Akonadi::Item> m_existingItems;
    ItemImportReader* m_reader;
    int m_pendingCreateJobs;
};

#endif // IMPORTITEMSJOB_H
