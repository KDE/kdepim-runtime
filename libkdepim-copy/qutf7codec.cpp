/*
  qutf7codec.cpp

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

#ifndef QT_NO_TEXTCODEC

int QUtf7Codec::mibEnum() const {
  return 1012;
}

int QStrictUtf7Codec::mibEnum() const {
  return -1012;
}

const char* QUtf7Codec::name() const {
  return "UTF-7";
}

const char* QStrictUtf7Codec::name() const {
  return "X-QT-UTF-7-STRICT";
}

const char* QUtf7Codec::mimeName() const {
  return "UTF-7";
}

bool QUtf7Codec::canEncode( QChar ) const {
  return TRUE;
}

bool QUtf7Codec::canEncode( const QString & ) const {
  return TRUE;
}

static uchar base64Set[] = {
  0x00, 0x00, 0x00, 0x00, // '\0' ...
  0x00, 0x11, 0xFF, 0xC0, // ' ' ... '?'
  0x7F, 0xFF, 0xFF, 0xE0, // '@' ... '_'
  0x7F, 0xFF, 0xFF, 0xE0  // '`' ... DEL
};

static uchar base64SetWithLastTwoBitsZero[] = {
  0x00, 0x00, 0x00, 0x00, // '\0' ...
  0x00, 0x00, 0x88, 0x80, // ' ' ... '?'
  0x44, 0x44, 0x44, 0x40, // '@' ... '_'
  0x11, 0x11, 0x11, 0x00  // '`' ... DEL
};

static uchar directSet[] = {
  0x00, 0x00, 0x00, 0x00, // '\0' ...
  0x01, 0xCF, 0xFF, 0xE1, // ' ' ... '?'
  0x7F, 0xFF, 0xFF, 0xE0, // '@' ... '_'
  0x7F, 0xFF, 0xFF, 0xE0  // '`' ... DEL
};

static uchar optDirectSet[] = {
  0x00, 0x00, 0x00, 0x00, // '\0' ...
  0x7E, 0x20, 0x00, 0x1E, // ' ' ... '?'
  0x80, 0x00, 0x00, 0x17, // '@' ... '_'
  0x80, 0x00, 0x00, 0x1C  // '`' ... DEL
};

static inline bool isOfSet(uchar ch, uchar* set) {
  return set[ ch/8 ] & (0x80 >> ( ch%8 ));
}

int QUtf7Codec::heuristicContentMatch(const char* chars, int len) const
{
  int stepNo = 0;
  int i;
  bool shifted = FALSE;
  bool rightAfterEscape = FALSE;
  bool onlyNullBitsSinceLastBoundary = TRUE;
  for ( i = 0; i < len ; i++ ) {
    if ((unsigned char)chars[i] >= 128) // 8bit chars not allowed.
      break;
    if (shifted) {
      if ( isOfSet(chars[i],base64Set) ) {
	switch (stepNo) {
	case 0:
	  onlyNullBitsSinceLastBoundary = TRUE;
	  break;
	case 3:
	  onlyNullBitsSinceLastBoundary
	    = isOfSet(chars[i],base64SetWithLastTwoBitsZero);
	  break;
	case 6:
	  onlyNullBitsSinceLastBoundary
	    = ( chars[i] == 'A' || chars[i] == 'Q' ||
		chars[i] == 'g' || chars[i] == 'w' );
	  break;
	default:
	   onlyNullBitsSinceLastBoundary
	     = onlyNullBitsSinceLastBoundary && (chars[i] == 'A');
	}
	stepNo = (stepNo + 1) % 8;
	rightAfterEscape = FALSE;
      } else {
	if (rightAfterEscape && chars[i] != '-')
	  break; // a '+' must be followed by '-' or a base64 char
	if (!onlyNullBitsSinceLastBoundary)
	  break; // non-zero bits in the tail of the base64 encoding
	shifted = FALSE;
	stepNo = 0;
      }
    } else {
      if (chars[i] == '+') {
	shifted = TRUE;
	rightAfterEscape = TRUE;
      }
    }
  }
  return i;
}

class QUtf7Decoder : public QTextDecoder {
  // the storage for our unicode char until it's finished
  ushort uc;
  // the state of the base64 decoding
  // can be 0 (just finished three unicode chars)
  //        1 (have the upper  6 bits of uc already)
  //        2 (have the upper 12 bits of uc already)
  //        3 (have the upper  2 bits of uc already)
  // ..........
  //        7 (have the upper 10 bits of uc already)
  //   =>   n (have the upper (n * 6) % 16 bits of uc already)
  // "stepNo" cycles through all it's values every three
  // unicode chars.
  char stepNo;
  // remembers if we are in shifted-sequence mode
  bool shifted;
  // remembers if we're just after the initial '+'
  // of a shifted-sequence.
  bool rightAfterEscape;
public:
  QUtf7Decoder() : uc(0), stepNo(0), shifted(FALSE), rightAfterEscape(FALSE)
  {
  }

private:
  inline void resetParser()
  {
    uc = 0;
    stepNo = 0;
    shifted = FALSE;
    rightAfterEscape = FALSE;
  }

public:
  QString toUnicode(const char* chars, int len)
  {
    QString result = "";
    for (int i=0; i<len; i++) {
      uchar ch = chars[i];

      //
      // check for 8bit char's:
      // 
      if ( ch > 127 ) {
	qWarning("QUtf7Decoder: 8bit char found in input. "
		 "Parser has been re-initialized!");
	resetParser();
	result += QChar::replacement;
	continue;
      }

      if (shifted) { // in shifted mode

	//
	// first, we check specialities that only occur
	// right after the escaping '+':
	//
	if ( rightAfterEscape && ch == '-' ) {
	  // a "+-" sequence is a short-circuit encoding
	  // for just '+':
	  resetParser();
	  result += QChar('+');
	  // we're already done for this "ch", so
	  continue;
	}

	//
	// Here we're going to extract the bits represented by "ch":
	//
	ushort bits;
	if ( ch >= 'A' && ch <= 'Z' ) {
	  bits = ch - 'A';
	} else if ( ch >= 'a' && ch <= 'z' ) {
	  bits = ch - 'a' + 26;
	} else if ( ch >= '0' && ch <= '9' ) {
	  bits = ch - '0' + 52;
	} else if ( ch == '+' ) {
	  bits = 62;
	} else if ( ch == '/' ) {
	  bits = 63;
	} else {
	  bits = 0; // keep compiler happy

	  //
	  // ch is not of the base64 alphabet.
	  // Here we are going to check the sequence's validity:
	  //
	  if ( rightAfterEscape ) {
	    // any non-base64 char following an escaping '+'
	    // makes for an ill-formed sequence.
	    // Note that we catch (the valid) "+-" pair
	    // right at the beginning.
	    qWarning("QUtf7Decoder: ill-formed input: "
		     "non-base64 char after escaping \"+\"!");
	  }
	  // pending bits from base64 encoding must be all 0:
	  if (stepNo >= 1 && uc) {
	    qWarning("QUtf7Decoder: ill-formed sequence: "
		     "non-zero bits in shifted-sequence tail!");
	  }
	  resetParser();

	  // a '-' signifies the end of the shifted-sequence,
	  // so we just swallow it.
	  if ( ch == '-' )
	    continue;
	  // end of validity checking. Process ch now...
	}

	if ( /*still*/ shifted ) {
	  //
	  // now we're going to stuff the "bits" bit bucket into
	  // the right position inside "uc", emitting a resulting
	  // QChar if possible.
	  //
	  switch (stepNo) {
	    // "bits" are the 6 msb's of uc
	  case 0: uc = bits << 10; break;

	  case 1: uc |= bits << 4; break;

	    // 4 bits of "bits" complete the first ushort
	  case 2: uc |= bits >> 2; result += QChar(uc);
	    // 2 bits of "bits" make the msb's of the next ushort
	          uc = bits << 14; break;
	  case 3: uc |= bits << 8; break;
	  case 4: uc |= bits << 2; break;

	    // 2 bits of "bits" complete the second ushort
	  case 5: uc |= bits >> 4; result += QChar(uc);
	    // 4 bits of "bits" make the msb's of the next ushort
	          uc = bits << 12; break;
	  case 6: uc |= bits << 6; break;

	    // these 6 bits complete the third ushort
	    // and also one round of 8 chars -> 3 ushort decoding
	  case 7: uc |= bits;      result += QChar(uc);
	          uc = 0;          break;
	  default: ;
	  } // switch (stepNo)
	  // increase the step counter
	  stepNo++;
	  stepNo %= 8;
	  rightAfterEscape = FALSE;
	  // and look at the next char.
	  continue;
	} // fi (still) shifted
      } // fi shifted

      //
      // if control reaches here, we either weren't in a
      // shifted sequence or we just left one by seeing
      // a non-base64-char.
      // Either way, we have to process "ch" outside
      // a shifted-sequence now:
      //
      if ( ch == '+' ) {
	// '+' is the escape char for entering a
	// shifted sequence:
	shifted = TRUE;
	stepNo = 0;
	// also, we're right at the beginning where
	// special rules apply:
	rightAfterEscape = TRUE;
      } else {
	// US-ASCII values are directly used
	result += QChar(ch);
      }
    }

    return result;

  } // toUnicode()

}; // class QUtf7Decoder

