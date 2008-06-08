/* This file is part of the KDE project
   Copyright (C) 2006-2007 Omat Holding B.V. <info@omat.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// Own
#include "imaplib.h"

// Qt
#include <QStringList>
#include <QRegExp>
#include <QStringList>
#include <QTimer>
#include <QTimerEvent>

// KDE
#include <KDebug>
#include <KLocale>

static inline bool endSimpleCommand( const QString& text )
{
    const static QRegExp rx( "(?:^|\n|\r)a02 (?:OK|NO|BAD)[^\n]+(?:\n)\r?$" );
    return text.indexOf( rx ) > -1;
}

Imaplib::Imaplib( QObject* parent,  const char* name )
        : QObject( parent ), m_currentState( NotConnected ), m_socket( 0 ),
        m_tls( false ), m_readyToSend( false )
{
    setObjectName( name );
    // kDebug();

    // This will process the queue with requests that should go to
    // the server
    startTimer( 20 );

    // This will prevent the autologout after 30 minutes, after each
    // read it will start the timer to send a noop after 25 minutes.
    m_preventLogOutTimer = new QTimer( this );
    connect( m_preventLogOutTimer, SIGNAL( timeout() ), SLOT( slotSendNoop() ) );

    // When a mailbox is selected, we want to sent a NOOP every minute
    // for that mailbox. That will get us an update of the mailbox.
    m_checkMailTimer = new QTimer( this );
    connect( m_checkMailTimer, SIGNAL( timeout() ), SLOT( slotSendNoop() ) );
    /*
        // testcases for the regex.

    kDebug() << "These should be true:";
    kDebug() << endSimpleCommand("bla\na02 OK bla\n\r");   // true
    kDebug() << endSimpleCommand("a02 OK bla\n");          // true
    kDebug() << endSimpleCommand("bla\na02 OK bla\n\r");   // true
    kDebug() << endSimpleCommand("bla\n\ra02 OK bla\n");   // true

    kDebug() << "These should be false:";
    kDebug() << endSimpleCommand("bla a02 OK bla");        // false
    kDebug() << endSimpleCommand("a02 OK bla");            // false
    kDebug() << endSimpleCommand("\n\ra02 OK bla");        // false
    kDebug() << endSimpleCommand("\na02 OK bla");          // false
    kDebug() << endSimpleCommand("bla\na02 OK bla");       // false
    kDebug() << endSimpleCommand("bla\na02 OK bla\n\rbla\n\r"); // false
    kDebug() << endSimpleCommand("a02 OK bla\n\rbla\n");   // false
    kDebug() << endSimpleCommand("bla\na02 OK bla\r");   // false

    */
}

Imaplib::~Imaplib()
{
    // kDebug();
    // queued jobs can be removed...
    m_queue.clear();
    m_currentQueueItem=Queue();
}

