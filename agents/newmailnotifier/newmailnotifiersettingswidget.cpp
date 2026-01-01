/*
    SPDX-FileCopyrightText: 2013-2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotifiersettingswidget.h"
using namespace Qt::Literals::StringLiterals;

#include "newmailnotifieragentsettings.h"
#include "newmailnotifierselectcollectionwidget.h"

#include "kdepim-runtime-version.h"

#include <KLineEditEventHandler>
#include <KLocalizedString>
#include <KNotifyConfigWidget>
#include <QLineEdit>
#include <QPushButton>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWhatsThis>

#include <KLazyLocalizedString>
#include <KSharedConfig>

static KLazyLocalizedString textToSpeakMessage = kli18n(
    "<qt>"
    "<p>Here you can define message. "
    "You can use:</p>"
    "<ul>"
    "<li>%s set subject</li>"
    "<li>%f set from</li>"
    "</ul>"
    "</qt>");

NewMailNotifierSettingsWidget::NewMailNotifierSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , mSelectCollection(new NewMailNotifierSelectCollectionWidget(parent))
{
    NewMailNotifierAgentSettings::instance(config);
    setObjectName("NewMailNotifierSettingsWidget"_L1);
    auto tab = new QTabWidget(parent);
    parent->layout()->addWidget(tab);

    auto settings = new QWidget;
    auto vbox = new QVBoxLayout(settings);

    auto grp = new QGroupBox(i18n("Choose which fields to show:"), parent);
    vbox->addWidget(grp);
    auto groupboxLayout = new QVBoxLayout;
    grp->setLayout(groupboxLayout);

    mShowPhoto = new QCheckBox(i18nc("@option:check", "Show Photo"), parent);
    mShowPhoto->setObjectName("mShowPhoto"_L1);
    groupboxLayout->addWidget(mShowPhoto);

    mShowFrom = new QCheckBox(i18nc("@option:check", "Show From"), parent);
    mShowFrom->setObjectName("mShowFrom"_L1);
    groupboxLayout->addWidget(mShowFrom);

    mShowSubject = new QCheckBox(i18nc("@option:check", "Show Subject"), parent);
    mShowSubject->setObjectName("mShowSubject"_L1);
    groupboxLayout->addWidget(mShowSubject);

    mShowFolders = new QCheckBox(i18nc("@option:check", "Show Folders"), parent);
    mShowFolders->setObjectName("mShowFolders"_L1);
    groupboxLayout->addWidget(mShowFolders);

    mExcludeMySelf = new QCheckBox(i18nc("@option:check", "Do not notify when email was sent by me"), parent);
    mExcludeMySelf->setObjectName("mExcludeMySelf"_L1);
    vbox->addWidget(mExcludeMySelf);

    mKeepPersistentNotification = new QCheckBox(i18nc("@option:check", "Keep Persistent Notification"), parent);
    mKeepPersistentNotification->setObjectName("mKeepPersistentNotification"_L1);
    vbox->addWidget(mKeepPersistentNotification);

    mAllowToShowMail = new QCheckBox(i18nc("@option:check", "Show Action Buttons"), parent);
    mAllowToShowMail->setObjectName("mAllowToShowMail"_L1);
    vbox->addWidget(mAllowToShowMail);

    auto hboxLayout = new QHBoxLayout;
    hboxLayout->setObjectName("hboxLayout"_L1);
    vbox->addLayout(hboxLayout);

    mReplyMail = new QCheckBox(i18nc("@option:check", "Reply Mail"), parent);
    mReplyMail->setObjectName("mReplyMail"_L1);
    hboxLayout->addWidget(mReplyMail);
    mReplyMail->setEnabled(false);

    mReplyMailTypeComboBox = new QComboBox(parent);
    mReplyMailTypeComboBox->setObjectName("mReplyMailTypeComboBox"_L1);
    mReplyMailTypeComboBox->setEnabled(false);
    mReplyMailTypeComboBox->addItems({i18n("Reply to Author"), i18n("Reply to All")});
    hboxLayout->addWidget(mReplyMailTypeComboBox);
    hboxLayout->addStretch(1);

    connect(mAllowToShowMail, &QCheckBox::clicked, this, [this](bool enabled) {
        updateReplyMail(enabled);
    });

    vbox->addStretch();
    tab->addTab(settings, i18n("Display"));
#if HAVE_TEXT_TO_SPEECH_SUPPORT
    auto textSpeakWidget = new QWidget;
    vbox = new QVBoxLayout;
    textSpeakWidget->setLayout(vbox);
    mTextToSpeak = new QCheckBox(i18nc("@option:check", "Enabled"), parent);
    mTextToSpeak->setObjectName("mTextToSpeak"_L1);
    vbox->addWidget(mTextToSpeak);

    auto howIsItWork = new QLabel(i18nc("@label:textbox", "<a href=\"whatsthis\">How does this work?</a>"), parent);
    howIsItWork->setObjectName("howIsItWork"_L1);
    howIsItWork->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    howIsItWork->setContextMenuPolicy(Qt::NoContextMenu);
    vbox->addWidget(howIsItWork);
    connect(howIsItWork, &QLabel::linkActivated, this, &NewMailNotifierSettingsWidget::slotHelpLinkClicked);

    auto textToSpeakLayout = new QHBoxLayout;
    textToSpeakLayout->setContentsMargins({});
    auto lab = new QLabel(i18nc("@label:textbox", "Message:"), parent);
    lab->setObjectName("labmessage"_L1);
    textToSpeakLayout->addWidget(lab);
    mTextToSpeakSetting = new QLineEdit(parent);
    mTextToSpeakSetting->setObjectName("mTextToSpeakSetting"_L1);
    mTextToSpeakSetting->setClearButtonEnabled(true);
    KLineEditEventHandler::catchReturnKey(mTextToSpeakSetting);

    mTextToSpeakSetting->setWhatsThis(textToSpeakMessage.toString());
    textToSpeakLayout->addWidget(mTextToSpeakSetting);
    vbox->addLayout(textToSpeakLayout);
    vbox->addStretch();
    tab->addTab(textSpeakWidget, i18n("Text to Speak"));
    connect(mTextToSpeak, &QCheckBox::toggled, mTextToSpeakSetting, &QLineEdit::setEnabled);
#endif
    mNotify = new KNotifyConfigWidget(parent);
    mNotify->setObjectName("mNotify"_L1);
    mNotify->setApplication(QStringLiteral("akonadi_newmailnotifier_agent"));
    tab->addTab(mNotify, i18n("Notify"));

    mSelectCollection->setObjectName("mSelectCollection"_L1);
    tab->addTab(mSelectCollection, i18n("Folders"));
}

NewMailNotifierSettingsWidget::~NewMailNotifierSettingsWidget()
{
    delete NewMailNotifierAgentSettings::self();
}

void NewMailNotifierSettingsWidget::updateReplyMail(bool enabled)
{
    mReplyMail->setEnabled(enabled);
    mReplyMailTypeComboBox->setEnabled(enabled);
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
#if HAVE_TEXT_TO_SPEECH_SUPPORT
    mTextToSpeak->setChecked(settings->textToSpeakEnabled());
    mTextToSpeakSetting->setEnabled(mTextToSpeak->isChecked());
    mTextToSpeakSetting->setText(settings->textToSpeak());
#endif
    mReplyMail->setChecked(settings->replyMail());
    mReplyMailTypeComboBox->setCurrentIndex(settings->replyMailType());

    updateReplyMail(mAllowToShowMail->isChecked());
}

bool NewMailNotifierSettingsWidget::save() const
{
    mSelectCollection->updateCollectionsRecursive();
    auto settings = NewMailNotifierAgentSettings::self();
    settings->setShowPhoto(mShowPhoto->isChecked());
    settings->setShowFrom(mShowFrom->isChecked());
    settings->setShowSubject(mShowSubject->isChecked());
    settings->setShowFolder(mShowFolders->isChecked());
    settings->setExcludeEmailsFromMe(mExcludeMySelf->isChecked());
    settings->setShowButtonToDisplayMail(mAllowToShowMail->isChecked());
    settings->setKeepPersistentNotification(mKeepPersistentNotification->isChecked());
#if HAVE_TEXT_TO_SPEECH_SUPPORT
    settings->setTextToSpeakEnabled(mTextToSpeak->isChecked());
    settings->setTextToSpeak(mTextToSpeakSetting->text());
#endif
    settings->setReplyMail(mReplyMail->isChecked());
    settings->setReplyMailType(mReplyMailTypeComboBox->currentIndex());
    settings->save();
    mNotify->save();

    return Akonadi::AgentConfigurationBase::save();
}

void NewMailNotifierSettingsWidget::slotHelpLinkClicked(const QString &)
{
    const QString help = textToSpeakMessage.toString();
    QWhatsThis::showText(QCursor::pos(), help);
}

#include "moc_newmailnotifiersettingswidget.cpp"
