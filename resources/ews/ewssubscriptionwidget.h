/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSSUBSCRIPTIONWIDGET_H
#define EWSSUBSCRIPTIONWIDGET_H

#include <QScopedPointer>
#include <QWidget>

#include "ewsid.h"

class EwsSubscriptionWidgetPrivate;
class EwsClient;
class EwsSettings;

class EwsSubscriptionWidget : public QWidget
{
    Q_OBJECT
public:
    EwsSubscriptionWidget(EwsClient &client, EwsSettings *settings, QWidget *parent);
    ~EwsSubscriptionWidget();

    QStringList subscribedList() const;
    bool subscribedListValid() const;
    bool subscriptionEnabled() const;
private:
    QScopedPointer<EwsSubscriptionWidgetPrivate> d_ptr;
    Q_DECLARE_PRIVATE(EwsSubscriptionWidget)
};

#endif
