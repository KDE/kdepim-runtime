/*
    Copyright (C) 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>


class QDialogButtonBox;
class EwsResource;
class EwsClient;
class KConfigDialogManager;
namespace Ui
{
class SetupServerView;
}
class KJob;
class EwsAutodiscoveryJob;
class EwsGetFolderRequest;
class ProgressDialog;
class EwsSubscriptionWidget;

class ConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConfigDialog(EwsResource *parentResource, EwsClient &client, WId windowId);
    ~ConfigDialog() override;
private:
    void save();
    void autoDiscoveryFinished(KJob *job);
    void tryConnectFinished(KJob *job);
    void performAutoDiscovery();
    void autoDiscoveryCancelled();
    void tryConnectCancelled();
    void setAutoDiscoveryNeeded();
    void dialogAccepted();
    void enableTryConnect();
    void tryConnect();
    void userAgentChanged(int index);

    QString fullUsername() const;

    EwsResource *mParentResource = nullptr;
    KConfigDialogManager *mConfigManager = nullptr;
    Ui::SetupServerView *mUi = nullptr;

    QDialogButtonBox *mButtonBox = nullptr;
    EwsAutodiscoveryJob *mAutoDiscoveryJob = nullptr;
    EwsGetFolderRequest *mTryConnectJob = nullptr;
    bool mAutoDiscoveryNeeded = false;
    bool mTryConnectNeeded = false;
    ProgressDialog *mProgressDialog = nullptr;
    EwsSubscriptionWidget *mSubWidget = nullptr;
};

#endif
