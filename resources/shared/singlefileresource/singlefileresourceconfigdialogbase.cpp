/*
    Copyright (c) 2008 Bertjan Broeksema <b.broeksema@kdemail.org>
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010,2011 David Jarvie <djarvie@kde.org>

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

#include "singlefileresourceconfigdialogbase.h"

#include <QTabWidget>
#include <KConfigDialogManager>
#include <KFileItem>
#include <KIO/Job>
#include <KWindowSystem>
#include <QUrl>
#include <QTimer>

#include <KLocalizedString>
#include <KSharedConfig>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Akonadi;

SingleFileResourceConfigDialogBase::SingleFileResourceConfigDialogBase(WId windowId) :
    QDialog(),
    mManager(Q_NULLPTR),
    mStatJob(Q_NULLPTR),
    mAppendedWidget(Q_NULLPTR),
    mDirUrlChecked(false),
    mMonitorEnabled(true),
    mLocalFileOnly(false)
{
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    ui.setupUi(mainWidget);
    ui.kcfg_Path->setMode(KFile::File);
    ui.statusLabel->setText(QString());

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SingleFileResourceConfigDialogBase::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SingleFileResourceConfigDialogBase::reject);
    mainLayout->addWidget(buttonBox);

    if (windowId) {
        KWindowSystem::setMainWindow(this, windowId);
    }

    ui.tabWidget->tabBar()->hide();

    connect(mOkButton, &QPushButton::clicked, this, &SingleFileResourceConfigDialogBase::save);

    connect(ui.kcfg_Path, &KUrlRequester::textChanged, this, &SingleFileResourceConfigDialogBase::validate);
    connect(ui.kcfg_MonitorFile, &QCheckBox::toggled, this, &SingleFileResourceConfigDialogBase::validate);
    ui.kcfg_Path->setFocus();
    QTimer::singleShot(0, this, &SingleFileResourceConfigDialogBase::validate);
    setMinimumSize(600, 540);
    readConfig();
}

SingleFileResourceConfigDialogBase::~SingleFileResourceConfigDialogBase()
{
    writeConfig();
}

void SingleFileResourceConfigDialogBase::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "SingleFileResourceConfigDialogBase");
    group.writeEntry("Size", size());
}

void SingleFileResourceConfigDialogBase::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "SingleFileResourceConfigDialogBase");
    const QSize sizeDialog = group.readEntry("Size", QSize(600, 540));
    if (sizeDialog.isValid()) {
        resize(sizeDialog);
    }
}

void SingleFileResourceConfigDialogBase::addPage(const QString &title, QWidget *page)
{
    ui.tabWidget->tabBar()->show();
    ui.tabWidget->addTab(page, title);
    mManager->addWidget(page);
    mManager->updateWidgets();
}

void SingleFileResourceConfigDialogBase::setFilter(const QString &filter)
{
    ui.kcfg_Path->setFilter(filter);
}

void SingleFileResourceConfigDialogBase::setMonitorEnabled(bool enable)
{
    mMonitorEnabled = enable;
    ui.groupBox_MonitorFile->setVisible(mMonitorEnabled);
}

void SingleFileResourceConfigDialogBase::setUrl(const QUrl &url)
{
    ui.kcfg_Path->setUrl(url);
}

QUrl SingleFileResourceConfigDialogBase::url() const
{
    return ui.kcfg_Path->url();
}

void SingleFileResourceConfigDialogBase::setLocalFileOnly(bool local)
{
    mLocalFileOnly = local;
    ui.kcfg_Path->setMode(mLocalFileOnly ? KFile::File | KFile::LocalOnly : KFile::File);
}

void SingleFileResourceConfigDialogBase::appendWidget(SingleFileValidatingWidget *widget)
{
    widget->setParent(static_cast<QWidget *>(ui.tab));
    ui.tabLayout->addWidget(widget);
    connect(widget, &SingleFileValidatingWidget::changed, this, &SingleFileResourceConfigDialogBase::validate);
    mAppendedWidget = widget;
}

void SingleFileResourceConfigDialogBase::validate()
{
    if (mAppendedWidget && !mAppendedWidget->validate()) {
        mOkButton->setEnabled(false);
        return;
    }

    const QUrl currentUrl = ui.kcfg_Path->url();
    if (ui.kcfg_Path->text().trimmed().isEmpty() || currentUrl.isEmpty()) {
        mOkButton->setEnabled(false);
        return;
    }

    if (currentUrl.isLocalFile()) {
        if (mMonitorEnabled) {
            ui.kcfg_MonitorFile->setEnabled(true);
        }
        ui.statusLabel->setText(QString());

        // The read-only checkbox used to be disabled if the file is read-only,
        // but it is then impossible to know at a later date if the file
        // permissions change, whether the user actually wanted the resource to be
        // read-only or not. So just leave the read-only checkbox untouched.
        mOkButton->setEnabled(true);
    } else {
        // Not a local file.
        if (mLocalFileOnly) {
            mOkButton->setEnabled(false);
            return;
        }
        if (mMonitorEnabled) {
            ui.kcfg_MonitorFile->setEnabled(false);
        }
        ui.statusLabel->setText(i18nc("@info:status", "Checking file information..."));

        if (mStatJob) {
            mStatJob->kill();
        }

        mStatJob = KIO::stat(currentUrl, KIO::DefaultFlags | KIO::HideProgressInfo);
        mStatJob->setDetails(2);   // All details.
        mStatJob->setSide(KIO::StatJob::SourceSide);

        connect(mStatJob, &KIO::StatJob::result, this, &SingleFileResourceConfigDialogBase::slotStatJobResult);

        // Allow the OK button to be disabled until the MetaJob is finished.
        mOkButton->setEnabled(false);
    }
}

void SingleFileResourceConfigDialogBase::slotStatJobResult(KJob *job)
{
    if (job->error() == KIO::ERR_DOES_NOT_EXIST && !mDirUrlChecked) {
        // The file did not exist, so let's see if the directory the file should
        // reside in supports writing.

        QUrl dirUrl(ui.kcfg_Path->url());
        dirUrl = KIO::upUrl(dirUrl);

        mStatJob = KIO::stat(dirUrl, KIO::DefaultFlags | KIO::HideProgressInfo);
        mStatJob->setDetails(2);   // All details.
        mStatJob->setSide(KIO::StatJob::SourceSide);

        connect(mStatJob, &KIO::StatJob::result, this, &SingleFileResourceConfigDialogBase::slotStatJobResult);

        // Make sure we don't check the whole path upwards.
        mDirUrlChecked = true;
        return;
    } else if (job->error()) {
        // It doesn't seem possible to read nor write from the location so leave the
        // ok button disabled
        ui.statusLabel->setText(QString());
        mOkButton->setEnabled(false);
        mDirUrlChecked = false;
        mStatJob = Q_NULLPTR;
        return;
    }

    ui.statusLabel->setText(QString());
    mOkButton->setEnabled(true);

    mDirUrlChecked = false;
    mStatJob = Q_NULLPTR;
}

SingleFileValidatingWidget::SingleFileValidatingWidget(QWidget *parent)
    : QWidget(parent)
{
}
