/* This file is part of the KDE project

   Copyright (C) 2006-2007 KovoKs <info@kovoks.nl>

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


#ifndef IMAPLIB_H
#define IMAPLIB_H

#include "imap_export.h"

#include <kimap/rfccodecs.h>
#include "socket.h"
#include <QWidget>

//class QSocket;
class QTimerEvent;

class Queue
{
public:
    enum Commands {
        None=0,
        NoResponse,
        Connecting,
        Auth,
        GetMailBoxList,
        CheckMail,
        SelectMailBox,
        SyncMailBox,
        GetHeaderList,
        GetHeaders,
        GetMessage,
        GetRecent,
        Copy,
        Expunge,
        SaveMessage,
        SaveMessageData,
        CreateMailBox,
        DeleteMailBox,
        RenameMailBox,
        Capability,
        IdleStart,
        IdleStop,
        Noop,
        Logout
    };

    Queue(): st( 0 ) {}
    explicit Queue( Commands state, const QString& mailbox,
                    const QString& command, const QString& comment=QString() )
            : st( state ), mb( mailbox ), com( command ), comm( comment ) { }
    int state() const {
        return st;
    }
    QString mailbox() const {
        return mb;
    }
    QString command() const {
        return com;
    }
    QString comment() const {
        return comm;
    }

private:
    int st;
    QString mb, com, comm;
};


/**
 * @class Imaplib
 * Responsible for communicating with the server and emitting the results back
 * @author Tom Albers <tomalbers@kde.nl>
 */
class IMAP_EXPORT Imaplib : public QWidget
{
    Q_OBJECT

public:
    /**
     * Contructor
     */
    Imaplib( QWidget* parent,  const char* name );

    /**
     * Destructor
     */
    ~Imaplib();

    /**
     * connect to the Imapserver. You can call this safely with
     * changing from socket to secure socket and vice versa.
     * @param server the server
     * @param port the port to connect to.
     * @param protocol what protocol
     */
    void startConnection( const QString&, int port, int sec );

    /**
     * login to the Imapserver. There is no need to escape things, this
     * lib will do that for you.
     * @param username the username
     * @param password the password
     */
    void login( const QString& username, const QString& password );

    /**
     * disconnects to the server.
     */
    void logout();

    /**
     * Will fetch the mailbox list, and emit a mailBoxList()
     * as a result.
     */
    void getMailBoxList();

    /**
     * Will fetch the headers of mailbox.
     */
    void getHeaders( const QString& mb, const QStringList& uids );

    /**
     * Convenience function to get the headers from a mailbox.
     */
    void getHeaderList( const QString& mb, int start, int end );

    /**
     * Use this to get the messages of a mailbox, all headers will be fetched.
     * The call will emit a mailBox() call when ready.
     * @param box the mailbox
     */
    void getMailBox( const QString& box );

    /**
     * Get the body of @p uid from the @p mb mailbox
     */
    void getMessage( const QString& mb, int uid );

    /**
     * This will not select the mailbox, but will update message count
     * and fetch new mail
     * @param box the mailbox
     */
    void checkMail( const QString& box );

    /**
     * add a flag to a message or messages. Keep uidmin and uidmax the same
     * if you want it for one message.
     * @param box the mailbox
     * @param uidmin minuid of the message
     * @param uidmax maxuid of the message
     * @param flag flag to add
     */
    void addFlag( const QString& box, int uidmin, int uidmax,
                  const QString& flag );

    /**
     * remove a flag from a message or messages. Keep uidmin and uidmax the same
     * if you want it for one message.
     * @param box the mailbox
     * @param uidmin minuid of the message
     * @param uidmax maxuid of the message
     * @param flag flag to add
     */
    void removeFlag( const QString& box, int uidmin, int uidmax,
                     const QString& flag );

    /**
     * removes deleted messages
     * @param box the mailbox
     */
    void expungeMailBox( const QString& box );

    /**
     * create a folder
     */
    void createMailBox( const QString& box );

    /**
     * delete a folder
     */
    void deleteMailBox( const QString& box );

    /**
     * rename folder @p oldfolder to @p newfolder
     */
    void renameMailBox( const QString& oldfolder, const QString& newfolder );

    /**
     * This will copy the message with @p uid from @p origbox to the
     * @p destbox mailbox. If you are searching for a move mailbox, then
     * use this function and add a add the deleted flag after it.
     */
    void copyMessage( const QString& origbox, int uid, const QString& destbox );

    /**
     * Store message with the content @p message in the @p mb mailbox, @p flags
     * can be used to set extra flags. \\Seen is default and set always.
     */
    void saveMessage( const QString& mb, const QString& message,
                      const QString& flags = QString() );

