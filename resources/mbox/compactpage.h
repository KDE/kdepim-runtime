/*
    SPDX-FileCopyrightText: 2009 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef COMPACTPAGE_H
#define COMPACTPAGE_H

#include <QWidget>

#include "ui_compactpage.h"

class KJob;

class CompactPage : public QWidget
{
    Q_OBJECT

public:
    explicit CompactPage(const QString &collectionId, QWidget *parent = nullptr);

private Q_SLOTS:
    void compact();
    void onCollectionFetchCheck(KJob *);
    void onCollectionFetchCompact(KJob *);
    void onCollectionModify(KJob *);

private: // Methods
    void checkCollectionId();

private: // Members
    QString mCollectionId;
    Ui::CompactPage ui;
};

#endif
