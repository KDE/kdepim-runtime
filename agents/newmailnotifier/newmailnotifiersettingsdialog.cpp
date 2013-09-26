/*
    Copyright (c) 2013 Laurent Montel <montel@kde.org>

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

#include "newmailnotifiersettingsdialog.h"
#include "newmailnotifierattribute.h"
#include "newmailnotifieragentsettings.h"

#include <KLocale>
#include <KNotifyConfigWidget>
#include <KLineEdit>
#include <KCheckableProxyModel>
#include <KPushButton>

#include <QTabWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QWhatsThis>

#include <Akonadi/CollectionView>
#include <Akonadi/CollectionModel>
#include <Akonadi/RecursiveCollectionFilterProxyModel>
#include <Akonadi/CollectionFilterProxyModel>
#include <Akonadi/CollectionModifyJob>

NewMailNotifierSettingsDialog::NewMailNotifierSettingsDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n("New Mail Notifier settings") );
    setWindowIcon( KIcon( QLatin1String("kmail") ) );
    setButtons( Ok|Cancel );
    connect(this, SIGNAL(okClicked()), this, SLOT(slotOkClicked()));

    QWidget *w = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout;
    w->setLayout(lay);
    QTabWidget *tab = new QTabWidget;
    lay->addWidget(tab);

    QWidget *settings = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    settings->setLayout(vbox);

    QGroupBox *grp = new QGroupBox(i18n("Choose which fields to show:"));
    vbox->addWidget(grp);
    QVBoxLayout *groupboxLayout = new QVBoxLayout;
    grp->setLayout(groupboxLayout);

    mShowPhoto = new QCheckBox(i18n("Show Photo"));
    mShowPhoto->setChecked(NewMailNotifierAgentSettings::showPhoto());
    groupboxLayout->addWidget(mShowPhoto);

    mShowFrom = new QCheckBox(i18n("Show From"));
    mShowFrom->setChecked(NewMailNotifierAgentSettings::showFrom());
    groupboxLayout->addWidget(mShowFrom);

    mShowSubject = new QCheckBox(i18n("Show Subject"));
    mShowSubject->setChecked(NewMailNotifierAgentSettings::showSubject());
    groupboxLayout->addWidget(mShowSubject);

    mShowFolders = new QCheckBox(i18n("Show Folders"));
    mShowFolders->setChecked(NewMailNotifierAgentSettings::showFolder());
    groupboxLayout->addWidget(mShowFolders);

    mExcludeMySelf = new QCheckBox(i18n("Do not notify when email was sent by me"));
    mExcludeMySelf->setChecked(NewMailNotifierAgentSettings::excludeEmailsFromMe());
    vbox->addWidget(mExcludeMySelf);

    vbox->addStretch();
    tab->addTab(settings, i18n("Display"));


    QWidget *textSpeakWidget = new QWidget;
    vbox = new QVBoxLayout;
    textSpeakWidget->setLayout(vbox);
    mTextToSpeak = new QCheckBox(i18n("Enabled"));
    mTextToSpeak->setChecked(NewMailNotifierAgentSettings::textToSpeakEnabled());
    vbox->addWidget(mTextToSpeak);

    QLabel *howIsItWork = new QLabel(i18n( "<a href=\"whatsthis\">How does this work?</a>" ));
    howIsItWork->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    vbox->addWidget(howIsItWork);
    connect(howIsItWork, SIGNAL(linkActivated(QString)),SLOT(slotHelpLinkClicked(QString)) );

    QHBoxLayout *textToSpeakLayout = new QHBoxLayout;
    textToSpeakLayout->setMargin(0);
    QLabel *lab = new QLabel(i18n("Message:"));
    textToSpeakLayout->addWidget(lab);
    mTextToSpeakSetting = new KLineEdit;
    mTextToSpeakSetting->setClearButtonShown(true);
    mTextToSpeakSetting->setText(NewMailNotifierAgentSettings::textToSpeak());
    mTextToSpeakSetting->setEnabled(mTextToSpeak->isChecked());
    textToSpeakLayout->addWidget(mTextToSpeakSetting);
    vbox->addLayout(textToSpeakLayout);
    vbox->addStretch();
    tab->addTab(textSpeakWidget, i18n("Text to Speak"));
    connect(mTextToSpeak, SIGNAL(toggled(bool)), mTextToSpeakSetting, SLOT(setEnabled(bool)));




    mNotify = new KNotifyConfigWidget(this);
    mNotify->setApplication(QLatin1String("akonadi_newmailnotifier_agent"));
    tab->addTab(mNotify, i18n("Notify"));


    QWidget *colsWidget = new QWidget(this);
    vbox = new QVBoxLayout(colsWidget);


    mCollectionModel = new Akonadi::CollectionModel(this);
    connect(mCollectionModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(slotCollectionsInserted(QModelIndex,int,int)));

    mSelectionModel = new QItemSelectionModel(mCollectionModel, this);

    KCheckableProxyModel *checkableProxy = new KCheckableProxyModel(this);
    checkableProxy->setSourceModel(mCollectionModel);
    checkableProxy->setSelectionModel(mSelectionModel);

    mCollectionFilter = new Akonadi::RecursiveCollectionFilterProxyModel(this);
    mCollectionFilter->addContentMimeTypeInclusionFilter(QLatin1String("message/rfc822"));
    mCollectionFilter->setSourceModel(checkableProxy);
    mCollectionFilter->setDynamicSortFilter(true);
    mCollectionFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);

    KLineEdit *searchLine = new KLineEdit(this);
    searchLine->setPlaceholderText(i18n("Search..."));
    searchLine->setClearButtonShown(true);
    connect(searchLine, SIGNAL(textChanged(QString)),
            this, SLOT(setCollectionFilter(QString)));

    vbox->addWidget(searchLine);

    mCollectionView = new Akonadi::CollectionView(this);
    mCollectionView->setModel(mCollectionFilter);
    mCollectionView->setAlternatingRowColors(true);
    vbox->addWidget(mCollectionView);

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);

    KPushButton *button = new KPushButton(i18n("&Select All"), this);
    connect(button, SIGNAL(clicked(bool)), this, SLOT(slotSelectAllCollections()));
    hbox->addWidget(button);

    button = new KPushButton(i18n("&Unselect All"), this);
    connect(button, SIGNAL(clicked(bool)), this, SLOT(slotUnselectAllCollections()));
    hbox->addWidget(button);
    hbox->addStretch(1);

    tab->addTab(colsWidget, i18n("Folders"));

    setMainWidget(w);
}

NewMailNotifierSettingsDialog::~NewMailNotifierSettingsDialog()
{
}

void NewMailNotifierSettingsDialog::setCollectionFilter(const QString &filter)
{
    mCollectionFilter->setSearchPattern(filter);
}

void NewMailNotifierSettingsDialog::slotHelpLinkClicked(const QString &)
{
    qDebug()<<" void NewMailNotifierSettingsDialog::slotHelpLinkClicked(const QString &)";
    const QString help =
            i18n( "<qt>"
                  "<p>Here you can define message. "
                  "You can use:</p>"
                  "<ul>"
                  "<li>%s set subject</li>"
                  "<li>%f set from</li>"
                  "</ul>"
                  "</qt>" );

    QWhatsThis::showText( QCursor::pos(), help );
}

void NewMailNotifierSettingsDialog::slotOkClicked()
{
    updateCollectionsRecursive(QModelIndex());

    NewMailNotifierAgentSettings::setShowPhoto(mShowPhoto->isChecked());
    NewMailNotifierAgentSettings::setShowFrom(mShowFrom->isChecked());
    NewMailNotifierAgentSettings::setShowSubject(mShowSubject->isChecked());
    NewMailNotifierAgentSettings::setShowFolder(mShowFolders->isChecked());
    NewMailNotifierAgentSettings::setExcludeEmailsFromMe(mExcludeMySelf->isChecked());
    NewMailNotifierAgentSettings::setTextToSpeakEnabled(mTextToSpeak->isChecked());
    NewMailNotifierAgentSettings::setTextToSpeak(mTextToSpeakSetting->text());
    NewMailNotifierAgentSettings::self()->writeConfig();
    mNotify->save();
    accept();
}

void NewMailNotifierSettingsDialog::slotCollectionsInserted(const QModelIndex &parent, int start, int end)
{
    for (int i = start; i <= end; ++i) {
        const QModelIndex index = mCollectionModel->index(i, 0, parent);
        if (!index.isValid()) {
            continue;
        }

        const Akonadi::Collection collection = index.data(Akonadi::CollectionModel::CollectionRole).value<Akonadi::Collection>();
        NewMailNotifierAttribute *attr = collection.attribute<NewMailNotifierAttribute>();
        if (!attr || !attr->ignoreNewMail()) {
            mSelectionModel->select(index, QItemSelectionModel::Select);
        }
    }

    mCollectionView->expandAll();
}

void NewMailNotifierSettingsDialog::slotSelectAllCollections()
{
    selectAllCollectionsRecursive(QModelIndex(), true);
}

void NewMailNotifierSettingsDialog::slotUnselectAllCollections()
{
    selectAllCollectionsRecursive(QModelIndex(), false);
}

void NewMailNotifierSettingsDialog::selectAllCollectionsRecursive(const QModelIndex &parent,
                                                                  bool select)
{
    for (int i = 0; i < mCollectionModel->rowCount(parent); ++i) {
        const QModelIndex index = mCollectionModel->index(i, 0, parent);
        if (mCollectionModel->hasChildren(index)) {
            selectAllCollectionsRecursive(index, select);
        }

        mSelectionModel->select(index, select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
    }
}

void NewMailNotifierSettingsDialog::updateCollectionsRecursive(const QModelIndex &parent)
{
    for (int i = 0; i < mCollectionModel->rowCount(parent); ++i) {
        const QModelIndex index = mCollectionModel->index(i, 0, parent);
        if (mCollectionModel->hasChildren(index)) {
            updateCollectionsRecursive(index);
        }

        const bool selected = mSelectionModel->isSelected(index);
        Akonadi::Collection collection = index.data(Akonadi::CollectionModel::CollectionRole).value<Akonadi::Collection>();

        NewMailNotifierAttribute *attr = collection.attribute<NewMailNotifierAttribute>();
        Akonadi::CollectionModifyJob *modifyJob = 0;
        if (selected && attr && attr->ignoreNewMail()) {
            collection.removeAttribute<NewMailNotifierAttribute>();
            modifyJob = new Akonadi::CollectionModifyJob(collection);
            modifyJob->setProperty("AttributeAdded", true);
        } else if (!selected && (!attr || !attr->ignoreNewMail())) {
            attr = collection.attribute<NewMailNotifierAttribute>(Akonadi::Entity::AddIfMissing);
            attr->setIgnoreNewMail(true);
            modifyJob = new Akonadi::CollectionModifyJob(collection);
            modifyJob->setProperty("AttributeAdded", false);
        }

        if (modifyJob) {
            connect(modifyJob, SIGNAL(finished(KJob*)), SLOT(slotModifyJobDone(KJob*)));
        }
    }
}

void NewMailNotifierSettingsDialog::slotModifyJobDone(KJob* job)
{
    Akonadi::CollectionModifyJob *modifyJob = qobject_cast<Akonadi::CollectionModifyJob*>(job);
    if (modifyJob && job->error()) {
        if (job->property("AttributeAdded").toBool()) {
            kWarning() << "Failed to append NewMailNotifierAttribute to collection"
                       << modifyJob->collection().id() << ":"
                       << job->errorString();
        } else {
            kWarning() << "Failed to remove NewMailNotifierAttribute from collection"
                       << modifyJob->collection().id() << ":"
                       << job->errorString();
        }
    }
}





#include "newmailnotifiersettingsdialog.moc"
