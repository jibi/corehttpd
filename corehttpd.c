#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/skbuff.h>

#include <net/ip.h>
#include <net/sock.h>
#include <net/tcp.h>

static struct socket *
init_listening_socket(unsigned int address, unsigned short port, int backlog) {
	struct socket *sock;
	struct sockaddr_in sin;

	/* socket() */
	sock_create(PF_UNIX, SOCK_STREAM, IPPROTO_TCP, &sock);

	/* bind() */
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(address);
	sin.sin_port = htons(port);

	sock->ops->bind(sock, (struct sockaddr *) &sin, sizeof(sin));

	/* listen() */
	sock->ops->listen(sock, backlog);
	sock->flags |= SO_ACCEPTCONN;	

	return sock;
}

int
init_module(void) {
	struct socket *sock;

	printk(KERN_INFO "Starting corehttpd\n");

	sock = init_listening_socket(INADDR_LOOPBACK, 81, 10);

	return 0;
}

void
cleanup_module(void) {

}

