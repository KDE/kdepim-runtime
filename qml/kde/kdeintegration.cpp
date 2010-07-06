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
#include <KIconLoader>
#include <KStandardDirs>
#include <QIcon>
#include <QPixmap>

#include <QScriptValue>
#include <QScriptContext>
#include <QScriptEngine>

static QString translate( const KLocalizedString &msg,
                          const QScriptContext *context, const int start )
{
  KLocalizedString string( msg );
  const int numArgs = context->argumentCount();

  for (int i = start; i < numArgs; ++i) {
    QVariant arg = context->argument(i).toVariant();

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

QScriptContext *KDEIntegration::getContext( const QScriptValue &v )
{
  QScriptEngine *engine = v.engine();

  if (!engine) {
      return 0;
  }

  QScriptContext *context = engine->currentContext();
  if (!context) {
      return 0;
  }

  return context;
}

QString KDEIntegration::i18n( const QScriptValue &array )
{
  QScriptContext *context = getContext(array);

  if (!context) {
      kWarning() << "No context !";
      return "";
  }

  if (context->argumentCount() < 1) {
      // ## TODO: (new str): context->throwError(i18n("i18n() takes at least one argument"));
      return "";
  }

  KLocalizedString message = ki18n(context->argument(0).toString().toUtf8());
  return translate(message, context, 1);
}


QString KDEIntegration::i18nc( const QScriptValue &array )
{
  QScriptContext *context = getContext(array);

  if (!context) {
      kWarning() << "No context !";
      return "";
  }

  if (context->argumentCount() < 2) {
      kWarning() << "i18nc() takes at least two arguments";
      //### TODO (new str): context->throwError(i18n("i18nc() takes at least two arguments"));
      return "";
  }

  KLocalizedString message = ki18nc(context->argument(0).toString().toUtf8(),
                                    context->argument(1).toString().toUtf8());

  return translate(message, context, 2);
}

QString KDEIntegration::i18np( const QScriptValue &array )
{
  QScriptContext *context = getContext(array);

  if (!context) {
      kWarning() << "No context !";
      return "";
  }

  if (context->argumentCount() < 2) {
      kWarning() << "i18np() takes at least two arguments";
      //### TODO (new str): context->throwError(i18n("i18np() takes at least two arguments"));
      return "";
  }

  KLocalizedString message = ki18np(context->argument(0).toString().toUtf8(),
                                    context->argument(1).toString().toUtf8());

  return translate(message, context, 2);
}

QString KDEIntegration::i18ncp( const QScriptValue &array )
{
  QScriptContext *context = getContext(array);

  if (!context) {
      kWarning() << "No context !";
      return "";
  }

  if (context->argumentCount() < 3) {
      kWarning() << "i18ncp() takes at least three arguments";
      //### TODO (new str): context->throwError(i18n("i18ncp() takes at least three arguments"));
      return "";
  }

  KLocalizedString message = ki18ncp(context->argument(0).toString().toUtf8(),
                                     context->argument(1).toString().toUtf8(),
                                     context->argument(2).toString().toUtf8());

  return translate(message, context, 3);
}

QString KDEIntegration::iconPath( const QString &iconName, int iconSize )
{
  return KIconLoader::global()->iconPath( iconName, -iconSize ); // yes, the minus there is correct...
}

QPixmap KDEIntegration::iconToPixmap(const QIcon& icon, int size )
{
  return icon.pixmap( size );
}

QString KDEIntegration::locate(const QString& type, const QString& filename)
{
  return KStandardDirs::locate( type.toLatin1(), filename );
}

#include "kdeintegration.moc"

