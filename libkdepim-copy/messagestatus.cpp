/*
    This file is part of KDEPIM.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "messagestatus.h"

#include <QString>

using namespace KPIM;


/** The message status format. These can be or'd together.
    Note, that the KMMsgStatusIgnored implies the
    status to be Read even if the flags are set
    to Unread or New. This is done in isRead()
    and related getters. So we can preserve the state
    when switching a thread to Ignored and back. */
enum MsgStatus
{
    KMMsgStatusUnknown =           0x00000000,
    KMMsgStatusNew =               0x00000001,
    KMMsgStatusUnread =            0x00000002,
    KMMsgStatusRead =              0x00000004,
    KMMsgStatusOld =               0x00000008,
    KMMsgStatusDeleted =           0x00000010,
    KMMsgStatusReplied =           0x00000020,
    KMMsgStatusForwarded =         0x00000040,
    KMMsgStatusQueued =            0x00000080,
    KMMsgStatusSent =              0x00000100,
    KMMsgStatusFlag =              0x00000200, // flag means important
    KMMsgStatusWatched =           0x00000400,
    KMMsgStatusIgnored =           0x00000800, // forces isRead()
    KMMsgStatusTodo =              0x00001000,
    KMMsgStatusSpam =              0x00002000,
    KMMsgStatusHam =               0x00004000,
    KMMsgStatusHasAttach =         0x00008000,
    KMMsgStatusHasNoAttach =       0x00010000  // TODO to be removed before KDE 4
};



MessageStatus::MessageStatus()
{
  mStatus = KMMsgStatusUnknown;
}


MessageStatus& MessageStatus::operator = ( const MessageStatus& other )
{
  mStatus = other.mStatus;
  return *this;
}

bool MessageStatus::operator == ( const MessageStatus& other ) const
{
  return ( mStatus == other.mStatus );
}

bool MessageStatus::operator != ( const MessageStatus& other ) const
{
  return ( mStatus != other.mStatus );
}

bool MessageStatus::operator & ( const MessageStatus& other ) const
{
  return ( mStatus & other.mStatus );
}

void MessageStatus::clear()
{
  mStatus = KMMsgStatusUnknown;
}

void MessageStatus::set( const MessageStatus& other )
{
  // Those stati are exclusive, but we have to lock at the
  // internal representation because Ignored can manipulate
  // the result of the getter methods.
  if ( other.mStatus & KMMsgStatusNew ) setNew();
  if ( other.mStatus & KMMsgStatusUnread ) setUnread();
  if ( other.mStatus & KMMsgStatusRead ) setRead();
  if ( other.mStatus & KMMsgStatusOld ) setOld();

  if ( other.isDeleted() ) setDeleted();
  if ( other.isReplied() ) setReplied();
  if ( other.isForwarded() ) setForwarded();
  if ( other.isQueued() ) setQueued();
  if ( other.isSent() ) setSent();
  if ( other.isImportant() ) setImportant();

  if ( other.isWatched() ) setWatched();
  if ( other.isIgnored() ) setIgnored();
  if ( other.isTodo() ) setTodo();
  if ( other.isSpam() ) setSpam();
  if ( other.isHam() ) setHam();
  if ( other.hasAttachment() ) setHasAttachment();
}

void MessageStatus::toggle( const MessageStatus& other )
{
  if ( other.isDeleted() )
    setDeleted( !( mStatus & KMMsgStatusDeleted ) );
  if ( other.isReplied() )
    setReplied( !( mStatus & KMMsgStatusReplied ) );
  if ( other.isForwarded() )
    setForwarded( !( mStatus & KMMsgStatusForwarded ) );
  if ( other.isQueued() )
    setQueued( !( mStatus & KMMsgStatusQueued ) );
  if ( other.isSent() )
    setSent( !( mStatus & KMMsgStatusSent ) );
  if ( other.isImportant() )
    setImportant( !( mStatus & KMMsgStatusFlag ) );

  if ( other.isWatched() )
    setWatched( !( mStatus & KMMsgStatusWatched ) );
  if ( other.isIgnored() )
    setIgnored( !( mStatus & KMMsgStatusIgnored ) );
  if ( other.isTodo() )
    setTodo( !( mStatus & KMMsgStatusTodo ) );
  if ( other.isSpam() )
    setSpam( !( mStatus & KMMsgStatusSpam ) );
  if ( other.isHam() )
    setHam( !( mStatus & KMMsgStatusHam ) );
  if ( other.hasAttachment() )
    setHasAttachment( !( mStatus & KMMsgStatusHasAttach ) );
}


