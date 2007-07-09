/*
  qutf7codec.h

  A QTextCodec for UTF-7 (rfc2152).
  Copyright (c) 2001 Marc Mutz <mutz@kde.org>
  See file COPYING for details

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, US

  As a special exception, permission is granted to use this plugin
  with any version of Qt by TrollTech AS, Norway. In this case, the
  use of this plugin doesn't cause the resulting executable to be
  covered by the GNU General Public License.
  This exception does not however invalidate any other reasons why the
  executable file might be covered by the GNU General Public License.
*/

#ifndef QUTF7CODEC_H
#define QUTF7CODEC_H

#ifndef QT_H
#include "qtextcodec.h"
#endif

#include <kdepim_export.h>

#ifndef QT_NO_TEXTCODEC

/** @short A QTextCodec for the UTF-7 transformation of Unicode.

    This is a QTextCodec for the UTF-7 transformation of Unicode,
    described in RFC2152.

    Use it as you would use any other QTextCodec. Only if you use the
    encoder directly (via makeEncoder), you should bear in mind
    that if your application needs the encoder to return to ASCII mode
    (like it's the case for RFC2047 mail header encoded words), you
    have to tell the encoder by requesting the encoding of a @em null
    QString.

    @author Marc Mutz <mutz@kde.org> */

class KDEPIM_EXPORT QUtf7Codec : public QTextCodec {
    bool encOpt, encLwsp;
public:
    QUtf7Codec() : QTextCodec() {}

    int mibEnum() const;
    QByteArray name() const;
    const char* mimeName() const;

    QTextDecoder* makeDecoder() const;
    QTextEncoder* makeEncoder() const;

    bool canEncode( QChar ) const;
    bool canEncode( const QString& ) const;

#ifdef __GNUC__
#warning Implement convertFromUnicode
#endif
    QByteArray convertFromUnicode( const QChar * /*input*/, int /*number*/, ConverterState * /*state*/ ) const { return QByteArray(); }
#ifdef __GNUC__
#warning Implement convertToUnicode
#endif
    QString convertToUnicode( const char * /*chars*/, int /*len*/, ConverterState * /*state*/ ) const { return QString(); }

    int heuristicContentMatch( const char* chars, int len ) const;
};

/** This is a version of @ref QUtf7Codec, which should only be used in
    MIME transfer. It differs from @ref QUtf7Codec only in that the
    encoder escapes additional characters (the RFC2152 "optional
    direct set"), which might not be allowed in RFC822/RFC2047 header
    fields.

    You should only use this codec for @em encoding, since it's output
    is pure UTF-7 and can equally well be decoded by @ref QUtf7Codec's
    decoder.

    To distinguish between the two variants, this class has MIB enum
    -1012 (the nagative of UTF-7) and the somewhat awkward name
    "X-QT-UTF-7-STRICT". The MIME preferred charset name is still
    "UTF-7", though.

    @short A variant of @ref QUtf7Codec, which protectes certain
    characters in MIME transport
    @author Marc Mutz <mutz@kde.org> */
class KDEPIM_EXPORT QStrictUtf7Codec : public QUtf7Codec {
public:
  QStrictUtf7Codec() : QUtf7Codec() {}

  QByteArray name() const;
  int mibEnum() const;

  QTextEncoder* makeEncoder() const;
};

#endif // QT_NO_TEXTCODEC

#endif // QUTF7CODEC_H
