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
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>

#include <QScriptValue>
#include <QScriptContext>
#include <QScriptEngine>
#include <qplatformdefs.h>

#ifdef _WIN32
#include <Windows.h>
#endif

static QString translate( const KLocalizedString &msg,
                          const QScriptContext *context, const int start, bool plural = false )
{
  KLocalizedString string( msg );
  const int numArgs = context->argumentCount();

  for (int i = start; i < numArgs; ++i) {
    const QVariant arg = context->argument(i).toVariant();

    // numbers provided from javascript seem to arrive always as double, which does not work for plural handling
    if ( i == start && plural ) {
      string = string.subs( arg.toInt() );
      continue;
    }

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

  return translate(message, context, 2, true);
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

  return translate(message, context, 3, true);
}

QString KDEIntegration::iconPath( const QString &iconName, int iconSize )
{
  return KIconLoader::global()->iconPath( iconName, -iconSize ); // yes, the minus there is correct...
}

QPixmap KDEIntegration::iconToPixmap(const QIcon& icon, int size )
{
  if ( icon.isNull() )
    return QPixmap();
  return icon.pixmap( size );
}

QString KDEIntegration::locate(const QString& type, const QString& filename)
{
  return KStandardDirs::locate( type.toLatin1(), filename );
}

qreal KDEIntegration::mm2px(qreal mm)
{
#ifdef _WIN32
//TODO: Cache value
  HMONITOR mon = MonitorFromWindow(QApplication::desktop()->winId(), MONITOR_DEFAULTTONEAREST);
  MONITORINFOEX moninfo;
  moninfo.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(mon, &moninfo);

  long monitorWidthInPixel = moninfo.rcMonitor.right - moninfo.rcMonitor.left;
  long monitorHeightinPixel = moninfo.rcMonitor.bottom - moninfo.rcMonitor.top;

  DISPLAY_DEVICE dd;
  dd.cb = sizeof(DISPLAY_DEVICE);
  EnumDisplayDevices(moninfo.szDevice, 0, &dd, 0);

  const QString deviceID = QString::fromUtf16( ( const unsigned short *)dd.DeviceID);

  QRegExp rx("^MONITOR\\\\(\\w+)\\\\");

  if (rx.indexIn(deviceID) != -1) {
    const QString devID = rx.cap(1);

    const QString baseRegKey =
      QLatin1String("SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\") + devID;

    HKEY registryHandle;
    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            reinterpret_cast<const wchar_t *>(baseRegKey.utf16()),
                            0,
                            KEY_READ,
                            &registryHandle);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(registryHandle);
        return mm * QApplication::desktop()->logicalDpiX() / 25.4;
    }

    DWORD keyLength = 0;
    res = RegQueryInfoKey(registryHandle, 0, 0, 0, 0, &keyLength, 0, 0, 0, 0, 0, 0);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(registryHandle);
        return mm * QApplication::desktop()->logicalDpiX() / 25.4;
    }

    //Utf16 so 2 Bytes per Character
    QByteArray buf(keyLength * 2, 0);
    keyLength++; //keyLength including the terminating 0, which is already in a QByteArray

    res = RegEnumKeyEx(registryHandle,
                       0,
                       reinterpret_cast<wchar_t *>(buf.data()),
                       &keyLength,
                       0,
                       0,
                       0,
                       0);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(registryHandle);
        return mm * QApplication::desktop()->logicalDpiX() / 25.4;
    }
    RegCloseKey(registryHandle);

    const QString guidString =  QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buf.data()));

    const QString edidRegString = QLatin1String("SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\")
                        + devID + QLatin1String("\\") + guidString + QLatin1String("\\Device Parameters");
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, reinterpret_cast<const wchar_t *>(edidRegString.utf16()),
                 0, KEY_READ, &registryHandle);

    DWORD dataSize;
    res = RegQueryValueEx(registryHandle, L"EDID", 0, 0, 0, &dataSize);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(registryHandle);
        return mm * QApplication::desktop()->logicalDpiX() / 25.4;
    }

    QByteArray data(dataSize, 0);
    res = RegQueryValueEx(registryHandle, L"EDID", 0, 0,
                          reinterpret_cast<unsigned char*>(data.data()), &dataSize);
    RegCloseKey(registryHandle);

    const int monitorWidthInCM = data.at(21);
    //int monitorHeightInCM = data.at(22);

    return (monitorWidthInPixel * mm) / (monitorWidthInCM * 10.0);
    //int pixelPerMM = monitorHeightinPixel / (monitorHeightInCM * 10.0);
  }
#endif

#ifdef Q_WS_MAEMO_5
  // N900 (which is the only thing actually running Maemo5) reports 96 dpi while its screen actually has 267 dpi
  return mm * 267 / 25.4;
#elif defined(MEEGO_EDITION_HARMATTAN)
  // N9[50] (which is the only thing actually running Maemo6) reports 96 dpi as well while its screen actually has 251 dpi
  return mm * 251 / 25.4;
#endif
  return mm * QApplication::desktop()->logicalDpiX() / 25.4;
}

#include "kdeintegration.moc"

