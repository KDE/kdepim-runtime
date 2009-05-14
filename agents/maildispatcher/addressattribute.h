/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef ADDRESSATTRIBUTE_H
#define ADDRESSATTRIBUTE_H

#include <QString>
#include <QStringList>

#include <Akonadi/Attribute>


namespace MailTransport
{
  class Transport;
}


/**
 * Attribute storing the From, To, CC, BCC addresses of a message.
 */
class AddressAttribute : public Akonadi::Attribute
{
  public:
    AddressAttribute( const QString &from = QString(),
                      const QStringList &to = QStringList(),
                      const QStringList &cc = QStringList(),
                      const QStringList &bcc = QStringList() );
    virtual ~AddressAttribute();

    virtual AddressAttribute* clone() const;
    virtual QByteArray type() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

    QString from() const;
    void setFrom( const QString &from );
    QStringList to() const;
    void setTo( const QStringList &to );
    QStringList cc() const;
    void setCc( const QStringList &cc );
    QStringList bcc() const;
    void setBcc( const QStringList &bcc );

  private:
    QString mFrom;
    QStringList mTo;
    QStringList mCc;
    QStringList mBcc;

};


#endif // ADDRESSATTRIBUTE_H
