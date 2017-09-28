/*
    This file is part of Akonadi KolabProxy.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "distributionlist.h"

#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>
#include <QDebug>

using namespace KolabV2;

static const char* s_unhandledTagAppName = "KOLABUNHANDLED"; // no hyphens in appnames!

// saving (contactgroup->xml)
DistributionList::DistributionList( const KContacts::ContactGroup* contactGroup )
{
  setFields( contactGroup );
}

// loading (xml->contactgroup)
DistributionList::DistributionList( const QString& xml )
{
  load( xml );
}

DistributionList::~DistributionList()
{
}

void DistributionList::setName( const QString& name )
{
  mName = name;
}

QString DistributionList::name() const
{
  return mName;
}

void KolabV2::DistributionList::loadDistrListMember( const QDomElement& element )
{
  Member member;
  for ( QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      QString tagName = e.tagName();
      if ( tagName == "display-name" )
        member.displayName = e.text();
      else if ( tagName == "smtp-address" )
        member.email = e.text();
      else if ( tagName == "uid" )
        member.uid = e.text();
    }
  }
  mDistrListMembers.append( member );
}

void DistributionList::saveDistrListMembers( QDomElement& element ) const
{
  QList<Member>::ConstIterator it = mDistrListMembers.constBegin();
  for( ; it != mDistrListMembers.constEnd(); ++it ) {
    QDomElement e = element.ownerDocument().createElement( "member" );
    element.appendChild( e );
    const Member& m = *it;
    if (!m.uid.isEmpty()) {
      writeString( e, "uid", m.uid );
    } else {
      writeString( e, "display-name", m.displayName );
      writeString( e, "smtp-address", m.email );
    }
  }
}

bool DistributionList::loadAttribute( QDomElement& element )
{
  const QString tagName = element.tagName();
  switch ( tagName[0].toLatin1() ) {
  case 'd':
    if ( tagName == "display-name" ) {
      setName( element.text() );
      return true;
    }
    break;
  case 'm':
    if ( tagName == "member" ) {
      loadDistrListMember( element );
      return true;
    }
    break;
  default:
    break;
  }
  return KolabBase::loadAttribute( element );
}

bool DistributionList::saveAttributes( QDomElement& element ) const
{
  // Save the base class elements
  KolabBase::saveAttributes( element );
  writeString( element, "display-name", name() );
  saveDistrListMembers( element );

  return true;
}

bool DistributionList::loadXML( const QDomDocument& document )
{
  QDomElement top = document.documentElement();

  if ( top.tagName() != "distribution-list" ) {
    qWarning( "XML error: Top tag was %s instead of the expected distribution-list",
              top.tagName().toAscii().data() );
    return false;
  }


  for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    if ( n.isComment() )
      continue;
    if ( n.isElement() ) {
      QDomElement e = n.toElement();
      if ( !loadAttribute( e ) ) {
        // Unhandled tag - save for later storage
        //qDebug() <<"Saving unhandled tag" << e.tagName();
        Custom c;
        c.app = s_unhandledTagAppName;
        c.name = e.tagName();
        c.value = e.text();
        mCustomList.append( c );
      }
    } else
      qDebug() <<"Node is not a comment or an element???";
  }

  return true;
}

QString DistributionList::saveXML() const
{
  QDomDocument document = domTree();
  QDomElement element = document.createElement( "distribution-list" );
  element.setAttribute( "version", "1.0" );
  saveAttributes( element );
  document.appendChild( element );
  return document.toString();
}

QString DistributionList::productID() const
{
  // TODO should we get name/version from desktop file?
  return QLatin1String( "Akonadi Kolab Proxy" );
}

// The saving is contactgroup -> DistributionList -> xml, this is the first part
void DistributionList::setFields( const KContacts::ContactGroup* contactGroup )
{
  KolabBase::setFields( contactGroup );

  setName( contactGroup->name() );

  // explicit contact data
  for ( uint index = 0; index < contactGroup->dataCount(); ++index ) {
    const KContacts::ContactGroup::Data& data = contactGroup->data( index );

    Member m;
    m.displayName = data.name();
    m.email = data.email();

    mDistrListMembers.append( m );
  }
  for ( uint index = 0; index < contactGroup->contactReferenceCount(); ++index ) {
    const KContacts::ContactGroup::ContactReference& data = contactGroup->contactReference( index );

    Member m;
    m.uid = data.uid();

    mDistrListMembers.append( m );
  }
  if (contactGroup->contactGroupReferenceCount() > 0) {
    qWarning() << "Tried to save contact group references, which should have been resolved already";
  }
}

// The loading is: xml -> DistributionList -> contactgroup, this is the second part
void DistributionList::saveTo( KContacts::ContactGroup* contactGroup )
{
  KolabBase::saveTo( contactGroup );

  contactGroup->setName( name() );

  QList<Member>::ConstIterator mit = mDistrListMembers.constBegin();
  for ( ; mit != mDistrListMembers.constEnd(); ++mit ) {
    if (!(*mit).uid.isEmpty()) {
      contactGroup->append(KContacts::ContactGroup::ContactReference( (*mit).uid ));
    } else {
      contactGroup->append(KContacts::ContactGroup::Data( (*mit).displayName, (*mit).email ));
    }
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
