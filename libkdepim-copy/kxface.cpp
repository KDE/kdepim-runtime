/*
  This file is part of libkdepim.

  Original compface:
  Copyright (c) James Ashton - Sydney University - June 1990.

  Additions for KDE:
  Copyright (c) 2004 Jakob Schröter <js@camaya.net>

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

#include "kxface.h"

#include <kdebug.h>

#include <qbitmap.h>
#include <qbuffer.h>
#include <qcstring.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qregexp.h>
#include <qstring.h>
#include <qpainter.h>

#include <stdlib.h>
#include <string.h>

#define GEN(g) F[h] ^= G.g[k]; break

#define BITSPERDIG 4
#define DIGITS (PIXELS / BITSPERDIG)
#define DIGSPERWORD 4
#define WORDSPERLINE (WIDTH / DIGSPERWORD / BITSPERDIG)

/* compressed output uses the full range of printable characters.
 * in ascii these are in a contiguous block so we just need to know
 * the first and last.  The total number of printables is needed too */
#define FIRSTPRINT '!'
#define LASTPRINT '~'
#define NUMPRINTS (LASTPRINT - FIRSTPRINT + 1)

/* output line length for compressed data */
#define MAXLINELEN 78

/* Portable, very large unsigned integer arithmetic is needed.
 * Implementation uses arrays of WORDs.  COMPs must have at least
 * twice as many bits as WORDs to handle intermediate results */
#define COMP unsigned long
#define WORDCARRY (1 << BITSPERWORD)
#define WORDMASK (WORDCARRY - 1)

#define ERR_OK		0	/* successful completion */
#define ERR_EXCESS	1	/* completed OK but some input was ignored */
#define ERR_INSUFF	-1	/* insufficient input.  Bad face format? */
#define ERR_INTERNAL	-2	/* Arithmetic overflow or buffer overflow */

#define BLACK 0
#define GREY 1
#define WHITE 2

#define MAX_XFACE_LENGTH 2048

using namespace KPIM;

KXFace::KXFace()
{
  NumProbs = 0;
}

KXFace::~KXFace()
{
}

QString KXFace::fromImage( const QImage &image )
{
  if( image.isNull() )
    return QString::null;

  QImage scaledImg = image.smoothScale( 48, 48 );
  QByteArray ba;
  QBuffer buffer( ba );
  buffer.open( IO_WriteOnly );
  scaledImg.save( &buffer, "XBM" );
  QString xbm( ba );
  xbm.remove( 0, xbm.find( "{" ) + 1 );
  xbm.truncate( xbm.find( "}" ) );
  xbm.remove( " " );
  xbm.remove( "," );
  xbm.remove( "0x" );
  xbm.remove( "\n" );
  xbm.truncate( 576 );
  QCString tmp = QCString( xbm.latin1() );
  uint len = tmp.length();
  for( uint i=0; i<len; ++i )
  {
    switch( tmp[i] )
    {
      case '1': tmp[i] = '8'; break;
      case '2': tmp[i] = '4'; break;
      case '3': tmp[i] = 'c'; break;
      case '4': tmp[i] = '2'; break;
      case '5': tmp[i] = 'a'; break;
      case '7': tmp[i] = 'e'; break;
      case '8': tmp[i] = '1'; break;
      case 'A':
      case 'a': tmp[i] = '5'; break;
      case 'B':
      case 'b': tmp[i] = 'd'; break;
      case 'C':
      case 'c': tmp[i] = '3'; break;
      case 'D':
      case 'd': tmp[i] = 'b'; break;
      case 'E':
      case 'e': tmp[i] = '7'; break;
    }
    if ( i % 2 )
    {
      char t = tmp[i];
      tmp[i] = tmp[i-1];
      tmp[i-1] = t;
    }
  }
  tmp.replace( QRegExp( "(\\w{12})" ), "\\1\n" );
  tmp.replace( QRegExp( "(\\w{4})" ), "0x\\1," );
  len = tmp.length();
  char *fbuf = (char *)malloc( len + 1 );
  strncpy( fbuf, (const char *)tmp, len );
  fbuf[len] = '\0';
  if ( !( status = setjmp( comp_env ) ) )
  {
    ReadFace( fbuf );
    GenFace();
    CompAll( fbuf );
  }
  QString ret( fbuf );
  free( fbuf );

  return ret;
}