    /**
     * Start monitoring the @p mb mailbox
     */
    void idleStart( const QString& mb );

    /**
     * Stop monitoring
     */
    void idleStop();

    /**
     * returns if a server is capabile of @p something
     */
    bool capable( const QString& something );

private:

    enum State {
        NotConnected = 0,
        Connected,
        NotAuthenticated,
        Authenticated,
        Selected,
        LoggedOut
    };

    State               m_currentState;
    Queue               m_currentQueueItem;
    QString             m_currentMailbox;
    Socket*             m_socket;
    QList<Queue>        m_queue;
    QStringList         m_capabilities;
    QString             m_protocol;
    bool                m_tls;

    bool                m_readyToSend;
    QTimer*             m_preventLogOutTimer;
    QTimer*             m_checkMailTimer;
    QString             m_received;

    /* Saves the current parameters of the connection. */
    QString             m_server;
    int                 m_port;

    void write( const QString& text );
    void selectMailBox( const QString& box );
    void timerEvent( QTimerEvent * );

signals:
    /**
     * This signal is emitted when the connection is up and running, and
     * is available to log in.
     */
    void login( Imaplib* );

    /**
     * This signal is emitted after a login() is received and the server
     * refused the credentials.
     */
    void loginFailed( Imaplib* );

    /**
     * This signal is emitted after a login() is received and the server
     * accepted the credentials.
     */
    void loginOk( Imaplib* );

    /**
     * Reports about the network status
     */
    void status( const QString& );

    /**
     * Reports about the network status done
     */
    void statusReady();

    /**
     * Reports about the network status, there has been an error, but
     * this class is done with it, so restore the cursor and do something
     */
    void statusError( const QString& );

    /**
     * Reports about the network status, there has been an error, fatal.
     */
    void error( const QString& );

    /**
     * disconnected.
     */
    void disconnected();

    /**
     * disconnected.
     */
    void unexpectedDisconnect();

    /**
     * This signal is emitted when the server gives an ALERT. The message
     * should be presented to the user in a visible way.
     */
    void alert( Imaplib*, const QString& );

    /**
     * This signal is emitted when the server has returned the mailboxlist.
     * this can be requested with getMailBoxList();
     */
    void currentFolders( const QStringList& );

    /**
     * This signal is emitted when a mailbox is added
     */
    void mailBoxAdded( const QString& );

    /**
     * This signal is emitted when a mailbox is deleted
     */
    void mailBoxDeleted( const QString& );

    /**
     * This signal is emitted when a mailbox is renamed
     */
    void mailBoxRenamed( const QString&, const QString& );

    /**
     * This signal is emitted when a message is saved
     */
    void saveDone();

    /**
     * this signal is emitted when the server has returned the headers of
     * the mailbox after a getMailBox(); @p values is a triplet in the form
     * of uid, mailbox, headers.
     */
    void headersInFolder( Imaplib*, const QString&, const QStringList& values );

    /**
     * The body is fetched and available for retrieval
     */
    void message( Imaplib*, const QString& mb, int uid, const QString& );

    /**
     * We know the amount of messages(@p amount) in the @p mb
     */
    void messageCount( Imaplib*, const QString& mb, int amount );

    /**
     * We know the amount of unseen messages(@p amount) in the @p mb
     */
    void unseenCount( Imaplib*, const QString& mb, int amount );

    /**
     * expunge is completed.
     */
    void expungeCompleted( Imaplib*, const QString& mb );

    /**
     * This will be emitted when we know which items are in the @p mb mailbox,
     * @p values will be in the form of "uid\nflags\n".
     */
    void uidsAndFlagsInFolder( Imaplib*, const QString& mb, const QStringList& values );

    /**
     * This function will be emitted and can be used to verify if a cache
     * still contains up-to-date info.
     */
    void integrity( const QString&, int, const QString&, const QString& );

private slots:
    void slotConnected();
    void slotRead( const QString& );
    void slotProcessQueue();
    void slotTLS();
    void slotSendNoop();
    void slotParseGetMailBoxList();
    void slotParseGetRecent();
    void slotParseCopy();
    void slotParseExpunge();
    void slotParseNoop();
    void slotParseCheckMail();
    void slotParseExists();
    void slotParseGetHeaderList();
    void slotParseGetMessage();
    void slotParseSaveMessage();
    void slotParseCreateMailBox();
    void slotParseDeleteMailBox();
    void slotParseRenameMailBox();
};

#endif // Imaplib_H

