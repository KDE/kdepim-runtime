/*
    This file is part of libkdepim.

    Copyright (c) 2003 Don Sanders <sanders@kde.org>
    Copyright (c) 2005 George Staikos <staikos@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "maillistdrag.h"
#include <qbuffer.h>
#include <qdatastream.h>
#include <qeventloop.h>
#include <kapplication.h>
#include <klocale.h>
#include <kprogress.h>

using namespace KPIM;

MailSummary::MailSummary( Q_UINT32 serialNumber, QString messageId, 
			  QString subject, QString from, QString to, 
			  time_t date )
    : mSerialNumber( serialNumber ), mMessageId( messageId ),
      mSubject( subject ), mFrom( from ), mTo( to ), mDate( date )
{}

Q_UINT32 MailSummary::serialNumber() 
{ 
    return mSerialNumber; 
}

QString MailSummary::messageId() 
{ 
    return mMessageId; 
}

QString MailSummary::subject() 
{ 
    return mSubject; 
}

QString MailSummary::from() 
{ 
    return mFrom; 
}

QString MailSummary::to() 
{ 
    return mTo; 
}

time_t MailSummary::date()
{
    return mDate;
}

void MailSummary::set( Q_UINT32 serialNumber, QString messageId, 
		       QString subject, QString from, QString to, time_t date )
{
    mSerialNumber = serialNumber;
    mMessageId = messageId;
    mSubject = subject;
    mFrom = from;
    mTo = to;
    mDate = date;
}

MailListDrag::MailListDrag( MailList mailList, QWidget * parent, MailTextSource *src )
    : QStoredDrag( MailListDrag::format(), parent ), _src(src)
{
    setMailList( mailList );
}

MailListDrag::~MailListDrag()
{
    delete _src;
    _src = 0;
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
QDataStream& operator<< ( QDataStream &s, MailSummary &d )
{
    s << d.serialNumber();
    s << d.messageId();
    s << d.subject();
    s << d.from();
    s << d.to();
    s << d.date();
    return s;
}

QDataStream& operator>> ( QDataStream &s, MailSummary &d )
{
    Q_UINT32 serialNumber;
    QString messageId, subject, from, to;
    time_t date;
    s >> serialNumber;
    s >> messageId;
    s >> subject;
    s >> from;
    s >> to;
    s >> date;
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
    QDataStream buffer( payload, IO_ReadOnly );
    if ( payload.size() ) {
	e->accept();
	buffer >> mailList;
	return TRUE;
    }
    return FALSE;
}

bool MailListDrag::decode( QByteArray& payload, MailList& mailList )
{
    QDataStream stream( payload, IO_ReadOnly );
    if ( payload.size() ) {
	stream >> mailList;
	return TRUE;
    }
    return FALSE;
}

bool MailListDrag::decode( QDropEvent* e, QByteArray &a )
{
    MailList mailList;
    if (decode( e, mailList )) {
	MailList::iterator it;
	QBuffer buffer( a );
	buffer.open( IO_WriteOnly );
	QDataStream stream( &buffer );
	for (it = mailList.begin(); it != mailList.end(); ++it) {
	    MailSummary mailDrag = *it;
	    stream << mailDrag.serialNumber();
	}
	buffer.close();
	return TRUE;
    }
    return FALSE;
}

void MailListDrag::setMailList( MailList mailList )
{
    QByteArray array;
    QBuffer buffer( array );
    buffer.open( IO_WriteOnly);
    QDataStream stream( array, IO_WriteOnly );
    stream << mailList;
    buffer.close();
    setEncodedData( array );
}

const char *MailListDrag::format(int i) const
{
    if (_src) {
        if (i == 0) {
            return "message/rfc822";
        } else {
            return QStoredDrag::format(i - 1);
        }
    }

    return QStoredDrag::format(i);
}

bool MailListDrag::provides(const char *mimeType) const
{
    if (_src && QCString(mimeType) == "message/rfc822") {
        return true;
    }

    return QStoredDrag::provides(mimeType);
}

QByteArray MailListDrag::encodedData(const char *mimeType) const
{
    if (QCString(mimeType) != "message/rfc822") {
        return QStoredDrag::encodedData(mimeType);
    }

    QByteArray rc; 
    if (_src) {
        MailList ml;
        QByteArray enc = QStoredDrag::encodedData(format());
        decode(enc, ml);

        KProgressDialog *dlg = new KProgressDialog(0, 0, QString::null, i18n("Retrieving and storing messages..."), true);
        dlg->setAllowCancel(true);
        dlg->progressBar()->setTotalSteps(ml.count());
        int i = 0;
        dlg->progressBar()->setValue(i);
        dlg->show();

        QTextStream *ts = new QTextStream(rc, IO_WriteOnly);
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
    return rc;
}