QBitmap KXFace::toBitmap(const QString &xface)
{
  if ( xface.length() > MAX_XFACE_LENGTH )
    return QBitmap();

  char *fbuf = (char *)malloc( MAX_XFACE_LENGTH );
  memset( fbuf, '\0', MAX_XFACE_LENGTH );
  strncpy( fbuf, xface.latin1(), xface.length() );
  QCString img;
  if ( !( status = setjmp( comp_env ) ) )
  {
    UnCompAll( fbuf );/* compress otherwise */
    UnGenFace();
    img = WriteFace();
  }
  free( fbuf );
  QBitmap b( 48, 48, true );
  b.loadFromData( img, "XBM" );
  b.setMask( b );
  return b;
}

//============================================================================
// more or less original compface 1.4 source

void KXFace::RevPush(const Prob *p)
{
  if (NumProbs >= PIXELS * 2 - 1)
    longjmp(comp_env, ERR_INTERNAL);
  ProbBuf[NumProbs++] = (Prob *) p;
}
 
void KXFace::BigPush(Prob *p)
{
  static unsigned char tmp;

  BigDiv(p->p_range, &tmp);
  BigMul(0);
  BigAdd(tmp + p->p_offset);
}

int KXFace::BigPop(register const Prob *p)
{
  static unsigned char tmp;
  register int i;

  BigDiv(0, &tmp);
  i = 0;
  while ((tmp < p->p_offset) || (tmp >= p->p_range + p->p_offset))
  {
    p++;
    i++;
  }
  BigMul(p->p_range);
  BigAdd(tmp - p->p_offset);
  return i;
}


/* Divide B by a storing the result in B and the remainder in the word
 * pointer to by r
 */
void KXFace::BigDiv(register unsigned char a, register unsigned char *r)
{
  register int i;
  register unsigned char *w;
  register COMP c, d;

  a &= WORDMASK;
  if ((a == 1) || (B.b_words == 0))
  {
    *r = 0;
    return;
  }
  if (a == 0)	/* treat this as a == WORDCARRY */
  {			/* and just shift everything right a WORD (unsigned char)*/
    i = --B.b_words;
    w = B.b_word;
    *r = *w;
    while (i--)
    {
      *w = *(w + 1);
      w++;
    }
    *w = 0;
    return;
  }
  w = B.b_word + (i = B.b_words);
  c = 0;
  while (i--)
  {
    c <<= BITSPERWORD;
    c += (COMP)*--w;
    d = c / (COMP)a;
    c = c % (COMP)a;
    *w = (unsigned char)(d & WORDMASK);
  }
  *r = c;
  if (B.b_word[B.b_words - 1] == 0)
    B.b_words--;
}

/* Multiply a by B storing the result in B
 */
void KXFace::BigMul(register unsigned char a)
{
  register int i;
  register unsigned char *w;
  register COMP c;

  a &= WORDMASK;
  if ((a == 1) || (B.b_words == 0))
    return;
  if (a == 0)	/* treat this as a == WORDCARRY */
  {			/* and just shift everything left a WORD (unsigned char) */
    if ((i = B.b_words++) >= MAXWORDS - 1)
      longjmp(comp_env, ERR_INTERNAL);
    w = B.b_word + i;
    while (i--)
    {
      *w = *(w - 1);
      w--;
    }
    *w = 0;
    return;
  }
  i = B.b_words;
  w = B.b_word;
  c = 0;
  while (i--)
  {
    c += (COMP)*w * (COMP)a;
    *(w++) = (unsigned char)(c & WORDMASK);
    c >>= BITSPERWORD;
  }
  if (c)
  {
    if (B.b_words++ >= MAXWORDS)
      longjmp(comp_env, ERR_INTERNAL);
    *w = (COMP)(c & WORDMASK);
  }
}

