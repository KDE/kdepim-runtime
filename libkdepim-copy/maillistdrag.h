/*
    This file is part of libkdepim.

    Copyright (c) 2003 Don Sanders <sanders@kde.org>
    Copyright (c) 2005 George Staikos <staikos@kde.org.
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
#ifndef maillistdrag_h
#define maillistdrag_h

#include "q3dragobject.h"
#include "qglobal.h"
#include <QDropEvent>
#include "time.h"

#include <kdepimmacros.h>

class KUrl;

/**
 * KDEPIM classes for drag and drop of mails
 * 
 * // Code example for drag and drop enabled widget
 *
 * void SomeWidget::contentsDropEvent(QDropEvent *e)
 * {
 *    if (e->provides(MailListDrag::format())) {
 *	MailList mailList;
 *	MailListDrag::decode( e, mailList );
 *      ...
 **/

namespace KPIM {

class KDE_EXPORT MailSummary 
{
public:
    MailSummary( quint32 serialNumber, const QString &messageId, const QString &subject, 
		 const QString &from, const QString &to, time_t date );
    MailSummary() {}
    ~MailSummary() {}

    /*** Set fields for this mail summary  ***/
    void set( quint32, const QString&, const QString&, const QString&, const QString&, time_t );

    /*** KMail unique identification number ***/
    quint32 serialNumber() const;

    /*** MD5 checksum of message identification string ***/
    QString messageId() const;

    /*** Subject of the message including prefixes ***/
    QString subject() const;

    /*** Simplified from address ***/
    QString from() const;

    /** Simplified to address ***/
    QString to() const;

    /*** Date the message was sent ***/
    time_t date() const;
    
    /** returns kmail:<serial number>/<message id> style uri */
    operator KUrl() const;

private:
    quint32 mSerialNumber;
    QString mMessageId, mSubject, mFrom, mTo;
    time_t mDate;
};

// List of mail summaries
typedef QList<MailSummary> MailList;

// Object for the drag object to call-back for message fulltext
class KDE_EXPORT MailTextSource {
public:
    MailTextSource() {}
    virtual ~MailTextSource() {}

    virtual QByteArray text(quint32 serialNumber) const = 0;
};

// Drag and drop object for mails

class KDE_EXPORT MailListDrag : public Q3DragObject
{
public:
    // Takes ownership of "src" and deletes it when done
    MailListDrag( const MailList &, QWidget * parent = 0, MailTextSource *src = 0 );
    ~MailListDrag();

    const char *format(int i) const;

    bool provides(const char *mimeType) const;

    QByteArray encodedData(const char *) const;

    /* Reset the list of mail summaries */
    void setMailList( const MailList & );

    /* The format for this drag - "x-kmail-drag/message-list" */
    static const char* format();
    
    /* Returns TRUE if the information in e can be decoded into a QString;
       otherwsie returns FALSE */
    static bool canDecode( QMimeSource* e );

    /* Attempts to decode the dropped information;
       Returns TRUE if successful; otherwise return false */
    static bool decode( QDropEvent* e, MailList& s );

    /* Attempts to decode the serialNumbers of the dropped information;
       Returns TRUE if successful; otherwise return false */
    static bool decode( QDropEvent* e, QByteArray& a );

    /* Attempts to decode the encoded MailList;
       Returns TRUE if successful; otherwise return false */
    static bool decode( QByteArray& a, MailList& s );

private:
    MailTextSource *_src;
    MailList mMailList;
    Q3DragObject *mUrlDrag;
};

}
#endif /*maillistdrag_h*/
