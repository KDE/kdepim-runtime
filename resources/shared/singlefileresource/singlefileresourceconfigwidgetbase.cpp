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

#include "singlefileresourceconfigwidgetbase.h"

#include <KIO/Job>
#include <QUrl>
#include <QTimer>

#include <KConfigDialogManager>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>
#include <QTabBar>
#include <QVBoxLayout>

using namespace Akonadi;

SingleFileResourceConfigWidgetBase::SingleFileResourceConfigWidgetBase(QWidget *parent)
    : QWidget(parent)
{
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    ui.setupUi(mainWidget);
    ui.kcfg_Path->setMode(KFile::File);
    ui.statusLabel->setText(QString());

    ui.tabWidget->tabBar()->hide();

    connect(ui.kcfg_Path, &KUrlRequester::textChanged, this, &SingleFileResourceConfigWidgetBase::validate);
    connect(ui.kcfg_MonitorFile, &QCheckBox::toggled, this, &SingleFileResourceConfigWidgetBase::validate);
    ui.kcfg_Path->setFocus();
    QTimer::singleShot(0, this, &SingleFileResourceConfigWidgetBase::validate);
}

SingleFileResourceConfigWidgetBase::~SingleFileResourceConfigWidgetBase()
{
}

void SingleFileResourceConfigWidgetBase::addPage(const QString &title, QWidget *page)
{
    ui.tabWidget->tabBar()->show();
    ui.tabWidget->addTab(page, title);
    mManager->addWidget(page);
    mManager->updateWidgets();
}

void SingleFileResourceConfigWidgetBase::setFilter(const QString &filter)
{
    ui.kcfg_Path->setFilter(filter);
}

void SingleFileResourceConfigWidgetBase::setMonitorEnabled(bool enable)
{
    mMonitorEnabled = enable;
    ui.groupBox_MonitorFile->setVisible(mMonitorEnabled);
}

void SingleFileResourceConfigWidgetBase::setUrl(const QUrl &url)
{
    ui.kcfg_Path->setUrl(url);
}

QUrl SingleFileResourceConfigWidgetBase::url() const
{
    return ui.kcfg_Path->url();
}

void SingleFileResourceConfigWidgetBase::setLocalFileOnly(bool local)
{
    mLocalFileOnly = local;
    ui.kcfg_Path->setMode(mLocalFileOnly ? KFile::File | KFile::LocalOnly : KFile::File);
}

void SingleFileResourceConfigWidgetBase::appendWidget(SingleFileValidatingWidget *widget)
{
    widget->setParent(static_cast<QWidget *>(ui.tab));
    ui.tabLayout->addWidget(widget);
    connect(widget, &SingleFileValidatingWidget::changed, this, &SingleFileResourceConfigWidgetBase::validate);
    mAppendedWidget = widget;
}

void SingleFileResourceConfigWidgetBase::validate()
{
    if (mAppendedWidget && !mAppendedWidget->validate()) {
        Q_EMIT okEnabled(false);
        return;
    }

    const QUrl currentUrl = ui.kcfg_Path->url();
    if (ui.kcfg_Path->text().trimmed().isEmpty() || currentUrl.isEmpty()) {
        Q_EMIT okEnabled(false);
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
        Q_EMIT okEnabled(true);
    } else {
        // Not a local file.
        if (mLocalFileOnly) {
            Q_EMIT okEnabled(false);
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

        connect(mStatJob, &KIO::StatJob::result, this, &SingleFileResourceConfigWidgetBase::slotStatJobResult);

        // Allow the OK button to be disabled until the MetaJob is finished.
        Q_EMIT okEnabled(false);
    }
}

void SingleFileResourceConfigWidgetBase::slotStatJobResult(KJob *job)
{
    if (job->error() == KIO::ERR_DOES_NOT_EXIST && !mDirUrlChecked) {
        // The file did not exist, so let's see if the directory the file should
        // reside in supports writing.

        QUrl dirUrl(ui.kcfg_Path->url());
        dirUrl = KIO::upUrl(dirUrl);

        mStatJob = KIO::stat(dirUrl, KIO::DefaultFlags | KIO::HideProgressInfo);
        mStatJob->setDetails(2);   // All details.
        mStatJob->setSide(KIO::StatJob::SourceSide);

        connect(mStatJob, &KIO::StatJob::result, this, &SingleFileResourceConfigWidgetBase::slotStatJobResult);

        // Make sure we don't check the whole path upwards.
        mDirUrlChecked = true;
        return;
    } else if (job->error()) {
        // It doesn't seem possible to read nor write from the location so leave the
        // ok button disabled
        ui.statusLabel->setText(QString());
        Q_EMIT okEnabled(false);
        mDirUrlChecked = false;
        mStatJob = nullptr;
        return;
    }

    ui.statusLabel->setText(QString());
    Q_EMIT okEnabled(true);

    mDirUrlChecked = false;
    mStatJob = nullptr;
}

SingleFileValidatingWidget::SingleFileValidatingWidget(QWidget *parent)
    : QWidget(parent)
{
}
