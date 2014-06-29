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
#include "newmailnotifierselectcollectionwidget.h"
#include "newmailnotifieragentsettings.h"

//QT5
//#include "kdepim-runtime-version.h"

#include <KLocalizedString>
#include <KNotifyConfigWidget>
#include <KLineEdit>
#include <KCheckableProxyModel>
#include <QPushButton>
#include <KHelpMenu>
#include <k4aboutdata.h>
#include <QIcon>

#include <QTabWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QWhatsThis>

#include <AkonadiWidgets/CollectionView>
#include <AkonadiCore/CollectionModel>
#include <AkonadiCore/RecursiveCollectionFilterProxyModel>
#include <AkonadiCore/CollectionFilterProxyModel>
#include <AkonadiCore/CollectionModifyJob>
#include <KSharedConfig>

static const char * textToSpeakMessage =
        I18N_NOOP( "<qt>"
              "<p>Here you can define message. "
              "You can use:</p>"
              "<ul>"
              "<li>%s set subject</li>"
              "<li>%f set from</li>"
              "</ul>"
              "</qt>" );

NewMailNotifierSettingsDialog::NewMailNotifierSettingsDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n("New Mail Notifier settings") );
    setWindowIcon( QIcon::fromTheme( QLatin1String("kmail") ) );
    setButtons( Help | Ok|Cancel );
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

    mAllowToShowMail = new QCheckBox(i18n("Show button to display mail"));
    mAllowToShowMail->setChecked(NewMailNotifierAgentSettings::showButtonToDisplayMail());
    vbox->addWidget(mAllowToShowMail);

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
    mTextToSpeakSetting->setWhatsThis(i18n(textToSpeakMessage));
    textToSpeakLayout->addWidget(mTextToSpeakSetting);
    vbox->addLayout(textToSpeakLayout);
    vbox->addStretch();
    tab->addTab(textSpeakWidget, i18n("Text to Speak"));
    connect(mTextToSpeak, SIGNAL(toggled(bool)), mTextToSpeakSetting, SLOT(setEnabled(bool)));

    mNotify = new KNotifyConfigWidget(this);
    mNotify->setApplication(QLatin1String("akonadi_newmailnotifier_agent"));
    tab->addTab(mNotify, i18n("Notify"));

    mSelectCollection = new NewMailNotifierSelectCollectionWidget;
    tab->addTab(mSelectCollection, i18n("Folders"));

    setMainWidget(w);

//QT5
    mAboutData = new K4AboutData(
                QByteArray( "newmailnotifieragent" ),
                QByteArray(),
                ki18n( "New Mail Notifier Agent" ),
                QByteArray( /*KDEPIM_RUNTIME_VERSION*/ "5.0" ),
                ki18n( "Notifies about new mail." ),
                K4AboutData::License_GPL_V2,
                ki18n( "Copyright (C) 2013 Laurent Montel" ) );

    mAboutData->addAuthor( ki18n( "Laurent Montel" ),
                         ki18n( "Maintainer" ), "montel@kde.org" );

    mAboutData->setProgramIconName( QLatin1String("kmail") );
    mAboutData->setTranslator( ki18nc( "NAME OF TRANSLATORS", "Your names" ),
                             ki18nc( "EMAIL OF TRANSLATORS", "Your emails" ) );

//QT5
#if 0
    KHelpMenu *helpMenu = new KHelpMenu(this, mAboutData, true);
    //Initialize menu
    QMenu *menu = helpMenu->menu();
    helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(QLatin1String("kmail")));
    setButtonMenu( Help, menu );
#endif
    readConfig();
}

NewMailNotifierSettingsDialog::~NewMailNotifierSettingsDialog()
{
    writeConfig();
    delete mAboutData;
}

static const char *myConfigGroupName = "NewMailNotifierDialog";

void NewMailNotifierSettingsDialog::readConfig()
{
    KConfigGroup group( KSharedConfig::openConfig(), myConfigGroupName );

    const QSize size = group.readEntry( "Size", QSize(500, 300) );
    if ( size.isValid() ) {
        resize( size );
    }
}

void NewMailNotifierSettingsDialog::writeConfig()
{
    KConfigGroup group( KSharedConfig::openConfig(), myConfigGroupName );
    group.writeEntry( "Size", size() );
    group.sync();
}


void NewMailNotifierSettingsDialog::slotHelpLinkClicked(const QString &)
{
    const QString help =
            i18n( textToSpeakMessage);

    QWhatsThis::showText( QCursor::pos(), help );
}

void NewMailNotifierSettingsDialog::slotOkClicked()
{
    mSelectCollection->updateCollectionsRecursive(QModelIndex());

    NewMailNotifierAgentSettings::setShowPhoto(mShowPhoto->isChecked());
    NewMailNotifierAgentSettings::setShowFrom(mShowFrom->isChecked());
    NewMailNotifierAgentSettings::setShowSubject(mShowSubject->isChecked());
    NewMailNotifierAgentSettings::setShowFolder(mShowFolders->isChecked());
    NewMailNotifierAgentSettings::setExcludeEmailsFromMe(mExcludeMySelf->isChecked());
    NewMailNotifierAgentSettings::setTextToSpeakEnabled(mTextToSpeak->isChecked());
    NewMailNotifierAgentSettings::setTextToSpeak(mTextToSpeakSetting->text());
    NewMailNotifierAgentSettings::setShowButtonToDisplayMail(mAllowToShowMail->isChecked());
    NewMailNotifierAgentSettings::self()->save();
    mNotify->save();
    accept();
}



