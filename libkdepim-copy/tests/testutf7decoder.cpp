#include "qutf7codec.h"
#include "qutf7codec.cpp"
#include <QTextStream>
#include <string.h>
#include <assert.h>

int main( int argc, char** )
{
  if ( argc == 1 ) {
    (void)new QUtf7Codec;

    QTextCodec * codec = QTextCodec::codecForName("utf-7");
    assert(codec);

    QTextIStream my_cin(stdin);
    my_cin.setCodec(codec);

    QTextOStream my_cout(stdout);

    QString buffer = my_cin.readAll();

    my_cout << buffer;
  } else {
    qWarning("usage: testutf7decoder string_to_decode\n");
  }
}
