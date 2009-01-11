#! /bin/sh

OUTBOXPATH=$HOME/.local/share/maildispatcher/outbox
#OUTBOXPATH=$HOME/ttt

TMPPATH=${OUTBOXPATH}/tmp
NEWPATH=${OUTBOXPATH}/new

TMPFILE=`mktemp --tmpdir=${TMPPATH} XXXXXXXXXX`
FAKEDATE=`LCALL=C date --rfc-2822`
cat >>${TMPFILE} <<EOF
From: Fake Sender <fake-sender@ingo-kloecker.de>
To: Dummy Recipient <dummy@ingo-kloecker.de>
Subject: Test Message
Date: ${FAKEDATE}

bla ... bla ... bla
EOF

mv ${TMPFILE} ${NEWPATH}
