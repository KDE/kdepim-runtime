/*
    This file is part of KDE.

    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

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

#include <qfile.h>
#include <qtextstream.h>
#include <qdom.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kstandarddirs.h>

static const KCmdLineOptions options[] =
{
  { "+file", I18N_NOOP("Input cfg XML file."), 0 },
  KCmdLineLastOption
};


class CfgEntry
{
  public:
    CfgEntry( const QString &group, const QString &type, const QString &key,
              const QString &name, const QString &label,
              const QString &defaultValue, const QStringList &values )
      : mGroup( group ), mType( type ), mKey( key ), mName( name ),
        mLabel( label ), mDefaultValue( defaultValue ), mValues( values )
    {
    }

    void setGroup( const QString &group ) { mGroup = group; }
    QString group() const { return mGroup; }
    
    void setType( const QString &type ) { mType = type; }
    QString type() const { return mType; }

    void setKey( const QString &key ) { mKey = key; }
    QString key() const { return mKey; }

    void setName( const QString &name ) { mName = name; }
    QString name() const { return mName; }

    void setLabel( const QString &label ) { mLabel = label; }
    QString label() const { return mLabel; }

    void setDefaultValue( const QString &d ) { mDefaultValue = d; }
    QString defaultValue() const { return mDefaultValue; }

    void setValues( const QStringList &d ) { mValues = d; }
    QStringList values() const { return mValues; }

    void dump() const
    {
      kdDebug() << "<entry>" << endl;
      kdDebug() << "  group: " << mGroup << endl;
      kdDebug() << "  type: " << mType << endl;
      kdDebug() << "  key: " << mKey << endl;
      kdDebug() << "  name: " << mName << endl;
      kdDebug() << "  label: " << mLabel << endl;
      kdDebug() << "  default: " << mDefaultValue << endl;
      kdDebug() << "</entry>" << endl;
    }

  private:
    QString mGroup;
    QString mType;
    QString mKey;
    QString mName;
    QString mLabel;
    QString mDefaultValue;
    QStringList mValues;
};


CfgEntry *parseEntry( const QString &group, const QDomElement &element )
{
  QString type = element.attribute( "type" );

  QString name;
  QString key;
  QString label;
  QString defaultValue;
  QStringList values;

  QDomNode n;
  for ( n = element.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    QDomElement e = n.toElement();
    QString tag = e.tagName();    
    if ( tag == "key" ) key = e.text();
    else if ( tag == "name" ) name = e.text();
    else if ( tag == "label" ) label = e.text();
    else if ( tag == "default" ) defaultValue = e.text();
    else if ( tag == "values" ) {
      QDomNode n2;
      for( n2 = e.firstChild(); !n2.isNull(); n2 = n2.nextSibling() ) {
        QDomElement e2 = n2.toElement();
        if ( e2.tagName() == "value" ) {
          values.append( e2.text() );
        }
      }
    }
  }
  
  if ( key.isEmpty() ) {
    key = name;
  }

  if ( name.isEmpty() ) {
    name = key;
    name.replace( " ", "" );
  }

  if ( label.isEmpty() ) {
    label = key;
  }
  
  if ( type.isEmpty() ) type = "QString";
  if ( name.isEmpty() || key.isEmpty() ) return 0;
  
  if ( ( type == "QString" || type == "QStringList" ) &&
       !defaultValue.isEmpty() ) {
    if ( defaultValue.left( 1 ) != "\"" ) defaultValue.prepend( "\"" );
    if ( defaultValue.right( 1 ) != "\"" ) defaultValue.append( "\"" );     
  } else if ( type == "QColor" ) {
    if ( !defaultValue.startsWith( "QColor" ) ) {
      defaultValue = "QColor( " + defaultValue + " )";
    }
  }
  
  return new CfgEntry( group, type, key, name, label, defaultValue,values );
}

/**
  Return parameter declaration for given type.
*/
QString param( const QString &type )
{
  if ( type == "QString" ) return "const QString &";
  else if ( type == "QStringList" ) return "const QStringList &";
  else if ( type == "QColor" ) return "const QColor &";
  else if ( type == "QFont" ) return "const QFont &";
  else return type;
}

