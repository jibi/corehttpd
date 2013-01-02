#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/skbuff.h>
#include <linux/kthread.h>

#include <net/ip.h>
#include <net/sock.h>
#include <net/tcp.h>

#include "debug.h"
#include "http.h"
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
send_msg(struct socket *sock, unsigned char *src_buffer, ssize_t len) {
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
recv_msg(struct socket *sock, unsigned char **dst_buffer, ssize_t maxlen) {
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	ssize_t req_len;

	unsigned char *buffer;

	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	buffer = (char *) kmalloc(sizeof(unsigned char) * (maxlen + 1), GFP_ATOMIC);

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

static int
handle_request(void *data) {
	struct socket *sock;
	unsigned char *buffer;
	struct http_request *req;
	ssize_t req_len;

	sock = (struct socket *) data;
	req_len = recv_msg(sock, &buffer, 1024);

	req = new_http_request(buffer, req_len);

	if (!parse_http_request(req)) {
		printk("request: %s\n", req->uri);
	}

	return 0;
}

static int
accept_loop(void *data) {
	struct socket *sock;
	int _ret;
	
	sock = (struct socket *) data;

	while (1) {
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

