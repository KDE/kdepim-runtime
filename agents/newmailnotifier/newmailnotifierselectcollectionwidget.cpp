/*
    Copyright (c) 2013-2018 Laurent Montel <montel@kde.org>

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

#include "newmailnotifierselectcollectionwidget.h"
#include <AkonadiCore/NewMailNotifierAttribute>

#include <CollectionModifyJob>
#include <CollectionFilterProxyModel>
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
#include <KRecursiveFilterProxyModel>
#else
#include <QSortFilterProxyModel>
#endif
#include <AkonadiCore/AttributeFactory>

#include <ChangeRecorder>
#include <EntityTreeModel>
#include <Collection>
#include <KMime/Message>

#include <KLocalizedString>
#include <QPushButton>
#include <KLineEdit>
#include "newmailnotifier_debug.h"

#include <QVBoxLayout>
#include <QIdentityProxyModel>
#include <QHBoxLayout>
#include <QTreeView>
#include <QLabel>

NewMailNotifierCollectionProxyModel::NewMailNotifierCollectionProxyModel(QObject *parent)
    : QIdentityProxyModel(parent)
{
}

QVariant NewMailNotifierCollectionProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::CheckStateRole) {
        if (index.isValid()) {
            const Akonadi::Collection collection
                = data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            if (mNotificationCollection.contains(collection)) {
                return mNotificationCollection.value(collection) ? Qt::Checked : Qt::Unchecked;
            } else {
                Akonadi::NewMailNotifierAttribute *attr = collection.attribute<Akonadi::NewMailNotifierAttribute>();
                if (!attr || !attr->ignoreNewMail()) {
                    return Qt::Checked;
                }
                return Qt::Unchecked;
            }
        }
    }
    return QIdentityProxyModel::data(index, role);
}

bool NewMailNotifierCollectionProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        if (index.isValid()) {
            const Akonadi::Collection collection
                = data(index, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            mNotificationCollection[collection] = (value == Qt::Checked);
            emit dataChanged(index, index);
            return true;
        }
    }

    return QIdentityProxyModel::setData(index, value, role);
}

Qt::ItemFlags NewMailNotifierCollectionProxyModel::flags(const QModelIndex &index) const
{
    if (index.isValid()) {
        return QIdentityProxyModel::flags(index) | Qt::ItemIsUserCheckable;
    } else {
        return QIdentityProxyModel::flags(index);
    }
}

QHash<Akonadi::Collection, bool> NewMailNotifierCollectionProxyModel::notificationCollection() const
{
    return mNotificationCollection;
}

NewMailNotifierSelectCollectionWidget::NewMailNotifierSelectCollectionWidget(QWidget *parent)
    : QWidget(parent)
{
    Akonadi::AttributeFactory::registerAttribute<Akonadi::NewMailNotifierAttribute>();
    QVBoxLayout *vbox = new QVBoxLayout(this);

    QLabel *label = new QLabel(i18n("Select which folders to monitor for new message notifications:"));
    vbox->addWidget(label);

    // Create a new change recorder.
    mChangeRecorder = new Akonadi::ChangeRecorder(this);
    mChangeRecorder->setMimeTypeMonitored(KMime::Message::mimeType());
    mChangeRecorder->fetchCollection(true);
    mChangeRecorder->setAllMonitored(true);

    mModel = new Akonadi::EntityTreeModel(mChangeRecorder, this);
    // Set the model to show only collections, not items.
    mModel->setItemPopulationStrategy(Akonadi::EntityTreeModel::NoItemPopulation);
    connect(mModel, &Akonadi::EntityTreeModel::collectionTreeFetched, this, &NewMailNotifierSelectCollectionWidget::slotCollectionTreeFetched);

    Akonadi::CollectionFilterProxyModel *mimeTypeProxy = new Akonadi::CollectionFilterProxyModel(this);
    mimeTypeProxy->setExcludeVirtualCollections(true);
    mimeTypeProxy->setDynamicSortFilter(true);
    mimeTypeProxy->addMimeTypeFilters(QStringList() << KMime::Message::mimeType());
    mimeTypeProxy->setSourceModel(mModel);

    mNewMailNotifierProxyModel = new NewMailNotifierCollectionProxyModel(this);
    mNewMailNotifierProxyModel->setSourceModel(mimeTypeProxy);

#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
    mCollectionFilter = new KRecursiveFilterProxyModel(this);
#else
    mCollectionFilter = new QSortFilterProxyModel(this);
    mCollectionFilter->setRecursiveFilteringEnabled(true);
#endif
    mCollectionFilter->setSourceModel(mNewMailNotifierProxyModel);
    mCollectionFilter->setDynamicSortFilter(true);
    mCollectionFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mCollectionFilter->setSortRole(Qt::DisplayRole);
    mCollectionFilter->setSortCaseSensitivity(Qt::CaseSensitive);
    mCollectionFilter->setSortLocaleAware(true);

    KLineEdit *searchLine = new KLineEdit(this);
    searchLine->setPlaceholderText(i18n("Search..."));
    searchLine->setClearButtonEnabled(true);
    connect(searchLine, &QLineEdit::textChanged,
            this, &NewMailNotifierSelectCollectionWidget::slotSetCollectionFilter);

    vbox->addWidget(searchLine);

    mFolderView = new QTreeView(this);
    mFolderView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mFolderView->setAlternatingRowColors(true);
    vbox->addWidget(mFolderView);

    mFolderView->setModel(mCollectionFilter);

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);

    QPushButton *button = new QPushButton(i18n("&Select All"), this);
    connect(button, &QPushButton::clicked, this, &NewMailNotifierSelectCollectionWidget::slotSelectAllCollections);
    hbox->addWidget(button);

    button = new QPushButton(i18n("&Unselect All"), this);
    connect(button, &QPushButton::clicked, this, &NewMailNotifierSelectCollectionWidget::slotUnselectAllCollections);
    hbox->addWidget(button);
    hbox->addStretch(1);
}

NewMailNotifierSelectCollectionWidget::~NewMailNotifierSelectCollectionWidget()
{
}

void NewMailNotifierSelectCollectionWidget::slotCollectionTreeFetched()
{
    mCollectionFilter->sort(0, Qt::AscendingOrder);
    mFolderView->expandAll();
}

void NewMailNotifierSelectCollectionWidget::slotSetCollectionFilter(const QString &filter)
{
    mCollectionFilter->setFilterWildcard(filter);
    mFolderView->expandAll();
}

void NewMailNotifierSelectCollectionWidget::slotSelectAllCollections()
{
    forceStatus(QModelIndex(), true);
}

void NewMailNotifierSelectCollectionWidget::slotUnselectAllCollections()
{
    forceStatus(QModelIndex(), false);
}

void NewMailNotifierSelectCollectionWidget::forceStatus(const QModelIndex &parent, bool status)
{
    const int nbCol = mNewMailNotifierProxyModel->rowCount(parent);
    for (int i = 0; i < nbCol; ++i) {
        const QModelIndex child = mNewMailNotifierProxyModel->index(i, 0, parent);
        mNewMailNotifierProxyModel->setData(child, status ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        forceStatus(child, status);
    }
}

void NewMailNotifierSelectCollectionWidget::updateCollectionsRecursive()
{
    QHashIterator<Akonadi::Collection, bool> i(mNewMailNotifierProxyModel->notificationCollection());
    while (i.hasNext()) {
        i.next();
        Akonadi::Collection collection = i.key();
        Akonadi::NewMailNotifierAttribute *attr = collection.attribute<Akonadi::NewMailNotifierAttribute>();
        Akonadi::CollectionModifyJob *modifyJob = nullptr;
        const bool selected = i.value();
        if (selected && attr && attr->ignoreNewMail()) {
            collection.removeAttribute<Akonadi::NewMailNotifierAttribute>();
            modifyJob = new Akonadi::CollectionModifyJob(collection);
            modifyJob->setProperty("AttributeAdded", true);
        } else if (!selected && (!attr || !attr->ignoreNewMail())) {
            attr = collection.attribute<Akonadi::NewMailNotifierAttribute>(Akonadi::Collection::AddIfMissing);
            attr->setIgnoreNewMail(true);
            modifyJob = new Akonadi::CollectionModifyJob(collection);
            modifyJob->setProperty("AttributeAdded", false);
        }

        if (modifyJob) {
            connect(modifyJob, &Akonadi::CollectionModifyJob::finished, this, &NewMailNotifierSelectCollectionWidget::slotModifyJobDone);
        }
    }
}

void NewMailNotifierSelectCollectionWidget::slotModifyJobDone(KJob *job)
{
    Akonadi::CollectionModifyJob *modifyJob = qobject_cast<Akonadi::CollectionModifyJob *>(job);
    if (modifyJob && job->error()) {
        if (job->property("AttributeAdded").toBool()) {
            qCWarning(NEWMAILNOTIFIER_LOG) << "Failed to append NewMailNotifierAttribute to collection"
                                           << modifyJob->collection().id() << ":"
                                           << job->errorString();
        } else {
            qCWarning(NEWMAILNOTIFIER_LOG) << "Failed to remove NewMailNotifierAttribute from collection"
                                           << modifyJob->collection().id() << ":"
                                           << job->errorString();
        }
    }
}
