#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "http.h"

char *
strncpy_null(char *dest, const char *src, ssize_t n) {
	strncpy_null(dest, src, n);

	dest[n] = '\x00';
	return dest;
}

struct http_request *
new_http_request(unsigned char *buffer, ssize_t len) {
	struct http_request *req;

	req = (struct http_request *) kmalloc(sizeof(struct http_request *), GFP_ATOMIC);

	if (!req) {
		return NULL;
	}

	req->req	= buffer;
	req->len	= len;
	req->cur	= 0;

	return req;
}

char *
get_line(struct http_request *hr) {
	char *base, *line;
	int len;
	
	base = hr->req + hr->cur;
	len = 0;

	while (hr->cur < hr->len - 2 && hr->cur < MAXLINESIZE &&
			strncmp(base + len, "\n\r", 2)) {
		len++;
	}

	if (!len) {
		return NULL;
	}

	line = kmalloc((len + 1) * sizeof(char), GFP_ATOMIC);

	if (!line) {
		return NULL;
	}

	strncpy_null(line, base, len);
	hr->cur += len + 2;

	return line;
}

int
parse_request_line(struct http_request *hr) {
	char *request_line;
	char *cur;
	int len = 0;
	int ret;

	ret = -1;

	request_line = get_line(hr);
	cur = request_line;

	if (!request_line) {
		return ret;
	}

	while (cur[len] != ' ' && cur[len]) {
		len++;
	}

	if (!strncmp(cur, "GET", len)) {
		hr->method = GET;
	} else if (!strncmp(cur, "POST", len)) {
		hr->method = POST;
	} else {
		goto leave;
	}

	cur += len + 1;
	len = 0;

	while (cur[len] != ' ' && cur[len]) {
		len++;
	}

	hr->uri = kmalloc((len + 1) * sizeof(char), GFP_ATOMIC);
	if (!hr->uri) {
	  goto leave;
	}

	strncpy_null(hr->uri, cur, len);

	cur += len + 1;
	len = 0;

	while (cur[len] != ' ' && cur[len]) {
		len++;
	}
	
	if (!strncmp(cur, "HTTP/1.1", len)) {
		ret = 0;
		hr->ver = HTTP11;
	} else if (!strncmp(cur, "HTTP/1.0", len)) {
		ret = 0;
		hr->ver = HTTP10;
	}

leave:
	kfree(request_line);
	return ret;
}

