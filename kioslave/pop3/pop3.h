/*
 * SPDX-FileCopyrightText: 1999, 2000 Alex Zepeda <zipzippy@sonic.net>
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once

#include <QUrl>
#include <kio/tcpslavebase.h>

#include <stdio.h>
#include <sys/types.h>

#define MAX_PACKET_LEN 4096

class POP3Protocol : public KIO::TCPSlaveBase
{
public:
    POP3Protocol(const QByteArray &pool, const QByteArray &app, bool SSL);
    ~POP3Protocol() override;

    void setHost(const QString &host, quint16 port, const QString &user, const QString &pass) override;

    void get(const QUrl &url) override;
    void stat(const QUrl &url) override;
    void del(const QUrl &url, bool isfile) override;
    void listDir(const QUrl &url) override;

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
        Invalid,
    };
    /**
     *  Send a command to the server, and wait for the  one-line-status
     *  reply via getResponse.  Similar rules apply.  If no buffer is
     *  specified, no data is passed back.
     */
    Resp command(const QByteArray &buf, char *r_buf = nullptr, unsigned int r_len = 0);

    /**
     *  All POP3 commands will generate a response.  Each response will
     *  either be prefixed with a "+OK " or a "-ERR ".  The getResponse
     *  function will wait until there's data to be read, and then read in
     *  the first line (the response), and copy the response sans +OK/-ERR
     *  into a buffer (up to len bytes) if one was passed to it.
     */
    Resp getResponse(char *buf, unsigned int len);

    /** Call int pop3_open() and report an error, if if fails */
    void openConnection() override;

    /**
     *  Attempt to properly shut down the POP3 connection by sending
     *  "QUIT\r\n" before closing the socket.
     */
    void closeConnection() override;

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

    // int m_cmd;
    unsigned short int m_iOldPort;
    unsigned short int m_iPort;
    QString m_sOldServer, m_sOldPass, m_sOldUser;
    QString m_sServer, m_sPass, m_sUser;
    bool m_try_apop, m_try_sasl, opened, supports_apop;
    QString m_sError;
    char readBuffer[MAX_PACKET_LEN];
    ssize_t readBufferLen;
};

