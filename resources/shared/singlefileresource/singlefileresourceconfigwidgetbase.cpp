/*
    SPDX-FileCopyrightText: 2008 Bertjan Broeksema <b.broeksema@kdemail.org>
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2010, 2011 David Jarvie <djarvie@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "singlefileresourceconfigwidgetbase.h"

#include <KIO/Job>
#include <KIO/StatJob>
#include <QTimer>

#include <KConfigDialogManager>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QFontDatabase>
#include <QPushButton>
#include <QStringList>
#include <QTabBar>
#include <QVBoxLayout>

using namespace Akonadi;

SingleFileResourceConfigWidgetBase::SingleFileResourceConfigWidgetBase(QWidget *parent)
    : QWidget(parent)
{
    auto mainWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    mainLayout->setContentsMargins({});
    ui.setupUi(mainWidget);
    ui.kcfg_Path->setMode(KFile::File);
    ui.kcfg_Path->setMimeTypeFilters(QStringList(QLatin1String("text/calendar")));
    ui.kcfg_Path->setToolTip(i18nc("@info:tooltip", "A file path or URL containing the calendar file. Once created, this location cannot be modified."));
    ui.kcfg_Path->setWhatsThis(xi18nc("@info:whatsthis",
                                      "Enter the path or URL to a file containing a valid calendar file. "
                                      "<para><note> Unfortunately, this path cannot be changed once the resource is created. "
                                      "To change the location, delete this resource and then create a new one with the updated path.</note></para>"));

    ui.statusLabel->setVisible(false);

    ui.tabWidget->tabBar()->hide();

    connect(ui.kcfg_Path, &KUrlRequester::textChanged, this, &SingleFileResourceConfigWidgetBase::validate);
    connect(ui.kcfg_MonitorFile, &QCheckBox::toggled, this, &SingleFileResourceConfigWidgetBase::validate);
    ui.kcfg_Path->setFocus();
    QTimer::singleShot(0, this, &SingleFileResourceConfigWidgetBase::validate);

    const auto smallFont = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
    ui.readOnlyLabel->setFont(smallFont);
    ui.monitoringLabel->setFont(smallFont);
    ui.pathLabel->setFont(smallFont);
    ui.periodicUpdateLabel->setFont(smallFont);
}

SingleFileResourceConfigWidgetBase::~SingleFileResourceConfigWidgetBase() = default;

void SingleFileResourceConfigWidgetBase::addPage(const QString &title, QWidget *page)
{
    ui.tabWidget->tabBar()->show();
    ui.tabWidget->addTab(page, title);
    mManager->addWidget(page);
    mManager->updateWidgets();
}

void SingleFileResourceConfigWidgetBase::setFilter(const QString &filter)
{
    ui.kcfg_Path->setNameFilter(filter);
}

void SingleFileResourceConfigWidgetBase::setMonitorEnabled(bool enable)
{
    mMonitorEnabled = enable;
    ui.kcfg_MonitorFile->setEnabled(mMonitorEnabled);
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
        ui.kcfg_MonitorFile->setChecked(false);
        ui.kcfg_MonitorFile->setEnabled(true);
        ui.kcfg_ReadOnly->setChecked(false);
        ui.kcfg_ReadOnly->setEnabled(true);
        Q_EMIT okEnabled(false);
        return;
    }

    if (currentUrl.isLocalFile()) {
        if (mMonitorEnabled) {
            ui.kcfg_MonitorFile->setEnabled(true);
        }
        ui.kcfg_ReadOnly->setChecked(false);
        ui.kcfg_ReadOnly->setEnabled(true);
        ui.statusLabel->setVisible(false);

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

        ui.kcfg_MonitorFile->setChecked(false);
        ui.kcfg_MonitorFile->setEnabled(false);
        ui.kcfg_ReadOnly->setChecked(true);
        ui.kcfg_ReadOnly->setEnabled(false);
        ui.statusLabel->setText(i18nc("@info:status", "Checking file information…"));
        ui.statusLabel->setVisible(true);

        if (mStatJob) {
            mStatJob->kill();
        }
        mStatJob = KIO::stat(currentUrl, KIO::StatJob::SourceSide, KIO::StatDetail::StatDefaultDetails, KIO::DefaultFlags | KIO::HideProgressInfo);

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

        mStatJob = KIO::stat(dirUrl, KIO::StatJob::SourceSide, KIO::StatDetail::StatDefaultDetails, KIO::DefaultFlags | KIO::HideProgressInfo);

        connect(mStatJob, &KIO::StatJob::result, this, &SingleFileResourceConfigWidgetBase::slotStatJobResult);

        // Make sure we don't check the whole path upwards.
        mDirUrlChecked = true;
        return;
    } else if (job->error()) {
        // It doesn't seem possible to read nor write from the location so leave the
        // ok button disabled
        ui.statusLabel->setVisible(false);
        Q_EMIT okEnabled(false);
        mDirUrlChecked = false;
        mStatJob = nullptr;
        return;
    }

    ui.statusLabel->setVisible(false);
    Q_EMIT okEnabled(true);

    mDirUrlChecked = false;
    mStatJob = nullptr;
}

SingleFileValidatingWidget::SingleFileValidatingWidget(QWidget *parent)
    : QWidget(parent)
{
}

#include "moc_singlefileresourceconfigwidgetbase.cpp"
