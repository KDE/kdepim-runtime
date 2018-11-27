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

#include "newmailnotifiersettingswidget.h"
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
#include <KSharedConfig>

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

NewMailNotifierSettingsWidget::NewMailNotifierSettingsWidget(KSharedConfigPtr config,
                                                             QWidget *parent,
                                                             const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    NewMailNotifierAgentSettings::instance(config);

    QVBoxLayout *mainLayout = new QVBoxLayout(parent);

    QTabWidget *tab = new QTabWidget;
    mainLayout->addWidget(tab);

    QWidget *settings = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    settings->setLayout(vbox);

    QGroupBox *grp = new QGroupBox(i18n("Choose which fields to show:"), parent);
    vbox->addWidget(grp);
    QVBoxLayout *groupboxLayout = new QVBoxLayout;
    grp->setLayout(groupboxLayout);

    mShowPhoto = new QCheckBox(i18n("Show Photo"), parent);
    mShowPhoto->setObjectName(QStringLiteral("mShowPhoto"));
    groupboxLayout->addWidget(mShowPhoto);

    mShowFrom = new QCheckBox(i18n("Show From"), parent);
    mShowFrom->setObjectName(QStringLiteral("mShowFrom"));
    groupboxLayout->addWidget(mShowFrom);

    mShowSubject = new QCheckBox(i18n("Show Subject"), parent);
    mShowSubject->setObjectName(QStringLiteral("mShowSubject"));
    groupboxLayout->addWidget(mShowSubject);

    mShowFolders = new QCheckBox(i18n("Show Folders"), parent);
    mShowFolders->setObjectName(QStringLiteral("mShowFolders"));
    groupboxLayout->addWidget(mShowFolders);

    mExcludeMySelf = new QCheckBox(i18n("Do not notify when email was sent by me"), parent);
    mExcludeMySelf->setObjectName(QStringLiteral("mExcludeMySelf"));
    vbox->addWidget(mExcludeMySelf);

    mAllowToShowMail = new QCheckBox(i18n("Show Action Buttons"), parent);
    mAllowToShowMail->setObjectName(QStringLiteral("mAllowToShowMail"));
    vbox->addWidget(mAllowToShowMail);

    mKeepPersistentNotification = new QCheckBox(i18n("Keep Persistent Notification"), parent);
    mKeepPersistentNotification->setObjectName(QStringLiteral("mKeepPersistentNotification"));
    vbox->addWidget(mKeepPersistentNotification);

    vbox->addStretch();
    tab->addTab(settings, i18n("Display"));

    QWidget *textSpeakWidget = new QWidget;
    vbox = new QVBoxLayout;
    textSpeakWidget->setLayout(vbox);
    mTextToSpeak = new QCheckBox(i18n("Enabled"), parent);
    mTextToSpeak->setObjectName(QStringLiteral("mTextToSpeak"));
    vbox->addWidget(mTextToSpeak);

    QLabel *howIsItWork = new QLabel(i18n("<a href=\"whatsthis\">How does this work?</a>"), parent);
    howIsItWork->setObjectName(QStringLiteral("howIsItWork"));
    howIsItWork->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    howIsItWork->setContextMenuPolicy(Qt::NoContextMenu);
    vbox->addWidget(howIsItWork);
    connect(howIsItWork, &QLabel::linkActivated, this, &NewMailNotifierSettingsDialog::slotHelpLinkClicked);

    QHBoxLayout *textToSpeakLayout = new QHBoxLayout;
    textToSpeakLayout->setMargin(0);
    QLabel *lab = new QLabel(i18n("Message:"), parent);
    lab->setObjectName(QStringLiteral("labmessage"));
    textToSpeakLayout->addWidget(lab);
    mTextToSpeakSetting = new QLineEdit;
    mTextToSpeakSetting->setObjectName(QStringLiteral("mTextToSpeakSetting"));
    mTextToSpeakSetting->setClearButtonEnabled(true);
    mTextToSpeakSetting->setWhatsThis(i18n(textToSpeakMessage));
    textToSpeakLayout->addWidget(mTextToSpeakSetting);
    vbox->addLayout(textToSpeakLayout);
    vbox->addStretch();
    tab->addTab(textSpeakWidget, i18n("Text to Speak"));
    connect(mTextToSpeak, &QCheckBox::toggled, mTextToSpeakSetting, &QLineEdit::setEnabled);

    mNotify = new KNotifyConfigWidget(parent);
    mNotify->setObjectName(QStringLiteral("mNotify"));
    mNotify->setApplication(QStringLiteral("akonadi_newmailnotifier_agent"));
    tab->addTab(mNotify, i18n("Notify"));

    mSelectCollection = new NewMailNotifierSelectCollectionWidget(parent);
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
    setKAboutData(aboutData);
}

NewMailNotifierSettingsWidget::~NewMailNotifierSettingsWidget()
{
    delete NewMailNotifierAgentSettings::self();
}

void NewMailNotifierSettingsWidget::load()
{
    Akonadi::AgentConfigurationBase::load();

    auto settings = NewMailNotifierAgentSettings::self();
    settings->load();

    mShowPhoto->setChecked(settings->showPhoto());
    mShowFrom->setChecked(settings->showFrom());
    mShowSubject->setChecked(settings->showSubject());
    mShowFolders->setChecked(settings->showFolder());
    mExcludeMySelf->setChecked(settings->excludeEmailsFromMe());
    mAllowToShowMail->setChecked(settings->showButtonToDisplayMail());
    mKeepPersistentNotification->setChecked(settings->keepPersistentNotification());
#ifdef HAVE_TEXTTOSPEECH
    mTextToSpeak->setChecked(settings->textToSpeakEnabled());
    mTextToSpeakSetting->setEnabled(mTextToSpeak->isChecked());
    mTextToSpeakSetting->setText(settings->textToSpeak());
#endif
}

bool NewMailNotifierSettingsWidget::save() const
{
    auto settings = NewMailNotifierAgentSettings::self();
    settings->setShowPhoto(mShowPhoto->isChecked());
    settings->setShowFrom(mShowFrom->isChecked());
    settings->setShowSubject(mShowSubject->isChecked());
    settings->setShowFolder(mShowFolders->isChecked());
    settings->setExcludeEmailsFromMe(mExcludeMySelf->isChecked());
    settings->setShowButtonToDisplayMail(mAllowToShowMail->isChecked());
    settings->setKeepPersistentNotification(mKeepPersistentNotification->isChecked());
#ifdef HAVE_TEXTTOSPEECH
    settings->setTextToSpeakEnabled(mTextToSpeak->isChecked());
    settings->setTextToSpeak(mTextToSpeakSetting->text());
#endif
    settings->save();
    mNotify->save();

    return Akonadi::AgentConfigurationBase::save();
}

void NewMailNotifierSettingsWidget::slotHelpLinkClicked(const QString &)
{
    const QString help
        = i18n(textToSpeakMessage);

    QWhatsThis::showText(QCursor::pos(), help);
}

