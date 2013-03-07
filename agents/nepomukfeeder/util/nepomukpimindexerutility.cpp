/*
    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "nepomukpimindexerutility.h"
#include "indexhelpermodel.h"
#include <feederqueue.h>
#include <nepomuk2/datamanagement.h>

#include <akonadi/entitytreemodel.h>
#include <akonadi/changerecorder.h>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/EntityDisplayAttribute>
#include <kactioncollection.h>
#include <kaction.h>
#include <KJob>
#include <KUrl>
#include <QUrl>
#include <QClipboard>
#include <kstatusbar.h>
#include <KStandardAction>

NepomukPIMindexerUtility::NepomukPIMindexerUtility()
    : KXmlGuiWindow(),
    mFeederQueue(new FeederQueue(this))
{
    KGlobal::locale()->insertCatalog( "akonadi_nepomuk_feeder" );
    // tell the KXmlGuiWindow that this is indeed the main widget
    QWidget* w = new QWidget(this);
    setCentralWidget(w);
    m_ui.setupUi(w);
    Akonadi::ChangeRecorder *monitor = new Akonadi::ChangeRecorder(this);
    monitor->itemFetchScope().fetchAttribute<Akonadi::EntityDisplayAttribute>();
    monitor->itemFetchScope().fetchFullPayload(false);
    IndexHelperModel *model = new IndexHelperModel(monitor, this);
    model->setItemPopulationStrategy(Akonadi::EntityTreeModel::LazyPopulation);
    
    m_ui.treeView->setModel(model);
    
    m_ui.treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    KActionCollection *actionCollection = this->actionCollection();
    KAction *action;

    action = actionCollection->addAction( "index" );
    action->setText( i18n( "Index" ) );
    connect( action, SIGNAL(triggered()), this, SLOT(indexCurrentlySelected()) );

    action = actionCollection->addAction( "reindex" );
    action->setText( i18n( "Re-Index" ) );
    connect( action, SIGNAL(triggered()), this, SLOT(reindexCurrentlySelected()) );

    action = actionCollection->addAction( "remove" );
    action->setText( i18n( "Remove Data" ) );
    connect( action, SIGNAL(triggered()), this, SLOT(removeDataOfCurrentlySelected()) );

    action = actionCollection->addAction( "copy" );
    action->setText( i18n( "Copy Url" ) );
    connect( action, SIGNAL(triggered()), this, SLOT(copyUrlFromDataCurrentlySelected()) );

    (void) KStandardAction::quit(this, SLOT(close()), actionCollection);

    
    mFeederQueue->setIndexingSpeed(FeederQueue::FullSpeed);
    mFeederQueue->setOnline(true);
    mFeederQueue->setReindexing(false);
    //mFeederQueue->setCheckAllItems(true);
    QObject::connect(mFeederQueue, SIGNAL(fullyIndexed()), this, SLOT(fullyIndexed()));
    QObject::connect(mFeederQueue, SIGNAL(progress(int)), this, SLOT(progress(int)));
    QObject::connect(mFeederQueue, SIGNAL(running(QString)), this, SLOT(running(QString)));

    setupGUI( Keys | StatusBar | Save | Create, "nepomukpimindexerutility.rc");

    m_ui.treeView->setXmlGuiClient(this);
}

NepomukPIMindexerUtility::~NepomukPIMindexerUtility()
{
}

void NepomukPIMindexerUtility::indexCurrentlySelected()
{
    mTime.start();
    const QModelIndexList &indexList = m_ui.treeView->selectionModel()->selectedRows(0);
    foreach (const QModelIndex &index, indexList) {
        Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
        if (item.isValid()) {
            statusBar()->showMessage(QString::fromLatin1("Indexing item: ").append(item.url().url()));
            mFeederQueue->addItem(item);
        } else {
            Akonadi::Collection collection = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            if (collection.isValid() && !collection.isVirtual() ) {
                statusBar()->showMessage(QString::fromLatin1("Indexing collection: ").append(collection.url().url()));
                mFeederQueue->addCollection(collection);
            }
        }
    }
}

void NepomukPIMindexerUtility::reindexCurrentlySelected()
{
    mFeederQueue->setReindexing(true);
    indexCurrentlySelected();
}

void NepomukPIMindexerUtility::removeDataOfCurrentlySelected()
{
    mTime.start();
    const QModelIndex &index = m_ui.treeView->currentIndex();
    Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    kDebug() << "removing data of item " << item.url();
    if (item.isValid()) {
        KJob *job = Nepomuk2::removeResources(QList<QUrl>() << item.url().url(), Nepomuk2::RemoveSubResoures);
        QObject::connect( job, SIGNAL(finished(KJob*)), this, SLOT(removalComplete(KJob*)) );
    } else {
        const Akonadi::Collection &collection =  index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        if (collection.isValid() && !collection.isVirtual()) {
            KJob *job = Nepomuk2::removeResources(QList<QUrl>() << collection.url().url(), Nepomuk2::RemoveSubResoures);
            QObject::connect( job, SIGNAL(finished(KJob*)), this, SLOT(removalComplete(KJob*)) );
        }
    }
}

void NepomukPIMindexerUtility::progress(int p) {
    kDebug() << "progress " << p;
    statusBar()->showMessage(QString::fromLatin1("Progress: ").append(QString::number(p)).append("%"));
}

void NepomukPIMindexerUtility::running(const QString &p) {
    kDebug() << "running " << p;
    statusBar()->showMessage(QString::fromLatin1("Running: ").append(p));
}

void NepomukPIMindexerUtility::fullyIndexed()
{
    kDebug() << "Time elapsed(ms): " << mTime.elapsed();
    statusBar()->showMessage(QString::fromLatin1("Indexing complete. Time elapsed(ms): ").append(QString::number(mTime.elapsed())));
}

void NepomukPIMindexerUtility::removalComplete(KJob *job) {
    kDebug() << "Time elapsed(ms): " << mTime.elapsed();
    statusBar()->showMessage(QString::fromLatin1("Removal complete. Time elapsed(ms): ").append(QString::number(mTime.elapsed())));
//     static_cast<IndexHelperModel*>(m_ui.treeView->model())->updateItem(mRemovedItem);
    if (job->error())
        kDebug() << job->errorString();
}

void NepomukPIMindexerUtility::copyUrlFromDataCurrentlySelected() {
    const QModelIndex &index = m_ui.treeView->currentIndex();
    Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    kDebug() << "url of item " << item.url();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(item.url().url());
}

#include "nepomukpimindexerutility.moc"
