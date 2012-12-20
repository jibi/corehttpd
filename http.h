#define MAXLINESIZE 1024

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
	int cur;

	enum http_methods method;
	enum http_versions ver;
	char *uri;
	char *host;
};

char *get_line(struct http_request *hr); 
int parse_request_line(struct http_request *hr); 
