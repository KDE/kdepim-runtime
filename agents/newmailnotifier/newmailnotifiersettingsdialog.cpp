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
#include "newmailnotifieragentsettings.h"

#include <KLocale>
#include <KNotifyConfigWidget>
#include <KLineEdit>

#include <QTabWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QWhatsThis>

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
    howIsItWork->setTextInteractionFlags(Qt::TextBrowserInteraction);
    vbox->addWidget(howIsItWork);
    connect(howIsItWork, SIGNAL(linkActivated(QString)),SLOT(slotHelpLinkClicked(QString)) );

    QHBoxLayout *textToSpeakLayout = new QHBoxLayout;
    textToSpeakLayout->setMargin(0);
    QLabel *lab = new QLabel(i18n("message:"));
    textToSpeakLayout->addWidget(lab);
    mTextToSpeakSetting = new KLineEdit;
    mTextToSpeakSetting->setText(NewMailNotifierAgentSettings::textToSpeak());
    textToSpeakLayout->addWidget(mTextToSpeakSetting);
    vbox->addLayout(textToSpeakLayout);
    vbox->addStretch();
    tab->addTab(textSpeakWidget, i18n("Text to Speak"));



    mNotify = new KNotifyConfigWidget(this);
    mNotify->setApplication(QLatin1String("akonadi_newmailnotifier_agent"));
    tab->addTab(mNotify, i18n("Notify"));

    setMainWidget(w);
}

NewMailNotifierSettingsDialog::~NewMailNotifierSettingsDialog()
{
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

#include "newmailnotifiersettingsdialog.moc"