bool MessageStatus::isOfUnknownStatus() const
{
  return ( mStatus == KMMsgStatusUnknown );
}

bool MessageStatus::isNew() const
{
  return ( mStatus & KMMsgStatusNew && !( mStatus & KMMsgStatusIgnored ) );
}

bool MessageStatus::isUnread() const
{
  return ( mStatus & KMMsgStatusUnread && !( mStatus & KMMsgStatusIgnored ) );
}

bool MessageStatus::isRead() const
{
  return ( mStatus & KMMsgStatusRead || mStatus & KMMsgStatusIgnored );
}

bool MessageStatus::isOld() const
{
  return ( mStatus & KMMsgStatusOld );
}

bool MessageStatus::isDeleted() const
{
  return ( mStatus & KMMsgStatusDeleted );
}

bool MessageStatus::isReplied() const
{
  return ( mStatus & KMMsgStatusReplied );
}

bool MessageStatus::isForwarded() const
{
  return ( mStatus & KMMsgStatusForwarded );
}

bool MessageStatus::isQueued() const
{
  return ( mStatus & KMMsgStatusQueued);
}

bool MessageStatus::isSent() const
{
   return ( mStatus & KMMsgStatusSent );
}

bool MessageStatus::isImportant() const
{
  return ( mStatus & KMMsgStatusFlag );
}

bool MessageStatus::isWatched() const
{
  return ( mStatus & KMMsgStatusWatched );
}

bool MessageStatus::isIgnored() const
{
  return ( mStatus & KMMsgStatusIgnored );
}

bool MessageStatus::isTodo() const
{
  return ( mStatus & KMMsgStatusTodo );
}

bool MessageStatus::isSpam() const
{
  return ( mStatus & KMMsgStatusSpam );
}

bool MessageStatus::isHam() const
{
  return ( mStatus & KMMsgStatusHam );
}

bool MessageStatus::hasAttachment() const
{
  return ( mStatus & KMMsgStatusHasAttach );
}



void MessageStatus::setNew()
{
  // new overrides old and read
  mStatus &= ~KMMsgStatusOld;
  mStatus &= ~KMMsgStatusRead;
  mStatus &= ~KMMsgStatusUnread;
  mStatus |= KMMsgStatusNew;
}

void MessageStatus::setUnread()
{
  // unread overrides read
  mStatus &= ~KMMsgStatusOld;
  mStatus &= ~KMMsgStatusRead;
  mStatus &= ~KMMsgStatusNew;
  mStatus |= KMMsgStatusUnread;
}

void MessageStatus::setRead()
{
  // Unset unread and new, set read
  mStatus &= ~KMMsgStatusUnread;
  mStatus &= ~KMMsgStatusNew;
  mStatus |= KMMsgStatusRead;
}

void MessageStatus::setOld()
{
  // old can't be new or unread
  mStatus &= ~KMMsgStatusNew;
  mStatus &= ~KMMsgStatusUnread;
  mStatus |= KMMsgStatusOld;
}

void MessageStatus::setDeleted( bool deleted )
{
  if ( deleted )
    mStatus |= KMMsgStatusDeleted;
  else
    mStatus &= ~KMMsgStatusDeleted;
}

void MessageStatus::setReplied( bool replied )
{
  if ( replied )
    mStatus |= KMMsgStatusReplied;
  else
    mStatus &= ~KMMsgStatusReplied;
}

void MessageStatus::setForwarded( bool forwarded )
{
  if ( forwarded )
    mStatus |= KMMsgStatusForwarded;
  else
    mStatus &= ~KMMsgStatusForwarded;
}

void MessageStatus::setQueued( bool queued )
{
  if ( queued )
    mStatus |= KMMsgStatusQueued;
  else
    mStatus &= ~KMMsgStatusQueued;
}

void MessageStatus::setSent( bool sent )
{
  if ( sent ) {
    mStatus &= ~KMMsgStatusQueued;
    // FIXME to be discussed if sent messages are Read
    mStatus &= ~KMMsgStatusUnread;
    mStatus &= ~KMMsgStatusNew;
    mStatus |= KMMsgStatusSent;
  }
  else {
    mStatus &= ~KMMsgStatusSent;
  }
}

