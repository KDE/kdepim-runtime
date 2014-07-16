/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef GMAILLINKITEMSTASK_H
#define GMAILLINKITEMSTASK_H

#include <QObject>
#include <QMultiHash>

#include <Akonadi/Collection>

class GmailResource;
class GmailRetrieveItemsTask;
class KJob;

class GmailLinkItemsTask : public QObject
{
    Q_OBJECT

public:
    explicit GmailLinkItemsTask(GmailRetrieveItemsTask *retrieveTask, GmailResource *parent0);
    virtual ~GmailLinkItemsTask();

Q_SIGNALS:
    void done();
    void status(int status, const QString &message = QString());

private Q_SLOTS:
    void linkItem(const QString &remoteId, const QVector<QByteArray> &labels);

    void onRetrievalDone();
    void resolveNextLabel();
    void onLabelResolved(KJob *job);
    void retrieveVirtualReferences();
    void onVirtualReferencesRetrieved(KJob *job);

private:
    void emitDone();

    QHash<QString, QVector<QByteArray> > mLinkMap;
    QList<QByteArray> mLabels;
    QMap<QByteArray, Akonadi::Collection> mLabelCollectionMap;

    GmailResource *mResource;
};

#endif // GMAILLINKITEMSTASK_H
