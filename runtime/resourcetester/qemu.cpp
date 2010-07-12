/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "qemu.h"

#include "global.h"
#include "test.h"

#include <KConfig>
#include <KDebug>
#include <KConfigGroup>
#include <KShell>
#include <KIO/NetAccess>

#include <qtest_kde.h>

#include <QDir>
#include <QFile>
#include <QTcpSocket>

QEmu::QEmu(QObject* parent) :
  QObject( parent ),
  mVMConfig( 0 ),
  mVMProcess( 0 ),
  mPortOffset( 42000 ), // TODO should be somewhat dynamic to allo running multiple instances in parallel
  mMonitorPort( -1 ),
  mStarted( false )
{
  Q_ASSERT( parent );
}

QEmu::~QEmu()
{
  if ( mVMProcess && mVMProcess->state() == QProcess::Running )
    stop();
  delete mVMConfig;
}

void QEmu::setVMConfig(const QString& configFileName)
{
  delete mVMConfig;
  mVMConfig = new KConfig( Global::basePath() + '/' + configFileName );
}

void QEmu::start()
{
  Q_ASSERT( mVMConfig );
  if ( mVMProcess ) {
    kWarning() << "VM already running.";
    return;
  }

  KConfigGroup emuConf( mVMConfig, "Emulator" );
  QStringList args = KShell::splitArgs( emuConf.readEntry( "Arguments", QString() ) );
  const QList<int> ports = emuConf.readEntry( "Ports", QList<int>() );
  foreach ( int port, ports ) {
    args << "-redir" << QString::fromLatin1( "tcp:%1::%2" ).arg( port + mPortOffset ).arg( port );
  }
  mMonitorPort = emuConf.readEntry( "MonitorPort", 23 ) + mPortOffset;
  args << "-monitor" << QString::fromLatin1( "tcp:127.0.0.1:%1,server,nowait" ).arg( mMonitorPort );
  args << "-hda" << vmImage();

  // If a SnapshotName is given in the configuration, load that snapshot
  // with -loadvm and assume that it's OK to write to he image file.
  // Othewise, use the -snapshot option to avoid changing the image
  // file.
  QString snapshotName = emuConf.readEntry( "SnapshotName", "" );
  if ( snapshotName.isEmpty() ) {
    args << "-snapshot";
  } else {
    args << "-loadvm" << snapshotName;
  }

  kDebug() << "Starting QEMU with arguments" << args << "...";

  mVMProcess = new KProcess( this );
  connect( mVMProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(vmFinished(int,QProcess::ExitStatus)) );
  mVMProcess->setProgram( "qemu", args );
  mVMProcess->start();
  mVMProcess->waitForStarted();
  mStarted = true;
  kDebug() << "QEMU started.";

  if ( emuConf.readEntry( "WaitForPorts", true ) ) {
    const QList<int> waitPorts = emuConf.readEntry( "WaitForPorts", QList<int>() );
    foreach ( int port, waitPorts )
      waitForPort( port );
  }
}

void QEmu::stop()
{
  mStarted = false;
  if ( !mVMProcess )
    return;
  kDebug() << "Stopping QEMU...";

  // send stop command via QEMU monitor
  QTcpSocket socket;
  socket.connectToHost( "localhost", mMonitorPort );
  if ( socket.waitForConnected() ) {
    socket.write( "quit\n");
    socket.flush();
    socket.waitForBytesWritten();
    mVMProcess->waitForFinished(10000);
  } else {
    kDebug() << "Unable to connect to QEMU monitor:" << socket.errorString();
  }

  if ( mVMProcess->state() == QProcess::Running ) {
    kDebug() << "qemu is still running. terminating";
    mVMProcess->terminate();
  }

  delete mVMProcess;
  mVMProcess = 0;
  kDebug() << "QEMU stopped.";
}

QString QEmu::vmImage() const
{
  KConfigGroup conf( mVMConfig, "Image" );

  const KUrl imageUrl = conf.readEntry( "Source", QString() );
  Q_ASSERT( !imageUrl.isEmpty() );

  const QString imageArchiveFileName = imageUrl.fileName();
  Q_ASSERT( !imageArchiveFileName.isEmpty() );

  // check if the image has been downloaded already
  const QString localArchiveFileName = Global::vmPath() + imageArchiveFileName;
  if ( !QFile::exists( localArchiveFileName ) ) {
    kDebug() << "Downloading VM image from" << imageUrl << "to" << localArchiveFileName << "...";
    const bool result = KIO::NetAccess::file_copy( imageUrl, localArchiveFileName, 0 );
    if ( !result )
      kFatal() << "Downloading" << imageUrl << "failed!";
    kDebug() << "Downloading VM image complete.";
  }

  // check if the image archive has been extracted yet
  const QString extractedDirName = Global::vmPath() + imageArchiveFileName + ".directory";
  if ( !QDir::root().exists( extractedDirName ) ) {
    kDebug() << "Extracting VM image...";
    QDir::root().mkpath( extractedDirName );
    KProcess proc;
    proc.setWorkingDirectory( extractedDirName );
    proc.setProgram( "tar", QStringList() << "xvf" << localArchiveFileName );
    proc.execute();
    kDebug() << "Extracting VM image complete.";
  }

  // check if the actual image file is there and return it
  const QString imageFile = extractedDirName + QDir::separator() + conf.readEntry( "File", QString() );
  if ( !QFile::exists( imageFile ) )
    kFatal() << "Image file" << imageFile << "does not exist.";

  return imageFile;
}

void QEmu::waitForPort(int port)
{
  kDebug() << "Waiting for port" << (port + mPortOffset) << "...";
  forever {
    QTcpSocket socket;
    socket.connectToHost( "localhost", port + mPortOffset );
    if ( !socket.waitForConnected( 5000 ) ) {
      QTest::qWait( 5000 );
      continue;
    }
    if ( socket.waitForReadyRead( 5000 ) )
      break;
  }
}

int QEmu::portOffset() const
{
  return mPortOffset;
}

void QEmu::vmFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED( exitCode );
  Q_UNUSED( exitStatus );
  if ( mStarted )
    Test::instance()->fail( "QEMU termineated!" );
}


#include "qemu.moc"
