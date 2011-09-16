/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "itemmodifyjob.h"

#include "session_p.h"

using namespace Akonadi;

class FileStore::ItemModifyJob::Private
{
  public:
    explicit Private( FileStore::ItemModifyJob *parent )
      : mIgnorePayload( false ), mParent( parent )
    {
    }

  public:
    bool mIgnorePayload;
    Item mItem;
    QSet<QByteArray> mParts;

  private:
    FileStore::ItemModifyJob *mParent;
};

FileStore::ItemModifyJob::ItemModifyJob( const Item &item, FileStore::AbstractJobSession *session )
  : FileStore::Job( session ), d( new Private( this ) )
{
  d->mItem = item;

  session->addJob( this );
}

FileStore::ItemModifyJob::~ItemModifyJob()
{
  delete d;
}

void FileStore::ItemModifyJob::setIgnorePayload( bool ignorePayload )
{
  d->mIgnorePayload = ignorePayload;
}

bool FileStore::ItemModifyJob::ignorePayload() const
{
  return d->mIgnorePayload;
}

Item FileStore::ItemModifyJob::item() const
{
  return d->mItem;
}

void FileStore::ItemModifyJob::setParts( const QSet<QByteArray>& parts )
{
    d->mParts = parts;
}

const QSet<QByteArray>& FileStore::ItemModifyJob::parts() const
{
  return d->mParts;
}

bool FileStore::ItemModifyJob::accept( FileStore::Job::Visitor *visitor )
{
  return visitor->visit( this );
}

void FileStore::ItemModifyJob::handleItemModified( const Item &item )
{
  d->mItem = item;
}

#include "itemmodifyjob.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
