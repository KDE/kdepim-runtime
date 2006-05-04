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
#include <qbuffer.h>
#include <qdatastream.h>
#include <qeventloop.h>
//Added by qt3to4:
#include <QTextStream>
#include <QDropEvent>
#include <kapplication.h>
#include <klocale.h>
#include <kprogressbar.h>
#include <kprogressdialog.h>
#include <kurl.h>
#include <k3urldrag.h>

using namespace KPIM;

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

MailListDrag::MailListDrag( const MailList &mailList, QWidget * parent, MailTextSource *src )
    : Q3DragObject( parent ), _src(src), mUrlDrag( 0 )
{
  setMailList( mailList );
}

MailListDrag::~MailListDrag()
{
    delete _src;
    _src = 0;
    delete mUrlDrag;
}

const char* MailListDrag::format()
{
    return "x-kmail-drag/message-list";
}

bool MailListDrag::canDecode( QMimeSource *e )
{
    return e->provides( MailListDrag::format() );
}

// Have to define before use
QDataStream& operator<< ( QDataStream &s, const MailSummary &d )
{
    s << d.serialNumber();
    s << d.messageId();
    s << d.subject();
    s << d.from();
    s << d.to();
    #warning Port me!
    //s << d.date();
    return s;
}

QDataStream& operator>> ( QDataStream &s, MailSummary &d )
{
    quint32 serialNumber;
    QString messageId, subject, from, to;
    time_t date = 0;
    s >> serialNumber;
    s >> messageId;
    s >> subject;
    s >> from;
    s >> to;
    #warning Port me!
    //s >> date;
    d.set( serialNumber, messageId, subject, from, to, date );
    return s;
}

QDataStream& operator<< ( QDataStream &s, MailList &mailList )
{
    MailList::iterator it;
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

bool MailListDrag::decode( QDropEvent* e, MailList& mailList )
{
    QByteArray payload = e->encodedData( MailListDrag::format() );
    QDataStream buffer( &payload, QIODevice::ReadOnly );
    if ( payload.size() ) {
	e->accept();
	buffer >> mailList;
	return true;
    }
    return false;
}

bool MailListDrag::decode( QByteArray& payload, MailList& mailList )
{
    QDataStream stream( &payload, QIODevice::ReadOnly );
    if ( payload.size() ) {
	stream >> mailList;
	return true;
    }
    return false;
}

bool MailListDrag::decode( QDropEvent* e, QByteArray &a )
{
    MailList mailList;
    if (decode( e, mailList )) {
	MailList::iterator it;
	QBuffer buffer( &a );
	buffer.open( QIODevice::WriteOnly );
	QDataStream stream( &buffer );
	for (it = mailList.begin(); it != mailList.end(); ++it) {
	    MailSummary mailDrag = *it;
	    stream << mailDrag.serialNumber();
	}
	buffer.close();
	return true;
    }
    return false;
}

void MailListDrag::setMailList( const MailList &mailList )
{
  mMailList = mailList;
  if ( mUrlDrag )
    delete mUrlDrag;

  KUrl::List urllist;
  QStringList labels;
  for ( MailList::ConstIterator it = mailList.begin(); it != mailList.end();
        ++it ) {
    urllist.append( (*it) );
    labels.append( KUrl::encode_string( ( *it ).subject() ) );
  }
  QMap<QString,QString> metadata;
  metadata["labels"] = labels.join( ":" );
  mUrlDrag = new K3URLDrag( urllist, metadata );
}

const char *MailListDrag::format(int i) const
{
  if ( i == 0 ) {
    return format();
  } else {
    return ( _src && i == 1 ? "message/rfc822" :
             mUrlDrag->format( i - ( _src ? 2 : 1 ) ) );
  }
}

bool MailListDrag::provides(const char *mimeType) const
{
    if (_src && QByteArray(mimeType) == "message/rfc822") {
        return true;
    } else if ( QByteArray( mimeType ) == format() )
      return true;

    return mUrlDrag->provides( mimeType );
}

QByteArray MailListDrag::encodedData(const char *mimeType) const
{
    if ( QByteArray(mimeType) == format() ) {
      QByteArray array;
      QBuffer buffer( &array, 0 );
      buffer.open( QIODevice::WriteOnly);
      QDataStream stream( &array, QIODevice::WriteOnly );
      stream << mMailList;
      buffer.close();
      return array;
    } else if ( QByteArray( mimeType ) == "message/rfc822" ) {

      QByteArray rc;
      if (_src) {
          KProgressDialog *dlg = new KProgressDialog( 0, QString(), i18n("Retrieving and storing messages..."), true);
          dlg->setAllowCancel(true);
          dlg->progressBar()->setMaximum(mMailList.count());
          int i = 0;
          dlg->progressBar()->setValue(i);
          dlg->show();

          QTextStream *ts = new QTextStream(rc, QIODevice::WriteOnly);
          for (MailList::ConstIterator it = mMailList.begin();
               it != mMailList.end(); ++it) {
              MailSummary mailDrag = *it;
              *ts << _src->text(mailDrag.serialNumber());
              if (dlg->wasCancelled()) {
                  break;
              }
              dlg->progressBar()->setValue(++i);
              #warning Port me!
              //kapp->eventLoop()->processEvents(QEventLoop::ExcludeSocketNotifiers);
          }

          delete dlg;
          delete ts;
      }
      return rc;
    } else {
      return mUrlDrag->encodedData( mimeType );
    }

    /* danimo, wth is this!?!?!
    QByteArray rc;
    if (_src) {
        MailList ml;
        QByteArray enc = Q3StoredDrag::encodedData(format());
        decode(enc, ml);

        KProgressDialog *dlg = new KProgressDialog(0, 0, QString(), i18n("Retrieving and storing messages..."), true);
        dlg->setAllowCancel(true);
        dlg->progressBar()->setTotalSteps(ml.count());
        int i = 0;
        dlg->progressBar()->setValue(i);
        dlg->show();

        QTextStream *ts = new QTextStream(rc, QIODevice::WriteOnly);
        for (MailList::ConstIterator it = ml.begin(); it != ml.end(); ++it) {
            MailSummary mailDrag = *it;
            *ts << _src->text(mailDrag.serialNumber());
            if (dlg->wasCancelled()) {
                break;
            }
            dlg->progressBar()->setValue(++i);
            kapp->eventLoop()->processEvents(QEventLoop::ExcludeSocketNotifiers);
        }

        delete dlg;
        delete ts;
    }
    return rc;*/
}
