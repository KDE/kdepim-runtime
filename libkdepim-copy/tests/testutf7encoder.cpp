#include "qutf7codec.h"
#include "qutf7codec.cpp"
#include <iostream.h>
#include <string.h>

void main( int argc, char * argv[] ) {
  if ( argc == 2 ) {
    QUtf7Codec * codec = new QUtf7Codec;

    QTextEncoder * enc;

    QString arg = QString::fromLatin1( argv[1] );
    int len;

    cout << "Original string:\n"
	 << "\"" << argv[1] << "\"\n" << endl;

    cout << "Encode optional direct set and whitespace:\n" << endl;
    codec->setEncodeWhitespace(TRUE);
    codec->setEncodeOptionalDirect(TRUE);
    enc = codec->makeEncoder();

    len = arg.length();
    cout << (enc->fromUnicode( arg, len )).data()
	 << "\n" << endl;

    cout << "Same as above, but call fromUnicode() char-wise:\n" << endl;

    delete enc;
    enc = codec->makeEncoder();

    for ( int i = 0 ; i < arg.length() ; i++ ) {
      len = 1;
      cout << (enc->fromUnicode( QString(arg[i]), len )).data();
    }
    cout << "\n" << endl;

    

    delete enc;

    cout << "Encode optional direct set and not whitespace:\n" << endl;
    codec->setEncodeWhitespace(FALSE);
    codec->setEncodeOptionalDirect(TRUE);
    enc = codec->makeEncoder();

    len = arg.length();
    cout << (enc->fromUnicode( arg, len )).data()
	 << "\n" << endl;

    delete enc;
    

    cout << "Don't encode optional direct set, but whitespace:\n" << endl;
    codec->setEncodeWhitespace(TRUE);
    codec->setEncodeOptionalDirect(FALSE);
    enc = codec->makeEncoder();

    len = arg.length();
    cout << (enc->fromUnicode( arg, len )).data()
	 << "\n" << endl;

    delete enc;
    

    cout << "Encode neither optional direct set, nor whitespace:\n" << endl;
    codec->setEncodeWhitespace(FALSE);
    codec->setEncodeOptionalDirect(FALSE);
    enc = codec->makeEncoder();

    len = arg.length();
    cout << (enc->fromUnicode( arg, len )).data()
	 << "\n" << endl;

    cout << "Same as above, but call fromUnicode() char-wise:\n" << endl;

    delete enc;
    enc = codec->makeEncoder();

    for ( int i = 0 ; i < arg.length() ; i++ ) {
      len = 1;
      cout << (enc->fromUnicode( QString(arg[i]), len )).data();
    }
    cout << "\n" << endl;

    
    delete enc;
    
    delete codec;
  } else {
    qWarning("usage: testutf7encoder string_to_encode\n");
  }
}