QTextDecoder* QUtf7Codec::makeDecoder() const
{
  return new QUtf7Decoder;
}


class QUtf7Encoder : public QTextEncoder {
  uchar dontNeedEncodingSet[16];
  ushort outbits;
  uint stepNo : 2;
  bool shifted : 1;
  bool mayContinueShiftedSequence : 1;
public:
  QUtf7Encoder(bool encOpt, bool encLwsp)
    : outbits(0), stepNo(0),
      shifted(FALSE), mayContinueShiftedSequence(FALSE)
  {
    for ( int i = 0; i < 16 ; i++) {
      dontNeedEncodingSet[i] = directSet[i];
      if (!encOpt)
	dontNeedEncodingSet[i] |= optDirectSet[i];
    }
    if(!encLwsp) {
      dontNeedEncodingSet[' '/8] |= 0x80 >> (' '%8);
      dontNeedEncodingSet['\n'/8] |= 0x80 >> ('\n'%8);
      dontNeedEncodingSet['\r'/8] |= 0x80 >> ('\r'%8);
      dontNeedEncodingSet['\t'/8] |= 0x80 >> ('\t'%8);
    }
  }

private:

  char toBase64( ushort u ) {
    if ( u < 26 )
      return (char)u + 'A';
    else if ( u < 52 )
      return (char)u - 26 + 'a';
    else if ( u < 62 )
      return (char)u - 52 + '0';
    else if ( u == 62 )
      return '+';
    else
      return '/';
  }