/* Add to a to B storing the result in B
 */
void KXFace::BigAdd(unsigned char a)
{
  register int i;
  register unsigned char *w;
  register COMP c;

  a &= WORDMASK;
  if (a == 0)
    return;
  i = 0;
  w = B.b_word;
  c = a;
  while ((i < B.b_words) && c)
  {
    c += (COMP)*w;
    *w++ = (unsigned char)(c & WORDMASK);
    c >>= BITSPERWORD;
    i++;
  }
  if ((i == B.b_words) && c)
  {
    if (B.b_words++ >= MAXWORDS)
      longjmp(comp_env, ERR_INTERNAL);
    *w = (COMP)(c & WORDMASK);
  }
}

void KXFace::BigClear()
{
  B.b_words = 0;
}

QCString KXFace::WriteFace()
{
  register char *s;
  register int i, j, bits, digits, words;
  int digsperword = DIGSPERWORD;
  int wordsperline = WORDSPERLINE;
  QCString t( "#define noname_width 48\n#define noname_height 48\nstatic char noname_bits[] = {\n " );
  j = t.length() - 1;

  s = F;
  bits = digits = words = i = 0;
  t.resize( MAX_XFACE_LENGTH );
  digsperword = 2;
  wordsperline = 15;
  while ( s < F + PIXELS )
  {
    if ( ( bits == 0 ) && ( digits == 0 ) )
    {
      t[j++] = '0';
      t[j++] = 'x';
    }
    if ( *(s++) )
      i = ( i >> 1 ) | 0x8;
    else
      i >>= 1;
    if ( ++bits == BITSPERDIG )
    {
      j++;
      t[j-( ( digits & 1 ) * 2 )] = *(i + HexDigits);
      bits = i = 0;
      if ( ++digits == digsperword )
      {
        if ( s >= F + PIXELS )
          break;
        t[j++] = ',';
        digits = 0;
        if ( ++words == wordsperline )
        {
          t[j++] = '\n';
          t[j++] = ' ';
          words = 0;
        }
      }
    }
  }
  t.resize( j + 1 );
  t += "};\n";
  return t;
}

void KXFace::UnCompAll(char *fbuf)
{
  register char *p;

  BigClear();
  BigRead(fbuf);
  p = F;
  while (p < F + PIXELS)
    *(p++) = 0;
  UnCompress(F, 16, 16, 0);
  UnCompress(F + 16, 16, 16, 0);
  UnCompress(F + 32, 16, 16, 0);
  UnCompress(F + WIDTH * 16, 16, 16, 0);
  UnCompress(F + WIDTH * 16 + 16, 16, 16, 0);
  UnCompress(F + WIDTH * 16 + 32, 16, 16, 0);
  UnCompress(F + WIDTH * 32, 16, 16, 0);
  UnCompress(F + WIDTH * 32 + 16, 16, 16, 0);
  UnCompress(F + WIDTH * 32 + 32, 16, 16, 0);
}

void KXFace::UnCompress(char *f, int wid, int hei, int lev)
{
  switch (BigPop(&levels[lev][0]))
  {
    case WHITE :
      return;
    case BLACK :
      PopGreys(f, wid, hei);
      return;
    default :
      wid /= 2;
      hei /= 2;
      lev++;
      UnCompress(f, wid, hei, lev);
      UnCompress(f + wid, wid, hei, lev);
      UnCompress(f + hei * WIDTH, wid, hei, lev);
      UnCompress(f + wid + hei * WIDTH, wid, hei, lev);
      return;
  }
}

