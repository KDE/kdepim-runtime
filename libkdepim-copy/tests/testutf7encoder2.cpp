#include "qutf7codec.h"
#include "qutf7codec.cpp"
#include <qtextstream.h>
#include <string.h>
#include <assert.h>
#include <iostream.h>

int main( int argc, char * argv[] ) {
  if ( argc == 1 ) {
    (void)new QUtf7Codec;

    QTextCodec * codec = QTextCodec::codecForName("utf-7");
    assert(codec);

    QTextIStream my_cin(stdin);

    QTextOStream my_cout(stdout);
    my_cout.setCodec(codec);
    
    QString buffer = my_cin.read();

    //    qDebug("buffer == " + buffer);

#ifdef USE_STREAM
    my_cout << buffer << endl;
#else
    QTextEncoder * enc = codec->makeEncoder();
#ifdef CHAR_WISE
    int len;
    for ( int i = 0 ; i < buffer.length() ; i++ ) {
      len = 1;
      cout << (enc->fromUnicode(QString(buffer[i]),len)).data();
    }
    cout << endl;
#else
    int len = buffer.length();
    cout << (enc->fromUnicode(buffer,len)).data() << endl;;
#endif // CHAR_WISE
    delete enc;
#endif // else USE_STREAM
  } else {
    qWarning("usage: testutf7encoder2 < infile > outfile\n");
  }
  QTextCodec::deleteAllCodecs();
}