void MessageStatus::setImportant( bool important )
{
  if ( important )
    mStatus |= KMMsgStatusFlag;
  else
    mStatus &= ~KMMsgStatusFlag;
}

// Watched and ignored are mutually exclusive
void MessageStatus::setWatched( bool watched )
{
  if ( watched ) {
    mStatus &= ~KMMsgStatusIgnored;
    mStatus |= KMMsgStatusWatched;
  } else {
    mStatus &= ~KMMsgStatusWatched;
  }
}

void MessageStatus::setIgnored( bool ignored )
{
  if ( ignored ) {
    mStatus &= ~KMMsgStatusWatched;
    mStatus |= KMMsgStatusIgnored;
  } else {
    mStatus &= ~KMMsgStatusIgnored;
  }
}

void MessageStatus::setTodo( bool todo )
{
  if ( todo )
    mStatus |= KMMsgStatusTodo;
  else
    mStatus &= ~KMMsgStatusTodo;
}

// Ham and Spam are mutually exclusive
void MessageStatus::setSpam( bool spam )
{
  if ( spam ) {
    mStatus &= ~KMMsgStatusHam;
    mStatus |= KMMsgStatusSpam;
  } else {
    mStatus &= ~KMMsgStatusSpam;
  }
}

void MessageStatus::setHam( bool ham )
{
  if ( ham ) {
    mStatus &= ~KMMsgStatusSpam;
    mStatus |= KMMsgStatusHam;
  } else {
    mStatus &= ~KMMsgStatusHam;
  }
}

void MessageStatus::setHasAttachment( bool withAttachment )
{
  if ( withAttachment ) {
    mStatus |= KMMsgStatusHasAttach;
    // FIXME next line to be removed
    mStatus &= ~KMMsgStatusHasNoAttach;
  }
  else {
    mStatus &= ~KMMsgStatusHasAttach;
    // FIXME next line to be removed
    mStatus |= KMMsgStatusHasNoAttach;
  }
}



const qint32 MessageStatus::toQInt32() const
{
  return mStatus;
}

void MessageStatus::fromQInt32( qint32 status)
{
  mStatus = status;
}

QString MessageStatus::getStatusStr() const
{
  QString sstr;
  if ( mStatus & KMMsgStatusNew ) sstr += 'N';
  if ( mStatus & KMMsgStatusUnread ) sstr += 'U';
  if ( mStatus & KMMsgStatusOld ) sstr += 'O';
  if ( mStatus & KMMsgStatusRead ) sstr += 'R';
  if ( mStatus & KMMsgStatusDeleted ) sstr += 'D';
  if ( mStatus & KMMsgStatusReplied ) sstr += 'A';
  if ( mStatus & KMMsgStatusForwarded ) sstr += 'F';
  if ( mStatus & KMMsgStatusQueued ) sstr += 'Q';
  if ( mStatus & KMMsgStatusTodo ) sstr += 'K';
  if ( mStatus & KMMsgStatusSent ) sstr += 'S';
  if ( mStatus & KMMsgStatusFlag ) sstr += 'G';
  if ( mStatus & KMMsgStatusWatched ) sstr += 'W';
  if ( mStatus & KMMsgStatusIgnored ) sstr += 'I';
  if ( mStatus & KMMsgStatusSpam ) sstr += 'P';
  if ( mStatus & KMMsgStatusHam ) sstr += 'H';
  if ( mStatus & KMMsgStatusHasAttach ) sstr += 'T';
  if ( mStatus & KMMsgStatusHasNoAttach ) sstr += 'C';

  return sstr;
}