void KXFace::BigWrite(register char *fbuf)
{
  static unsigned char tmp;
  static char buf[DIGITS];
  register char *s;
  register int i;

  s = buf;
  while (B.b_words > 0)
  {
    BigDiv(NUMPRINTS, &tmp);
    *(s++) = tmp + FIRSTPRINT;
  }
  i = 7;	// leave room for the field name on the first line
  *(fbuf++) = ' ';
  while (s-- > buf)
  {
    if (i == 0)
      *(fbuf++) = ' ';
    *(fbuf++) = *s;
    if (++i >= MAXLINELEN)
    {
      *(fbuf++) = '\n';
      i = 0;
    }
  }
  if (i > 0)
    *(fbuf++) = '\n';
  *(fbuf++) = '\0';
}

void KXFace::BigRead(register char *fbuf)
{
  register int c;

  while (*fbuf != '\0')
  {
    c = *(fbuf++);
    if ((c < FIRSTPRINT) || (c > LASTPRINT))
      continue;
    BigMul(NUMPRINTS);
    BigAdd((unsigned char)(c - FIRSTPRINT));
  }
}

void KXFace::ReadFace(char *fbuf)
{
  register int c, i;
  register char *s, *t;

  t = s = fbuf;
  for(i = strlen(s); i > 0; i--)
  {
    c = (int)*(s++);
    if ((c >= '0') && (c <= '9'))
    {
      if (t >= fbuf + DIGITS)
      {
        status = ERR_EXCESS;
        break;
      }
      *(t++) = c - '0';
    }
    else if ((c >= 'A') && (c <= 'F'))
    {
      if (t >= fbuf + DIGITS)
      {
        status = ERR_EXCESS;
        break;
      }
      *(t++) = c - 'A' + 10;
    }
    else if ((c >= 'a') && (c <= 'f'))
    {
      if (t >= fbuf + DIGITS)
      {
        status = ERR_EXCESS;
        break;
      }
      *(t++) = c - 'a' + 10;
    }
    else if (((c == 'x') || (c == 'X')) && (t > fbuf) && (*(t-1) == 0))
      t--;
  }
  if (t < fbuf + DIGITS)
    longjmp(comp_env, ERR_INSUFF);
  s = fbuf;
  t = F;
  c = 1 << (BITSPERDIG - 1);
  while (t < F + PIXELS)
  {
    *(t++) = (*s & c) ? 1 : 0;
    if ((c >>= 1) == 0)
    {
      s++;
      c = 1 << (BITSPERDIG - 1);
    }
  }
}

void KXFace::GenFace()
{
  static char newp[PIXELS];
  register char *f1;
  register char *f2;
  register int i;

  f1 = newp;
  f2 = F;
  i = PIXELS;
  while (i-- > 0)
    *(f1++) = *(f2++);
  Gen(newp);
}

void KXFace::UnGenFace()
{
  Gen(F);
}

// static
void KXFace::Gen(register char *f)
{
  register int m, l, k, j, i, h;

  for (j = 0; j < HEIGHT;  j++)
  {
    for (i = 0; i < WIDTH;  i++)
    {
      h = i + j * WIDTH;
      k = 0;
      for (l = i - 2; l <= i + 2; l++)
        for (m = j - 2; m <= j; m++)
      {
        if ((l >= i) && (m == j))
          continue;
        if ((l > 0) && (l <= WIDTH) && (m > 0))
          k = *(f + l + m * WIDTH) ? k * 2 + 1 : k * 2;
      }
      switch (i)
      {
        case 1 :
          switch (j)
          {
            case 1 : GEN(g_22);
            case 2 : GEN(g_21);
            default : GEN(g_20);
          }
          break;
        case 2 :
          switch (j)
          {
            case 1 : GEN(g_12);
            case 2 : GEN(g_11);
            default : GEN(g_10);
          }
          break;
        case WIDTH - 1 :
          switch (j)
          {
            case 1 : GEN(g_42);
            case 2 : GEN(g_41);
            default : GEN(g_40);
          }
          break;
        case WIDTH :
          switch (j)
          {
            case 1 : GEN(g_32);
            case 2 : GEN(g_31);
            default : GEN(g_30);
          }
          break;
        default :
          switch (j)
          {
            case 1 : GEN(g_02);
            case 2 : GEN(g_01);
            default : GEN(g_00);
          }
          break;
      }
    }
  }
}

