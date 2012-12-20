#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "http.h"

char *
get_line(struct http_request *hr) {
	char *base, *line;
	int len;
	
	base = hr->req + hr->cur;
	line = kmalloc(MAXLINESIZE * sizeof(char), GFP_ATOMIC);
	len = 0;

	while (hr->cur < hr->len - 2 && hr->cur < MAXLINESIZE && 
		base[len] != '\n' && base[len + 1] != '\r') { 
		len++;
	}

	strncpy(line, base, len);
	hr->cur += len + 2;

	return line;
}

int
parse_request_line(struct http_request *hr) {
	char *request_line = get_line(hr);
	char *cur = request_line;
	int len = 0;
	int ret;

	ret = -1;

	if (!request_line) {
		goto leave;
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

	strncpy(hr->uri, cur, len);

	cur += len + 1;
	len = 0;

	while (cur[len] != ' ' && cur[len]) {
		len++;
	}
	
	if (!strncmp(cur, "HTTP/1.1", len)) {
		ret = 0;
		hr->ver = HTTP11;
	} else if (!strncmp(cur, "HTTP/1.0", len)) {
		hr->ver = HTTP10;
		ret = 0;
	} 

leave:
	kfree(request_line);
	return ret;
}

