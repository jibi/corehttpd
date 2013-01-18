#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "parser.h"

/*
 * based on puma http parser
 * see https://github.com/puma/puma
 */

char *
get_marked_string(char *fpc, char *mark) {
	char *buffer;

	buffer = (char *) kmalloc((fpc - mark + 1) * sizeof(char), GFP_ATOMIC);

	strncpy(buffer, mark, fpc - mark);
	buffer[fpc - mark] = 0;

	return buffer;
}

%%{
	machine http_parser;

	action mark {
		mark = fpc;
	}

	action request_uri {
		req->uri = get_marked_string(fpc, mark);
	}

	action fragment {

	}

	action request_method {
		char *tmp;

		tmp = get_marked_string(fpc, mark);

		if (!strcmp(tmp, "GET")) {
			req->method = GET;
		} else if (!strcmp(tmp, "POST")) {
			req->method = POST;
		} else {
			req->leg = 0;
		}

		kfree(tmp);
	}

	action http_version {
		char *tmp;

		tmp = get_marked_string(fpc, mark);

		if (!strcmp(tmp, "HTTP/1.0")) {
			req->ver = HTTP10;
		} else if (!strcmp(tmp, "HTTP/1.1")) {
			req->ver = HTTP11;
		} else {
			req->leg = 0;
		}

		kfree(tmp);
	}

	action write_field {

	}

	action write_value {

	}

	action done {
		if (req->leg) {
			req->parsed = 1;
		}
	}

	CRLF = "\r\n";

	CTL = (cntrl | 127);
	safe = ("$" | "-" | "_" | ".");
	extra = ("!" | "*" | "'" | "(" | ")" | ",");
	reserved = (";" | "/" | "?" | ":" | "@" | "&" | "=" | "+");
	unsafe = (CTL | " " | "\"" | "#" | "%" | "<" | ">");
	national = any -- (alpha | digit | reserved | extra | safe | unsafe);
	unreserved = (alpha | digit | safe | extra | national);
	escape = ("%" xdigit xdigit);
	uchar = (unreserved | escape);
	pchar = (uchar | ":" | "@" | "&" | "=" | "+");
	tspecials = ("(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\\" | "\"" | "/" | "[" | "]" | "?" | "=" | "{" | "}" | " " | "\t");

	token = (ascii -- (CTL | tspecials));

	scheme = ( alpha | digit | "+" | "-" | "." )* ;
	absolute_uri = (scheme ":" (uchar | reserved )*);

	path = ( pchar+ ( "/" pchar* )* );
	query = ( uchar | reserved )*;
	param = ( pchar | "/" )*;
	params = ( param ( ";" param )* );
	rel_path = ( path? (";" params)? ) ("?" query)?;
	absolute_path = ( "/"+ rel_path );

	Request_URI = ( "*" | absolute_uri | absolute_path ) >mark %request_uri;
	Fragment = ( uchar | reserved )* >mark %fragment;
	Method = ( upper | digit | safe ){1,20} >mark %request_method;

	http_number = ( digit+ "." digit+ ) ;
	HTTP_Version = ( "HTTP/" http_number ) >mark %http_version ;
	Request_Line = ( Method " " Request_URI ("#" Fragment){0,1} " " HTTP_Version CRLF ) ;

	field_name = ( token -- ":" )+ >mark %write_field;
	field_value = any* >mark %write_value;
	message_header = field_name ":" " "* field_value :> CRLF;

	Request = Request_Line ( message_header )* ( CRLF @done );

	main := Request;

}%%

%% write data;

struct http_request *
new_http_request(char *buffer, ssize_t len) {
	struct http_request *req;

	req = (struct http_request *) kmalloc(sizeof(struct http_request *), GFP_ATOMIC);

	if (!req) {
		return NULL;
	}

	req->req	= buffer;
	req->len	= len;
	req->parsed	= 0;
	req->leg	= 1;
	req->cs		= http_parser_start;

	return req;
}

int
parse_http_request(struct http_request *req) {
	int cs;
	char *mark = NULL;
	char *p, *pe;

	cs = req->cs;
	p = req->req;
	pe = p + req->len;

	%% write exec;

	req->cs = cs;

	if (strlen(p) > 0) {
		req->next_req = p;
		req->next_req_len = strlen(p);
	} else {
		req->next_req = NULL;
	}
	
	return 0;
}

// vim: filetype=c