QString addFunction( const QString &type )
{
  QString f = "addItem";

  QString t;
  
  if ( type == "QString" || type == "QStringList" || type == "QColor" ||
       type == "QFont" ) {
    t = type.mid( 1 );
  } else {
    t = type;
    t.replace( 0, 1, t.left( 1 ).upper() );
  }

  f += t;
  
  return f;
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "cfgc", I18N_NOOP("Config Compiler"), "0.1" );
  aboutData.addAuthor( "Cornelius Schumacher", 0, "schumacher@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app( false, false );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if ( args->count() != 1 ) {
    kdError() << "Too few arguments." << endl;
    return 1;
  }

  QString inputFilename = QString::fromLocal8Bit( args->arg( 0 ) );
  
  kdDebug() << "Input file: " << inputFilename << endl;

  QFile input( inputFilename );

  QDomDocument doc;
  if ( !doc.setContent( &input ) ) {
    kdError() << "Unable to load document." << endl;
    return 1;
  }

  QDomElement cfgElement = doc.documentElement();

  if ( cfgElement.isNull() ) {
    kdError() << "No document in cfg file" << endl;
    return 1;
  }

  QString cfgFileName;
  QString className;
  QString fileName;
  QString inherits;
  bool singleton = false;
  QString memberVariables;
  QPtrList<CfgEntry> entries;
  entries.setAutoDelete( true );
  QStringList includes;

  QDomNode n;
  for ( n = cfgElement.firstChild(); !n.isNull(); n = n.nextSibling() ) {
    QDomElement e = n.toElement();
    
    QString tag = e.tagName();
    if ( tag == "cfgfile" ) {
      cfgFileName = e.attribute( "name" );
    } else if ( tag == "class" ) {
      className = e.attribute( "name" );
      fileName = e.attribute( "file" );
      inherits = e.attribute( "inherits" );      
      singleton = e.attribute( "singleton" ) == "true";
      memberVariables = e.attribute( "membervariables" );
      QDomNode n2;
      for( n2 = e.firstChild(); !n2.isNull(); n2 = n2.nextSibling() ) {
        QDomElement e2 = n2.toElement();
        if ( e2.tagName() == "include" ) {
          includes.append( e2.attribute( "file" ) );
        }
      }
    } else if ( tag == "group" ) {
      QString group = e.attribute( "name" );
      if ( group.isEmpty() ) {
        kdError() << "Group without name" << endl;
        return 1;
      }
      QDomNode n2;
      for( n2 = e.firstChild(); !n2.isNull(); n2 = n2.nextSibling() ) {
        QDomElement e2 = n2.toElement();
        CfgEntry *entry = parseEntry( group, e2 );
        if ( entry ) entries.append( entry );
        else {
          kdError() << "Can't parse entry." << endl;
          return 1;
        }
      }
    }
  }

  if ( inherits.isEmpty() ) inherits = "KPrefs";
  
  if ( className.isEmpty() ) {
    kdError() << "Class name missing" << endl;
    return 1;
  }

  if ( entries.isEmpty() ) {
    kdWarning() << "No entries." << endl;
  }

#if 0
  CfgEntry *cfg;
  for( cfg = entries.first(); cfg; cfg = entries.next() ) {
    cfg->dump();
  }
