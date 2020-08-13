/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSPROGRESSDIALOG_H
#define EWSPROGRESSDIALOG_H

#include <QDialog>

class EwsProgressDialog : public QDialog
{
    Q_OBJECT
public:
    enum Type {
        AutoDiscovery,
        TryConnect
    };

    explicit EwsProgressDialog(QWidget *parent, Type type);
    ~EwsProgressDialog() override;
};

#endif
