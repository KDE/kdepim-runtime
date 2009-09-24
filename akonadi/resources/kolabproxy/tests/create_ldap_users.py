#!/usr/bin/env python
"""Create some few kolab users in ldap.

Usage:

create_ldap_users.py [-h hostname] [-p port] [-u admin_dn] [-n num] [-o offset] [-t type] [--member dnmember] basedn cmd [passwd]

cmd may be one of

  add      add num (default 10) users named autoNUM

  delete   delete all users named auto*.  deletion is done by setting
           kolabDeleteFlag so that the kolabd will later remove the users.

Host and port default to localhost and the standard ldap port.

The type can be "user" (default) or "group".

The admin_dn should be the initial part of the dn of the admin account
to use, that ist admin + ',cn=internal,' + base_dn will be used as the
dn for the ldap bind.  The default is 'cn=manager'.

Offset will be the start number for additions and deletion (default = 0).
of users only.

dn_member is the dn of the initial member for groups.


Security considerations:
    The connection is not encrypted by default and thus the password
    can be sniffed.
"""

# requires the python-ldap module from
# http://python-ldap.sourceforge.net/
# on debian woody it's packaged as python-ldap

#import random
import re
import sys
import getopt
import ldap
import ldap.dn
import ldap.modlist
import getpass
import time
import sha
import base64
import httplib

def open_ldap(ldapuri, admin_dn_part, pwd = None):
    conn = ldap.initialize(ldapuri)
    if not pwd:
        pwd = getpass.getpass("ldap bind password:")
    conn.simple_bind_s(admin_dn_part + "," + "cn=internal" + "," + base_dn,
                       pwd)
    return conn

def fetch_kolab_info(conn):
    try:
        return conn.search_s("k=kolab," + base_dn, ldap.SCOPE_BASE,
                             filterstr='(objectClass=*)')[0][1]
    except ldap.INVALID_DN_SYNTAX:
        # should only happen if the server is no Kolab server, because
        # k=... is only valid on Kolab servers.  Since the script should
        # also support non-kolab servers to an extent, we return None to
        # indicate that the server is no Kolab server.
        return None

def set_delete_flag(conn, filterstr, offset):
    hosts = fetch_kolab_info(conn)["kolabHost"]
    result = conn.search_s(base_dn, ldap.SCOPE_ONELEVEL, filterstr=filterstr)
    for dn, attrs in result:
        n = re.match(r'cn=[^,0-9]+(\d+),',dn).group(1)
        if int(n) >= offset:
            conn.modify_s(dn, [(ldap.MOD_ADD, "kolabDeleteFlag", hosts)])
            print dn, hosts

def random_salt(length):
    """Returns a random salt for use with salted password hashes"""
    random = open("/dev/urandom")
    try:
        return random.read(length)
    finally:
        random.close()

SSHA_PREFIX = "{SSHA}"
def encode_ssha(password, salt):
    """SSHA-Encodes the password with the given salt"""
    digester = sha.new(password)
    digester.update(salt)
    return SSHA_PREFIX + base64.b64encode(digester.digest() + salt)

def encode_clear(password, salt):
    return password


def mail_domain_from_basedn(base_dn):
    """Determine the mail-domain from the dn.
    The function simple concatenates the values of the dc=... parts of
    the dn separated by periods."""
    print "mail_domain_from_basedn", base_dn
    return ".".join([item[0][1]
                     for item in ldap.dn.str2dn(base_dn) if item[0][0] == "dc"])


def add_user(conn, num_users, offset, set_password=None,
             http_hostname="localhost", http_port=80):
    common_attrs = {
        'objectClass': ['top', 'inetOrgPerson'],
        }

    kolab_info = fetch_kolab_info(conn)

    if kolab_info:
        mail_domain = kolab_info["postfix-mydomain"][0]
        # use the first of the kolab hosts as home server.
        kolab_home_server = kolab_info["kolabHost"][0]

        common_attrs['objectClass'].append("kolabInetOrgPerson")
        common_attrs['kolabHomeServer'] = [kolab_home_server]
        common_attrs['kolabInvitationPolicy'] = ['ACT_MANUAL']

        password_encoder = encode_ssha
    else:
        # The server is not a Kolab server, which currently means that
        # it's the courier server in the qemu image.  Here, the password
        # is unencrypted and the domain can be inferred from the base
        # dn.
        mail_domain = mail_domain_from_basedn(base_dn)
        password_encoder = encode_clear

    if set_password is not None:
        common_attrs["userPassword"] = password_encoder(set_password, random_salt(8))

    users =  [("test%d" % n, "auto", "autotest%d" % n)
              for n in range(offset, num_users + offset)]
    for sn, givenName, mailuid in users:
        #time.sleep(random.randrange(0,4))
        descr = common_attrs.copy()
        cn = givenName + " " + sn
        descr["cn"] = [cn]
        descr["givenName"] = [givenName]
        descr["sn"] = [sn]
        descr["mail"] = descr["uid"] = [mailuid + "@" + mail_domain]
        dn = "cn=" + cn + "," + base_dn
        print dn, descr
        print conn.add_s(dn, ldap.modlist.addModlist(descr))

    if not kolab_info:
        # The server is not a Kolab server, which currently means that
        # it's the courier server in the qemu image which requires an
        # extra step to finish the user creation process.
        conn = httplib.HTTPConnection(http_hostname, http_port)
        conn.request("GET", "/courier-maildir-trigger")
        response = conn.getresponse()
        if response.status != 200:
            raise RuntimeError("User creation trigger failed: %d: %s"
                               % (response.status, response.reason))


