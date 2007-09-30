/*
    This file is part of libkdepim.

    Copyright (c) 2003 Don Sanders <sanders@kde.org>
    Copyright (c) 2005 George Staikos <staikos@kde.org>
    Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>

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

#include "maillistdrag.h"
#include "kdepimprotocols.h"
#include <QBuffer>
#include <QDataStream>
#include <QEventLoop>
#include <QProgressBar>
#include <QTextStream>
#include <QDropEvent>
#include <KDateTime>
#include <klocale.h>
#include <kprogressdialog.h>
#include <kurl.h>

using namespace KPIM;


// Have to define before use
QDataStream& operator<< ( QDataStream &s, const MailSummary &d )
{
  s << d.serialNumber();
  s << d.messageId();
  s << d.subject();
  s << d.from();
  s << d.to();
  KDateTime tempTime;
  tempTime.setTime_t( d.date() );
  s << tempTime.dateTime();
  return s;
}

QDataStream& operator>> ( QDataStream &s, MailSummary &d )
{
  quint32 serialNumber;
  QString messageId, subject, from, to;
  time_t date;
  s >> serialNumber;
  s >> messageId;
  s >> subject;
  s >> from;
  s >> to;
  QDateTime tempTime;
  s >> tempTime;
  date = KDateTime( tempTime ).toTime_t();
  d.set( serialNumber, messageId, subject, from, to, date );
  return s;
}

QDataStream& operator<< ( QDataStream &s, const MailList &mailList )
{
  MailList::const_iterator it;
  for (it = mailList.begin(); it != mailList.end(); ++it) {
    MailSummary mailDrag = *it;
    s << mailDrag;
  }
  return s;
}

QDataStream& operator>> ( QDataStream &s, MailList &mailList )
{
  mailList.clear();
  MailSummary mailDrag;
  while (!s.atEnd()) {
    s >> mailDrag;
    mailList.append( mailDrag );
  }
  return s;
}

MailSummary::MailSummary( quint32 serialNumber, const QString &messageId,
			  const QString &subject, const QString &from, const QString &to,
			  time_t date )
    : mSerialNumber( serialNumber ), mMessageId( messageId ),
      mSubject( subject ), mFrom( from ), mTo( to ), mDate( date )
{}

quint32 MailSummary::serialNumber() const
{
    return mSerialNumber;
}

QString MailSummary::messageId()  const
{
    return mMessageId;
}

QString MailSummary::subject() const
{
    return mSubject;
}

QString MailSummary::from() const
{
    return mFrom;
}

QString MailSummary::to() const
{
    return mTo;
}

time_t MailSummary::date() const
{
    return mDate;
}

void MailSummary::set( quint32 serialNumber, const QString &messageId,
		       const QString &subject, const QString &from, const QString &to, time_t date )
{
    mSerialNumber = serialNumber;
    mMessageId = messageId;
    mSubject = subject;
    mFrom = from;
    mTo = to;
    mDate = date;
}

MailSummary::operator KUrl() const
{
  return KUrl( KDEPIMPROTOCOL_EMAIL + QString::number( serialNumber() ) + '/' + messageId() );
}


QString MailList::mimeDataType()
{
    return "x-kmail-drag/message-list";
}

bool MailList::canDecode( const QMimeData *md )
{
  return md->hasFormat( mimeDataType() );
}

void MailList::populateMimeData( QMimeData *md, MailTextSource *src )
{
  /* We have three different possible mime types: x-kmail-drag/message-list, message/rfc822, and URL
     Add them in this order */

  /* Popuplate the MimeData with the custom streaming x-kmail-drag/message-list mime type */
  if ( count() ) {
    QByteArray array;
    QBuffer buffer( &array, 0 );
    buffer.open( QIODevice::WriteOnly);
    QDataStream stream( &array, QIODevice::WriteOnly );
    stream << (*this);
    buffer.close();
    md->setData( MailList::mimeDataType(), array );
  }


  /* Popuplate the MimeData with the standard message/rfc822 mime type, only
     possible if we are given a MailTextSource */
  if ( src && count() ) {
    QByteArray rc;
    KProgressDialog *dlg = new KProgressDialog( 0, QString(), i18n("Retrieving and storing messages..."));
    dlg->setWindowModality(Qt::WindowModal);
    dlg->setAllowCancel(true);
    dlg->progressBar()->setMaximum( count());
    int i = 0;
    dlg->progressBar()->setValue(i);
    dlg->show();

    QTextStream *ts = new QTextStream(rc, QIODevice::WriteOnly);
    for ( MailList::ConstIterator it = begin(); it != end(); ++it ) {
      MailSummary mailDrag = *it;
      *ts << src->text(mailDrag.serialNumber());
      if (dlg->wasCancelled()) {
        break;
      }
      dlg->progressBar()->setValue(++i);
#ifdef __GNUC__
      #warning Port me!
#endif
      //kapp->eventLoop()->processEvents(QEventLoop::ExcludeSocketNotifiers);
    }
    delete dlg;
    delete ts;

    md->setData( "message/rfc822", rc );
  }


  /* Popuplate the MimeData with the URLs of the messages */
  KUrl::List urllist;
  QStringList labels;
  for ( MailList::ConstIterator it = begin(); it != end(); ++it ) {
    urllist.append( (*it) );
    labels.append( KUrl::toPercentEncoding( ( *it ).subject() ) );
  }
  QMap<QString,QString> metadata;
  metadata["labels"] = labels.join( ":" );
  urllist.populateMimeData( md, metadata );

}

MailList MailList::fromMimeData( const QMimeData *md )
{
  if ( canDecode(md) ) {
    return decode( md->data( mimeDataType() ) );
  } else {
    return MailList();
  }
}

MailList MailList::decode( const QByteArray& payload )
{
  MailList mailList;
  // A read-only data stream
  QDataStream stream( payload );
  if ( payload.size() ) {
    stream >> mailList;
  }
  return mailList;
}

QByteArray MailList::serialsFromMimeData( const QMimeData *md )
{
  MailList mailList = fromMimeData( md );
  if ( mailList.count() ) {
    MailList::iterator it;
    QByteArray a;
    QBuffer buffer( &a );
    buffer.open( QIODevice::WriteOnly );
    QDataStream stream( &buffer );
    for (it = mailList.begin(); it != mailList.end(); ++it) {
      MailSummary mailDrag = *it;
      stream << mailDrag.serialNumber();
    }
    buffer.close();
    return a;
  } else {
    return QByteArray();
  }
}

