#! /bin/sh

OUTBOXPATH=$HOME/.ako-maildir.directory/outbox

TMPPATH=${OUTBOXPATH}/tmp
NEWPATH=${OUTBOXPATH}/new

TMPFILE=`mktemp -p ${TMPPATH} XXXXXXXXXX`
FAKEDATE=`LCALL=C date --rfc-2822`
cat >>${TMPFILE} <<EOF
From: Fake Sender <fake-sender@naiba.md>
To: Dummy Recipient <idanoka@gmail.com>
Subject: Test Message
Date: ${FAKEDATE}

bla ... bla ... bla
EOF

mv ${TMPFILE} ${NEWPATH}