void Imaplib::slotRead( const QString& received )
{
    // kDebug() << received << " for " << m_currentQueueItem.state();

    // ignore empty, but not when we do tls, just after the handshake
    // it will return an empty string.
    if ( received.isEmpty() && !m_tls )
        return;

    // get the untagged alert...
    if ( received.indexOf( "* OK [ALERT]" ) > -1 ) {
        emit alert( this, received.mid( 13 ) );
        m_currentQueueItem=Queue();
        return;
    }

    if ( m_currentQueueItem.state() == Queue::Logout &&
            received.indexOf( "* BYE" ) > -1 ) {
        m_currentState = LoggedOut;
        m_queue.clear();
        m_currentQueueItem=Queue();
        emit status( i18n( "Offline" ) );
        return;
    }

    // Handle the response per connection state.
    if ( m_currentState == Connected ) {
        // The reply is a response to a connect. Lets see if the input
        // starts with * OK
        if ( received.indexOf( "* OK" ) != -1 ) {
            // Lets see if the server return the capabilities directly, so
            // we don't have to poll for it.
            if ( received.indexOf( "* OK [CAPABILITY" ) != -1 ) {
                m_capabilities =
                    received.mid( 13 ).toLower().split( ' ' );
            }

            if ( m_tls ) {
                m_capabilities.empty(); // after tls, we refetch.
                m_socket->write( "a02 starttls" );
                return;
            }
        }

        // In this state, we can receive a response...
        if ( received.indexOf( "* CAPABILITY" ) != -1 )
            m_capabilities = received.mid( 13 ).toLower().split( ' ' );

        // Now, we should have the capabilities, carry on...
        if ( m_capabilities.count() > 1 ) {
            m_currentState = NotAuthenticated;
            kDebug( ) << "Log in, we can do:" << m_capabilities << m_capabilities.count();
            emit login( this );
            return;
        } else {
            m_queue.append( Queue( Queue::Capability, QString(),
                                   "CAPABILITY" ) );
            m_currentQueueItem = Queue();
            return;
        }
        return;
    } else if ( m_currentState == NotAuthenticated ) {
        // if we receive a response in this state, this will mean an
        // authentication attemped is made.
        if ( m_currentQueueItem.state() == Queue::Auth ) {
            if ( received.indexOf( "a02 NO" ) != -1 ||
                    received.indexOf( "a02 BAD" ) != -1 ) {
                emit loginFailed( this );
                m_currentQueueItem=Queue();
            } else {
                m_currentState = Authenticated;
                m_preventLogOutTimer->setSingleShot( true );
                m_preventLogOutTimer->start( 25*60*1000 );
                emit loginOk( this );
                m_currentQueueItem = Queue();
            }
        }
    } else if ( m_currentState == Authenticated ) {
        m_received = received;
        switch ( m_currentQueueItem.state() ) {
        case Queue::GetMailBoxList:
            slotParseGetMailBoxList();
            break;
        case Queue::GetRecent:
            slotParseGetRecent();
            break;
        case Queue::Noop:
            slotParseNoop();
            break;
        case Queue::Copy:
            slotParseCopy();
            break;
        case Queue::CheckMail:
            slotParseCheckMail();
            break;
        case Queue::Expunge:
            slotParseExpunge();
            break;
        case Queue::SyncMailBox:
            slotParseExists();
            break;
        case Queue::SelectMailBox:
            slotSelectMailBox();
            break;
        case Queue::SaveMessage:
            slotParseSaveMessage();
            break;
        case Queue::SaveMessageData:
            slotParseSaveMessageData();
            break;
        case Queue::GetHeaderList:
            slotParseGetHeaderList();
            break;
        case Queue::GetHeaders:
            slotParseGetMessage();
            break;
        case Queue::GetMessage:
            slotParseGetMessage();
            break;
        case Queue::CreateMailBox:
            slotParseCreateMailBox();
            break;
        case Queue::DeleteMailBox:
            slotParseDeleteMailBox();
            break;
        case Queue::RenameMailBox:
            slotParseRenameMailBox();
            break;
        default:
            m_currentQueueItem = Queue();
            slotProcessQueue();
        }
    }
}

void Imaplib::startConnection( const QString& server, int port,
                               int sec )
{
    m_currentMailbox = QString();
    m_currentQueueItem = Queue();
    m_queue.clear();

    if ( m_socket ) {
        kDebug( ) << "Delete connection";
        m_socket->aboutToClose();
        m_socket->deleteLater();
        m_socket = 0;
    }

    m_socket  = new Socket( this );
    m_socket->setObjectName( "Imap" );
    m_socket->setServer( server );

    switch ( sec ) {
    case 1:
        if ( port == 0 )
            port = 143;
        m_socket->setPort( port );
        break;
    case 2:
        if ( port == 0 )
            port = 993;
        m_socket->setPort( port );
        m_socket->setSecure( true );
        break;
    case 3:
        if ( port == 0 )
            port = 143;
        m_socket->setPort( port );
        m_socket->setTls( true );
        m_tls = true;
        break;
    }

    connect( m_socket, SIGNAL( data( const QString& ) ),
             SLOT( slotRead( const QString& ) ) );
    connect( m_socket, SIGNAL( connected() ),
             SLOT( slotConnected() ) );
    connect( m_socket, SIGNAL( disconnected() ),
             SIGNAL( disconnected() ) );
    connect( m_socket, SIGNAL( unexpectedDisconnect() ),
             SIGNAL( unexpectedDisconnect() ) );

    emit status( i18n( "Connecting" ) );
    m_currentQueueItem= Queue( Queue::Connecting,"","" );
    m_socket->reconnect();
}

