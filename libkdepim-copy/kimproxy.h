/*    
	kimproxy.h
	
	IM service library for KDE
	
	Copyright (c) 2004 Will Stephenson   <lists@stevello.free-online.co.uk>

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

#ifndef KIMPROXY_H
#define KIMPROXY_H

#include <qstringlist.h>


#define IM_SERVICE_TYPE "DCOP/InstantMessenger"
#define IM_CLIENT_PREFERENCES_FILE "default_components"
#define IM_CLIENT_PREFERENCES_SECTION "InstantMessenger"
#define IM_CLIENT_PREFERENCES_ENTRY "imClient"

#include "kimproxyiface.h"

class DCOPClient;
class KIMIface_stub;
class KURL;
class KIMProxy : public QObject, virtual public KIMProxyIface
{
	Q_OBJECT

	public:
		/**
		* Construct an instance of the proxy library
		* @param your app's DCOP client 
		*/
		KIMProxy( DCOPClient *);
		~KIMProxy();

		/**
		* Get the proxy ready to connect
		* Discover the preferred IM client and connect to it
		*/
		bool initialize();

		/**
		* Obtain a list of IM-contactable entries in the KDE 
		* address book.
		* @return a list of KABC uids.
		*/
		QStringList imAddresseeUids();
		
		/**
		* Obtain a list of  KDE address book entries who are 
		* currently reachable.
		* @return a list of KABC uids.
		*/
		QStringList reachableContacts();
		
		/**
		* Obtain a list of  KDE address book entries who are 
		* currently online.
		* @return a list of KABC uids.
		*/
		QStringList onlineContacts();
		
		/**
		* Obtain a list of  KDE address book entries who may
		* receive file transfers.
		* @return a list of KABC uids.
		*/
		QStringList fileTransferContacts();

		/** 
		* Confirm if a given KABC uid is known to KIMProxy
		*/
		bool isPresent( const QString& uid );
		
		/**
		* Obtain the IM status as a number (see KIMIface) for the specified addressee
		* @param uid the KABC uid you want the status for.
		*/
		int presenceNumeric( const QString& uid );
		
		/**
		* Obtain the IM status as a i18ned string for the specified addressee
		* @param uid the KABC uid you want the status for.
		*/
		QString presenceString( const QString& uid );

		/**
		* Obtain the icon representing IM status for the specified addressee
		* @param uid the KABC uid you want the status for.
		*/
		QPixmap presenceIcon( const QString& uid );

		/** 
		* @return Whether the specified addressee can receive files.
		*/
		bool canReceiveFiles( const QString & uid );
		
		/** 
		* Some media are unidirectional ( eg, sending SMS via a web interface).
		* @return Whether the specified addressee can respond.
		*/
		bool canRespond( const QString & uid );
		
		/**
		* Get the KABC uid corresponding to the supplied IM address
		* @return a KABC uid or null if none found/
		*/
		QString locate( const QString & contactId, const QString & protocol );
		
		/**
		* Get the supplied addressee's current context (home, work, or any).  
		* @return A context, or null if not supported.
		*/
		QString context( const QString & uid );
		
		/**
		* Start a chat session with the specified addressee
		* @param uid the KABC uid you want to chat with.
		*/
		void chatWithContact( const QString& uid );

		/**
		* Send a single message to the specified addressee
		* Any response will be handled by the IM client as a normal 
		* conversation.
		* @param uid the KABC uid you want to chat with.
		* @param message the message to send them.
		*/
		void messageContact( const QString& uid, const QString& message );

		/**
		* Send the file to the contact
		*/
		void sendFile(const QString &metaContactId, const KURL &sourceURL, const QString &altFileName = QString::null, uint fileSize = 0);

		/**
		* Add a contact to the contact list
		* @return whether the add succeeded.  False may signal already present, protocol not supported, or add operation not supported.
		*/
		bool addContact( const QString &contactId, const QString &protocol );
		
		/**
		* Just exists to let the idl compiler make the DCOP signal for this
		*/
		void contactStatusChanged( QString uid );
		public slots:
			void unregisteredFromDCOP( const QCString& appId );
signals:
		void sigContactStatusChanged( QString uid );
	protected:
		KIMIface_stub *m_im_client_stub;
		DCOPClient *m_dc;
};

#endif

