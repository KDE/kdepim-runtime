/*
    Copyright (C) 2015-2017 Krzysztof Nowicki <krissn@op.pl>

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

#ifndef EWSMTACONFIGDIALOG_H
#define EWSMTACONFIGDIALOG_H

#include <QDialog>

class QDialogButtonBox;
namespace Akonadi {
}
namespace Ui {
class SetupServerView;
}
class EwsMtaResource;

class EwsMtaConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EwsMtaConfigDialog(EwsMtaResource *parentResource, WId windowId);
    ~EwsMtaConfigDialog() override;
private Q_SLOTS:
    void save();
private:
    QDialogButtonBox *mButtonBox = nullptr;
    EwsMtaResource *mParentResource = nullptr;
    Ui::SetupServerView *mUi = nullptr;
};

#endif
