#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/skbuff.h>
#include <linux/kthread.h>

#include <net/ip.h>
#include <net/sock.h>
#include <net/tcp.h>

#include "debug.h"
#include "parser.h"

struct socket *server_sock;

static struct socket *
init_listening_socket(unsigned int address, unsigned short port, int backlog) {
	struct socket *sock;
	struct sockaddr_in sin;

	int _ret;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(address);
	sin.sin_port = htons(port);

	_ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	check_ret_value("sock_create", NULL);

	sock->sk->sk_reuse = 1;

	_ret = kernel_bind(sock, (struct sockaddr *) &sin, sizeof(sin));
	check_ret_value("kernel_bind", NULL);

	_ret = kernel_listen(sock, backlog);
	check_ret_value("kernel_listen", NULL);

	return sock;
}

ssize_t
send_msg(struct socket *sock, char *src_buffer, ssize_t len) {
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	iov.iov_base = src_buffer;
	iov.iov_len = len;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_sendmsg(sock,&msg,len);
	set_fs(oldfs);

	return size;
}

ssize_t
recv_msg(struct socket *sock, char **dst_buffer, ssize_t maxlen) {
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	ssize_t req_len;

	char *buffer;

	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	buffer = (char *) kmalloc(sizeof(char) * (maxlen + 1), GFP_ATOMIC);

	iov.iov_base = buffer;
	iov.iov_len = (size_t) maxlen * sizeof(char);

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	req_len = sock_recvmsg(sock, &msg, maxlen, 0);
	set_fs(oldfs);

	buffer[req_len] = '\x00';

	*dst_buffer = buffer;
	return req_len;
}

/* Get next chunk of request string.
 * If next_req is not NULL, new request data starts from that pointer.
 * If partial is set, current request is not completly parsed and only req and
 * len need to be updated */

static int
get_request (struct socket *sock, struct http_request **req, char *next_req,
	int next_req_len, char partial) {

	char *str;
	ssize_t len;

	if (next_req) {
		str = next_req;
		len = next_req_len;
	} else {
		len = recv_msg(sock, &str, 1024);

		if (len <= 0) {
			kfree(str);
			return -1;
		}
	}

	if (partial) {
		(*req)->req = str;
		(*req)->len = len;
	} else {
		*req = new_http_request(str, len);
	}

	return 0;
}

static int
handle_request(void *data) {
	struct socket *sock;
	struct http_request *req = NULL;

	char *next_req = NULL;
	int next_req_len = 0;

	sock = (struct socket *) data;

	while (!get_request(sock, &req, next_req, next_req_len, 0)) {

		parse_http_request(req);

		while (!req->parsed) {
			if (!get_request(sock, &req, 0, 0, 1)) {
				parse_http_request(req);
			} else {
				return -1;
			}
		}

		printk("request: %s\n", req->uri);

		/* send response */

		/* TODO free old req */
		next_req = req->next_req;
		next_req_len = req->next_req_len;
	}

	return 0;
}

static int
accept_loop(void *data) {
	struct socket *sock;
	int _ret;
	
	sock = (struct socket *) data;

/*	while (1) */ {
		struct socket *new_sock;
		struct task_struct *handle_request_thread;

		_ret = kernel_accept(sock, &new_sock, 0);
		check_ret_value("sock_create", 0);

		if (new_sock) {
			handle_request_thread = kthread_create(handle_request, new_sock, "[corehttpd_req]");
			wake_up_process(handle_request_thread);
		}
	}

	return 0;
}

int
init_module(void) {
	struct task_struct *accept_loop_thread;

	printk(KERN_INFO "Starting corehttpd\n");

	server_sock = init_listening_socket(INADDR_ANY, 81, 10);
	accept_loop_thread = kthread_create(accept_loop, server_sock, "[corehttpd_acc_loop]");
	wake_up_process(accept_loop_thread);

	return 0;
}

void
cleanup_module(void) {
	sock_release(server_sock);
}

MODULE_LICENSE("GPL");
