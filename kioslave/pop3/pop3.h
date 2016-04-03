/*
 * Copyright (c) 1999,2000 Alex Zepeda <zipzippy@sonic.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _POP3_H
#define _POP3_H

#include "pop3-config.h"

#include <kio/tcpslavebase.h>
#include <QUrl>

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>

#define MAX_PACKET_LEN 4096

class POP3Protocol : public KIO::TCPSlaveBase
{
public:
    POP3Protocol(const QByteArray &pool, const QByteArray &app, bool SSL);
    virtual ~ POP3Protocol();

    virtual void setHost(const QString &host, quint16 port,
                         const QString &user, const QString &pass) Q_DECL_OVERRIDE;

    void get(const QUrl &url) Q_DECL_OVERRIDE;
    void stat(const QUrl &url) Q_DECL_OVERRIDE;
    void del(const QUrl &url, bool isfile) Q_DECL_OVERRIDE;
    void listDir(const QUrl &url) Q_DECL_OVERRIDE;

protected:

    ssize_t myRead(void *data, ssize_t len);
    ssize_t myReadLine(char *data, ssize_t len);

    /**
     * This returns the size of a message as a long integer.
     * This is useful as an internal member, because the "other"
     * getSize command will emit a signal, which would be harder
     * to trap when doing something like listing a directory.
     */
    size_t realGetSize(unsigned int msg_num);

    /**
     *  Send a command to the server. Using this function, getResponse
     *  has to be called separately.
     */
    bool sendCommand(const QByteArray &cmd);

    enum Resp {
        Err,
        Ok,
        Cont,
        Invalid
    };
    /**
     *  Send a command to the server, and wait for the  one-line-status
     *  reply via getResponse.  Similar rules apply.  If no buffer is
     *  specified, no data is passed back.
     */
    Resp command(const QByteArray &buf, char *r_buf = 0, unsigned int r_len = 0);

    /**
     *  All POP3 commands will generate a response.  Each response will
     *  either be prefixed with a "+OK " or a "-ERR ".  The getResponse
     *  function will wait until there's data to be read, and then read in
     *  the first line (the response), and copy the response sans +OK/-ERR
     *  into a buffer (up to len bytes) if one was passed to it.
     */
    Resp getResponse(char *buf, unsigned int len);

    /** Call int pop3_open() and report an error, if if fails */
    void openConnection() Q_DECL_OVERRIDE;

    /**
     *  Attempt to properly shut down the POP3 connection by sending
     *  "QUIT\r\n" before closing the socket.
     */
    void closeConnection() Q_DECL_OVERRIDE;

    /**
     * Attempt to initiate a POP3 connection via a TCP socket.  If no port
     * is passed, port 110 is assumed, if no user || password is
     * specified, the user is prompted for them.
     */
    bool pop3_open();
    /**
     * Authenticate via APOP
     */
    int loginAPOP(const char *challenge, KIO::AuthInfo &ai);

    bool saslInteract(void *in, KIO::AuthInfo &ai);
    /**
     * Authenticate via SASL
     */
    int loginSASL(KIO::AuthInfo &ai);
    /**
     * Authenticate via traditional USER/PASS
     */
    bool loginPASS(KIO::AuthInfo &ai);

    //int m_cmd;
    unsigned short int m_iOldPort;
    unsigned short int m_iPort;
    struct timeval m_tTimeout;
    QString m_sOldServer, m_sOldPass, m_sOldUser;
    QString m_sServer, m_sPass, m_sUser;
    bool m_try_apop, m_try_sasl, opened, supports_apop;
    QString m_sError;
    char readBuffer[MAX_PACKET_LEN];
    ssize_t readBufferLen;
};

#endif
