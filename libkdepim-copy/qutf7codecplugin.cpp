/*
  qutf7codecplugin.cpp

  A QTextCodec for UTF-7 (rfc2152).
  Copyright (c) 2001 Marc Mutz <mutz@kde.org>
  See file COPYING for details

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, US

  As a special exception, permission is granted to use this plugin
  with any version of Qt by TrollTech AS, Norway. In this case, the
  use of this plugin doesn't cause the resulting executable to be
  covered by the GNU General Public License.
  This exception does not however invalidate any other reasons why the
  executable file might be covered by the GNU General Public License.
*/

#include "qutf7codec.h"

#include <qtextcodecplugin.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>

class QTextCodec;

// ######### This file isn't compiled currently

class QUtf7CodecPlugin : public QTextCodecPlugin {
public:
  QUtf7CodecPlugin() {}

  QStringList names() const { return QStringList() << "UTF-7" << "X-QT-UTF-7-STRICT"; }
  QValueList<int> mibEnums() const { return QValueList<int>() << 1012 << -1012; }
  QTextCodec * createForMib( int );
  QTextCodec * createForName( const QString & );
};

QTextCodec * QUtf7CodecPlugin::createForMib( int mib ) {
  if ( mib == 1012 )
    return new QUtf7Codec();
  else if ( mib == -1012 )
    return new QStrictUtf7Codec();
  return 0;
}

QTextCodec * QUtf7CodecPlugin::createForName( const QString & name ) {
  if ( name == "UTF-7" )
    return new QUtf7Codec();
  else if ( name == "X-QT-UTF-7-STRICT" )
    return new QStrictUtf7Codec();
  return 0;
}

KDE_Q_EXPORT_PLUGIN( QUtf7CodecPlugin );
