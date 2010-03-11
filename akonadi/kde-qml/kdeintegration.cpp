/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "kdeintegration.h"

#include <KDebug>
#include <KLocale>

static QString translate( const KLocalizedString &_string, const QVariantList &args = QVariantList() )
{
  KLocalizedString string( _string );
  foreach ( const QVariant &arg, args ) {
    switch ( arg.type() ) {
      case QVariant::Char:
        string = string.subs( arg.toChar() );
        break;
      case QVariant::Int:
        string = string.subs( arg.toInt() );
        break;
      case QVariant::UInt:
        string = string.subs( arg.toUInt() );
        break;
      case QVariant::LongLong:
        string = string.subs( arg.toLongLong() );
        break;
      case QVariant::ULongLong:
        string = string.subs( arg.toULongLong() );
        break;
      case QVariant::Double:
        string = string.subs( arg.toDouble() );
        break;
      case QVariant::String:
        string = string.subs( arg.toString() );
        break;
      default:
        kWarning() << "Unknown i18n argument type:" << arg;
    }
  }
  return string.toString();
}

KDEIntegration::KDEIntegration( QObject *parent ) : QObject( parent )
{
}

QString KDEIntegration::i18n( const QString &message )
{
  return translate( ki18n( message.toLatin1().constData() ) );
}

QString KDEIntegration::i18na( const QString &message, const QVariantList &args )
{
  return translate( ki18n( message.toLatin1().constData() ), args );
}

QString KDEIntegration::i18nc( const QString &context, const QString &message )
{
  return translate( ki18nc( context.toLatin1().constData(), message.toLatin1().constData() ) );
}

QString KDEIntegration::i18nca( const QString &context, const QString &message, const QVariantList &args )
{
  return translate( ki18nc( context.toLatin1().constData(), message.toLatin1().constData() ), args );
}

QString KDEIntegration::i18np( const QString &singular, const QString &plural, int amount )
{
  return translate( ki18np( singular.toLatin1().constData(), plural.toLatin1().constData() ), QVariantList() << QVariant( amount ) );
}

QString KDEIntegration::i18npa( const QString &singular, const QString &plural, int amount, const QVariantList &args )
{
  QVariantList _args;
  _args.append( QVariant( amount ) );
  _args.append( args );
  return translate( ki18np( singular.toLatin1().constData(), plural.toLatin1().constData() ), _args );
}

QString KDEIntegration::i18ncp( const QString &context, const QString &singular, const QString &plural, int amount )
{
  return translate( ki18ncp( context.toLatin1().constData(), singular.toLatin1().constData(), plural.toLatin1().constData() ), QVariantList() << QVariant( amount ) );
}

QString KDEIntegration::i18ncpa( const QString &context, const QString &singular, const QString &plural, int amount, const QVariantList &args )
{
  QVariantList _args;
  _args.append( QVariant( amount ) );
  _args.append( args );
  return translate( ki18ncp( context.toLatin1().constData(), singular.toLatin1().constData(), plural.toLatin1().constData() ), _args );
}

QString KDEIntegration::iconPath( const QString &iconName, int iconSize )
{
  return KIconLoader::global()->iconPath( iconName, -iconSize ); // yes, the minus there is correct...
}

#include "kdeintegration.moc"