void MessageStatus::setStatusFromStr( QString aStr )
{
  mStatus = KMMsgStatusUnknown;

  if ( aStr.contains( 'N' ) ) setNew();
  if ( aStr.contains( 'U' ) ) setUnread();
  if ( aStr.contains( 'O' ) ) setOld();
  if ( aStr.contains( 'R' ) ) setRead();
  if ( aStr.contains( 'D' ) ) setDeleted();
  if ( aStr.contains( 'A' ) ) setReplied();
  if ( aStr.contains( 'F' ) ) setForwarded();
  if ( aStr.contains( 'Q' ) ) setQueued();
  if ( aStr.contains( 'K' ) ) setTodo();
  if ( aStr.contains( 'S' ) ) setSent();
  if ( aStr.contains( 'G' ) ) setImportant();
  if ( aStr.contains( 'W' ) ) setWatched();
  if ( aStr.contains( 'I' ) ) setIgnored();
  if ( aStr.contains( 'P' ) ) setSpam();
  if ( aStr.contains( 'H' ) ) setHam();
  if ( aStr.contains( 'T' ) ) setHasAttachment();
  if ( aStr.contains( 'C' ) ) setHasAttachment( false );
}

QString MessageStatus::getSortRank() const
{
  QString sstr = "bcbbbbbbbb";

  // put watched ones first, then normal ones, ignored ones last
  if ( mStatus & KMMsgStatusWatched ) sstr[0] = 'a';
  if ( mStatus & KMMsgStatusIgnored ) sstr[0] = 'c';

  // Second level. One of new, old, read, unread
  if ( mStatus & KMMsgStatusNew ) sstr[1] = 'a';
  if ( mStatus & KMMsgStatusUnread ) sstr[1] = 'b';
  //if ( mStatus & KMMsgStatusOld ) sstr[1] = 'c';
  //if ( mStatus & KMMsgStatusRead ) sstr[1] = 'c';

  // Third level. In somewhat arbitrary order.
  if ( mStatus & KMMsgStatusDeleted ) sstr[2] = 'a';
  if ( mStatus & KMMsgStatusFlag ) sstr[3] = 'a';
  if ( mStatus & KMMsgStatusReplied ) sstr[4] = 'a';
  if ( mStatus & KMMsgStatusForwarded ) sstr[5] = 'a';
  if ( mStatus & KMMsgStatusQueued ) sstr[6] = 'a';
  if ( mStatus & KMMsgStatusSent ) sstr[7] = 'a';
  if ( mStatus & KMMsgStatusHam ) sstr[8] = 'a';
  if ( mStatus & KMMsgStatusSpam ) sstr[8] = 'c';
  if ( mStatus & KMMsgStatusTodo ) sstr[9] = 'a';

  return sstr;
}



MessageStatus MessageStatus::statusNew()
{
  MessageStatus st;
  st.setNew();
  return st;
}

MessageStatus MessageStatus::statusRead()
{
  MessageStatus st;
  st.setRead();
  return st;
}

MessageStatus MessageStatus::statusUnread()
{
  MessageStatus st;
  st.setUnread();
  return st;
}

MessageStatus MessageStatus::statusNewAndUnread()
{
  MessageStatus st;
  st.setUnread();
  st.setNew();
  return st;
}

MessageStatus MessageStatus::statusOld()
{
  MessageStatus st;
  st.setOld();
  return st;
}

MessageStatus MessageStatus::statusDeleted()
{
  MessageStatus st;
  st.setDeleted();
  return st;
}

MessageStatus MessageStatus::statusReplied()
{
  MessageStatus st;
  st.setReplied();
  return st;
}

MessageStatus MessageStatus::statusForwarded()
{
  MessageStatus st;
  st.setForwarded();
  return st;
}

MessageStatus MessageStatus::statusQueued()
{
  MessageStatus st;
  st.setQueued();
  return st;
}

MessageStatus MessageStatus::statusSent()
{
  MessageStatus st;
  st.setSent();
  return st;
}

MessageStatus MessageStatus::statusImportant()
{
  MessageStatus st;
  st.setImportant();
  return st;
}

MessageStatus MessageStatus::statusWatched()
{
  MessageStatus st;
  st.setWatched();
  return st;
}

MessageStatus MessageStatus::statusIgnored()
{
  MessageStatus st;
  st.setIgnored();
  return st;
}

MessageStatus MessageStatus::statusTodo()
{
  MessageStatus st;
  st.setTodo();
  return st;
}

MessageStatus MessageStatus::statusSpam()
{
  MessageStatus st;
  st.setSpam();
  return st;
}

MessageStatus MessageStatus::statusHam()
{
  MessageStatus st;
  st.setHam();
  return st;
}

MessageStatus MessageStatus::statusHasAttachment()
{
  MessageStatus st;
  st.setHasAttachment();
  return st;
}