void KXFace::PopGreys(char *f, int wid, int hei)
{
  if (wid > 3)
  {
    wid /= 2;
    hei /= 2;
    PopGreys(f, wid, hei);
    PopGreys(f + wid, wid, hei);
    PopGreys(f + WIDTH * hei, wid, hei);
    PopGreys(f + WIDTH * hei + wid, wid, hei);
  }
  else
  {
    wid = BigPop(freqs);
    if (wid & 1)
      *f = 1;
    if (wid & 2)
      *(f + 1) = 1;
    if (wid & 4)
      *(f + WIDTH) = 1;
    if (wid & 8)
      *(f + WIDTH + 1) = 1;
  }
}

void KXFace::CompAll(char *fbuf)
{
  Compress(F, 16, 16, 0);
  Compress(F + 16, 16, 16, 0);
  Compress(F + 32, 16, 16, 0);
  Compress(F + WIDTH * 16, 16, 16, 0);
  Compress(F + WIDTH * 16 + 16, 16, 16, 0);
  Compress(F + WIDTH * 16 + 32, 16, 16, 0);
  Compress(F + WIDTH * 32, 16, 16, 0);
  Compress(F + WIDTH * 32 + 16, 16, 16, 0);
  Compress(F + WIDTH * 32 + 32, 16, 16, 0);
  BigClear();
  while (NumProbs > 0)
    BigPush(ProbBuf[--NumProbs]);
  BigWrite(fbuf);
}

void KXFace::Compress(register char *f, register int wid, register int hei, register int lev)
{
  if (AllWhite(f, wid, hei))
  {
    RevPush(&levels[lev][WHITE]);
    return;
  }
  if (AllBlack(f, wid, hei))
  {
    RevPush(&levels[lev][BLACK]);
    PushGreys(f, wid, hei);
    return;
  }
  RevPush(&levels[lev][GREY]);
  wid /= 2;
  hei /= 2;
  lev++;
  Compress(f, wid, hei, lev);
  Compress(f + wid, wid, hei, lev);
  Compress(f + hei * WIDTH, wid, hei, lev);
  Compress(f + wid + hei * WIDTH, wid, hei, lev);
}

int KXFace::AllWhite(char *f, int wid, int hei)
{
  return ((*f == 0) && Same(f, wid, hei));
}

int KXFace::AllBlack(char *f, int wid, int hei)
{
  if (wid > 3)
  {
    wid /= 2;
    hei /= 2;
    return (AllBlack(f, wid, hei) && AllBlack(f + wid, wid, hei) &&
        AllBlack(f + WIDTH * hei, wid, hei) &&
        AllBlack(f + WIDTH * hei + wid, wid, hei));
  }
  else
    return (*f || *(f + 1) || *(f + WIDTH) || *(f + WIDTH + 1));
}

int KXFace::Same(register char *f, register int wid, register int hei)
{
  register char val, *row;
  register int x;

  val = *f;
  while (hei--)
  {
    row = f;
    x = wid;
    while (x--)
      if (*(row++) != val)
        return(0);
    f += WIDTH;
  }
  return 1;
}

void KXFace::PushGreys(char *f, int wid, int hei)
{
  if (wid > 3)
  {
    wid /= 2;
    hei /= 2;
    PushGreys(f, wid, hei);
    PushGreys(f + wid, wid, hei);
    PushGreys(f + WIDTH * hei, wid, hei);
    PushGreys(f + WIDTH * hei + wid, wid, hei);
  }
  else
    RevPush(freqs + *f + 2 * *(f + 1) + 4 * *(f + WIDTH) +
        8 * *(f + WIDTH + 1));
}


#include "kxface.moc"
