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

#include "newmailnotifiersettingsdialog.h"
#include "newmailnotifierattribute.h"
#include "newmailnotifierselectcollectionwidget.h"
#include "newmailnotifieragentsettings.h"

#include "kdepim-runtime-version.h"

#include <KLocalizedString>
#include <KNotifyConfigWidget>
#include <QLineEdit>
#include <KCheckableProxyModel>
#include <QPushButton>
#include <KHelpMenu>
#include <kaboutdata.h>
#include <QIcon>

#include <QTabWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QWhatsThis>
#include <QAction>

#include <AkonadiWidgets/CollectionView>
#include <AkonadiCore/RecursiveCollectionFilterProxyModel>
#include <AkonadiCore/CollectionFilterProxyModel>
#include <AkonadiCore/CollectionModifyJob>
#include <KSharedConfig>
#include <QDialogButtonBox>
#include <KConfigGroup>

static const char *textToSpeakMessage
    = I18N_NOOP("<qt>"
                "<p>Here you can define message. "
                "You can use:</p>"
                "<ul>"
                "<li>%s set subject</li>"
                "<li>%f set from</li>"
                "</ul>"
                "</qt>");

NewMailNotifierSettingsDialog::NewMailNotifierSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18n("New Mail Notifier settings"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NewMailNotifierSettingsDialog::slotOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NewMailNotifierSettingsDialog::reject);

    QTabWidget *tab = new QTabWidget;
    mainLayout->addWidget(tab);

    QWidget *settings = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    settings->setLayout(vbox);

    QGroupBox *grp = new QGroupBox(i18n("Choose which fields to show:"), this);
    vbox->addWidget(grp);
    QVBoxLayout *groupboxLayout = new QVBoxLayout;
    grp->setLayout(groupboxLayout);

    mShowPhoto = new QCheckBox(i18n("Show Photo"), this);
    mShowPhoto->setObjectName(QStringLiteral("mShowPhoto"));
    mShowPhoto->setChecked(NewMailNotifierAgentSettings::showPhoto());
    groupboxLayout->addWidget(mShowPhoto);

    mShowFrom = new QCheckBox(i18n("Show From"), this);
    mShowFrom->setObjectName(QStringLiteral("mShowFrom"));
    mShowFrom->setChecked(NewMailNotifierAgentSettings::showFrom());
    groupboxLayout->addWidget(mShowFrom);

    mShowSubject = new QCheckBox(i18n("Show Subject"), this);
    mShowSubject->setObjectName(QStringLiteral("mShowSubject"));
    mShowSubject->setChecked(NewMailNotifierAgentSettings::showSubject());
    groupboxLayout->addWidget(mShowSubject);

    mShowFolders = new QCheckBox(i18n("Show Folders"), this);
    mShowFolders->setObjectName(QStringLiteral("mShowFolders"));
    mShowFolders->setChecked(NewMailNotifierAgentSettings::showFolder());
    groupboxLayout->addWidget(mShowFolders);

    mExcludeMySelf = new QCheckBox(i18n("Do not notify when email was sent by me"), this);
    mExcludeMySelf->setObjectName(QStringLiteral("mExcludeMySelf"));
    mExcludeMySelf->setChecked(NewMailNotifierAgentSettings::excludeEmailsFromMe());
    vbox->addWidget(mExcludeMySelf);

    mAllowToShowMail = new QCheckBox(i18n("Show Action Buttons"), this);
    mAllowToShowMail->setObjectName(QStringLiteral("mAllowToShowMail"));
    mAllowToShowMail->setChecked(NewMailNotifierAgentSettings::showButtonToDisplayMail());
    vbox->addWidget(mAllowToShowMail);

    mKeepPersistentNotification = new QCheckBox(i18n("Keep Persistent Notification"), this);
    mKeepPersistentNotification->setObjectName(QStringLiteral("mKeepPersistentNotification"));
    mKeepPersistentNotification->setChecked(NewMailNotifierAgentSettings::keepPersistentNotification());
    vbox->addWidget(mKeepPersistentNotification);

    vbox->addStretch();
    tab->addTab(settings, i18n("Display"));

    QWidget *textSpeakWidget = new QWidget;
    vbox = new QVBoxLayout;
    textSpeakWidget->setLayout(vbox);
    mTextToSpeak = new QCheckBox(i18n("Enabled"), this);
    mTextToSpeak->setObjectName(QStringLiteral("mTextToSpeak"));
    mTextToSpeak->setChecked(NewMailNotifierAgentSettings::textToSpeakEnabled());
    vbox->addWidget(mTextToSpeak);

    QLabel *howIsItWork = new QLabel(i18n("<a href=\"whatsthis\">How does this work?</a>"), this);
    howIsItWork->setObjectName(QStringLiteral("howIsItWork"));
    howIsItWork->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    howIsItWork->setContextMenuPolicy(Qt::NoContextMenu);
    vbox->addWidget(howIsItWork);
    connect(howIsItWork, &QLabel::linkActivated, this, &NewMailNotifierSettingsDialog::slotHelpLinkClicked);

    QHBoxLayout *textToSpeakLayout = new QHBoxLayout;
    textToSpeakLayout->setMargin(0);
    QLabel *lab = new QLabel(i18n("Message:"), this);
    lab->setObjectName(QStringLiteral("labmessage"));
    textToSpeakLayout->addWidget(lab);
    mTextToSpeakSetting = new QLineEdit;
    mTextToSpeakSetting->setObjectName(QStringLiteral("mTextToSpeakSetting"));
    mTextToSpeakSetting->setClearButtonEnabled(true);
    mTextToSpeakSetting->setText(NewMailNotifierAgentSettings::textToSpeak());
    mTextToSpeakSetting->setEnabled(mTextToSpeak->isChecked());
    mTextToSpeakSetting->setWhatsThis(i18n(textToSpeakMessage));
    textToSpeakLayout->addWidget(mTextToSpeakSetting);
    vbox->addLayout(textToSpeakLayout);
    vbox->addStretch();
    tab->addTab(textSpeakWidget, i18n("Text to Speak"));
    connect(mTextToSpeak, &QCheckBox::toggled, mTextToSpeakSetting, &QLineEdit::setEnabled);

    mNotify = new KNotifyConfigWidget(this);
    mNotify->setObjectName(QStringLiteral("mNotify"));
    mNotify->setApplication(QStringLiteral("akonadi_newmailnotifier_agent"));
    tab->addTab(mNotify, i18n("Notify"));

    mSelectCollection = new NewMailNotifierSelectCollectionWidget(this);
    mSelectCollection->setObjectName(QStringLiteral("mSelectCollection"));
    tab->addTab(mSelectCollection, i18n("Folders"));

    KAboutData aboutData = KAboutData(
        QStringLiteral("newmailnotifieragent"),
        i18n("New Mail Notifier Agent"),
        QStringLiteral(KDEPIM_RUNTIME_VERSION),
        i18n("Notify about new mails."),
        KAboutLicense::GPL_V2,
        i18n("Copyright (C) 2013-2018 Laurent Montel"));

    aboutData.addAuthor(i18n("Laurent Montel"),
                        i18n("Maintainer"), QStringLiteral("montel@kde.org"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                            i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    KHelpMenu *helpMenu = new KHelpMenu(this, aboutData, true);
    helpMenu->action(KHelpMenu::menuHelpContents)->setVisible(false);
    //Initialize menu
    QMenu *menu = helpMenu->menu();
    helpMenu->action(KHelpMenu::menuAboutApp)->setIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    buttonBox->button(QDialogButtonBox::Help)->setMenu(menu);

    mainLayout->addWidget(buttonBox);
    readConfig();
}

NewMailNotifierSettingsDialog::~NewMailNotifierSettingsDialog()
{
    writeConfig();
}

static const char *myConfigGroupName = "NewMailNotifierDialog";

void NewMailNotifierSettingsDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), myConfigGroupName);

    const QSize size = group.readEntry("Size", QSize(500, 300));
    if (size.isValid()) {
        resize(size);
    }
}

void NewMailNotifierSettingsDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), myConfigGroupName);
    group.writeEntry("Size", size());
    group.sync();
}

void NewMailNotifierSettingsDialog::slotHelpLinkClicked(const QString &)
{
    const QString help
        = i18n(textToSpeakMessage);

    QWhatsThis::showText(QCursor::pos(), help);
}

void NewMailNotifierSettingsDialog::slotOkClicked()
{
    mSelectCollection->updateCollectionsRecursive();

    NewMailNotifierAgentSettings::setShowPhoto(mShowPhoto->isChecked());
    NewMailNotifierAgentSettings::setShowFrom(mShowFrom->isChecked());
    NewMailNotifierAgentSettings::setShowSubject(mShowSubject->isChecked());
    NewMailNotifierAgentSettings::setShowFolder(mShowFolders->isChecked());
    NewMailNotifierAgentSettings::setExcludeEmailsFromMe(mExcludeMySelf->isChecked());
    NewMailNotifierAgentSettings::setKeepPersistentNotification(mKeepPersistentNotification->isChecked());
    NewMailNotifierAgentSettings::setTextToSpeakEnabled(mTextToSpeak->isChecked());
    NewMailNotifierAgentSettings::setTextToSpeak(mTextToSpeakSetting->text());
    NewMailNotifierAgentSettings::setShowButtonToDisplayMail(mAllowToShowMail->isChecked());
    NewMailNotifierAgentSettings::self()->save();
    mNotify->save();
    accept();
}