  void addToShiftedSequence(QCString::Iterator & t, ushort u) {
    switch (stepNo) {
      // no outbits; use uppermost 6 bits of u
    case 0:
      *t++ = toBase64( u >> 10 );
      *t++ = toBase64( (u & 0x03FF /* umask top 6 bits */ ) >> 4 );
      // save 4 lowest-order bits in outbits[5..2]
      outbits = (u & 0x000F) << 2;
      break;

      // outbits available; use top two bits of u to complete
      // the previous char
    case 1:
      if (!mayContinueShiftedSequence) {
	// if mayContinue, this char has already been written
	*t++ = toBase64( outbits | ( u >> 14 ) );
      }
      *t++ = toBase64( (u & 0x3F00 /* mask top 2 bits */ ) >> 8 );
      *t++ = toBase64( (u & 0x00FC /* mask msbyte */ ) >> 2 );
      // save 2 lowest-significant bits in outbits[5..4]
      outbits = (u & 0x0003) << 4;
      break;

      // outbits available; use top four bits of u to complete
      // the previous char
    case 2:
      if (!mayContinueShiftedSequence) {
	// if mayContinue, this char has already been written
	*t++ = toBase64( outbits | ( u >> 12 ) );
      }
      *t++ = toBase64( (u & 0x0FFF) >> 6 );
      *t++ = toBase64( u & 0x003F );
      break;

    default: ;
    }
    stepNo = (stepNo + 1) % 3;
  }

  void endShiftedSequence(QCString::Iterator & t) {
    switch (stepNo) {
    case 1: // four outbits still to be written
    case 2: // two outbits still to be written
      *t++ = toBase64( outbits );
      break;
    case 0:      // nothing to do
    default: ;
    }
    outbits = 0;
  }

  // depending on the stepNo, checks whether we can continue
  // an already ended shifted-sequence with char "u".
  // This is only possible if the topmost bits fit the
  // already written ones (which are all 0 between calls)
  bool continueOK( ushort u ) {
    return stepNo == 0 ||
      ( stepNo == 1 && (u & 0xF000) == 0 ) ||
      ( stepNo == 2 && (u & 0xC000) == 0 );
  }

