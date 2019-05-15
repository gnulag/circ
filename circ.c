#include <err.h>	// err for panics
#include <errno.h>	// errno
#include <unistd.h>	// read, write

#include <stdio.h>	// puts

#include "lib/config/config.h"
#include "irc.h"

irc_server s = {
	"Snoonet",
	"irc.snoonet.org",
	"6697",
	NULL,
	true
};

int
main (int argc, char** argv)
{
	puts ("setting up connection");
	int ret = irc_server_connect (&s);
	puts ("connection setup");
	if (ret == -1)
		err (1, "");

	puts ("sending nick/user info");
	/* Send nick/user info to register the client */
	char nick_cmd[] = "NICK circey\r\n";
	char user_cmd[] = "USER circey * * :Circey Khan\r\n";
	ret |= irc_write_bytes (&s, nick_cmd, sizeof (nick_cmd));
	ret |= irc_write_bytes (&s, user_cmd, sizeof (user_cmd));
	if (ret == -1 || errno)
		err (1, "");
	puts ("sent nick/user info");

	puts ("entering read/print loop");
	char c;
	/* While exactly one byte is read, print it back */
	while (irc_read_bytes (&s, &c, 1) == 1)
		/* Just print what the server says for now */
		write (1, &c, 1);

	return 0;
}
