/*
    Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "kjotsmigrator.h"

#include <QDomDocument>
#include <QDomElement>
#include <QTextCodec>
#include <QTextDocument>

#include <KLocale>
#include <KStandardDirs>

#include <KMime/Message>

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentInstanceCreateJob>
#include <akonadi/item.h>
#include <Akonadi/Collection>
#include <Akonadi/EntityDisplayAttribute>

#include "entitytreecreatejob.h"

using namespace Akonadi;

KJotsMigrator::KJotsMigrator()
  : KMigratorBase()
{
  m_dataDir = QDir( KStandardDirs::locateLocal("data","kjots" ) );
  m_dataDir.setNameFilters( QStringList( "*.book" ) );

  if ( !m_dataDir.exists() )
  {
    return;
  }

  const QString &kjotsCfgFile = KStandardDirs::locateLocal( "config", QString( "kjotsrc" ) );
  // Check if migration has already been done and abort if so.
  unicode = false; // Retrieve from settings.

  migrate();
}

KJotsMigrator::~KJotsMigrator()
{

}

void KJotsMigrator::migrate()
{
  emit message( Info, i18n("Beginning KJots migration...") );
  createAgentInstance("akonadi_notes_resource", this, SLOT(notesResourceCreated(KJob*)));
}

void KJotsMigrator::notesResourceCreated( KJob *job )
{
  if ( job->error() ) {
    emit message( Error, i18n( "Failed to create resource for local notes: %1", job->errorText() ) );
    deleteLater();
    return;
  }
  emit message( Info, i18n( "Created local notes resource." ) );

  AgentInstance instance = static_cast< AgentInstanceCreateJob* >( job )->instance();

  instance.setName( i18nc( "Default name for resource holding notes", "Local Notes" ) );

  // Get the root collection of this resource.
  // m_resourceCollection

  const QString &kjotsCfgFile = KStandardDirs::locateLocal( "config", QString( "kjotsrc" ) );
//   mConfig = new KConfig( kmailCfgFile );
//   mAccounts = mConfig->groupList().filter( QRegExp( "Account \\d+" ) );
//   mIt = mAccounts.begin();

  m_backupDir = QDir( KStandardDirs::locateLocal("data","kjots/.backup" ) );
  if ( !m_backupDir.exists() )
  {
    m_backupDir.mkpath( "." );
  }

  m_bookFiles = m_dataDir.entryList();

  migrateNext();
}

void KJotsMigrator::migrateNext()
{
  if ( m_bookFiles.isEmpty() )
  {
    migrationFinished();
    return;
  }

  QString bookFileName = m_bookFiles.takeFirst();
  emit message( Info, i18nc("A migration tool is migrating the file named %1", "Migrating \"%1\"...").arg( bookFileName ) );
  migrateLegacyBook( bookFileName );
}

void KJotsMigrator::migrationFinished()
{

}

// This method taken from KJotsBook::openBook
void KJotsMigrator::migrateLegacyBook(const QString& fileName)
{
  QFile file(fileName);
  QDomDocument doc( "KJots" );
  bool oldBook = false;

  if ( !file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
      emit message( Error, i18n("Failed to open file: \"%1\"").arg( fileName ) );
      return;
  }

  //Determine if this is a KDE3.5 era book.
  QByteArray firstLine = file.readLine();
  file.reset();

  if ( !firstLine.startsWith( "<?xml" ) ) { // krazy:exclude=strings
      emit message( Info, i18n("%1 is a KDE 3.5 era book").arg( fileName ) );

      QTextStream st(&file);
      if ( unicode ) {
          st.setCodec( "UTF-8" );
      } else {
          st.setCodec( QTextCodec::codecForLocale () );
      }

      doc.setContent( st.readAll() );
      oldBook = true;
  } else {
      doc.setContent(&file);
  }

  QDomElement docElem = doc.documentElement();

  if ( docElem.tagName() ==  "KJots" ) {
    QDomNode n = docElem.firstChild();
    while( !n.isNull() ) {
      QDomElement e = n.toElement(); // try to convert the node to an element.
      if( !e.isNull() && e.tagName() == "KJotsBook" ) {
        parseBookXml(e, oldBook, m_resourceCollection, 0);
      }
      n = n.nextSibling();
    }
  }
  EntityTreeCreateJob *job = new EntityTreeCreateJob(m_collectionLists, m_items, this);
  connect(job, SIGNAL(finished(KJob*)), SLOT(bookMigrateJobFinished(KJob*)));
  m_collectionLists.clear();
  m_items.clear();
}

void KJotsMigrator::bookMigrateJobFinished(KJob* job)
{
  if (job->error())
  {
    QString filename;
    emit message(Error, i18n("Error migrating the book \"%1\"").arg(filename));
  }
  migrateNext();
}


void KJotsMigrator::parseBookXml( QDomElement &me, bool oldBook, const Collection &parentCollection, int depth )
{
  Collection collection;
  collection.setParentCollection( parentCollection );
  collection.setContentMimeTypes( QStringList( "text/x-vnd.akonadi.note" ) );

  QDomNode n = me.firstChild();

  EntityDisplayAttribute *eda = new EntityDisplayAttribute();
  eda->setIconName( "x-office-address-book" );
  while( !n.isNull() ) {
    QDomElement e = n.toElement(); // try to convert the node to an element.
    if ( !e.isNull() ) {

      if ( e.tagName() == "Title" )
      {
          collection.setName( e.text() );
      }
      else
      if ( e.tagName() == "ID" )
      {
        // Legacy ID attribute?
          // setId(e.text().toULongLong());
      }
      else
      if ( e.tagName() == "Color" )
      {
        QColor color( e.text() );
        eda->setBackgroundColor( color );
      }
      if ( e.tagName() == "KJotsPage" ) {
        parsePageXml(e, oldBook, collection );
      }
      else if ( e.tagName() == "KJotsBook" ) {
        parseBookXml(e, oldBook, collection, depth + 1 );
      } else {
        // Fatal error. Continue.
      }
    }
    n = n.nextSibling();
  }
  collection.addAttribute(eda);

  if (m_collectionLists.size() == depth)
    m_collectionLists.append(Collection::List());

  Q_ASSERT(m_collectionLists.size() < depth);
  m_collectionLists[depth].append(collection);

}

void KJotsMigrator::parsePageXml(QDomElement&me , bool oldBook, const Collection &parentCollection )
{
  // serialize document to kmime message and append to items.
  KMime::Message::Ptr note = KMime::Message::Ptr( new KMime::Message() );
  QByteArray encoding = "utf-8";

  QTextDocument document;
  EntityDisplayAttribute *eda = new EntityDisplayAttribute();
  eda->setIconName( "text-plain" );
  if ( me.tagName() == "KJotsPage" )
  {
    QDomNode n = me.firstChild();
    while( !n.isNull() )
    {
      // Titles and Ids?
      QDomElement e = n.toElement();
      if ( !e.isNull() )
      {
        if ( e.tagName() == "Title" )
        {
            note->subject()->fromUnicodeString( e.text(), encoding );
        }
        else
        if ( e.tagName() == "ID" )
        {
          // Legacy ID attribute?
            // setId(e.text().toULongLong());
        }
        else
        if ( e.tagName() == "Color" )
        {
          QColor color( e.text() );
          eda->setBackgroundColor( color );
        }
        if ( e.tagName() == "Text" )
        {
          QString bodyText = e.text();

          //This is for 3.5 era book support. Remove when appropriate.
          if ( e.hasAttribute( "fixed" ) ) {
              bodyText.replace("]]&gt;", "]]>");
          }

          if ( oldBook ) {
              // Ensure that whitespace is reproduced as in kjots of kde3.5.
              // https://bugs.kde.org/show_bug.cgi?id=175100
              document.setPlainText(bodyText);
          } else {
            if ( Qt::mightBeRichText( bodyText ) )
              document.setHtml(bodyText);
            else
              document.setPlainText(bodyText);
          }
        }
        else
        {
          // Fatal error move on.
        }
      }
      n = n.nextSibling();
    }
  }
  note->assemble();

  Item item;
  item.setParentCollection( parentCollection );
  item.setMimeType( "text/x-vnd.akonadi.note" );
  item.setPayload<KMime::Message::Ptr>(note);
  item.addAttribute(eda);

  m_items.append(item);
}

void KJotsMigrator::migrationFailed(const QString& errorMsg, const Akonadi::AgentInstance& instance)
{

}

