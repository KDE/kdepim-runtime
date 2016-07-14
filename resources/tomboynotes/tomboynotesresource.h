/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef TOMBOYNOTESRESOURCE_H
#define TOMBOYNOTESRESOURCE_H

#include <AkonadiAgentBase/ResourceBase>
#include <KIO/AccessManager>

class TomboyNotesResource : public Akonadi::ResourceBase,
                            public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    TomboyNotesResource(const QString &id);
    ~TomboyNotesResource();

public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;

protected Q_SLOTS:
    // Standard akonadi slots
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &col) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;

    // Slots for Job result handling
    void onAuthorizationFinished(KJob *kjob);
    void onCollectionsRetrieved(KJob *kjob);
    void onItemChangeCommitted(KJob *kjob);
    void onItemRetrieved(KJob *kjob);
    void onItemsRetrieved(KJob *kjob);

private Q_SLOTS:
    // SSL error handling
    void onSslError(QNetworkReply *reply, const QList<QSslError> &errors);

protected:
    void aboutToQuit() Q_DECL_OVERRIDE;

    // Standard akonadi
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void itemRemoved(const Akonadi::Item &item) Q_DECL_OVERRIDE;

private:
    bool configurationNotValid() const;

    void retryAfterFailure(const QString &errorMessage);
    // Status handling
    void showError(const QString &errorText);
    QTimer *mStatusMessageTimer;

    // Only one UploadJob should run per time
    bool mUploadJobProcessRunning;

    KIO::AccessManager *mManager;
};

#endif
