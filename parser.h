enum http_methods {
	GET,
	POST
};

enum http_versions {
	HTTP10,
	HTTP11
};

struct http_request {
	char *req;
	int len;

	char *next_req;
	int next_req_len;

	enum http_methods method;
	enum http_versions ver;
	char *uri;
	char *host;

	int cs;

	char parsed, leg;
};

char *get_line(struct http_request *hr);
int parse_http_request(struct http_request *hr);
struct http_request *new_http_request(char *buffer, ssize_t len);
int parse_request_line(struct http_request *hr);