void Imaplib::login( const QString& username, const QString& password )
{
    // kDebug();
    m_queue.prepend( Queue( Queue::Auth, "",
                            "login \"" + KIMAP::quoteIMAP( username )
                            + "\" \"" + KIMAP::quoteIMAP( password ) + "\"" ) );
    m_currentQueueItem = Queue();
}

void Imaplib::logout()
{
    // kDebug();
    m_socket->aboutToClose();
    m_queue.prepend( Queue( Queue::Logout, "", "logout" ) );
}

void Imaplib::getMailBoxList()
{
    // kDebug();
    m_queue.append( Queue( Queue::GetMailBoxList,"", "LIST \"\" \"*\"",
                           i18n( "Retrieving mailbox list" ) ) );
}

void Imaplib::getMailBox( const QString& box )
{
    // kDebug() << box;
    const QString box2 = KIMAP::encodeImapFolderName( box );
    m_queue.prepend( Queue( Queue::SyncMailBox, box, "SELECT \""+box2+"\"" ) );
}

void Imaplib::getHeaders( const QString& mb, const QStringList& uids )
{
    // kDebug() << << uids << mb;
    if ( uids.count() == 0 || mb.isEmpty() )
        return;

    // Join first, then split it in blocks of 250.
    QString complete = uids.join( "," );

    int begin = 1;
    while ( !complete.isEmpty() ) {
        QString part = complete.section( ',',0,250 );
        complete = complete.section( ',',251 );

        int end = begin + 250;
        if ( end > uids.count() )
            end = uids.count();

        // Priority; above the checkmail calls in the queue.
        m_queue.prepend( Queue( Queue::GetHeaders, mb,
                                "UID FETCH " + part
                                + " (RFC822.SIZE BODY.PEEK[HEADER.FIELDS (FROM TO CC SUBJECT "
                                "DATE IN-REPLY-TO MESSAGE-ID)])",
                                i18n( "Retrieving headers [%1-%2] for %3", begin, end, mb ) ) );

        begin += 250;
    }
}

void Imaplib::getHeaderList( const QString& mb, int start, int end )
{
    // kDebug() << start << "-" << end << "for" << mb;
    if ( !( end>=1 ) || end < start || !( start>=1 ) )
        return;

    // Put this in queue with priority above the checkmail things...
    m_queue.prepend( Queue( Queue::GetHeaderList, mb,
                            "FETCH " + QString::number( start ) + ':'
                            + QString::number( end ) + " (UID FLAGS)",
                            i18n( "Retrieving status list for %1", mb ) ) );
}

void Imaplib::getMessage( const QString& mb, int uid )
{
    kDebug( );
    m_queue.prepend( Queue( Queue::GetMessage, mb,
                            "UID FETCH " + QString::number( uid )
                            + ':' + QString::number( uid ) + " BODY[]",
                            i18n( "Retrieving message" ) ) );
}

void Imaplib::checkMail( const QString& box )
{
    // kDebug() << box;
    const QString box2 = KIMAP::encodeImapFolderName( box );
    m_queue.append( Queue( Queue::CheckMail, box,
                           "STATUS \""+box2+"\" (MESSAGES UNSEEN UIDVALIDITY UIDNEXT)",
                           i18n( "Checking mail for %1", box ) ) );
}

void Imaplib::addFlag( const QString& box, int min, int max,
                       const QByteArray& flag )
{
    // kDebug() << box << " - " << min << " - " << max << " - " << flag;
    m_queue.append( Queue( Queue::NoResponse, box,
                           "UID STORE "
                           + QString::number( min ) + ':' + QString::number( max )
                           + " +FLAGS (" + flag + ')' ) );
}

void Imaplib::setFlags( const QString& box, int min, int max,
                        const QByteArray& flag )
{
    // kDebug() << box << " - " << min << " - " << max << " - " << flag;
    m_queue.append( Queue( Queue::NoResponse, box,
                           "UID STORE "
                           + QString::number( min ) + ':' + QString::number( max )
                           + " FLAGS (" + flag + ')' ) );
}

