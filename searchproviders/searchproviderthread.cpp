/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "searchproviderthread.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

class Akonadi::SearchProviderThreadPrivate
{
  public:
    int type;
    QList<int> fetchResponseIds;
    QString fetchResponseField;
    QStringList fetchResponse;
};

Akonadi::SearchProviderThread::SearchProviderThread(QObject * parent) :
  QThread( parent ), d( new SearchProviderThreadPrivate )
{
  connect( this, SIGNAL(finished()), SLOT(slotFinished()) );
}

Akonadi::SearchProviderThread::~ SearchProviderThread()
{
  delete d;
}

void Akonadi::SearchProviderThread::run()
{
  switch ( d->type ) {
    case FetchResponse:
      d->fetchResponse = generateFetchResponse( d->fetchResponseIds, d->fetchResponseField );
      break;
  };
}

void Akonadi::SearchProviderThread::slotFinished()
{
  emit finished( this );
}

void Akonadi::SearchProviderThread::requestFetchResponse(const QList< int > uids, const QString & field)
{
  d->type = FetchResponse;
  d->fetchResponseIds = uids;
  d->fetchResponseField = field;
}

QStringList Akonadi::SearchProviderThread::fetchResponse() const
{
  return d->fetchResponse;
}

#include "searchproviderthread.moc"