  void processDoesntNeedEncoding(QCString::Iterator & t, ushort ch) {
    // doesn't need encoding
    if (shifted) {
      endShiftedSequence(t);
      // add "lead-out" to dis-ambiguate following chars:
      if (isOfSet((char)ch,base64Set) || ch == '-' ) {
	*t++ = '-';
      }
    } else if (mayContinueShiftedSequence) {
      // if mayContinue is set, this means the
      // shifted-sequence needs a lead-out.
      mayContinueShiftedSequence = FALSE;
      if (isOfSet(ch,base64Set) || ch == '-' ) {
	*t++ = '-';
      }
    }
    *t++ = (uchar)ch;
    shifted = FALSE;
    stepNo = 0;
  }

public:
  QCString fromUnicode(const QString & uc, int & len_in_out)
  {
    // allocate place for worst case:
    //   len/2 * (5+1) for an alternating sequence of e.g. "A\",
    // + 4             for a worst-case of another +ABC encoded char
    // + 1             for the trailing \0
    // 
    int maxreslen = 3 * len_in_out + 5;
    QCString result( maxreslen );

#if 0
    //    if (len_in_out == 1) {
    cout << "\nlen_in_out: " << len_in_out
	 <<"; shifted: " << (shifted ? "true" : "false")
	 << ";\n" << "mayContinue: "
	 << (mayContinueShiftedSequence ? "true" : "false")
	 << "; stepNo: " << stepNo << ";\n"
	 << "outbits: " << outbits << endl;
      //    }
#endif

    // source and destination cursor
    const QChar * s = uc.unicode();
    QCString::Iterator t = result.data();

    if ( uc.isNull() ) {
      // return to ascii requested:
      if ( mayContinueShiftedSequence )
	*t++ = '-';
    } else {
      // normal operation:
      for (int i = 0 ; i < len_in_out ;
	   i++/*, checkOutBuf(result,maxreslen,t,i,len_in_out,5)*/ ) {
	ushort ch = s[i].unicode();
	
	//
	// first, we check whether we might get around encoding:
	//
	if ( ch < 128 ) {
	  //
	  // ch is usAscii, so we have a chance that we don't
	  // need to encode it.
	  //
	  if ( isOfSet((uchar)ch,dontNeedEncodingSet) ) {
	    processDoesntNeedEncoding(t,ch);
	    continue;
	  } else if ( ch == '+' ) {
	    // '+' is the shift escape character
	    if (shifted || mayContinueShiftedSequence) {
	      // if we are already in shifted mode, we just
	      // encode the '+', too. Compare
	      // 24bits ("-+-") + some from ending the shifted-sequence
	      // with 21,33 bits
	      addToShiftedSequence(t,ch);
	      mayContinueShiftedSequence = FALSE;
	      shifted = TRUE;
	    } else {
	      // shortcut encoding of '+':
	      *t++ = '+';
	      *t++ = '-';
	    }
	    continue; // done
	  } // else fall through to encoding
	}
	//
	// need encoding
	//
	if (!shifted && (!mayContinueShiftedSequence || !continueOK(ch) ) ) {
	  *t++ = '+';
	  stepNo = 0;
	}
	addToShiftedSequence(t,ch);
	shifted = TRUE;
	mayContinueShiftedSequence = FALSE;
      }

      if ( shifted ) {
	endShiftedSequence(t);
	mayContinueShiftedSequence = TRUE;
      };
      shifted = FALSE;
    }

    *t = '\0';
    len_in_out = t - result.data();

#if 0
    cout << "len_in_out: " << len_in_out << "; "
	 << "mayContinue: " << (mayContinueShiftedSequence ? "true" : "false")
	 << "; stepNo: " << stepNo << endl;
#endif

    Q_ASSERT(len_in_out <= maxreslen-1);

    return result;
  } // fromUnicode()

}; // class QUtf7Encoder

QTextEncoder* QUtf7Codec::makeEncoder() const {
  return new QUtf7Encoder( false, false );
}

QTextEncoder* QStrictUtf7Codec::makeEncoder() const {
  return new QUtf7Encoder( true, false );
}

#endif // QT_NO_TEXTCODEC