void Imaplib::removeFlag( const QString& box, int min, int max,
                          const QByteArray& flag )
{
    // kDebug() << box << " - " << uid << " - " << flag;
    m_queue.append( Queue( Queue::NoResponse, box,
                           "UID STORE "
                           + QString::number( min ) + ':' + QString::number( max )
                           + " -FLAGS (" + flag + ')' ) );
}

void Imaplib::expungeMailBox( const QString& box )
{
    // kDebug() << box;
    const QString box2 = KIMAP::encodeImapFolderName( box );
    m_queue.append( Queue( Queue::Expunge, box, "EXPUNGE" ) );
}

void Imaplib::createMailBox( const QString& box )
{
    // kDebug() << box;
    const QString box2 = KIMAP::encodeImapFolderName( box );
    m_queue.append( Queue( Queue::CreateMailBox, box, "CREATE \"" + box2 + "\"" ) );
}

void Imaplib::deleteMailBox( const QString& box )
{
    // kDebug() << box;
    const QString box2 = KIMAP::encodeImapFolderName( box );

    // We can not delete the mailbox if it is open..
    if ( m_currentMailbox == box )
        m_queue.append( Queue( Queue::SelectMailBox, "INBOX", "SELECT \"INBOX\"" ) );
    m_queue.append( Queue( Queue::DeleteMailBox, box, "DELETE \"" + box2 + "\"" ) );
}

void Imaplib::renameMailBox( const QString& oldbox, const QString& newbox )
{
    // kDebug() << oldbox << "->" << newbox;
    const QString oldbox2 = KIMAP::encodeImapFolderName( oldbox );
    const QString newbox2 = KIMAP::encodeImapFolderName( newbox );
    m_queue.append( Queue( Queue::RenameMailBox, oldbox + '~' + newbox,
                           "RENAME \"" + oldbox2 + "\" \"" + newbox2 + "\"",
                           i18n( "Renaming mailbox from %1 to %2", oldbox, newbox ) ) );
}

void Imaplib::copyMessage( const QString& origbox, int uid,
                           const QString& destbox )
{
    // kDebug() << origbox << " - " << uid << " -> " << destbox;
    const QString destbox2 = KIMAP::encodeImapFolderName( destbox );
    m_queue.append( Queue( Queue::Copy, origbox,
                           "UID COPY " + QString::number( uid ) + " \"" + destbox2+ "\"",
                           i18n( "Moving message to %1", destbox ) ) );
}

void Imaplib::saveMessage( const QString& mb, const QString& message,
                           const QString& flags )
{
    // kDebug() << message;
    const QString mb2 = KIMAP::encodeImapFolderName( mb );
    QString flagsformatted = "\\seen";
    if ( !flags.isEmpty() )
        flagsformatted += ' ' + flags;
    m_queue.append( Queue( Queue::SaveMessage, mb,
                           "APPEND \"" + mb2 + "\" (" + flagsformatted + ") {"
                           + QString::number( message.length() )
                           + '}' ) );
    m_queue.append( Queue( Queue::SaveMessageData, mb, message ) );
}

void Imaplib::idleStart( const QString& mb )
{
    // kDebug();
    m_queue.append( Queue( Queue::IdleStart, mb, "IDLE" ) );
}

void Imaplib::idleStop()
{
    // kDebug();
    m_queue.prepend( Queue( Queue::IdleStop, QString(), "DONE" ) );
}

bool Imaplib::capable( const QString& something )
{
    // kDebug() << something;
    return m_capabilities.indexOf( something ) > -1;
}



// ------------------- Private functions -----------------------//


void Imaplib::write( const QString& text )
{
    // kDebug() << text;
    if ( m_socket )
        m_socket->write( "a02 "+text );
}

void Imaplib::selectMailBox( const QString& box )
{
    // kDebug() << box;
    const QString box2 = KIMAP::encodeImapFolderName( box );
    m_queue.prepend( Queue( Queue::SelectMailBox, box, "SELECT \""+box2+"\"" ) );
}

// ------------------- Private slots --------------------------//


