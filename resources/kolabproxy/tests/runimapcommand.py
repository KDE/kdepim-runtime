#! /usr/bin/env python

import sys
import optparse
import inspect
import imaplib

class AnnotationsMixin:

    def __install_commands_in_imaplib(self):
        imaplib.Commands['GETANNOTATION'] = ('AUTH', 'SELECTED')
        imaplib.Commands['SETANNOTATION'] = ('AUTH', 'SELECTED')

    def __to_argument_string(self, arg):
        if isinstance(arg, basestring):
            return self._quote(arg)
        elif arg is None:
            return "NIL"
        else:
            return "(%s)" % (" ".join(self.__to_argument_string(item)
                                      for item in arg),)

    def getannotation(self, *args):
        # normally we would implement this command like the other
        # commands in IMAP with an explicit list of parameters.  For
        # tests it's useful to be able to omit parameters or to send too
        # many, so we allow an arbitrary number of parameters
        self.__install_commands_in_imaplib()
        args = tuple(self.__to_argument_string(item) for item in args)
        typ, dat = self._simple_command('GETANNOTATION', *args)
        return self._untagged_response(typ, dat, 'ANNOTATION')

    def setannotation(self, *args):
        # normally we would implement this command like the other
        # commands in IMAP with an explicit list of parameters.  For
        # tests it's useful to be able to omit parameters or to send too
        # many, so we allow an arbitrary number of parameters
        self.__install_commands_in_imaplib()
        args = tuple(self.__to_argument_string(item) for item in args)
        return self._simple_command('SETANNOTATION', *args)


class IMAP4_annotations(AnnotationsMixin, imaplib.IMAP4):

    pass


class IMAP4_SSL_annotations(AnnotationsMixin, imaplib.IMAP4_SSL):

    pass



class ImapError(Exception):

    def __init__(self, host, port, imap_result):
        Exception.__init__(self, "Unexpected response from imap server"
                           " (%r:%d):%r" % (host, port, imap_result))
        self.imap_result = imap_result


class ImapConnection(object):

    def __init__(self, host, port, login, password, use_ssl):
        self.host = host
        self.port = port
        if use_ssl:
            cls = IMAP4_SSL_annotations
        else:
            cls = IMAP4_annotations
        self.error_class = cls.error
        self.imap = cls(host, port)
        self.auth_plain(login, password)

    def check_res(self, res, ok_responses=("OK",)):
        assert len(res) == 2
        if res[0] not in ok_responses:
            raise ImapError(self.host, self.port, res)
        data = res[1]
        # for some reason imaplib uses a one-element list containing
        # None to indicate that no untagged responses came from the
        # server.  Translate this to an empty list.
        if data == [None]:
            data = []
        return data

    def auth_plain(self, login, password):
        self.check_res(self.imap.login(login, password))

    def logout(self):
        self.check_res(self.imap.logout(), ok_responses=("OK", "BYE"))

    @property
    def capabilities(self):
        return self.imap.capabilities

    def create(self, mailbox):
        self.check_res(self.imap.create(mailbox))

    def delete(self, mailbox):
        self.check_res(self.imap.delete(mailbox))

    def rename(self, oldmailbox, newmailbox):
        self.check_res(self.imap.rename(oldmailbox, newmailbox))

    def getannotation(self, *args):
        return self.check_res(self.imap.getannotation(*args))

    def setannotation(self, *args):
        return self.check_res(self.imap.setannotation(*args))

    def setacl(self, *args):
        return self.check_res(self.imap.setacl(*args))

    def append(self, *args):
        return self.check_res(self.imap.append(*args))


def command_create(connection, mailbox):
    """Create a mailbox.  Parameter: MAILBOX
    Example: create INBOX/MyCalendar
    """
    connection.create(mailbox)

def command_setacl(connection, mailbox, user, rights):
    """Set an ACL. Parameters: MAILBOX USERNAME RIGHTS
    Example: setacl INBOX/MyCalendar someone@example.com lrs
    """
    connection.setacl(mailbox, user, rights)

def command_setannotation(connection, mailbox, annotation, value):
    """Set an annotation.  Parameters: MAILBOX ANNOTATION VALUE
    Example: setannotation INBOX/MyCalendar /vendor/kolab/folder-type event
    """
    connection.setannotation(mailbox, annotation, ["value.shared", value])

def command_append(connection, mailbox, filename):
    """Append a message to a mailbox.  Parameters: MAILBOX FILENAME
    Example: append INBOX/MyCalendar testdata/mytestmail
    """
    connection.append(mailbox, None, None, open(filename).read())


def find_commands():
    commands = [obj for obj in globals().values()
                    if inspect.isfunction(obj)
                       and obj.__name__.startswith("command_")]
    commands.sort()
    return commands


class OptionParserWithCommands(optparse.OptionParser):

    def format_help(self, formatter=None):
        option_help = optparse.OptionParser.format_help(self,
                                                        formatter=formatter)
        command_help = ["commands:"]
        for command in find_commands():
            doc_lines = command.__doc__.splitlines()
            command_help.append("  %-15s %s"
                                % (command.__name__[len("command_"):],
                                   doc_lines[0]))
            command_help.extend(doc_lines[1:])
        return option_help + "\n" + "\n".join(command_help) + "\n"


def main():
    parser = OptionParserWithCommands(usage=("%prog [options]"
                                             " [command [arg1 ...]]"))
    parser.set_defaults(host="localhost")
    parser.add_option("--host",
                      help=("Hostname of the IMAP server"
                            " (default 'localhost')."))
    parser.add_option("--port", type="int",
                      help=("Port number of the IMAP server (required)."))
    parser.add_option("--ssl", action="store_true",
                      help=("Enable SSL."))
    parser.add_option("--user",
                      help=("Username for login (required)."))
    parser.add_option("--password",
                      help=("Password for login (required)."))

    options, args = parser.parse_args()

    missing_required_option = False
    for optname in "port user password".split():
        if getattr(options, optname) is None:
            print >>sys.stderr, "Missing required option %r" % optname
            missing_required_option = True
    if missing_required_option:
        sys.exit(1)

    if len(args) == 0:
        print >>sys.stderr, "No command given"
        sys.exit(1)

    command_name = args[0]
    command_args = args[1:]
    command = globals().get("command_" + command_name)
    if command is None:
        print >>sys.stderr, "Unknown command %r" % command_name
        sys.exit(1)

    expected_num_args = len(inspect.getargspec(command)[0]) - 1
    if len(command_args) != expected_num_args:
        print >>sys.stderr, "--%s requires %d arguments, got %d" % \
              (options.command, expected_num_args, len(command_args))
        sys.exit(1)

    connection = ImapConnection(options.host, options.port,
                                options.user, options.password,
                                options.ssl)
    command(connection, *command_args)
    connection.logout()


if __name__ == "__main__":
    main()