#endif

  QString headerFileName = fileName + ".h";
  QString implementationFileName = fileName + ".cpp";

  QFile header( headerFileName );
  if ( !header.open( IO_WriteOnly ) ) {
    kdError() << "Can't open '" << headerFileName << "for writing." << endl;
    return 1;
  }

  QTextStream h( &header );
  

  h << "// This file is generated by cfgc." << endl;
  h << "// All changes you do to this file will be lost." << endl;

  h << "#ifndef " << className.upper() << "_H" << endl;
  h << "#define " << className.upper() << "_H" << endl << endl;

  // Includes
  QStringList::ConstIterator it;
  for( it = includes.begin(); it != includes.end(); ++it ) {
    h << "#include <" << *it << ">" << endl;
  }

  if ( includes.count() > 0 ) h << endl;

  h << "#include <kprefs.h>" << endl << endl;

  // Class declaration header
  h << "class " << className << " : public " << inherits << endl;
  h << "{" << endl;
  h << "  public:" << endl;

  // enums
  CfgEntry *e;
  for( e = entries.first(); e; e = entries.next() ) {
    QStringList values = e->values();
    if ( !values.isEmpty() ) {
      h << "    enum { " << values.join( ", " ) << " };" << endl;
    }
  }

  h << endl;
  
  // Constructor or singleton accessor
  if ( !singleton ) h << "    " << className << "();" << endl;
  else h << "    static " << className << " *self();" << endl;

  // Destructor
  h << "    ~" << className << "();" << endl << endl;

  for( e = entries.first(); e; e = entries.next() ) {
    QString n = e->name();
    QString t = e->type();

    // Manipulator
    h << "    /**" << endl;
    h << "      Set " << e->label() << endl;
    h << "    */" << endl;    
    h << "    void set" << n << "( " << param( t ) << " v )" << endl;
    h << "    {" << endl;
    h << "      m" << n << " = v;" << endl;
    h << "    }" << endl << endl;
  
    // Accessor
    QString accessor = n;
    accessor.replace( 0, 1, n.left( 1 ).lower() );
    h << "    /**" << endl;
    h << "      Get " << e->label() << endl;
    h << "    */" << endl;
    h << "    " << t << " " << accessor << "() const " << endl;
    h << "    {" << endl;
    h << "      return m" << n << ";" << endl;
    h << "    }" << endl;

    h << endl;
  }

  h << "  private:" << endl;
  
  // Private constructor for singleton
  if ( singleton ) {
    h << "    " << className << "();" << endl;
    h << "    static " << className << " *mSelf;" << endl << endl;
  }

  // Member variables

  if ( !memberVariables.isEmpty() && memberVariables != "private" ) {
    h << "  " << memberVariables << ":" << endl;
  }

  QString group;
  for( e = entries.first(); e; e = entries.next() ) {
    if ( e->group() != group ) {
      group = e->group();
      h << endl;
      h << "    // " << group << endl;
    }
    h << "    " << e->type() << " m" << e->name() << ";" << endl;
  }

  h << "};" << endl << endl;

  h << "#endif" << endl;


  header.close();

  QFile implementation( implementationFileName );
  if ( !implementation.open( IO_WriteOnly ) ) {
    kdError() << "Can't open '" << implementationFileName << "for writing."
              << endl;
    return 1;
  }

  QTextStream cpp( &implementation );


  cpp << "// This file is generated by cfgc." << endl;
  cpp << "// All changes you do to this file will be lost." << endl << endl;

  cpp << "#include \"" << headerFileName << "\"" << endl << endl;

  // Static class pointer for singleton
  if ( singleton ) {
    cpp << "#include <kstaticdeleter.h>" << endl;

    cpp << endl;

    cpp << className << " *" << className << "::mSelf = 0;" << endl;
    cpp << "static KStaticDeleter<" << className << "> staticDeleter;" << endl << endl;

    cpp << className << " *" << className << "::self()" << endl;
    cpp << "{" << endl;
    cpp << "  if ( !mSelf ) {" << endl;
    cpp << "    mSelf = staticDeleter.setObject( new " << className << "() );" << endl;
    cpp << "    mSelf->readConfig();" << endl;
    cpp << "  }" << endl << endl;
    cpp << "  return mSelf;" << endl;
    cpp << "}" << endl << endl;
  }

  // Constructor
  cpp << className << "::" << className << "()" << endl;
  cpp << "  : " << inherits << "(";
  if ( !cfgFileName.isEmpty() ) cpp << " \"" << cfgFileName << "\" ";
  cpp << ")" << endl;
  cpp << "{" << endl;

  group = QString::null;
  for( e = entries.first(); e; e = entries.next() ) {
    if ( e->group() != group ) {
      if ( !group.isEmpty() ) cpp << endl;
      group = e->group();
      cpp << "  setCurrentGroup( \"" << group << "\" );" << endl << endl;
    }
    
    cpp << "  " << addFunction( e->type() ) << "( \"" << e->key() << "\", m"
        << e->name();
    if ( !e->defaultValue().isEmpty() ) cpp << ", " << e->defaultValue();
    cpp << " );" << endl;
  }

  cpp << "}" << endl << endl;

  // Destructor
  cpp << className << "::~" << className << "()" << endl;
  cpp << "{" << endl;
  if ( singleton ) {
    cpp << "  if ( mSelf == this )" << endl;
    cpp << "    mSelf = staticDeleter.setObject( 0 );" << endl;
  }
  cpp << "}" << endl << endl;

  implementation.close();
}
