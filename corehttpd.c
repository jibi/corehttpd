#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/skbuff.h>
#include <linux/kthread.h>

#include <net/ip.h>
#include <net/sock.h>
#include <net/tcp.h>

static struct socket *
init_listening_socket(unsigned int address, unsigned short port, int backlog) {
	struct socket *sock;
	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(address);
	sin.sin_port = htons(port);

	sock_create(PF_UNIX, SOCK_STREAM, IPPROTO_TCP, &sock);
	kernel_bind(sock, (struct sockaddr *) &sin, sizeof(sin));
	kernel_listen(sock, backlog);

	return sock;
}

static int 
handle_request(void *data) {
	struct socket *sock;
	struct msghdr msg;
	struct iovec iov;

	char *buffer;

	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	buffer = (char *) kmalloc(sizeof(char) * 1024, GFP_ATOMIC);

	iov.iov_base = buffer;
	iov.iov_len = (size_t) 1024 * sizeof(char);

	sock = (struct socket *) data;

	return 0;
}

int
init_module(void) {
	struct socket *sock;

	printk(KERN_INFO "Starting corehttpd\n");

	sock = init_listening_socket(INADDR_LOOPBACK, 81, 10);

	while (1) {
		struct socket *new_sock;

		kernel_accept(sock, &new_sock, 0);

		if (new_sock) {
			kthread_create(handle_request, new_sock, "[corehttpd_req]");
		}
	}
	return 0;
}

void
cleanup_module(void) {

}