void Imaplib::slotConnected()
{
    // connection estabilished.
    kDebug( );
    m_currentState = Connected;
    m_currentQueueItem = Queue();
}

void Imaplib::slotSendNoop()
{
//    kDebug();
    m_queue.append( Queue( Queue::Noop, "","NOOP" ) );
}

// -------------------- Parsers -------------------------------------//

void Imaplib::slotParseGetMailBoxList()
{
    // kDebug();

    static QString all_data;
    all_data.append( m_received );

    if ( !endSimpleCommand( all_data ) )
        return;

    QStringList result;

    // Microsoft Server returns quotes sometimes:
    // * LIST (\Marked \HasNoChildren) "/" bbullshit
    // * LIST (\Marked \HasNoChildren) "/" "Trash/Deleted Items"

    QRegExp rx1( "LIST \\(.*\\) \".\" (.*)\n" );
    rx1.setMinimal( true );

    QMap<QString, bool> newList;
    int start = 0;
    while ( start != -1 ) {
        start = rx1.indexIn( all_data, start );

        if ( start == -1 )
            break;

        QString y = rx1.cap( 1 ).trimmed();

        // Remove them if they are there.... Think MS.
        if ( y.startsWith( '\"' ) && y.endsWith( '\"' ) )
            y=y.mid( 1, y.length()-2 );

        result.append( KIMAP::decodeImapFolderName( y ) );

        ++start;
    }

    all_data.clear();
    emit currentFolders( result );
    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseGetRecent()
{
    // kDebug();

    QRegExp rx( "SEARCH (.*)a02" );
    if ( rx.indexIn( m_received.trimmed() ) != -1 ) {
        QString r = rx.cap( 1 ).trimmed();
        QStringList t =
            r.split( ' ' );

        emit unseenCount( this, m_currentQueueItem.mailbox(), t.count() );

        QStringList results;
        QStringList::iterator it = t.begin();
        while ( it != t.end() ) {
            results.append( *it );
            results.append( QString() ); // no flag, that means unseen...
            ++it;
        }

        emit uidsAndFlagsInFolder( this, m_currentQueueItem.mailbox(), results );
    } else
        emit unseenCount( this, m_currentQueueItem.mailbox(), 0 );

    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseCopy()
{
    // kDebug();

    QRegExp rx( "a02 OK" );
    if ( rx.indexIn( m_received.trimmed() ) == -1 ) {
        kDebug( ) << "Store failed, removing the delete command";
        // TODO: emit something for the move people...
        // m_queue.pop_front();
    }
    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotSelectMailBox()
{
    // this is where the box is selected, don't emit anything.
    kDebug();
    m_currentMailbox=m_currentQueueItem.mailbox();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseNoop()
{
    m_currentQueueItem = Queue();
    slotProcessQueue();
    return;

    // Make sure we have all data....
    static QString all_data;
    all_data.append( m_received );

    if ( !endSimpleCommand( all_data ) )
        return;

    // process things....
    // kDebug() << "NOOP: " << all_data << "ENDNOOP";

    // we will receive something like, we need to fetch it again, because we
    // need the uid ;-(
    //* 8 FETCH (FLAGS (\Recent \Seen $Label2))
    QRegExp rx( "\\* (\\d+) FETCH \\(" );
    int start = 0;
    while ( start != -1 ) {
        start = rx.indexIn( m_received.trimmed(), start );

        if ( start == -1 )
            break;

        int i = rx.cap( 1 ).toInt();
        getHeaderList( m_currentMailbox, i, i );
        kDebug( ) << "Flag changed, regetting it.";

        ++start;
    }

    // done!
    all_data = QString::null;
    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseCheckMail()
{
    kDebug( );

    // kDebug() << "checkmailresult: " << m_currentQueueItem.mailbox() << ":" << m_received;
    QRegExp rx1( "STATUS (.+) \\(" );
    rx1.setMinimal( true );

    QString mb, uidvalidity, uidnext;
    int totalShouldBe=0;

    if ( rx1.indexIn( m_received.trimmed() ) != -1 ) {
        mb = rx1.cap( 1 );

        //regex in qt is weird...
        if ( mb.startsWith( "\"" ) && mb.endsWith( "\"" ) )
            mb = mb.mid( 1,mb.length()-2 );

        rx1.setPattern( "[ (]MESSAGES (\\d+)[ )]" );
        if ( rx1.indexIn( m_received.trimmed() ) != -1 ) {
            totalShouldBe = rx1.cap( 1 ).toInt();
            kDebug() << "Emitting messagecount";
            emit messageCount( this, mb, totalShouldBe );
        }

        rx1.setPattern( "[ (]UNSEEN (\\d+)[ )]" );
        if ( rx1.indexIn( m_received.trimmed() ) != -1 )
            emit unseenCount( this, mb, rx1.cap( 1 ).toInt() );

        rx1.setPattern( "[ (]UIDVALIDITY (\\d+)[ )]" );
        if ( rx1.indexIn( m_received.trimmed() ) != -1 )
            uidvalidity = rx1.cap( 1 );

        rx1.setPattern( "[ (]UIDNEXT (\\d+)[ )]" );
        if ( rx1.indexIn( m_received.trimmed() ) != -1 )
            uidnext = rx1.cap( 1 );
    } else {
        // we appereantly m_received garbage, should not happen, but
        // there are more things in life, requeue and get on with it.
        // possible loop of course...
        // kDebug() << "unexpected response received" << m_received;
        m_queue.append( m_currentQueueItem );
        m_currentQueueItem = Queue();
        slotProcessQueue();
        emit statusError( i18n( "Something is wrong..." ) );
        return;
    }

    emit integrity( mb, totalShouldBe, uidvalidity, uidnext );
    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseExpunge()
{
    emit expungeCompleted( this, m_currentQueueItem.mailbox() );
    slotParseExists();
}

void Imaplib::slotParseSaveMessage()
{
    // kDebug() << " : " << m_received;
    m_currentQueueItem = m_queue.first();
    if ( m_socket )
        m_socket->write( m_currentQueueItem.command() );
    m_queue.pop_front();
}

void Imaplib::slotParseSaveMessageData()
{
    if ( !capable( "uidplus" ) ) {
        emit saveDone( -1 );
        return;
    }

    QRegExp rx1( "\\[APPENDUID \\d+ (\\d+)\\]" );
    if ( rx1.indexIn( m_received.trimmed() ) != -1 )
        emit saveDone( rx1.cap( 1 ).toInt() );
    else
        emit saveDone( -1 );

    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseCreateMailBox()
{
    // kDebug() << " : " << m_received;

    if ( m_received.indexOf( "a02 OK" ) == -1 )
        emit mailBoxAdded( false );
    else
        emit mailBoxAdded( true );

    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseDeleteMailBox()
{
    // kDebug() << " : " << m_received;

    if ( m_received.indexOf( "a02 OK" ) == -1 )
        emit mailBoxDeleted( false, m_currentQueueItem.mailbox() );
    else
        emit mailBoxDeleted( true, m_currentQueueItem.mailbox() );

    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseRenameMailBox()
{
    // kDebug() << " : " << m_received;

    if ( m_received.indexOf( "a02 OK" ) == -1 )
        kDebug( ) << "Failed to rename folder";
    else {
        QString boxes = m_currentQueueItem.mailbox();
        QStringList i = boxes.split( "~" );
        emit mailBoxRenamed( i[0],i[1] );
    }

    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseExists()
{
    kDebug( ) << " for " << m_received;

    m_currentMailbox=m_currentQueueItem.mailbox();

    // Error handling
    if ( m_received.indexOf( "a02 NO" ) != -1 ) {
        kDebug() << "OOPS, that's not good";
        // if we get here because the queue was delayed, then we
        // need to remove it from the queue to prevent a loop.
        if ( m_queue.count() > 0 ) {
            Queue nextQueueItem = m_queue.first();
            if ( m_currentQueueItem.mailbox() == nextQueueItem.mailbox() ) {
                kDebug( ) << "Removing this and next command";
                m_queue.pop_front();
            }
        }

        m_checkMailTimer->stop();

        emit statusError( i18n( "Failed to select the mailbox" ) );
        emit uidsAndFlagsInFolder( this, m_currentQueueItem.mailbox(), QStringList() );

        m_currentQueueItem = Queue();
        slotProcessQueue();
        return;
    }

    m_checkMailTimer->start( 20*1000 );

    QString uidvalidity, uidnext;
    QRegExp rx1( " OK \\[UIDVALIDITY (\\d+)\\]" );
    if ( rx1.indexIn( m_received.trimmed() ) != -1 )
        uidvalidity = rx1.cap( 1 );

    rx1.setPattern( " OK \\[UIDNEXT (\\d+)\\]" );
    if ( rx1.indexIn( m_received.trimmed() ) != -1 )
        uidnext = rx1.cap( 1 );

    rx1.setPattern( " (\\d+) EXISTS" );
    if ( rx1.indexIn( m_received.trimmed() ) != -1 ) {

        int amount = rx1.cap( 1 ).toInt();
        kDebug() << "Emitting messagecount";
        emit messageCount( this, m_currentQueueItem.mailbox(), amount );

        if ( m_currentQueueItem.state() == Queue::SyncMailBox )
            getHeaderList( m_currentMailbox, 1, amount );
        else
            emit integrity( m_currentMailbox, amount, uidvalidity, uidnext );
    }

    // Sync will not emit the unseen count, so do a checkmail after that.
    //if ( m_currentQueueItem.state() == Queue::SyncMailBox )
    //    checkMail( m_currentMailbox );

    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseGetHeaderList()
{
    // kDebug();

    static QString all_data;
    all_data.append( m_received );

    if ( !endSimpleCommand( all_data ) )
        return;

    QStringList results;

    // * 1 FETCH (UID 1 FLAGS (\Seen))                  // CYRUS
    // * 3 FETCH (FLAGS (\Deleted \Seen) UID 26135)     // Courier
    QRegExp rx1( "UID [\\(]?(\\d*)[\\)]?" );
    QRegExp rx2( "FLAGS \\((.*)\\)" );
    rx2.setMinimal( true );

    QStringList fetchlist, allList, updateList;

    all_data = all_data.trimmed();

    int start = 0, start2 = 0;
    while ( start != -1 ) {
        start = rx1.indexIn( all_data, start );
        if ( start == -1 )
            break;

        results.append( rx1.cap( 1 ) );

        start2 = rx2.indexIn( all_data, start2 );
        if ( start2 != -1 )
            results.append( rx2.cap( 1 ) );

        ++start2;
        ++start;
    }
    emit uidsAndFlagsInFolder( this, m_currentQueueItem.mailbox(), results );

    all_data.clear();
    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}

void Imaplib::slotParseGetMessage()
{
    kDebug( );

    // Courier:
    //|* 1 FETCH (UID 1 RFC822.SIZE 8609 BODY[HEADER.FIELDS ("FROM" "SUBJECT" "DATE")] {148}|
    // Cyrus:
    //|* 1 FETCH (UID 26133 RFC822.SIZE 8609 BODY[HEADER.FIELDS (FROM SUBJECT DATE)] {139}|
    // Dovecot:
    //|* 1 FETCH (RFC822.SIZE 8609 UID 2 BODY[HEADER.FIELDS (FROM TO CC SUBJECT DATE IN-REPLY-TO MESSAGE-ID)] {337}

    static QString all_data;
    all_data.append( m_received );

    if ( !endSimpleCommand( all_data ) )
        return;

    QRegExp rx0;
    QRegExp rx1;
    QRegExp rx2;
    if ( m_currentQueueItem.state() == Queue::GetHeaders ) {
        rx0.setPattern( "UID (\\d*)" );
        rx1.setPattern( "RFC822.SIZE (\\d*)" );
        rx2.setPattern( QRegExp::escape( " BODY[HEADER.FIELDS " )
                        + "\\(\"?FROM\"? \"?TO\"? \"?CC\"? "
                        + "\"?SUBJECT\"? \"?DATE\"? \"?IN-REPLY-TO\"? "
                        + "\"?MESSAGE-ID\"?\\)\\] "
                        + QRegExp::escape( "{" ) + "(\\d*)" + QRegExp::escape( "}" ) );
    } else {
        rx0.setPattern( "UID (\\d*)" );
        rx2.setPattern( QRegExp::escape( "BODY[] " )
                        + QRegExp::escape( "{" ) + "(\\d*)" + QRegExp::escape( "}" ) );
    }

    QStringList headersToSend;
    while ( rx2.indexIn( all_data.trimmed() ) != -1 ) {
        // First capture the body part:
        QString size = rx2.cap( 1 );
        int startBody = rx2.pos( 1 )+size.length()+2;
        QString text = all_data.mid( startBody, size.toInt() ).trimmed();

        // Second, remove the complete regex part from the string.
        all_data = all_data.left( rx2.pos( 0 ) )+all_data.mid( startBody+size.toInt() );

        // Now figure out the uid.
        int uid=0;
        if ( rx0.indexIn( all_data.trimmed() ) != -1 )
            uid = rx0.cap( 1 ).toInt();

        if ( m_currentQueueItem.state() == Queue::GetHeaders ) {
            QString size;
            if ( m_currentQueueItem.state() == Queue::GetHeaders &&
                    rx1.indexIn( all_data.trimmed() ) != -1 )
                size = "Size: " + rx1.cap( 1 );

            text.append( "\n" );
            text.append( size );

            headersToSend.append( rx0.cap( 1 ) );
            headersToSend.append( m_currentQueueItem.mailbox() );
            headersToSend.append( text );
        } else
            emit message( this, m_currentQueueItem.mailbox(), uid, text );

        // Find the next occorunce of FETCH and strip untill there.
        all_data = all_data.mid( all_data.indexOf( "FETCH",
                                 rx0.pos( 1 ),Qt::CaseInsensitive ) );
    }

    if ( m_currentQueueItem.state() == Queue::GetHeaders )
        emit headersInFolder( this, m_currentQueueItem.mailbox(), headersToSend );

    // kDebug() << "done";
    all_data.clear();
    emit statusReady();
    m_currentQueueItem = Queue();
    slotProcessQueue();
}


// ---------------------------- Management -----------------------//


void Imaplib::slotProcessQueue()
{

    // kDebug() << m_queue.count() << m_socket->available() << m_currentQueueItem.state();
    if ( !m_socket || !m_queue.count() ||
            m_currentQueueItem.state() != Queue::None ||
            !m_socket->available() )
        return;

    m_currentQueueItem = m_queue.first();
    // Dont enable, as it exposes the password on login.
    // kDebug( ) << m_currentQueueItem.command();

    if ( m_currentMailbox == m_currentQueueItem.mailbox() ||
            m_currentQueueItem.mailbox().isEmpty() ||
            m_currentQueueItem.state() == Queue::SyncMailBox ||
            m_currentQueueItem.state() == Queue::SelectMailBox ||
            m_currentQueueItem.state() == Queue::CreateMailBox ||
            m_currentQueueItem.state() == Queue::DeleteMailBox ||
            m_currentQueueItem.state() == Queue::RenameMailBox ||
            m_currentQueueItem.state() == Queue::Noop ||
            m_currentQueueItem.state() == Queue::Capability ||
            m_currentQueueItem.state() == Queue::CheckMail ) {
        if ( m_currentQueueItem.state() == Queue::CheckMail &&
                m_currentQueueItem.mailbox() == m_currentMailbox )
            m_currentQueueItem = Queue( Queue::GetRecent, m_currentMailbox,
                                        "UID SEARCH UNSEEN" );

        if ( !m_currentQueueItem.comment().isEmpty() )
            emit status( m_currentQueueItem.comment() );

        write( m_currentQueueItem.command() );
        m_queue.pop_front();
    } else {
        // We need to switch to that mb first
        /*kDebug() << "Delayed, first selecting "<< m_currentQueueItem.mailbox()
                << " instead of " << m_currentMailbox << " -> for: "
        << m_currentQueueItem.state() */
        selectMailBox( m_currentQueueItem.mailbox() );
        m_currentQueueItem=Queue();
    }
}

void Imaplib::timerEvent( QTimerEvent * )
{
    slotProcessQueue();
}


void Imaplib::slotTLS()
{
    // kDebug();
    m_socket->write( "a02 starttls" );
}

#include "imaplib.moc"