def delete_auto_users(conn, offset):
    set_delete_flag(conn, "(&(objectClass=kolabInetOrgPerson)(cn=auto*))", 
        offset)


def add_groups(conn, num_entries, member, offset=0):
    kolab_info = fetch_kolab_info(conn)

    mail_domain = kolab_info["postfix-mydomain"][0]
    # use the first of the kolab hosts as home server.
    kolab_home_server = kolab_info["kolabHost"][0]

    common_attrs = {
        'objectClass': ['top', 'groupOfNames', 'kolabGroupOfNames'],
        }

    groups =  ["testgrp%d@%s" % (n, mail_domain)
               for n in range(offset, offset + num_entries)]
    for mailuid in groups:
        descr = common_attrs.copy()
        descr["cn"] = descr["mail"] = [mailuid]
        descr["member"] = [member]
        dn = "cn=" + mailuid + "," + base_dn
        print dn, descr
        print conn.add_s(dn, ldap.modlist.addModlist(descr))


def delete_auto_groups(conn, offset=0):
    set_delete_flag(conn, "(&(objectClass=kolabGroupOfNames)(cn=testgrp*))", offset)


def main():
    global base_dn

    hostname = "localhost"
    port = None
    admin_dn_part = "cn=manager"
    base_dn = None
    num_entries = 10
    entry_type = "user"
    group_member = None
    offset = 0
    set_password = None

    opts, args = getopt.getopt(sys.argv[1:], 'h:p:u:n:o:t:',
                               ["host=", "port=", "user=", "num=", "offset=",
                                "set-password=", "type=", "member="])
    for optchar, value in opts:
        if optchar in ("-h", "--host"):
            hostname = value
        elif optchar in ("-p", "--port"):
            port = int(value)
        elif optchar in ("-u", "--user"):
            admin_dn_part = value
        elif optchar in ("-o", "--offset"):
            offset = int(value)
        elif optchar == "--set-password":
            set_password = value
        elif optchar in ("-t", "--type"):
            entry_type = value
        elif optchar == "--member":
            # dn of the initial member.  groups may not be empty in Kolab
            group_member = value
        elif optchar in ("-n", "--num"):
            num_entries = int(value)
        else:
            print >>sys.stderr, "Unknown option", optchar
            sys.exit(1)

    if len(args) < 2:
        print >>sys.stderr, "missing parameters"
        sys.exit(1)
    elif len(args) > 3:
        print >>sys.stderr, "too many parameters"
        sys.exit(1)

    base_dn, cmd, pwd = args

    if cmd not in ("add", "delete"):
        print >>sys.stderr, "unknown command", repr(cmd)
        sys.exit(1)


    if port is not None:
        ldap_uri ="ldap://%s:%d" % (hostname, port)
    else:
        ldap_uri ="ldap://%s" % (hostname,)



    # derive the http_port from the ldap port, assuming both are the
    # standard ports for the respective protocol and are offset by the
    # same value.
    if port is not None:
        http_port = port - 389 + 80
    else:
        http_port = 80

    conn = open_ldap(ldap_uri, admin_dn_part, pwd)
    if entry_type == "user":
        if cmd == "add":
            add_user(conn, num_entries, offset, set_password=set_password,
                     http_hostname=hostname, http_port=http_port)
        elif cmd == "delete":
            delete_auto_users(conn, offset)
    elif entry_type == "group":
        if cmd == "add":
            add_groups(conn, num_entries, group_member, offset)
        elif cmd == "delete":
            delete_auto_groups(conn, offset)



if __name__ == "__main__":
    main()
