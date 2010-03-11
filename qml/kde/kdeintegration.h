/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef KDEINTEGRATION_H
#define KDEINTEGRATION_H

#include <QObject>
#include <QStringList>
#include <QVariant>

class KDEIntegration : public QObject
{
  Q_OBJECT
  public:
    explicit KDEIntegration(QObject* parent = 0);

  public slots:
    QString i18n( const QString &message );
    QString i18na( const QString &message, const QVariantList &args );
    QString i18nc( const QString &context, const QString &message );
    QString i18nca( const QString &context, const QString &message, const QVariantList &args );
    QString i18np( const QString &singular, const QString &plural, int amount );
    QString i18npa( const QString &singular, const QString &plural, int amount, const QVariantList &args );
    QString i18ncp( const QString &context, const QString &singular, const QString &plural, int amount );
    QString i18ncpa( const QString &context, const QString &singular, const QString &plural, int amount, const QVariantList &args );
};

#endif

